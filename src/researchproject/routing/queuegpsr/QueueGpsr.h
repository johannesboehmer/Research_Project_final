//
// Copyright (C) 2013 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __RESEARCHPROJECT_QUEUEGPSR_H
#define __RESEARCHPROJECT_QUEUEGPSR_H

#include "inet/common/ModuleRefByPar.h"
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/packet/Packet.h"
#include "inet/mobility/contract/IMobility.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/contract/IL3AddressType.h"
#include "inet/networklayer/contract/INetfilter.h"
#include "inet/networklayer/contract/IRoutingTable.h"
#include "inet/routing/base/RoutingProtocolBase.h"
#include "QueueGpsr_m.h"
#include "inet/routing/gpsr/PositionTable.h"
#include "inet/transportlayer/udp/UdpHeader_m.h"

// Use INET namespace to avoid symbol conflicts
using namespace omnetpp;
using namespace inet;

namespace researchproject {

/**
 * QueueGpsr - Local copy of INET GPSR with delay-aware tiebreaker
 * 
 * This class implements the Greedy Perimeter Stateless Routing for Wireless Networks.
 * The implementation supports both GG and RNG planarization algorithms.
 *
 * Extended with queue-aware neighbor selection for improved routing under congestion.
 *
 * For more information on the routing algorithm, see the GPSR paper
 * http://www.eecs.harvard.edu/~htk/publication/2000-mobi-karp-kung.pdf
 */
class QueueGpsr : public RoutingProtocolBase, public cListener, public NetfilterBase::HookBase
{
  private:
    // GPSR parameters
    GpsrPlanarizationMode planarizationMode = static_cast<GpsrPlanarizationMode>(-1);
    const char *interfaces = nullptr;
    simtime_t beaconInterval;
    simtime_t maxJitter;
    simtime_t neighborValidityInterval;
    bool displayBubbles;

    // context
    cModule *host = nullptr;
    opp_component_ptr<IMobility> mobility;
    const IL3AddressType *addressType = nullptr;
    ModuleRefByPar<IInterfaceTable> interfaceTable;
    const char *outputInterface = nullptr;
    ModuleRefByPar<IRoutingTable> routingTable; // TODO delete when necessary functions are moved to interface table
    ModuleRefByPar<INetfilter> networkProtocol;
    PositionTable& globalPositionTable = SIMULATION_SHARED_VARIABLE(globalPositionTable); // KLUDGE implement position registry protocol

    // packet size
    int positionByteLength = -1;

    // internal
    cMessage *beaconTimer = nullptr;
    cMessage *purgeNeighborsTimer = nullptr;
    cMessage *queueMonitorTimer = nullptr;  // STEP 2 AUDIT: periodic queue monitoring
    cMessage *neighborTableDebugTimer = nullptr;  // STEP 4 AUDIT: one-shot neighbor table dump
    cMessage *preloadDurabilityTimer = nullptr;  // PRELOAD DURABILITY: monitor congested relay queue
    PositionTable neighborPositionTable;
    
  // Phase 3: neighbor TX backlog with timestamp for aging
  struct NeighborQueueInfo {
      uint32_t bytes;
      simtime_t lastUpdate;
  };
  std::map<L3Address, NeighborQueueInfo> neighborTxBacklogBytes;
  bool enableQueueDelay = false;
  
  // Local transmit backlog counter (UDP/IP level, avoids MAC queue API issues)
  mutable unsigned long localTxBacklogBytes = 0;
    
    // Delay tiebreaker parameters (Phase 2/3)
    bool enableDelayTiebreaker = false;
    double distanceEqualityThreshold = 1.0;  // meters - when distances considered equal
    double delayEstimationFactor = 0.001;    // seconds per meter (simulated delay)
    
    // Delay tiebreaker statistics
    simsignal_t tiebreakerActivationsSignal;
    long tiebreakerActivations = 0;
    long greedySelections = 0;

    // CPU offload capacity (Phase 4)
    double cpuTotalHz = 0;           // total CPU capacity in Hz
    double offloadShareMin = 0;      // min fraction of CPU for offloading
    double offloadShareMax = 0;      // max fraction of CPU for offloading
    double cpuOffloadHz = 0;         // effective CPU capacity available for offloading (initialized randomly)
    double cpuOffloadBacklogCycles = 0;  // current backlog of offloaded work in CPU cycles
    
    // Neighbor CPU offload capacity tracking
    struct NeighborCpuInfo {
        double cpuOffloadHz;
        double cpuOffloadBacklogCycles;
        simtime_t lastUpdate;
    };
    std::map<L3Address, NeighborCpuInfo> neighborCpuCapacity;

  public:
    QueueGpsr();
    virtual ~QueueGpsr();

  protected:
    // module interface
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessageWhenUp(cMessage *message) override;

  private:
    // handling messages
    void processSelfMessage(cMessage *message);
    void processMessage(cMessage *message);

    // handling beacon timers
    void scheduleBeaconTimer();
    void processBeaconTimer();

    // handling purge neighbors timers
    void schedulePurgeNeighborsTimer();
    void processPurgeNeighborsTimer();

    // STEP 2 AUDIT: queue monitoring timer
    void processQueueMonitorTimer();
    
    // STEP 4 AUDIT: neighbor table debug timer
    void processNeighborTableDebug();
    
    // PRELOAD DURABILITY: monitor congested relay queue (t=20-35s)
    void processPreloadDurabilityTimer();

    // handling UDP packets
    void sendUdpPacket(Packet *packet);
    void processUdpPacket(Packet *packet);

    // handling beacons
    const Ptr<GpsrBeacon> createBeacon();
    void sendBeacon(const Ptr<GpsrBeacon>& beacon);
    void processBeacon(Packet *packet);

    // handling packets
    GpsrOption *createGpsrOption(L3Address destination);
    int computeOptionLength(GpsrOption *gpsrOption);
    void setGpsrOptionOnNetworkDatagram(Packet *packet, const Ptr<const NetworkHeaderBase>& networkHeader, GpsrOption *gpsrOption);

    // returns nullptr if not found
    GpsrOption *findGpsrOptionInNetworkDatagramForUpdate(const Ptr<NetworkHeaderBase>& networkHeader);
    const GpsrOption *findGpsrOptionInNetworkDatagram(const Ptr<const NetworkHeaderBase>& networkHeader) const;

    // throws an error when not found
    GpsrOption *getGpsrOptionFromNetworkDatagramForUpdate(const Ptr<NetworkHeaderBase>& networkHeader);
    const GpsrOption *getGpsrOptionFromNetworkDatagram(const Ptr<const NetworkHeaderBase>& networkHeader) const;

    // configuration
    void configureInterfaces();

    // position
    Coord lookupPositionInGlobalRegistry(const L3Address& address) const;
    void storePositionInGlobalRegistry(const L3Address& address, const Coord& position) const;
    void storeSelfPositionInGlobalRegistry() const;
    Coord computeIntersectionInsideLineSegments(Coord& begin1, Coord& end1, Coord& begin2, Coord& end2) const;
    Coord getNeighborPosition(const L3Address& address) const;

    // angle
    double getVectorAngle(Coord vector) const;
    double getNeighborAngle(const L3Address& address) const;

    // address
    std::string getHostName() const;
    L3Address getSelfAddress() const;
    L3Address getSenderNeighborAddress(const Ptr<const NetworkHeaderBase>& networkHeader) const;

    // neighbor
    simtime_t getNextNeighborExpiration();
    void purgeNeighbors();
    std::vector<L3Address> getPlanarNeighbors() const;
    std::vector<L3Address> getPlanarNeighborsCounterClockwise(double startAngle) const;
    
    // Delay tiebreaker helper (Phase 2/3)
    double estimateNeighborDelay(const L3Address& address) const;
  // Phase 3 helper: read local TX backlog bytes from MAC queue
  unsigned long getLocalTxBacklogBytes() const;

    // Diagnostic: enumerate MAC submodules and report which implement IPacketCollection
    void auditMacQueues() const;

    // next hop
    L3Address findNextHop(const L3Address& destination, GpsrOption *gpsrOption);
    L3Address findGreedyRoutingNextHop(const L3Address& destination, GpsrOption *gpsrOption);
    L3Address findPerimeterRoutingNextHop(const L3Address& destination, GpsrOption *gpsrOption);

    // routing
    Result routeDatagram(Packet *datagram, GpsrOption *gpsrOption);

    // netfilter
    virtual Result datagramPreRoutingHook(Packet *datagram) override;
    virtual Result datagramForwardHook(Packet *datagram) override { return ACCEPT; }
    virtual Result datagramPostRoutingHook(Packet *datagram) override { return ACCEPT; }
    virtual Result datagramLocalInHook(Packet *datagram) override { return ACCEPT; }
    virtual Result datagramLocalOutHook(Packet *datagram) override;

    // lifecycle
    virtual void finish() override;
    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;

    // notification
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;
};

} // namespace researchproject

#endif

