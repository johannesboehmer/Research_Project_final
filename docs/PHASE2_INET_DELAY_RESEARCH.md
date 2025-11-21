# Phase 2 Research: INET GPSR Delay Tiebreaker Implementation

## Date: 2025-11-06

## Research Objective
Identify patterns, extension points, and best practices in INET 4.5 for implementing a delay-aware tiebreaker in GPSR's next-hop selection.

---

## 1. GPSR Module Architecture Analysis

### Module Location
- **Path**: `inet4.5/src/inet/routing/gpsr/`
- **Key Files**:
  - `Gpsr.h` / `Gpsr.cc` - Main routing logic
  - `PositionTable.h` / `PositionTable.cc` - Neighbor position storage
  - `Gpsr.msg` - Beacon and option definitions
  - `Gpsr.ned` - Module parameters

### Class Hierarchy
```
RoutingProtocolBase (inet/routing/base)
    └── Gpsr (implements NetfilterBase::HookBase, cListener)
```

### Key Extension Points Identified

#### 1.1 Next-Hop Selection (Primary Integration Point)
**Method**: `L3Address Gpsr::findGreedyRoutingNextHop(const L3Address& destination, GpsrOption *gpsrOption)`
- **Location**: `Gpsr.cc:485-530`
- **Current Logic**:
  ```cpp
  // Iterates through neighbors
  for (auto& neighborAddress : neighborAddresses) {
      Coord neighborPosition = neighborPositionTable.getPosition(neighborAddress);
      double neighborDistance = (destinationPosition - neighborPosition).length();
      if (neighborDistance < bestDistance) {
          bestDistance = neighborDistance;
          bestNeighbor = neighborAddress;
      }
  }
  ```
- **Tiebreaker Insertion Point**: After distance comparison, when `neighborDistance == bestDistance`
- **Access**: All neighbor addresses via `neighborPositionTable.getAddresses()`

#### 1.2 Neighbor Information Storage
**Class**: `PositionTable`
- **Data Structure**: `std::map<L3Address, std::pair<simtime_t, Coord>>`
- **Current Storage**:
  - L3Address (neighbor ID)
  - simtime_t (timestamp of last beacon)
  - Coord (geographic position)
- **Extension Opportunity**: Enhance to store additional metrics (delay, queue length)

#### 1.3 Beacon Processing
**Method**: `void Gpsr::processBeacon(Packet *packet)`
- **Location**: `Gpsr.cc:~350-380`
- **Current Action**: Updates `neighborPositionTable` with position and timestamp
- **Extension Point**: Add delay/queue metrics to beacon payload

---

## 2. INET Timing and Statistics Patterns

### 2.1 Queue Statistics Access Patterns

**Interface Table Module Reference** (from AODV pattern):
```cpp
// In Gpsr.h
ModuleRefByPar<IInterfaceTable> interfaceTable;

// In Gpsr.cc initialize()
interfaceTable.reference(this, "interfaceTableModule", true);

// Access interface
NetworkInterface *ifEntry = interfaceTable->findInterfaceByName("wlan0");
```

**Queue Submodule Access Pattern**:
```cpp
// Navigate to MAC layer queue (common pattern)
cModule *wlanModule = ifEntry->getInterfaceModule();  // host.wlan[0]
cModule *queueModule = wlanModule->getSubmodule("queue");

// Access queue statistics
if (queueModule && queueModule->hasPar("queueLength")) {
    int queueLen = queueModule->par("queueLength").intValue();
}
```

### 2.2 Delay Measurement Patterns in INET

**Flow Measurement Tags** (inet/queueing/flow):
- `QueueingTimeTag` - Time spent in queues
- `DelayingTimeTag` - Total delay through network stack
- Pattern: Tags attached to packets, measured at collection points

**E2E Delay Pattern** (from UdpBasicApp/UdpSink):
```cpp
// Sender records send time in packet
packet->setTimestamp(simTime());

// Receiver calculates delay
simtime_t delay = simTime() - packet->getTimestamp();
```

### 2.3 Statistics Recording Conventions

**Signal-Based Statistics** (INET standard):
```cpp
// Define signal
simsignal_t delaySignal = registerSignal("queueDelay");

// Emit value
emit(delaySignal, delayValue);

// In NED file
@statistic[queueDelay](title="Queue Delay"; source=queueDelay; 
    record=vector,histogram,stats; unit=s);
```

**Naming Conventions Observed**:
- `packetReceived:count` / `packetSent:count`
- `queueLength:timeavg` / `queueLength:max`
- `endToEndDelay:mean` / `endToEndDelay:histogram`
- CamelCase for C++, colon notation for statistics

---

## 3. Neighbor Metric Collection Strategies

### Strategy 1: Beacon Piggybacking (INET Pattern)
**Observed in**: GPSR beacon mechanism, AODV HELLO messages

**Implementation**:
1. Extend `GpsrBeacon` message definition to include delay metric
2. Sender calculates local queue delay/length before sending beacon
3. Receiver extracts metric and stores in enhanced PositionTable
4. Tiebreaker uses stored metric during next-hop selection

**Pros**: Minimal overhead (reuses existing beacons), follows INET patterns
**Cons**: Metric freshness depends on beacon interval (10s default)

### Strategy 2: Promiscuous Overhearing (INET Pattern)
**Observed in**: TODO comments in Gpsr.cc suggesting future implementation

**Implementation**:
1. Listen to all transmissions from neighbors (promiscuous mode)
2. Infer queue state from packet frequency/inter-arrival times
3. Update neighbor metrics continuously

**Pros**: Real-time metrics
**Cons**: Complex, higher overhead, not needed for Phase 2

### Strategy 3: Simulated/Estimated Delay (Simplest for Phase 2)
**Pattern**: Used in INET examples for initial validation

**Implementation**:
1. Calculate estimated delay based on distance + constant factor
2. Or use random jitter to simulate varying delays
3. No protocol changes needed initially

**Pros**: Validates tiebreaker logic without protocol modification
**Cons**: Not realistic, placeholder only

---

## 4. Design Decision for Phase 2

### Chosen Approach: Beacon Piggybacking with Simulated Delay

**Rationale**:
- Mirrors INET's existing beacon mechanism
- Minimal protocol changes
- Allows validation of tiebreaker logic
- Sets foundation for Phase 3 (real queue integration)

### Implementation Plan

#### 4.1 Message Definition Changes
**File**: `src/researchproject/routing/gpsr/GpsrDelayBeacon.msg`
```msg
import inet.routing.gpsr.GpsrBeacon;

packet GpsrDelayBeacon extends GpsrBeacon {
    double estimatedDelay @unit(s) = 0s;  // One-hop delay estimate
}
```

#### 4.2 Enhanced Position Table
**File**: `src/researchproject/routing/gpsr/DelayPositionTable.h`
```cpp
class DelayPositionTable : public inet::PositionTable {
private:
    std::map<L3Address, double> addressToDelayMap;
public:
    void setDelay(const L3Address& address, double delay);
    double getDelay(const L3Address& address) const;
    bool hasDelay(const L3Address& address) const;
};
```

#### 4.3 Extended GPSR Module
**File**: `src/researchproject/routing/gpsr/GpsrWithDelayTiebreaker.cc`
```cpp
class GpsrWithDelayTiebreaker : public inet::Gpsr {
protected:
    bool enableDelayTiebreaker = false;
    DelayPositionTable delayNeighborTable;
    
    virtual L3Address findGreedyRoutingNextHop(
        const L3Address& destination, 
        GpsrOption *gpsrOption) override;
        
    virtual void processBeacon(Packet *packet) override;
    
    double estimateNeighborDelay(const L3Address& address);
};

// In findGreedyRoutingNextHop():
// When multiple neighbors have same distance:
if (std::abs(neighborDistance - bestDistance) < EPSILON) {
    // TIEBREAKER: Choose neighbor with lower delay
    double neighborDelay = delayNeighborTable.getDelay(neighborAddress);
    double bestDelay = delayNeighborTable.getDelay(bestNeighbor);
    if (neighborDelay < bestDelay) {
        bestNeighbor = neighborAddress;
        bestDelay = neighborDelay;
    }
}
```

#### 4.4 NED Module Definition
**File**: `src/researchproject/routing/gpsr/GpsrWithDelayTiebreaker.ned`
```ned
package researchproject.routing.gpsr;

import inet.routing.gpsr.Gpsr;

simple GpsrWithDelayTiebreaker extends Gpsr {
    parameters:
        bool enableDelayTiebreaker = default(true);
        double delayEstimationFactor = default(0.001);  // Simulated delay per meter
        
        @signal[delayTiebreakerActivations](type=long);
        @statistic[delayTiebreakerActivations](title="Delay Tiebreaker Activations";
            record=count,sum; interpolationmode=none);
}
```

---

## 5. Module Paths and Extension Points Summary

### Files to Create/Modify

**New Research Project Files**:
1. `src/researchproject/routing/gpsr/GpsrDelayBeacon.msg`
2. `src/researchproject/routing/gpsr/DelayPositionTable.h/.cc`
3. `src/researchproject/routing/gpsr/GpsrWithDelayTiebreaker.ned`
4. `src/researchproject/routing/gpsr/GpsrWithDelayTiebreaker.h/.cc`
5. `src/researchproject/node/DelayGpsrRouter.ned` (extends GpsrRouter, swaps in delay module)

**Configuration Files**:
1. `simulations/delay_tiebreaker/omnetpp.ini` (extends baseline_gpsr/omnetpp.ini)
2. `simulations/delay_tiebreaker/CongestedNetwork.ned` (reduced capacity)
3. `simulations/delay_tiebreaker/package.ned`

### INET Modules Referenced
- `inet::Gpsr` (base class)
- `inet::PositionTable` (enhanced for delay storage)
- `inet::GpsrBeacon` (extended with delay field)
- `inet::GpsrRouter` (NED composition pattern)

---

## 6. Metrics Collection Plan

### 6.1 Statistics to Record

**Baseline Metrics** (already recorded):
- ✓ Packet delivery rate (PDR)
- ✓ E2E delay
- ✓ Hop count
- ✓ Control overhead (beacons)

**New Tiebreaker-Specific Metrics**:
```ini
# In omnetpp.ini
**.gpsr.delayTiebreakerActivations:count  # How often tiebreaker used
**.gpsr.averageNeighborDelay:stats        # Delay distribution in neighbor table
**.gpsr.greedyModeSelections:count        # Normal greedy selections
```

### 6.2 Result Directory Structure
```
results/
├── baseline_gpsr/
│   └── Baseline-#0.sca  (already exists)
├── delay_tiebreaker/
│   ├── DelayTiebreaker-#0.sca
│   └── DelayTiebreaker-#0.vec
└── congested_cross/
    ├── Baseline-#0.sca
    └── DelayTiebreaker-#0.sca
```

---

## 7. Congested Scenario Design

### Scenario: congested_cross

**Purpose**: Create delay differentiation between paths to validate tiebreaker

**Configuration Changes from Baseline**:
```ini
[Config CongestedCross]
extends = Baseline
description = "Cross traffic to create congestion hotspots"

# Reduce link capacity to surface delays
**.wlan[*].bitrate = 1Mbps  # Down from 2Mbps

# Larger packets to increase congestion
**.app[0].messageLength = 1024B  # Up from 512B

# Higher traffic rate
**.app[0].sendInterval = exponential(0.5s)  # Up from 1s

# Grid positioning instead of random (predictable paths)
**.host[*].mobility.initialX = (index mod 5) * 250m
**.host[*].mobility.initialY = floor(index / 5) * 250m

# Traffic pattern: diagonal cross
# Flow 1: host[0] → host[19] (bottom-left to top-right)
# Flow 2: host[4] → host[15] (bottom-right to top-left)
# Flows share middle region, creating congestion
**.host[0].app[0].destAddresses = "10.0.0.20"
**.host[4].app[0].destAddresses = "10.0.0.16"
```

**Expected Behavior**:
- Center nodes (host[10-14]) become congested
- Paths through congested nodes have higher delays
- Delay tiebreaker should route around congestion when greedy distance is equal

---

## 8. Validation Approach

### Phase 2A: Sanity Tests (30-60s)
1. Verify module instantiates without errors
2. Check beacons sent with delay field
3. Confirm tiebreaker activates (delayTiebreakerActivations > 0)
4. Validate packets delivered (PDR > 0)

### Phase 2B: Comparative Runs (300s)
**Fixed Seed Set**: `repeat = 1` with `seed-set = ${repetition}`
**Configurations**:
- Baseline-Congested: Standard GPSR on congested_cross
- DelayTiebreaker-Congested: Delay-aware GPSR on congested_cross

**Metrics Comparison**:
| Metric | Baseline | Delay TB | Expected Improvement |
|--------|----------|----------|---------------------|
| PDR | X% | Y% | +5-15% (fewer drops) |
| E2E Delay | X ms | Y ms | -10-30% (avoid congestion) |
| Hop Count | X | Y | ±0-1 (similar paths) |
| Tiebreaker Acts | 0 | >0 | N/A |

---

## 9. Implementation Checklist

### Step 1: Research (COMPLETED)
- [x] Analyze GPSR architecture
- [x] Identify extension points
- [x] Study INET patterns for metrics
- [x] Document design decisions

### Step 2: Minimal Implementation
- [ ] Create GpsrDelayBeacon.msg
- [ ] Implement DelayPositionTable
- [ ] Extend Gpsr → GpsrWithDelayTiebreaker
- [ ] Add tiebreaker logic to findGreedyRoutingNextHop
- [ ] Create DelayGpsrRouter.ned
- [ ] Add package.ned declarations

### Step 3: Configuration
- [ ] Create simulations/delay_tiebreaker/omnetpp.ini
- [ ] Add CongestedCross configuration
- [ ] Configure statistics recording
- [ ] Set up result directories

### Step 4: Validation
- [ ] Compile module (make)
- [ ] Run 30s sanity test
- [ ] Verify tiebreaker activation
- [ ] Run 300s baseline vs delay comparison
- [ ] Export results to CSV

### Step 5: Documentation
- [ ] Update research notes with findings
- [ ] Create Phase 2 design document
- [ ] Document tiebreaker effectiveness
- [ ] Prepare for Phase 3 (real queue integration)

---

## 10. Key Insights from INET Analysis

1. **Inheritance Pattern**: INET heavily uses C++ inheritance for protocol extensions
2. **Module References**: `ModuleRefByPar<T>` pattern for accessing other modules
3. **Signal-Based Stats**: All metrics via signals, not direct variable emission
4. **NED Composition**: New routers created by swapping submodules, not modifying base
5. **Beacon Extension**: Adding fields to existing messages is standard practice
6. **Position Table**: Central data structure for neighbor info, easily extensible
7. **Distance Tiebreaker**: Currently uses first-found when distances equal (deterministic but arbitrary)

---

## Next Steps
Proceed to minimal implementation following documented patterns, keeping changes isolated to research project modules without modifying INET core.
