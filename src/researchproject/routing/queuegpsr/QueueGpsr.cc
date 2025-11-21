//
// Copyright (C) 2013 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "QueueGpsr.h"

#include <algorithm>
#include <map>
#include <sstream>

#include "inet/queueing/contract/IPacketQueue.h"
#include "inet/queueing/contract/IPacketCollection.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "inet/common/INETUtils.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/UserPriorityTag_m.h"
#include "inet/networklayer/common/HopLimitTag_m.h"
#include "inet/networklayer/common/IpProtocolId_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/common/L3Tools.h"
#include "inet/networklayer/common/NextHopAddressTag_m.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#ifdef INET_WITH_IPv4
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#endif

#ifdef INET_WITH_IPv6
#include "inet/networklayer/ipv6/Ipv6ExtensionHeaders_m.h"
#include "inet/networklayer/ipv6/Ipv6InterfaceData.h"
#endif

#ifdef INET_WITH_NEXTHOP
#include "inet/networklayer/nexthop/NextHopForwardingHeader_m.h"
#endif

namespace researchproject {

Define_Module(QueueGpsr);

static inline double determinant(double a1, double a2, double b1, double b2)
{
    return a1 * b2 - a2 * b1;
}

QueueGpsr::QueueGpsr()
{
}

QueueGpsr::~QueueGpsr()
{
    cancelAndDelete(beaconTimer);
    cancelAndDelete(purgeNeighborsTimer);
    cancelAndDelete(queueMonitorTimer);
    cancelAndDelete(neighborTableDebugTimer);
    cancelAndDelete(preloadDurabilityTimer);
}

//
// module interface
//

void QueueGpsr::initialize(int stage)
{
    if (stage == INITSTAGE_ROUTING_PROTOCOLS)
        addressType = getSelfAddress().getAddressType();

    RoutingProtocolBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        // Gpsr parameters
        const char *planarizationModeString = par("planarizationMode");
        if (!strcmp(planarizationModeString, ""))
            planarizationMode = GPSR_NO_PLANARIZATION;
        else if (!strcmp(planarizationModeString, "GG"))
            planarizationMode = GPSR_GG_PLANARIZATION;
        else if (!strcmp(planarizationModeString, "RNG"))
            planarizationMode = GPSR_RNG_PLANARIZATION;
        else
            throw cRuntimeError("Unknown planarization mode");
        interfaces = par("interfaces");
        beaconInterval = par("beaconInterval");
        maxJitter = par("maxJitter");
        neighborValidityInterval = par("neighborValidityInterval");
        displayBubbles = par("displayBubbles");
        // delay tiebreaker parameters (Phase 2/3)
        enableDelayTiebreaker = par("enableDelayTiebreaker");
        distanceEqualityThreshold = par("distanceEqualityThreshold");
        delayEstimationFactor = par("delayEstimationFactor");
        tiebreakerActivations = 0;
        greedySelections = 0;
        tiebreakerActivationsSignal = registerSignal("tiebreakerActivations");
        // context
        host = getContainingNode(this);
        interfaceTable.reference(this, "interfaceTableModule", true);
        outputInterface = par("outputInterface");
        mobility = check_and_cast<IMobility *>(host->getSubmodule("mobility"));
        routingTable.reference(this, "routingTableModule", true);
        networkProtocol.reference(this, "networkProtocolModule", true);
        // internal
        beaconTimer = new cMessage("BeaconTimer");
        purgeNeighborsTimer = new cMessage("PurgeNeighborsTimer");
        queueMonitorTimer = new cMessage("QueueMonitorTimer");  // STEP 2 AUDIT timer
        neighborTableDebugTimer = new cMessage("NeighborTableDebugTimer");  // STEP 4 AUDIT timer
        preloadDurabilityTimer = new cMessage("PreloadDurabilityTimer");  // PRELOAD DURABILITY timer
        // packet size
        positionByteLength = par("positionByteLength");
        // KLUDGE implement position registry protocol
        globalPositionTable.clear();
        // read Phase 3 gating parameter
        enableQueueDelay = par("enableQueueDelay");
    }
    else if (stage == INITSTAGE_ROUTING_PROTOCOLS) {
        registerProtocol(Protocol::manet, gate("ipOut"), gate("ipIn"));
        host->subscribe(linkBrokenSignal, this);
        networkProtocol->registerHook(0, this);
        WATCH(neighborPositionTable);
        
        // STEP 1 AUDIT: Module wiring proof with full details (using stdout for visibility)
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "STEP 1 AUDIT: QueueGpsr Module Initialization\n";
        std::cout << "  Module Path: " << getFullPath() << "\n";
        std::cout << "  Module Type: " << getComponentType()->getName() << "\n";
        std::cout << "  Host: " << getContainingNode(this)->getFullName() << "\n";
        std::cout << "  enableDelayTiebreaker: " << (enableDelayTiebreaker ? "TRUE" : "FALSE") << "\n";
        std::cout << "  enableQueueDelay: " << (enableQueueDelay ? "TRUE" : "FALSE") << "\n";
        std::cout << "  distanceEqualityThreshold: " << distanceEqualityThreshold << " m\n";
        std::cout << "  delayEstimationFactor: " << delayEstimationFactor << " s/m\n";
        
        // DEBUG: Print all interface names
        std::cout << "  Available interfaces in InterfaceTable:\n";
        for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
            auto iface = interfaceTable->getInterface(i);
            std::cout << "    [" << i << "] " << iface->getInterfaceName() << " (id=" << iface->getInterfaceId() << ")\n";
        }
        std::cout << "  outputInterface parameter: \"" << outputInterface << "\"\n";
        
        // Verify the configured outputInterface exists
        auto networkInterface = interfaceTable->findInterfaceByName(outputInterface);
        if (networkInterface) {
            std::cout << "  ✓ Output interface found: " << networkInterface->getInterfaceName() 
                      << " (id=" << networkInterface->getInterfaceId() << ")\n";
        } else {
            std::cout << "  ✗ ERROR: Output interface '" << outputInterface << "' NOT FOUND!\n";
        }
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::flush;
        
        // STEP 2 AUDIT: Schedule queue monitoring timer (only for host[7])
        if (strcmp(getContainingNode(this)->getFullName(), "host[7]") == 0) {
            scheduleAt(simTime() + 1.0, queueMonitorTimer);
            std::cout << "STEP 2 AUDIT: Queue monitoring scheduled for host[7] starting at t=1s\n" << std::flush;
        }
        
        // STEP 4 AUDIT: Schedule neighbor table debug (only for host[0] at t=9s - right before main flow)
        if (strcmp(getContainingNode(this)->getFullName(), "host[0]") == 0) {
            scheduleAt(9.0, neighborTableDebugTimer);
            std::cout << "STEP 4 AUDIT: Neighbor table debug scheduled for host[0] at t=9s (before main flow)\n" << std::flush;
        }
        
        // PRELOAD DURABILITY: Monitor host[2]'s queue from t=20-35s
        if (strcmp(getContainingNode(this)->getFullName(), "host[2]") == 0) {
            scheduleAt(20.0, preloadDurabilityTimer);
            std::cout << "PRELOAD DURABILITY: Queue monitoring scheduled for host[2] from t=20s\n" << std::flush;
        }
    }
        // AUDIT: enumerate MAC submodules and report queue implementations
        auditMacQueues();


}

void QueueGpsr::handleMessageWhenUp(cMessage *message)
{
    if (message->isSelfMessage())
        processSelfMessage(message);
    else
        processMessage(message);
}

//
// handling messages
//

void QueueGpsr::processSelfMessage(cMessage *message)
{
    if (message == beaconTimer)
        processBeaconTimer();
    else if (message == purgeNeighborsTimer)
        processPurgeNeighborsTimer();
    else if (message == queueMonitorTimer)
        processQueueMonitorTimer();  // STEP 2 AUDIT
    else if (message == neighborTableDebugTimer)
        processNeighborTableDebug();  // STEP 4 AUDIT
    else if (message == preloadDurabilityTimer)
        processPreloadDurabilityTimer();  // PRELOAD DURABILITY
    else
        throw cRuntimeError("Unknown self message");
}

void QueueGpsr::processMessage(cMessage *message)
{
    if (auto pk = dynamic_cast<Packet *>(message))
        processUdpPacket(pk);
    else
        throw cRuntimeError("Unknown message");
}

//
// beacon timers
//

void QueueGpsr::scheduleBeaconTimer()
{
    EV_DEBUG << "Scheduling beacon timer" << endl;
    scheduleAfter(beaconInterval + uniform(-1, 1) * maxJitter, beaconTimer);
}

void QueueGpsr::processBeaconTimer()
{
    EV_DEBUG << "Processing beacon timer" << endl;
    
    // DIAGNOSTIC: Verify beacon timer is not already scheduled (should be false during processing)
    if (beaconTimer->isScheduled()) {
        std::cerr << "⚠️  WARNING: beaconTimer already scheduled during processBeaconTimer() at t=" 
                  << simTime() << "s on " << getContainingNode(this)->getFullName() << std::endl;
    }
    
    // Note: No decay needed - getLocalTxBacklogBytes() now reads actual queue directly
    
    const L3Address selfAddress = getSelfAddress();
    if (!selfAddress.isUnspecified()) {
        sendBeacon(createBeacon());
        storeSelfPositionInGlobalRegistry();
    }
    scheduleBeaconTimer();
    schedulePurgeNeighborsTimer();
    
    // DIAGNOSTIC: Verify beacon timer was re-scheduled successfully
    if (!beaconTimer->isScheduled()) {
        std::cerr << "❌ ERROR: beaconTimer NOT re-scheduled after processBeaconTimer() at t=" 
                  << simTime() << "s on " << getContainingNode(this)->getFullName() << std::endl;
    }
}

//
// handling purge neighbors timers
//

void QueueGpsr::schedulePurgeNeighborsTimer()
{
    EV_DEBUG << "Scheduling purge neighbors timer" << endl;
    simtime_t nextExpiration = getNextNeighborExpiration();
    if (nextExpiration == SimTime::getMaxTime()) {
        if (purgeNeighborsTimer->isScheduled())
            cancelEvent(purgeNeighborsTimer);
    }
    else {
        if (!purgeNeighborsTimer->isScheduled())
            scheduleAt(nextExpiration, purgeNeighborsTimer);
        else {
            if (purgeNeighborsTimer->getArrivalTime() != nextExpiration) {
                rescheduleAt(nextExpiration, purgeNeighborsTimer);
            }
        }
    }
}

void QueueGpsr::processPurgeNeighborsTimer()
{
    EV_DEBUG << "Processing purge neighbors timer" << endl;
    purgeNeighbors();
    schedulePurgeNeighborsTimer();
}

//
// STEP 2 AUDIT: Queue monitoring (every 1 second for host[7])
//

void QueueGpsr::processQueueMonitorTimer()
{
    // Only monitor host[7] to reduce noise
    if (strcmp(getContainingNode(this)->getFullName(), "host[7]") == 0) {
        unsigned long backlog = getLocalTxBacklogBytes();
        
        // Try to resolve the actual queue module path for documentation
        std::string queuePath = "UNKNOWN";
        try {
            cModule *wlanModule = getContainingNode(this)->getSubmodule("wlan", 0);
            if (wlanModule) {
                cModule *queueModule = wlanModule->getSubmodule("queue");
                if (queueModule) {
                    queuePath = queueModule->getFullPath();
                }
            }
        } catch (...) {
            queuePath = "ERROR_RESOLVING";
        }
        
        std::cout << "━━━ STEP 2 AUDIT: Queue Tap ━━━\n";
        std::cout << "  Host: host[7]\n";
        std::cout << "  Time: " << simTime() << " s\n";
        std::cout << "  Resolved Queue Path: " << queuePath << "\n";
        std::cout << "  LocalTxBacklogBytes: " << backlog << " bytes\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::flush;
    }
    
    // Reschedule for 1 second later
    scheduleAt(simTime() + 1.0, queueMonitorTimer);
}

//
// PRELOAD DURABILITY: Monitor congested relay's queue (t=20-35s)
//

void QueueGpsr::processPreloadDurabilityTimer()
{
    // Only monitor host[2] (congested relay in micro diamond test)
    if (strcmp(getContainingNode(this)->getFullName(), "host[2]") == 0) {
        unsigned long backlog = getLocalTxBacklogBytes();
        
        std::cout << "⏰ PRELOAD DURABILITY [host[2]]: t=" << simTime() << "s → Queue=" << backlog << " bytes";
        if (backlog > 50000) {
            std::cout << " 🔴 SATURATED";
        } else if (backlog > 20000) {
            std::cout << " 🟠 HEAVILY LOADED";
        } else if (backlog > 5000) {
            std::cout << " 🟡 MODERATE";
        } else if (backlog > 0) {
            std::cout << " 🟢 LIGHT";
        } else {
            std::cout << " ⚪ EMPTY";
        }
        std::cout << "\n" << std::flush;
    }
    
    // Reschedule every 1 second until t=35s
    if (simTime() < 35.0) {
        scheduleAt(simTime() + 1.0, preloadDurabilityTimer);
    }
}

//
// STEP 4 AUDIT: Neighbor table debug (one-shot at t=29s for host[0])
// ENHANCED: Show distance-to-dest, age, and GPSR-forward candidate analysis
//

void QueueGpsr::processNeighborTableDebug()
{
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║ STEP 4 AUDIT: Neighbor Table Snapshot (Pre-Routing Decision) ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
    std::cout << "  Host: " << getContainingNode(this)->getFullName() << "\n";
            std::cout << "  Time: " << simTime() << " s\n";
            std::cout << "  Purpose: Verify beacon propagation before main flow starts at t=10s\n";
            std::cout << "\n";    // Get my position and destination position for distance calculations
    Coord myPos = mobility->getCurrentPosition();
    // Assume destination is host[3] for micro diamond test
    L3Address destAddr = L3Address(Ipv4Address("10.0.0.4"));
    Coord destPos;
    bool hasDestPos = false;
    if (globalPositionTable.hasPosition(destAddr)) {
        destPos = globalPositionTable.getPosition(destAddr);
        hasDestPos = true;
    }
    double myDistToDest = hasDestPos ? myPos.distance(destPos) : -1.0;
    
    std::cout << "  My Position: (" << myPos.x << ", " << myPos.y << ", " << myPos.z << ")\n";
    if (hasDestPos) {
        std::cout << "  Dest Position: (" << destPos.x << ", " << destPos.y << ", " << destPos.z << ")\n";
        std::cout << "  My Distance to Dest: " << myDistToDest << " m\n";
    }
    std::cout << "\n";
    
    // Dump position table with detailed analysis
    std::cout << "  ┌─ Neighbor Position Table (with GPSR Analysis) ────────────┐\n";
    std::vector<L3Address> neighborAddrs = neighborPositionTable.getAddresses();
    std::cout << "  │ Total neighbors: " << neighborAddrs.size() << "\n";
    if (neighborAddrs.empty()) {
        std::cout << "  │ ⚠️  WARNING: No neighbors discovered! Check beacon interval.\n";
    } else {
        int forwardCandidates = 0;
        for (const L3Address& addr : neighborAddrs) {
            if (neighborPositionTable.hasPosition(addr)) {
                Coord neighborPos = neighborPositionTable.getPosition(addr);
                double neighborDistToDest = hasDestPos ? neighborPos.distance(destPos) : -1.0;
                bool isForwardCandidate = hasDestPos && (neighborDistToDest < myDistToDest);
                
                std::cout << "  │\n";
                std::cout << "  │   Neighbor: " << addr << "\n";
                std::cout << "  │     Position: (" << neighborPos.x << ", " << neighborPos.y << ", " << neighborPos.z << ")\n";
                if (hasDestPos) {
                    std::cout << "  │     Dist to Dest: " << neighborDistToDest << " m";
                    if (isForwardCandidate) {
                        std::cout << " ✓ GPSR-FORWARD";
                        forwardCandidates++;
                    } else {
                        std::cout << " ✗ NOT-FORWARD (>= my distance)";
                    }
                    std::cout << "\n";
                }
                
                // Check queue backlog for this neighbor
                auto backlogIt = neighborTxBacklogBytes.find(addr);
                if (backlogIt != neighborTxBacklogBytes.end()) {
                    const NeighborQueueInfo& info = backlogIt->second;
                    uint32_t backlog = info.bytes;
                    simtime_t age = simTime() - info.lastUpdate;
                    std::cout << "  │     Queue Backlog: " << backlog << " bytes (age: " << age << "s)";
                    if (backlog > 10000) {
                        std::cout << " 🔴 HEAVILY CONGESTED";
                    } else if (backlog > 1000) {
                        std::cout << " 🟡 MODERATE";
                    } else if (backlog > 0) {
                        std::cout << " 🟢 LIGHT";
                    } else {
                        std::cout << " ⚪ IDLE";
                    }
                    std::cout << "\n";
                } else {
                    std::cout << "  │     Queue Backlog: NO DATA (no beacon received yet)\n";
                }
            }
        }
        std::cout << "  │\n";
        std::cout << "  │ ═══ GPSR Analysis ════════════════════════════════════════\n";
        std::cout << "  │   GPSR-Forward Candidates: " << forwardCandidates << "\n";
        if (forwardCandidates >= 2) {
            std::cout << "  │   Status: ✅ TIE SCENARIO POSSIBLE (≥2 forward neighbors)\n";
        } else if (forwardCandidates == 1) {
            std::cout << "  │   Status: ⚠️  ONLY 1 FORWARD NEIGHBOR (no tie to break)\n";
        } else {
            std::cout << "  │   Status: ❌ NO FORWARD NEIGHBORS (perimeter mode)\n";
        }
    }
    std::cout << "  └───────────────────────────────────────────────────────────┘\n";
    std::cout << "\n";
    std::cout << "  Next event: Main flow starts at t=30s (1 second from now)\n";
    std::cout << "  Expected: If ≥2 forward candidates, tiebreaker should activate\n";
    std::cout << "═══════════════════════════════════════════════════════════════\n";
    std::cout << std::flush;
    
    // This is a one-shot timer, don't reschedule
}

//
// handling UDP packets
//

void QueueGpsr::sendUdpPacket(Packet *packet)
{
    send(packet, "ipOut");
}

void QueueGpsr::processUdpPacket(Packet *packet)
{
    packet->popAtFront<UdpHeader>();
    processBeacon(packet);
    schedulePurgeNeighborsTimer();
}

//
// handling beacons
//

const Ptr<GpsrBeacon> QueueGpsr::createBeacon()
{
    const auto& beacon = makeShared<GpsrBeacon>();
    beacon->setAddress(getSelfAddress());
    beacon->setPosition(mobility->getCurrentPosition());
    
    // Calculate chunk length: address + position + txBacklogBytes field (uint32_t = 4 bytes)
    B beaconLength = B(getSelfAddress().getAddressType()->getAddressByteLength() + positionByteLength + sizeof(uint32_t));
    beacon->setChunkLength(beaconLength);
    
    // include local TX backlog bytes in beacon (Phase 3, optional)
    if (enableQueueDelay) {
        uint32_t localBacklog = (uint32_t)getLocalTxBacklogBytes();
        beacon->setTxBacklogBytes(localBacklog);
        
        // STEP 3 AUDIT: Log beacon transmission with nonzero backlog (host[7] and host[1])
        if (localBacklog > 0 && (strcmp(getContainingNode(this)->getFullName(), "host[7]") == 0 || 
                                  strcmp(getContainingNode(this)->getFullName(), "host[1]") == 0)) {
            std::cout << "━━━ STEP 3 AUDIT: Beacon Transmission ━━━\n";
            std::cout << "  Sender: " << getContainingNode(this)->getFullName() << "\n";
            std::cout << "  Time: " << simTime() << " s\n";
            std::cout << "  txBacklogBytes in beacon: " << localBacklog << " bytes\n";
            std::cout << "  (Will be received by neighbors within ~1s)\n";
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::flush;
        }
        
        // DIAGNOSTIC: Log ALL beacon transmissions to track beacon frequency and queue measurement
        std::cout << "🟦 Beacon TX [" << getContainingNode(this)->getFullName() << "]: t=" << simTime() 
                  << "s | Q=" << localBacklog << " bytes | nextBeacon≈t=" 
                  << (simTime() + beaconInterval).dbl() << "s" << std::endl;
    } else {
        beacon->setTxBacklogBytes(0);  // Explicit zero when queue-aware disabled
    }
    return beacon;
}

void QueueGpsr::sendBeacon(const Ptr<GpsrBeacon>& beacon)
{
    EV_INFO << "Sending beacon: address = " << beacon->getAddress() << ", position = " << beacon->getPosition() << endl;
    Packet *udpPacket = new Packet("GPSRBeacon");
    udpPacket->insertAtBack(beacon);
    auto udpHeader = makeShared<UdpHeader>();
    udpHeader->setSourcePort(GPSR_UDP_PORT);
    udpHeader->setDestinationPort(GPSR_UDP_PORT);
    udpHeader->setCrcMode(CRC_DISABLED);
    udpPacket->insertAtFront(udpHeader);
    auto addresses = udpPacket->addTag<L3AddressReq>();
    addresses->setSrcAddress(getSelfAddress());
    addresses->setDestAddress(addressType->getLinkLocalManetRoutersMulticastAddress());
    udpPacket->addTag<HopLimitReq>()->setHopLimit(255);
    udpPacket->addTag<PacketProtocolTag>()->setProtocol(&Protocol::manet);
    udpPacket->addTag<DispatchProtocolReq>()->setProtocol(addressType->getNetworkProtocol());
    
    // Mark beacons as highest-priority control traffic (802.11e AC_VO = priority 7)
    // This prevents MAC-level suppression during congestion
    udpPacket->addTag<UserPriorityReq>()->setUserPriority(7);
    
    sendUdpPacket(udpPacket);
}

void QueueGpsr::processBeacon(Packet *packet)
{
    const auto& beacon = packet->peekAtFront<GpsrBeacon>();
    EV_INFO << "Processing beacon: address = " << beacon->getAddress() << ", position = " << beacon->getPosition() << endl;
    neighborPositionTable.setPosition(beacon->getAddress(), beacon->getPosition());
    
    // CRITICAL FIX: Also register neighbor position in global table so routing can find destination positions
    storePositionInGlobalRegistry(beacon->getAddress(), beacon->getPosition());
    
    // DEBUG: Log ALL beacon receptions for validation
    std::cout << "[BEACON-RX] " << getContainingNode(this)->getFullName() 
              << " t=" << simTime() << " from=" << beacon->getAddress() 
              << " pos=" << beacon->getPosition() << std::endl;
    
    // DEBUG: Check destination awareness for host[1] during early phase
    if (strcmp(getContainingNode(this)->getFullName(), "host[1]") == 0 && simTime() >= 4.0 && simTime() <= 8.0) {
        // Check if we now know about destination
        L3Address destAddr = L3Address(Ipv4Address("10.0.0.4"));
        if (globalPositionTable.hasPosition(destAddr)) {
            std::cout << "  ✓ host[1] NOW knows dest position: " << globalPositionTable.getPosition(destAddr) << std::endl;
        } else {
            std::cout << "  ✗ host[1] still doesn't know dest position" << std::endl;
        }
    }
    
    // store neighbor TX backlog if present (Phase 3) WITH TIMESTAMP for aging
    uint32_t nb = 0;
    try {
        // generated accessor exists after msg change
        nb = beacon->getTxBacklogBytes();
        NeighborQueueInfo info;
        info.bytes = nb;
        info.lastUpdate = simTime();
        neighborTxBacklogBytes[beacon->getAddress()] = info;
    }
    catch (...) {
        // older beacons may not contain the field; ignore
    }
    
    // STEP 3 AUDIT: Show beacon reception and neighbor table update (host[0] for micro diamond test)
    // LOG ALL BEACONS to debug why Relay A isn't visible
    if (strcmp(getContainingNode(this)->getFullName(), "host[0]") == 0) {
        if (nb > 0) {
            std::cout << "━━━ STEP 3 AUDIT: Beacon Reception ━━━\n";
            std::cout << "  Receiver: host[0]\n";
            std::cout << "  Time: " << simTime() << " s\n";
            std::cout << "  Sender: " << beacon->getAddress() << "\n";
            std::cout << "  txBacklogBytes in beacon: " << nb << " bytes\n";
            std::cout << "  Stored in neighborTxBacklogBytes[" << beacon->getAddress() << "] = {" << nb << " bytes, t=" << simTime() << "}\n";
            std::cout << "  Neighbor table size: " << neighborTxBacklogBytes.size() << " entries\n";
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::flush;
        } else {
            std::cout << "🔵 Beacon RX [host[0]]: t=" << simTime() << "s from " << beacon->getAddress() 
                      << " | Q=" << nb << " bytes (IDLE)\n" << std::flush;
        }
    }
    
    delete packet;
}

//
// handling packets
//

GpsrOption *QueueGpsr::createGpsrOption(L3Address destination)
{
    GpsrOption *gpsrOption = new GpsrOption();
    gpsrOption->setRoutingMode(GPSR_GREEDY_ROUTING);
    gpsrOption->setDestinationPosition(lookupPositionInGlobalRegistry(destination));
    gpsrOption->setLength(computeOptionLength(gpsrOption));
    return gpsrOption;
}

int QueueGpsr::computeOptionLength(GpsrOption *option)
{
    // routingMode
    int routingModeBytes = 1;
    // destinationPosition, perimeterRoutingStartPosition, perimeterRoutingForwardPosition
    int positionsBytes = 3 * positionByteLength;
    // currentFaceFirstSenderAddress, currentFaceFirstReceiverAddress, senderAddress
    int addressesBytes = 3 * getSelfAddress().getAddressType()->getAddressByteLength();
    // type and length
    int tlBytes = 1 + 1;

    return tlBytes + routingModeBytes + positionsBytes + addressesBytes;
}

//
// configuration
//

void QueueGpsr::configureInterfaces()
{
    // join multicast groups
    cPatternMatcher interfaceMatcher(interfaces, false, true, false);
    for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
        NetworkInterface *networkInterface = interfaceTable->getInterface(i);
        if (networkInterface->isMulticast() && interfaceMatcher.matches(networkInterface->getInterfaceName()))
            networkInterface->joinMulticastGroup(addressType->getLinkLocalManetRoutersMulticastAddress());
    }
}

//
// position
//

// KLUDGE implement position registry protocol
Coord QueueGpsr::lookupPositionInGlobalRegistry(const L3Address& address) const
{
    // KLUDGE implement position registry protocol
    return globalPositionTable.getPosition(address);
}

void QueueGpsr::storePositionInGlobalRegistry(const L3Address& address, const Coord& position) const
{
    // KLUDGE implement position registry protocol
    globalPositionTable.setPosition(address, position);
}

void QueueGpsr::storeSelfPositionInGlobalRegistry() const
{
    auto selfAddress = getSelfAddress();
    if (!selfAddress.isUnspecified())
        storePositionInGlobalRegistry(selfAddress, mobility->getCurrentPosition());
}

Coord QueueGpsr::computeIntersectionInsideLineSegments(Coord& begin1, Coord& end1, Coord& begin2, Coord& end2) const
{
    // NOTE: we must explicitly avoid computing the intersection points inside due to double instability
    if (begin1 == begin2 || begin1 == end2 || end1 == begin2 || end1 == end2)
        return Coord::NIL;
    else {
        double x1 = begin1.x;
        double y1 = begin1.y;
        double x2 = end1.x;
        double y2 = end1.y;
        double x3 = begin2.x;
        double y3 = begin2.y;
        double x4 = end2.x;
        double y4 = end2.y;
        double a = determinant(x1, y1, x2, y2);
        double b = determinant(x3, y3, x4, y4);
        double c = determinant(x1 - x2, y1 - y2, x3 - x4, y3 - y4);
        double x = determinant(a, x1 - x2, b, x3 - x4) / c;
        double y = determinant(a, y1 - y2, b, y3 - y4) / c;
        if ((x <= x1 && x <= x2) || (x >= x1 && x >= x2) || (x <= x3 && x <= x4) || (x >= x3 && x >= x4) ||
            (y <= y1 && y <= y2) || (y >= y1 && y >= y2) || (y <= y3 && y <= y4) || (y >= y3 && y >= y4))
            return Coord::NIL;
        else
            return Coord(x, y, 0);
    }
}

void QueueGpsr::auditMacQueues() const
{
    try {
        cModule *wlanModule = getContainingNode(this)->getSubmodule("wlan", 0);
        if (!wlanModule) {
            std::cout << "[AUDIT] No wlan[0] on " << getContainingNode(this)->getFullName() << "\n" << std::flush;
            return;
        }

        cModule *macModule = wlanModule->getSubmodule("mac");
        if (!macModule) {
            std::cout << "[AUDIT] No mac submodule in wlan[0] on " << getContainingNode(this)->getFullName() << "\n" << std::flush;
            return;
        }

        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "AUDIT: MAC submodule tree for " << getContainingNode(this)->getFullName() << "\n";

        std::function<void(cModule*, int)> visit = [&](cModule *mod, int depth) {
            std::string indent(depth * 2, ' ');
            const char *typeName = "<unknown>";
            if (mod->getComponentType())
                typeName = mod->getComponentType()->getName();

            // Check whether this module implements IPacketCollection
            auto *qc = dynamic_cast<queueing::IPacketCollection *>(mod);
            if (qc) {
                std::cout << indent << "[Q] " << mod->getFullPath() << " : " << typeName << " (implements IPacketCollection)\n";
            }
            else {
                std::cout << indent << "[ ] " << mod->getFullPath() << " : " << typeName << "\n";
            }

            for (cModule::SubmoduleIterator it(mod); !it.end(); ++it) {
                visit(*it, depth + 1);
            }
        };

        visit(macModule, 0);
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::flush;
    }
    catch (std::exception &e) {
        std::cout << "[AUDIT-EXCEPTION] " << e.what() << "\n" << std::flush;
    }
    catch (...) {
        std::cout << "[AUDIT-EXCEPTION] unknown error while auditing MAC submodules\n" << std::flush;
    }
}


unsigned long QueueGpsr::getLocalTxBacklogBytes() const
{
    // Read actual MAC TX queue backlog (all sub-queues)
    // Path: wlan[0].mac.tx.queue (PriorityQueue with multiple sub-queues)
    
    static int callCount = 0;
    bool shouldLog = (++callCount <= 10);  // Log first 10 calls
    
    if (shouldLog) {
        std::cout << "[Q-CALL #" << callCount << "] getLocalTxBacklogBytes() called on " 
                  << host->getFullName() << " at t=" << simTime() << "s" << std::endl;
    }
    
    try {
        // Find the wlan[0] interface on this host
        cModule *wlanModule = host->getSubmodule("wlan", 0);
        if (!wlanModule) {
            std::cout << "[Q-ERROR] No wlan[0] module found on host " << host->getFullName() << std::endl;
            return 0;
        }
        
        // Navigate to mac.tx.queue
        cModule *macModule = wlanModule->getSubmodule("mac");
        if (!macModule) {
            std::cout << "[Q-ERROR] No mac module found in wlan[0]" << std::endl;
            return 0;
        }
        
        // Navigate to MAC queue - in INET 4.5, it's at mac.dcf.channelAccess.pendingQueue
        // Collect all modules implementing IPacketCollection under the MAC subtree
        std::vector<cModule *> queues;
        std::function<void(cModule*)> collect = [&](cModule *mod) {
            if (!mod) return;
            auto *qc = dynamic_cast<queueing::IPacketCollection *>(mod);
            if (qc)
                queues.push_back(mod);
            for (cModule::SubmoduleIterator it(mod); !it.end(); ++it)
                collect(*it);
        };

        collect(macModule);

        if (queues.empty()) {
            static int errorCount = 0;
            if (++errorCount <= 5) {
                std::cout << "[Q-ERROR] No IPacketCollection found in MAC tree for " 
                         << host->getFullName() << std::endl;
            }
            return 0;
        }

        // Sum lengths and optionally log per-queue sizes
        unsigned long totalBytes = 0;
        int totalPackets = 0;
        for (auto *qm : queues) {
            auto *qc = dynamic_cast<queueing::IPacketCollection *>(qm);
            if (!qc) continue;
            b len = qc->getTotalLength();
            unsigned long bytes = (unsigned long)len.get();
            int npk = qc->getNumPackets();
            totalBytes += bytes;
            totalPackets += npk;

            // Log per-queue info for first few reads or when non-zero
            static int perqLogCount = 0;
            if (++perqLogCount <= 50 || bytes > 0) {
                std::cout << "[Q-PERQ] " << host->getFullName() << " t=" << simTime() 
                          << "s: queue=" << bytes << " bytes (" << npk << " pkt) [path=" << qm->getFullPath() << "]" << std::endl;
            }
        }

        // DIAGNOSTIC: aggregate log
        static int aggLogCount = 0;
        if (++aggLogCount <= 20 || totalBytes > 0) {
            std::cout << "[Q-AGG] " << host->getFullName() << " t=" << simTime() << "s: TOTAL=" 
                      << totalBytes << " bytes (" << totalPackets << " pkt) across " << queues.size() << " queues" << std::endl;
        }

        return totalBytes;
        
    } catch (std::exception& e) {
        std::cout << "[Q-EXCEPTION] Error reading MAC queue: " << e.what() << std::endl;
        return 0;
    }
}

Coord QueueGpsr::getNeighborPosition(const L3Address& address) const
{
    return neighborPositionTable.getPosition(address);
}

//
// angle
//

double QueueGpsr::getVectorAngle(Coord vector) const
{
    ASSERT(vector != Coord::ZERO);
    double angle = atan2(-vector.y, vector.x);
    if (angle < 0)
        angle += 2 * M_PI;
    return angle;
}

double QueueGpsr::getNeighborAngle(const L3Address& address) const
{
    return getVectorAngle(getNeighborPosition(address) - mobility->getCurrentPosition());
}

//
// address
//

std::string QueueGpsr::getHostName() const
{
    return host->getFullName();
}

L3Address QueueGpsr::getSelfAddress() const
{
    // TODO choose self address based on a new 'interfaces' parameter
    L3Address ret = routingTable->getRouterIdAsGeneric();
#ifdef INET_WITH_IPv6
    if (ret.getType() == L3Address::IPv6) {
        for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
            NetworkInterface *ie = interfaceTable->getInterface(i);
            if ((!ie->isLoopback())) {
                if (auto ipv6Data = ie->findProtocolData<Ipv6InterfaceData>()) {
                    ret = ipv6Data->getPreferredAddress();
                    break;
                }
            }
        }
    }
#endif
    return ret;
}

L3Address QueueGpsr::getSenderNeighborAddress(const Ptr<const NetworkHeaderBase>& networkHeader) const
{
    const GpsrOption *gpsrOption = getGpsrOptionFromNetworkDatagram(networkHeader);
    return gpsrOption->getSenderAddress();
}

//
// neighbor
//

simtime_t QueueGpsr::getNextNeighborExpiration()
{
    simtime_t oldestPosition = neighborPositionTable.getOldestPosition();
    if (oldestPosition == SimTime::getMaxTime())
        return oldestPosition;
    else
        return oldestPosition + neighborValidityInterval;
}

void QueueGpsr::purgeNeighbors()
{
    neighborPositionTable.removeOldPositions(simTime() - neighborValidityInterval);
}

double QueueGpsr::estimateNeighborDelay(const L3Address& address) const
{
    
    EV_INFO << "📊 estimateNeighborDelay() called for " << address 
            << " | enableQueueDelay=" << enableQueueDelay << endl;
    
    Coord selfPosition = mobility->getCurrentPosition();
    Coord neighborPosition = neighborPositionTable.getPosition(address);
    double distance = (neighborPosition - selfPosition).length();
    
    double delay = distance * delayEstimationFactor;
    if (enableQueueDelay) {
        EV_INFO << "   enableQueueDelay is TRUE, checking neighbor backlog map..." << endl;
        
        // add queueing delay term based on neighbor backlog (if known and fresh)
        auto it = neighborTxBacklogBytes.find(address);
        if (it != neighborTxBacklogBytes.end()) {
            EV_INFO << "   ✅ Found neighbor in backlog map, bytes=" << it->second.bytes << endl;
            
            const NeighborQueueInfo& info = it->second;
            
            // AGING CHECK: Discard stale queue info (older than 1.5× beacon interval)
            simtime_t age = simTime() - info.lastUpdate;
            simtime_t maxAge = beaconInterval * 3;
            
            // Log age check for ALL routing decisions to track freshness
            std::cout << "[AGE-CHECK] t=" << simTime() << " " << getContainingNode(this)->getFullName()
                     << ": neighbor=" << address << " age=" << age << "s maxAge=" << maxAge << "s"
                     << " Q=" << info.bytes << " bytes"
                     << (age > maxAge ? " ⚠️ STALE" : " ✓ FRESH") << std::endl;
            
            // DIAGNOSTIC: Additional detail for source node during validation window
            bool isSourceNode = (strcmp(getContainingNode(this)->getFullName(), "host[0]") == 0);
            if (isSourceNode && simTime() >= 9.0 && simTime() <= 15.0) {
                std::cout << "    🕒 Age check for " << address << ": age=" << age 
                          << "s, maxAge=" << maxAge << "s";
                if (age > maxAge) {
                    std::cout << " ⚠️  STALE (age > maxAge, will use distance-only)" << std::endl;
                } else {
                    std::cout << " ✓ FRESH (age <= maxAge, will use Q/R)" << std::endl;
                }
            }
            
            if (age > maxAge) {
                // Queue info is stale - ignore it
                EV_DETAIL << "Ignoring stale queue info for neighbor " << address 
                         << " (age=" << age << "s, maxAge=" << maxAge << "s)" << endl;
                if (isSourceNode && simTime() >= 9.0 && simTime() <= 15.0) {
                    std::cout << "    ⏭️  Skipping Q/R for " << address 
                              << " (stale beacon from t=" << info.lastUpdate << "s)" << std::endl;
                }
                return delay; // return distance-only delay
            }
            
            double backlogBytes = (double) info.bytes;
            
            // Read neighbor's transmitter bitrate dynamically (INET 4.5.x canonical path)
            double bitrate = 0.0; // in bps
            
            // Resolve neighbor host module from L3 address
            L3AddressResolver resolver;
            cModule *neighborHost = resolver.findHostWithAddress(address);
            
            std::cout << "    🔍 [estimateNeighborDelay] Resolving bitrate for " << address 
                     << " | backlogBytes=" << backlogBytes << std::endl;
            std::cout << "       neighborHost: " << (neighborHost ? neighborHost->getFullPath() : "nullptr") << std::endl;
            
            if (neighborHost) {
                // Navigate to neighbor's wlan[0].radio.transmitter.bitrate
                cModule *wlan = neighborHost->getSubmodule("wlan", 0);
                std::cout << "       wlan[0]: " << (wlan ? wlan->getFullPath() : "nullptr") << std::endl;
                
                if (wlan) {
                    cModule *radio = wlan->getSubmodule("radio");
                    std::cout << "       radio: " << (radio ? radio->getFullPath() : "nullptr") << std::endl;
                    
                    if (radio) {
                        cModule *transmitter = radio->getSubmodule("transmitter");
                        std::cout << "       transmitter: " << (transmitter ? transmitter->getFullPath() : "nullptr") << std::endl;
                        
                        if (transmitter && transmitter->hasPar("bitrate")) {
                            try {
                                bitrate = transmitter->par("bitrate").doubleValue();
                                std::cout << "       ✅ bitrate read: " << bitrate << " bps (" << (bitrate/1e6) << " Mbps)" << std::endl;
                                
                                // Guard against unspecified/auto bitrate (≤0)
                                if (bitrate <= 0.0) {
                                    std::cout << "       ⚠️  bitrate ≤0, treating as unknown" << std::endl;
                                    bitrate = 0.0;
                                }
                            }
                            catch (...) { 
                                std::cout << "       ⚠️  Exception reading bitrate parameter" << std::endl;
                                bitrate = 0.0; 
                            }
                        } else {
                            std::cout << "       ⚠️  transmitter has no bitrate parameter" << std::endl;
                        }
                    }
                }
            } else {
                std::cout << "       ⚠️  L3AddressResolver could not find host" << std::endl;
            }
            
            if (bitrate > 0.0) {
                double backlogBits = backlogBytes * 8.0;
                double queueDelay = backlogBits / bitrate;
                delay += queueDelay; // seconds
                std::cout << "       ✅ Q/R calculated: " << backlogBytes << " bytes / " << (bitrate/1e6) 
                         << " Mbps = " << queueDelay << "s | Total delay=" << delay << "s" << std::endl;
            } else {
                std::cout << "       ⚠️  Could not read bitrate - using distance-only delay (" << delay << "s)" << std::endl;
            }
        }
    }

    return delay;
}

std::vector<L3Address> QueueGpsr::getPlanarNeighbors() const
{
    std::vector<L3Address> planarNeighbors;
    std::vector<L3Address> neighborAddresses = neighborPositionTable.getAddresses();
    Coord selfPosition = mobility->getCurrentPosition();
    for (auto it = neighborAddresses.begin(); it != neighborAddresses.end(); it++) {
        auto neighborAddress = *it;
        Coord neighborPosition = neighborPositionTable.getPosition(neighborAddress);
        if (planarizationMode == GPSR_NO_PLANARIZATION)
            return neighborAddresses;
        else if (planarizationMode == GPSR_RNG_PLANARIZATION) {
            double neighborDistance = (neighborPosition - selfPosition).length();
            for (auto& witnessAddress : neighborAddresses) {
                Coord witnessPosition = neighborPositionTable.getPosition(witnessAddress);
                double witnessDistance = (witnessPosition - selfPosition).length();
                double neighborWitnessDistance = (witnessPosition - neighborPosition).length();
                if (neighborAddress == witnessAddress)
                    continue;
                else if (neighborDistance > std::max(witnessDistance, neighborWitnessDistance))
                    goto eliminate;
            }
        }
        else if (planarizationMode == GPSR_GG_PLANARIZATION) {
            Coord middlePosition = (selfPosition + neighborPosition) / 2;
            double neighborDistance = (neighborPosition - middlePosition).length();
            for (auto& witnessAddress : neighborAddresses) {
                Coord witnessPosition = neighborPositionTable.getPosition(witnessAddress);
                double witnessDistance = (witnessPosition - middlePosition).length();
                if (neighborAddress == witnessAddress)
                    continue;
                else if (witnessDistance < neighborDistance)
                    goto eliminate;
            }
        }
        else
            throw cRuntimeError("Unknown planarization mode");
        planarNeighbors.push_back(*it);
      eliminate:;
    }
    return planarNeighbors;
}

std::vector<L3Address> QueueGpsr::getPlanarNeighborsCounterClockwise(double startAngle) const
{
    std::vector<L3Address> neighborAddresses = getPlanarNeighbors();
    std::sort(neighborAddresses.begin(), neighborAddresses.end(), [&] (const L3Address& address1, const L3Address& address2) {
        // NOTE: make sure the neighbor at startAngle goes to the end
        auto angle1 = getNeighborAngle(address1) - startAngle;
        auto angle2 = getNeighborAngle(address2) - startAngle;
        if (angle1 <= 0)
            angle1 += 2 * M_PI;
        if (angle2 <= 0)
            angle2 += 2 * M_PI;
        return angle1 < angle2;
    });
    return neighborAddresses;
}

//
// next hop
//

L3Address QueueGpsr::findNextHop(const L3Address& destination, GpsrOption *gpsrOption)
{
    switch (gpsrOption->getRoutingMode()) {
        case GPSR_GREEDY_ROUTING: return findGreedyRoutingNextHop(destination, gpsrOption);
        case GPSR_PERIMETER_ROUTING: return findPerimeterRoutingNextHop(destination, gpsrOption);
        default: throw cRuntimeError("Unknown routing mode");
    }
}

L3Address QueueGpsr::findGreedyRoutingNextHop(const L3Address& destination, GpsrOption *gpsrOption)
{
    EV_DEBUG << "Finding next hop using greedy routing: destination = " << destination << endl;
    L3Address selfAddress = getSelfAddress();
    Coord selfPosition = mobility->getCurrentPosition();
    Coord destinationPosition = gpsrOption->getDestinationPosition();
    double bestDistance = (destinationPosition - selfPosition).length();
    L3Address bestNeighbor;
    double bestDelay = INFINITY;  // Track best delay for tiebreaker
    
    // STEP 4 AUDIT: Log routing decision for source node (host[0]) only
    bool isSourceNode = (strcmp(getContainingNode(this)->getFullName(), "host[0]") == 0);
    if (isSourceNode && simTime() >= 15.0) {
        std::cout << "\n━━━━━━ STEP 4 AUDIT: Greedy Routing Decision ━━━━━━\n";
        std::cout << "  Time: " << simTime() << " s\n";
        std::cout << "  Source: host[0]\n";
        std::cout << "  Destination: " << destination << "\n";
        std::cout << "  My position: (" << selfPosition.x << ", " << selfPosition.y << ")\n";
        std::cout << "  Dest position: (" << destinationPosition.x << ", " << destinationPosition.y << ")\n";
        std::cout << "  My distance to dest: " << bestDistance << " m\n";
        std::cout << "  Evaluating " << neighborPositionTable.getAddresses().size() << " neighbors:\n";
    }
    
    std::vector<L3Address> neighborAddresses = neighborPositionTable.getAddresses();
    for (auto& neighborAddress : neighborAddresses) {
        Coord neighborPosition = neighborPositionTable.getPosition(neighborAddress);
        double neighborDistance = (destinationPosition - neighborPosition).length();
        
        // STEP 4 AUDIT: Log each candidate evaluation with Q/R breakdown
        if (isSourceNode && simTime() >= 15.0) {
            double candidateDelay = estimateNeighborDelay(neighborAddress);
            uint32_t candidateBacklog = 0;
            double queueDelayTerm = 0.0;
            double distanceDelayTerm = neighborDistance * delayEstimationFactor;
            double linkRateMbps = 0.0;
            
            auto it = neighborTxBacklogBytes.find(neighborAddress);
            if (it != neighborTxBacklogBytes.end()) {
                candidateBacklog = it->second.bytes;
                
                // Calculate Q/R if queue-aware enabled
                if (enableQueueDelay && candidateBacklog > 0) {
                    // Read bitrate from NEIGHBOR's radio.transmitter.bitrate (INET 4.5.x path)
                    L3AddressResolver resolver;
                    cModule *neighborHost = resolver.findHostWithAddress(neighborAddress);
                    if (neighborHost) {
                        cModule *wlan = neighborHost->getSubmodule("wlan", 0);
                        if (wlan) {
                            cModule *radio = wlan->getSubmodule("radio");
                            if (radio) {
                                cModule *transmitter = radio->getSubmodule("transmitter");
                                if (transmitter && transmitter->hasPar("bitrate")) {
                                    try {
                                        double bitrate = transmitter->par("bitrate").doubleValue();
                                        if (bitrate > 0.0) {
                                            linkRateMbps = bitrate / 1e6;  // Convert to Mbps
                                            queueDelayTerm = (candidateBacklog * 8.0) / bitrate;
                                        }
                                    } catch (...) {}
                                }
                            }
                        }
                    }
                }
            }
            
            std::cout << "    Candidate: " << neighborAddress 
                     << " | Dist: " << neighborDistance << "m"
                     << " | Q=" << candidateBacklog << " bytes"
                     << " | R=" << linkRateMbps << " Mbps"
                     << " | Q/R=" << queueDelayTerm << "s"
                     << " | D×factor=" << distanceDelayTerm << "s"
                     << " | Total delay=" << candidateDelay << "s\n";
        }
        
        // DEBUG: Log distance comparisons when tiebreaker enabled
        if (enableDelayTiebreaker && !bestNeighbor.isUnspecified()) {
            double distanceDiff = fabs(neighborDistance - bestDistance);
            EV_DETAIL << "Comparing neighbor " << neighborAddress 
                      << ": neighborDist=" << neighborDistance << "m, bestDist=" << bestDistance 
                      << "m, diff=" << distanceDiff << "m, threshold=" << distanceEqualityThreshold 
                      << "m" << endl;
        }
        
        // Delay tiebreaker logic (Phase 2/3)
        if (enableDelayTiebreaker && neighborDistance < bestDistance) {
            // This neighbor is strictly closer - it becomes the new best
            bestDistance = neighborDistance;
            bestNeighbor = neighborAddress;
            bestDelay = estimateNeighborDelay(neighborAddress);
            greedySelections++;
        }
        else if (enableDelayTiebreaker && 
                 !bestNeighbor.isUnspecified() &&
                 fabs(neighborDistance - bestDistance) < distanceEqualityThreshold) {
            // Neighbors are equidistant (within threshold) - use delay tiebreaker
            double neighborDelay = estimateNeighborDelay(neighborAddress);
            
            // Get queue info for detailed logging
            double bestQueueBytes = 0, neighborQueueBytes = 0;
            auto bestIt = neighborTxBacklogBytes.find(bestNeighbor);
            auto neighIt = neighborTxBacklogBytes.find(neighborAddress);
            if (bestIt != neighborTxBacklogBytes.end()) bestQueueBytes = bestIt->second.bytes;
            if (neighIt != neighborTxBacklogBytes.end()) neighborQueueBytes = neighIt->second.bytes;
            
            // Log ALL ties with full details including queue sizes
            std::cout << "[TIE] t=" << simTime() << " " << getContainingNode(this)->getFullName()
                     << ": dst=" << destination 
                     << " | best=" << bestNeighbor << " dist=" << bestDistance << "m delay=" << bestDelay << "s Q=" << bestQueueBytes << "B"
                     << " | challenger=" << neighborAddress << " dist=" << neighborDistance << "m delay=" << neighborDelay << "s Q=" << neighborQueueBytes << "B"
                     << " | distDiff=" << fabs(neighborDistance - bestDistance) << "m" << std::endl;
            
            if (isSourceNode && simTime() >= 15.0) {
                std::cout << "    🔀 TIE DETECTED! Candidates equidistant (diff=" 
                         << fabs(neighborDistance - bestDistance) << "m < " 
                         << distanceEqualityThreshold << "m threshold)\n";
                std::cout << "       Current best: " << bestNeighbor << " delay=" << bestDelay << "s\n";
                std::cout << "       Challenger: " << neighborAddress << " delay=" << neighborDelay << "s\n";
            }
            
            EV_INFO << "TIE DETECTED! neighbor=" << neighborAddress 
                    << " neighborDist=" << neighborDistance << "m bestDist=" << bestDistance 
                    << "m diff=" << fabs(neighborDistance - bestDistance) << "m threshold=" 
                    << distanceEqualityThreshold << "m neighborDelay=" << neighborDelay 
                    << "s bestDelay=" << bestDelay << "s" << endl;
            
            if (neighborDelay < bestDelay) {
                double previousBestDelay = bestDelay;  // Save for logging before update
                bestDistance = neighborDistance;
                bestNeighbor = neighborAddress;
                bestDelay = neighborDelay;
                tiebreakerActivations++;
                emit(tiebreakerActivationsSignal, tiebreakerActivations);
                
                // Log ALL tiebreaker activations with full context
                std::cout << "[TIEBREAKER-WIN] t=" << simTime() << " " << getContainingNode(this)->getFullName()
                         << ": chose=" << neighborAddress << " delay=" << neighborDelay << "s"
                         << " over=" << bestNeighbor << " delay=" << previousBestDelay << "s"
                         << " | activations=" << tiebreakerActivations << std::endl;
                
                if (isSourceNode && simTime() >= 15.0) {
                    std::cout << "       ✅ TIEBREAKER ACTIVATED! Chose " << neighborAddress 
                             << " (lower delay: " << neighborDelay << "s < " << previousBestDelay << "s)\n";
                    std::cout << "       Total tiebreaker activations: " << tiebreakerActivations << "\n";
                }
                
                EV_DEBUG << "Tiebreaker activated: selected neighbor " << neighborAddress 
                         << " with delay " << neighborDelay << "s over previous best with delay " 
                         << bestDelay << "s (distances: " << neighborDistance << "m vs " 
                         << bestDistance << "m)" << endl;
            }
        }
        else if (!enableDelayTiebreaker && neighborDistance < bestDistance) {
            // Original GPSR logic: just select strictly closer neighbor
            bestDistance = neighborDistance;
            bestNeighbor = neighborAddress;
        }
    }
    
    // STEP 4 AUDIT: Log final decision
    if (isSourceNode && simTime() >= 15.0) {
        if (!bestNeighbor.isUnspecified()) {
            std::cout << "  ──────────────────────────────────────────\n";
            std::cout << "  ✓ SELECTED: " << bestNeighbor << "\n";
            std::cout << "    Distance to dest: " << bestDistance << " m\n";
            std::cout << "    Estimated delay: " << bestDelay << " s\n";
            std::cout << "    Tiebreaker activations (total): " << tiebreakerActivations << "\n";
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::flush;
        } else {
            std::cout << "  ✗ NO GREEDY NEIGHBOR (switching to perimeter)\n";
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::flush;
        }
    }
    
    if (bestNeighbor.isUnspecified()) {
        EV_DEBUG << "Switching to perimeter routing: destination = " << destination << endl;
        if (displayBubbles && hasGUI())
            getContainingNode(host)->bubble("Switching to perimeter routing");
        gpsrOption->setRoutingMode(GPSR_PERIMETER_ROUTING);
        gpsrOption->setPerimeterRoutingStartPosition(selfPosition);
        gpsrOption->setPerimeterRoutingForwardPosition(selfPosition);
        gpsrOption->setCurrentFaceFirstSenderAddress(selfAddress);
        gpsrOption->setCurrentFaceFirstReceiverAddress(L3Address());
        return findPerimeterRoutingNextHop(destination, gpsrOption);
    }
    else
        return bestNeighbor;
}

L3Address QueueGpsr::findPerimeterRoutingNextHop(const L3Address& destination, GpsrOption *gpsrOption)
{
    EV_DEBUG << "Finding next hop using perimeter routing: destination = " << destination << endl;
    L3Address selfAddress = getSelfAddress();
    Coord selfPosition = mobility->getCurrentPosition();
    Coord perimeterRoutingStartPosition = gpsrOption->getPerimeterRoutingStartPosition();
    Coord destinationPosition = gpsrOption->getDestinationPosition();
    double selfDistance = (destinationPosition - selfPosition).length();
    double perimeterRoutingStartDistance = (destinationPosition - perimeterRoutingStartPosition).length();
    if (selfDistance < perimeterRoutingStartDistance) {
        EV_DEBUG << "Switching to greedy routing: destination = " << destination << endl;
        if (displayBubbles && hasGUI())
            getContainingNode(host)->bubble("Switching to greedy routing");
        gpsrOption->setRoutingMode(GPSR_GREEDY_ROUTING);
        gpsrOption->setPerimeterRoutingStartPosition(Coord());
        gpsrOption->setPerimeterRoutingForwardPosition(Coord());
        gpsrOption->setCurrentFaceFirstSenderAddress(L3Address());
        gpsrOption->setCurrentFaceFirstReceiverAddress(L3Address());
        return findGreedyRoutingNextHop(destination, gpsrOption);
    }
    else {
        const L3Address& firstSenderAddress = gpsrOption->getCurrentFaceFirstSenderAddress();
        const L3Address& firstReceiverAddress = gpsrOption->getCurrentFaceFirstReceiverAddress();
        auto senderNeighborAddress = gpsrOption->getSenderAddress();
        auto neighborAngle = senderNeighborAddress.isUnspecified() ? getVectorAngle(destinationPosition - mobility->getCurrentPosition()) : getNeighborAngle(senderNeighborAddress);
        L3Address selectedNeighborAddress;
        std::vector<L3Address> neighborAddresses = getPlanarNeighborsCounterClockwise(neighborAngle);
        for (auto& neighborAddress : neighborAddresses) {
            Coord neighborPosition = getNeighborPosition(neighborAddress);
            Coord intersection = computeIntersectionInsideLineSegments(perimeterRoutingStartPosition, destinationPosition, selfPosition, neighborPosition);
            if (std::isnan(intersection.x)) {
                selectedNeighborAddress = neighborAddress;
                break;
            }
            else {
                EV_DEBUG << "Edge to next hop intersects: intersection = " << intersection << ", nextNeighbor = " << selectedNeighborAddress << ", firstSender = " << firstSenderAddress << ", firstReceiver = " << firstReceiverAddress << ", destination = " << destination << endl;
                gpsrOption->setCurrentFaceFirstSenderAddress(selfAddress);
                gpsrOption->setCurrentFaceFirstReceiverAddress(L3Address());
                gpsrOption->setPerimeterRoutingForwardPosition(intersection);
            }
        }
        if (selectedNeighborAddress.isUnspecified()) {
            EV_DEBUG << "No suitable planar graph neighbor found in perimeter routing: firstSender = " << firstSenderAddress << ", firstReceiver = " << firstReceiverAddress << ", destination = " << destination << endl;
            return L3Address();
        }
        else if (firstSenderAddress == selfAddress && firstReceiverAddress == selectedNeighborAddress) {
            EV_DEBUG << "End of perimeter reached: firstSender = " << firstSenderAddress << ", firstReceiver = " << firstReceiverAddress << ", destination = " << destination << endl;
            if (displayBubbles && hasGUI())
                getContainingNode(host)->bubble("End of perimeter reached");
            return L3Address();
        }
        else {
            if (gpsrOption->getCurrentFaceFirstReceiverAddress().isUnspecified())
                gpsrOption->setCurrentFaceFirstReceiverAddress(selectedNeighborAddress);
            return selectedNeighborAddress;
        }
    }
}

//
// routing
//

INetfilter::IHook::Result QueueGpsr::routeDatagram(Packet *datagram, GpsrOption *gpsrOption)
{
    const auto& networkHeader = getNetworkProtocolHeader(datagram);
    const L3Address& source = networkHeader->getSourceAddress();
    const L3Address& destination = networkHeader->getDestinationAddress();
    EV_INFO << "Finding next hop: source = " << source << ", destination = " << destination << endl;
    auto nextHop = findNextHop(destination, gpsrOption);
    
    // DEBUG: Log ALL routing decisions with comprehensive details
    std::cout << "[ROUTE] t=" << simTime() << " " << getContainingNode(this)->getFullName() 
              << ": src=" << source << " dst=" << destination << " nextHop=" << nextHop << std::endl;
    
    datagram->addTagIfAbsent<NextHopAddressReq>()->setNextHopAddress(nextHop);
    if (nextHop.isUnspecified()) {
        EV_WARN << "No next hop found, dropping packet: source = " << source << ", destination = " << destination << endl;
        std::cout << "[DROP] " << getContainingNode(this)->getFullName() << ": No next hop for dst=" << destination << std::endl;
        if (displayBubbles && hasGUI())
            getContainingNode(host)->bubble("No next hop found, dropping packet");
        return DROP;
    }
    else {
        EV_INFO << "Next hop found: source = " << source << ", destination = " << destination << ", nextHop: " << nextHop << endl;
        gpsrOption->setSenderAddress(getSelfAddress());
        auto networkInterface = CHK(interfaceTable->findInterfaceByName(outputInterface));
        datagram->addTagIfAbsent<InterfaceReq>()->setInterfaceId(networkInterface->getInterfaceId());
        return ACCEPT;
    }
}

void QueueGpsr::setGpsrOptionOnNetworkDatagram(Packet *packet, const Ptr<const NetworkHeaderBase>& networkHeader, GpsrOption *gpsrOption)
{
    packet->trimFront();
#ifdef INET_WITH_IPv4
    if (dynamicPtrCast<const Ipv4Header>(networkHeader)) {
        auto ipv4Header = removeNetworkProtocolHeader<Ipv4Header>(packet);
        gpsrOption->setType(IPOPTION_TLV_GPSR);
        B oldHlen = ipv4Header->calculateHeaderByteLength();
        ASSERT(ipv4Header->getHeaderLength() == oldHlen);
        ipv4Header->addOption(gpsrOption);
        B newHlen = ipv4Header->calculateHeaderByteLength();
        ipv4Header->setHeaderLength(newHlen);
        ipv4Header->addChunkLength(newHlen - oldHlen);
        ipv4Header->setTotalLengthField(ipv4Header->getTotalLengthField() + newHlen - oldHlen);
        insertNetworkProtocolHeader(packet, Protocol::ipv4, ipv4Header);
    }
    else
#endif
#ifdef INET_WITH_IPv6
    if (dynamicPtrCast<const Ipv6Header>(networkHeader)) {
        auto ipv6Header = removeNetworkProtocolHeader<Ipv6Header>(packet);
        gpsrOption->setType(IPv6TLVOPTION_TLV_GPSR);
        B oldHlen = ipv6Header->calculateHeaderByteLength();
        Ipv6HopByHopOptionsHeader *hdr = check_and_cast_nullable<Ipv6HopByHopOptionsHeader *>(ipv6Header->findExtensionHeaderByTypeForUpdate(IP_PROT_IPv6EXT_HOP));
        if (hdr == nullptr) {
            hdr = new Ipv6HopByHopOptionsHeader();
            hdr->setByteLength(B(8));
            ipv6Header->addExtensionHeader(hdr);
        }
        hdr->getTlvOptionsForUpdate().appendTlvOption(gpsrOption);
        hdr->setByteLength(B(utils::roundUp(2 + B(hdr->getTlvOptions().getLength()).get(), 8)));
        B newHlen = ipv6Header->calculateHeaderByteLength();
        ipv6Header->addChunkLength(newHlen - oldHlen);
        insertNetworkProtocolHeader(packet, Protocol::ipv6, ipv6Header);
    }
    else
#endif
#ifdef INET_WITH_NEXTHOP
    if (dynamicPtrCast<const NextHopForwardingHeader>(networkHeader)) {
        auto nextHopHeader = removeNetworkProtocolHeader<NextHopForwardingHeader>(packet);
        gpsrOption->setType(NEXTHOP_TLVOPTION_TLV_GPSR);
        int oldHlen = nextHopHeader->getTlvOptions().getLength();
        nextHopHeader->getTlvOptionsForUpdate().appendTlvOption(gpsrOption);
        int newHlen = nextHopHeader->getTlvOptions().getLength();
        nextHopHeader->addChunkLength(B(newHlen - oldHlen));
        insertNetworkProtocolHeader(packet, Protocol::nextHopForwarding, nextHopHeader);
    }
    else
#endif
    {
    }
}

const GpsrOption *QueueGpsr::findGpsrOptionInNetworkDatagram(const Ptr<const NetworkHeaderBase>& networkHeader) const
{
    const GpsrOption *gpsrOption = nullptr;

#ifdef INET_WITH_IPv4
    if (auto ipv4Header = dynamicPtrCast<const Ipv4Header>(networkHeader)) {
        gpsrOption = check_and_cast_nullable<const GpsrOption *>(ipv4Header->findOptionByType(IPOPTION_TLV_GPSR));
    }
    else
#endif
#ifdef INET_WITH_IPv6
    if (auto ipv6Header = dynamicPtrCast<const Ipv6Header>(networkHeader)) {
        const Ipv6HopByHopOptionsHeader *hdr = check_and_cast_nullable<const Ipv6HopByHopOptionsHeader *>(ipv6Header->findExtensionHeaderByType(IP_PROT_IPv6EXT_HOP));
        if (hdr != nullptr) {
            int i = (hdr->getTlvOptions().findByType(IPv6TLVOPTION_TLV_GPSR));
            if (i >= 0)
                gpsrOption = check_and_cast<const GpsrOption *>(hdr->getTlvOptions().getTlvOption(i));
        }
    }
    else
#endif
#ifdef INET_WITH_NEXTHOP
    if (auto nextHopHeader = dynamicPtrCast<const NextHopForwardingHeader>(networkHeader)) {
        int i = (nextHopHeader->getTlvOptions().findByType(NEXTHOP_TLVOPTION_TLV_GPSR));
        if (i >= 0)
            gpsrOption = check_and_cast<const GpsrOption *>(nextHopHeader->getTlvOptions().getTlvOption(i));
    }
    else
#endif
    {
    }
    return gpsrOption;
}

GpsrOption *QueueGpsr::findGpsrOptionInNetworkDatagramForUpdate(const Ptr<NetworkHeaderBase>& networkHeader)
{
    GpsrOption *gpsrOption = nullptr;

#ifdef INET_WITH_IPv4
    if (auto ipv4Header = dynamicPtrCast<Ipv4Header>(networkHeader)) {
        gpsrOption = check_and_cast_nullable<GpsrOption *>(ipv4Header->findMutableOptionByType(IPOPTION_TLV_GPSR));
    }
    else
#endif
#ifdef INET_WITH_IPv6
    if (auto ipv6Header = dynamicPtrCast<Ipv6Header>(networkHeader)) {
        Ipv6HopByHopOptionsHeader *hdr = check_and_cast_nullable<Ipv6HopByHopOptionsHeader *>(ipv6Header->findExtensionHeaderByTypeForUpdate(IP_PROT_IPv6EXT_HOP));
        if (hdr != nullptr) {
            int i = (hdr->getTlvOptions().findByType(IPv6TLVOPTION_TLV_GPSR));
            if (i >= 0)
                gpsrOption = check_and_cast<GpsrOption *>(hdr->getTlvOptionsForUpdate().getTlvOptionForUpdate(i));
        }
    }
    else
#endif
#ifdef INET_WITH_NEXTHOP
    if (auto nextHopHeader = dynamicPtrCast<NextHopForwardingHeader>(networkHeader)) {
        int i = (nextHopHeader->getTlvOptions().findByType(NEXTHOP_TLVOPTION_TLV_GPSR));
        if (i >= 0)
            gpsrOption = check_and_cast<GpsrOption *>(nextHopHeader->getTlvOptionsForUpdate().getTlvOptionForUpdate(i));
    }
    else
#endif
    {
    }
    return gpsrOption;
}

const GpsrOption *QueueGpsr::getGpsrOptionFromNetworkDatagram(const Ptr<const NetworkHeaderBase>& networkHeader) const
{
    const GpsrOption *gpsrOption = findGpsrOptionInNetworkDatagram(networkHeader);
    if (gpsrOption == nullptr)
        throw cRuntimeError("Gpsr option not found in datagram!");
    return gpsrOption;
}

GpsrOption *QueueGpsr::getGpsrOptionFromNetworkDatagramForUpdate(const Ptr<NetworkHeaderBase>& networkHeader)
{
    GpsrOption *gpsrOption = findGpsrOptionInNetworkDatagramForUpdate(networkHeader);
    if (gpsrOption == nullptr)
        throw cRuntimeError("Gpsr option not found in datagram!");
    return gpsrOption;
}

//
// netfilter
//

INetfilter::IHook::Result QueueGpsr::datagramPreRoutingHook(Packet *datagram)
{
    Enter_Method("datagramPreRoutingHook");
    const auto& networkHeader = getNetworkProtocolHeader(datagram);
    const L3Address& destination = networkHeader->getDestinationAddress();
    if (destination.isMulticast() || destination.isBroadcast() || routingTable->isLocalAddress(destination))
        return ACCEPT;
    else {
        // KLUDGE this allows overwriting the GPSR option inside
        auto gpsrOption = const_cast<GpsrOption *>(getGpsrOptionFromNetworkDatagram(networkHeader));
        return routeDatagram(datagram, gpsrOption);
    }
}

INetfilter::IHook::Result QueueGpsr::datagramLocalOutHook(Packet *packet)
{
    Enter_Method("datagramLocalOutHook");
    const auto& networkHeader = getNetworkProtocolHeader(packet);
    const L3Address& destination = networkHeader->getDestinationAddress();
    if (destination.isMulticast() || destination.isBroadcast() || routingTable->isLocalAddress(destination)) {
        return ACCEPT;
    } else {
        // Track local TX backlog: increment when packet enters routing
        if (enableQueueDelay) {
            localTxBacklogBytes += packet->getByteLength();
        }
        
        GpsrOption *gpsrOption = createGpsrOption(networkHeader->getDestinationAddress());
        setGpsrOptionOnNetworkDatagram(packet, networkHeader, gpsrOption);
        return routeDatagram(packet, gpsrOption);
    }
}

//
// lifecycle
//

void QueueGpsr::finish()
{
    // Record tiebreaker statistics
    recordScalar("tiebreakerActivations", tiebreakerActivations);
    recordScalar("greedySelections", greedySelections);
    if (greedySelections > 0) {
        double tiebreakerRatio = (double)tiebreakerActivations / (double)greedySelections;
        recordScalar("tiebreakerRatio", tiebreakerRatio);
    }
}

void QueueGpsr::handleStartOperation(LifecycleOperation *operation)
{
    configureInterfaces();
    storeSelfPositionInGlobalRegistry();
    scheduleBeaconTimer();
}

void QueueGpsr::handleStopOperation(LifecycleOperation *operation)
{
    // TODO send a beacon to remove ourself from peers neighbor position table
    neighborPositionTable.clear();
    cancelEvent(beaconTimer);
    cancelEvent(purgeNeighborsTimer);
}

void QueueGpsr::handleCrashOperation(LifecycleOperation *operation)
{
    neighborPositionTable.clear();
    cancelEvent(beaconTimer);
    cancelEvent(purgeNeighborsTimer);
}

//
// notification
//

void QueueGpsr::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    Enter_Method("%s", cComponent::getSignalName(signalID));

    if (signalID == linkBrokenSignal) {
        EV_WARN << "Received link break" << endl;
        // TODO remove the neighbor
    }
}

} // namespace researchproject

