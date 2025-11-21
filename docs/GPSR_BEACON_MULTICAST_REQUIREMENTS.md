# GPSR Beacon and Multicast Requirements

**Purpose**: Document INET GPSR beacon delivery mechanism, multicast group membership, and initialization requirements.

**Status**: Research-only (no configuration changes yet)

---

## Beacon Delivery Mechanism

### Overview
INET GPSR uses UDP-based beaconing with link-local multicast for neighbor discovery. All nodes must join a specific multicast group to receive beacons.

---

## Key Code Locations

### Beacon Creation and Sending
**File**: `inet4.5/src/inet/routing/gpsr/Gpsr.cc`

#### `sendBeacon()` (lines 219-234)
```cpp
void Gpsr::sendBeacon(const Ptr<GpsrBeacon>& beacon)
{
    EV_INFO << "Sending beacon: address = " << beacon->getAddress() 
            << ", position = " << beacon->getPosition() << endl;
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
    sendUdpPacket(udpPacket);
}
```

**Key observations**:
- **Transport**: UDP with `GPSR_UDP_PORT` (both source and destination)
- **Destination**: `addressType->getLinkLocalManetRoutersMulticastAddress()`
- **Hop limit**: 255 (link-local only, not forwarded)
- **Protocol tag**: `Protocol::manet`

### Beacon Timer Scheduling
**File**: `inet4.5/src/inet/routing/gpsr/Gpsr.cc`

#### `scheduleBeaconTimer()` (lines 138-142)
```cpp
void Gpsr::scheduleBeaconTimer()
{
    EV_DEBUG << "Scheduling beacon timer" << endl;
    scheduleAfter(beaconInterval + uniform(-1, 1) * maxJitter, beaconTimer);
}
```

#### `processBeaconTimer()` (lines 144-153)
```cpp
void Gpsr::processBeaconTimer()
{
    EV_DEBUG << "Processing beacon timer" << endl;
    const L3Address selfAddress = getSelfAddress();
    if (!selfAddress.isUnspecified()) {
        sendBeacon(createBeacon());
        storeSelfPositionInGlobalRegistry();
    }
    scheduleBeaconTimer();
    schedulePurgeNeighborsTimer();
}
```

**Key observations**:
- Beacons sent periodically: `beaconInterval` ± jitter
- Only sent if node has valid L3Address (after network initialization)
- Neighbor purging scheduled after each beacon

---

## Multicast Group Membership

### Interface Configuration
**File**: `inet4.5/src/inet/routing/gpsr/Gpsr.cc`

#### `configureInterfaces()` (lines 276-284)
```cpp
void Gpsr::configureInterfaces()
{
    // join multicast groups
    cPatternMatcher interfaceMatcher(interfaces, false, true, false);
    for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
        NetworkInterface *networkInterface = interfaceTable->getInterface(i);
        if (networkInterface->isMulticast() && interfaceMatcher.matches(networkInterface->getInterfaceName()))
            networkInterface->joinMulticastGroup(addressType->getLinkLocalManetRoutersMulticastAddress());
    }
}
```

**Key observations**:
- **Initialization point**: `configureInterfaces()` called during module startup
- **Interface filter**: Only multicast-capable interfaces matching `interfaces` parameter
- **Multicast group**: `getLinkLocalManetRoutersMulticastAddress()`
- **Requirement**: All GPSR nodes MUST join this group to receive beacons

---

## Multicast Address Constants

### IPv4 Link-Local MANET Routers
**File**: `inet4.5/src/inet/networklayer/contract/ipv4/Ipv4Address.cc`

```cpp
const Ipv4Address Ipv4Address::LL_MANET_ROUTERS("224.0.0.109");
```

**File**: `inet4.5/src/inet/networklayer/contract/ipv4/Ipv4AddressType.h`

```cpp
virtual L3Address getLinkLocalManetRoutersMulticastAddress() const override { 
    return Ipv4Address::LL_MANET_ROUTERS; 
}
```

**Multicast address**: `224.0.0.109`
- **IANA assigned**: RFC-compliant link-local scope multicast for MANET routing
- **Hop limit**: 255 (link-local, not routed beyond one hop)
- **Purpose**: All MANET routers on the link receive beacons

### IPv6 Link-Local MANET Routers
**File**: `inet4.5/src/inet/networklayer/contract/ipv6/Ipv6AddressType.h`

```cpp
virtual L3Address getLinkLocalManetRoutersMulticastAddress() const override { 
    return Ipv6Address::LL_MANET_ROUTERS; 
}
```

**Note**: IPv6 uses `Ipv6Address::LL_MANET_ROUTERS` (address not examined, but follows same pattern)

---

## Beacon Reception

### Processing Flow
**File**: `inet4.5/src/inet/routing/gpsr/Gpsr.cc`

#### `processBeacon()` (lines 237-242)
```cpp
void Gpsr::processBeacon(Packet *packet)
{
    const auto& beacon = packet->peekAtFront<GpsrBeacon>();
    EV_INFO << "Processing beacon: address = " << beacon->getAddress() 
            << ", position = " << beacon->getPosition() << endl;
    neighborPositionTable.setPosition(beacon->getAddress(), beacon->getPosition());
    delete packet;
}
```

**Key observations**:
- Updates `neighborPositionTable` with sender's position
- Neighbor entry timestamp updated implicitly by `setPosition()`
- No acknowledgment sent (beacons are one-way)

---

## Initialization Sequence

### GPSR Module Startup (stage-based)
**File**: `inet4.5/src/inet/routing/gpsr/Gpsr.cc`

1. **Stage 0-5**: Network layer initialization (IP address assignment)
2. **Stage 6+**: GPSR initialization
   - `initialize()` calls `configureInterfaces()` → **Multicast group join**
   - `scheduleBeaconTimer()` → First beacon scheduled
3. **First beacon**: Sent after `beaconInterval` + jitter
4. **Neighbor discovery**: Begins when first beacon received

**Critical requirement**: Node L3Address must be assigned BEFORE first beacon can be sent.

---

## Configuration Parameters

### GPSR Parameters (from Gpsr.ned)
**File**: `inet4.5/src/inet/routing/gpsr/Gpsr.ned`

```ned
double beaconInterval @unit(s) = default(10s);
double maxJitter @unit(s) = default(5s);
double neighborValidityInterval @unit(s) = default(45s);
string interfaces = default("*");
```

**Current baseline configuration** (`simulations/baseline_gpsr/omnetpp.ini`):
```ini
**.gpsr.beaconInterval = 10s
**.gpsr.maxJitter = 5s
**.gpsr.neighborValidityInterval = 45s
**.gpsr.interfaces = "wlan0"
```

**Beacon timing**:
- First beacon: 10s ± 5s after node initialization
- Subsequent beacons: Every 10s ± 5s
- Neighbor expiry: 45s after last beacon received

---

## Potential Issues with Current Baseline

### Issue 1: Multicast Group Join Timing
**Symptom**: Zero packet reception at application layer

**Hypothesis**:
- GPSR's `configureInterfaces()` joins multicast group on `wlan0`
- But timing may conflict with IP address assignment or interface activation
- If join happens BEFORE IP configuration completes, multicast membership may fail silently

**Evidence needed**:
- Check logs for "joinMulticastGroup" calls
- Verify multicast routing table entries in network layer
- Confirm beacon transmission vs. beacon reception counts

### Issue 2: UDP Port Binding
**Symptom**: Beacons sent but not received

**Hypothesis**:
- GPSR uses UDP port `GPSR_UDP_PORT` for beacons
- If UDP socket not properly bound, beacons may be dropped
- Or UDP layer may not forward multicast packets to GPSR module

**Evidence needed**:
- Verify UDP socket creation and binding in GPSR initialization
- Check UDP layer statistics for dropped packets

### Issue 3: Interface Name Mismatch
**Current configuration**:
```ini
**.gpsr.interfaces = "wlan0"
```

**Potential issue**:
- GpsrRouter uses `wlan[*]` array (not `wlan0` specifically)
- If interface named differently, multicast join fails silently
- INET auto-naming may assign "wlan[0]" instead of "wlan0"

**Verification**:
- Check actual interface names in simulation logs
- Try `interfaces = "*"` to match all multicast interfaces (INET default)

---

## INET GPSR Example Configuration

### Example: manetrouting/gpsr
**File**: `inet4.5/examples/manetrouting/gpsr/omnetpp.ini`

```ini
**.gpsr.interfaces = "wlan0"
**.gpsr.beaconInterval = 10s
**.gpsr.maxJitter = 5s
**.gpsr.neighborValidityInterval = 45s
```

**Network file**: `inet4.5/examples/manetrouting/gpsr/GPSRNetwork.ned`
- Uses: `inet.node.gpsr.GpsrRouter` (same as our baseline)
- Radio: `Ieee80211ScalarRadio` (same as our baseline)
- Traffic: `PingApp` (different from our UdpBasicApp)

**Key difference**: INET example uses `PingApp`, not `UdpBasicApp`

---

## Comparison: PingApp vs UdpBasicApp

### PingApp Configuration (INET GPSR example)
```ini
**.host[0].numApps = 1
**.host[0].app[0].typename = "PingApp"
**.host[0].app[0].destAddr = "host[1](ipv4)"
**.host[0].app[0].startTime = 100s
```

**Address format**: `"host[1](ipv4)"` includes protocol hint

### UdpBasicApp Configuration (Current baseline)
```ini
**.app[0].typename = "UdpBasicApp"
**.host[0].app[0].destAddresses = "host[15]"
**.host[0].app[0].startTime = 0s
```

**Address format**: `"host[15]"` without protocol hint

**Potential issue**:
- UdpBasicApp may need address resolution via L3AddressResolver
- Format `"host[15]"` may fail to resolve without configurator support
- INET PingApp uses explicit `(ipv4)` hint for resolution

---

## Recommendations for Baseline Verification

### Check 1: Verify Interface Names
**Command** (add to omnetpp.ini temporarily):
```ini
**.wlan[*].interfaceTableModule = "^.interfaceTable"
**.gpsr.interfaces = "*"  # Match all interfaces instead of "wlan0"
```

**Rationale**: Eliminate interface name mismatch as failure cause.

### Check 2: Verify Multicast Membership
**Log analysis** (after simulation):
```bash
grep "joinMulticastGroup" results/*.log
grep "224.0.0.109" results/*.log
```

**Expected**: Each host joins 224.0.0.109 on wlan[0] during initialization.

### Check 3: Compare Beacon Sent vs Received
**Query** (after simulation):
```bash
opp_scavetool query -f 'module =~ *.host[*].gpsr AND name =~ *beacon*' results/*.sca
```

**Expected**: beaconsSent ≈ beaconsReceived × (numHosts - 1)

### Check 4: Verify UDP Socket Binding
**Code review** (INET source):
- Check `Gpsr::initialize()` for UDP socket creation
- Verify UDP module routes multicast packets to GPSR

---

## Summary

### GPSR Beacon Mechanism
- **Transport**: UDP with link-local multicast
- **Multicast address**: 224.0.0.109 (IPv4) or IPv6 equivalent
- **Initialization**: Nodes join multicast group on all matching interfaces
- **Timing**: Beacons sent every 10s ± 5s (configurable)

### Critical Requirements
1. **Multicast group join**: All nodes must join `LL_MANET_ROUTERS` multicast group
2. **Interface configuration**: `interfaces` parameter must match actual interface names
3. **IP address assignment**: Node must have valid L3Address before sending beacons
4. **UDP binding**: GPSR must bind UDP socket to receive multicast packets

### Next Diagnostic Steps
1. Verify interface names in simulation logs
2. Check multicast group membership in network layer
3. Compare beacon sent vs. received statistics
4. Investigate UDP socket binding in GPSR initialization

### Potential Fixes (Not Applied Yet)
1. Change `interfaces = "*"` to match all interfaces
2. Use PingApp instead of UdpBasicApp for address resolution
3. Verify IP configurator assigns addresses before GPSR initialization
4. Switch to UnitDisk radio to eliminate PHY/MAC uncertainties

---

**References**:
- Gpsr.cc beacon code: lines 206-242
- configureInterfaces(): lines 276-284
- IPv4 multicast address: `inet4.5/src/inet/networklayer/contract/ipv4/Ipv4Address.cc:39`
- INET GPSR example: `inet4.5/examples/manetrouting/gpsr/omnetpp.ini`
