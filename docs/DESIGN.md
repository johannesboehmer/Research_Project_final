# Queue-Aware GPSR Design Documentation

## Architecture Overview

### Layered Design (INET-aligned)

This project follows INET's OSI-layer organization:

```
┌─────────────────────────────────────────┐
│  Application Layer (not modified)       │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│  Network Layer                          │
│  - QueueGPSR Routing Module             │
│    (researchproject.routing.queuegpsr)  │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│  Link Layer                             │
│  - Queue Inspector                      │
│    (researchproject.linklayer.queue)    │
│  - MAC (INET standard)                  │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│  Physical Layer (INET standard)         │
└─────────────────────────────────────────┘
```

## Module Organization

### `researchproject.routing.queuegpsr` Package

**Purpose:** Next-hop selection with queue awareness

**Key Components:**
- `QueueGpsr.ned` - Main routing module (extends INET's Gpsr)
- `QueueGpsr.cc/h` - Implementation
- `QueuePositionTable.cc/h` - Extended neighbor table with queue info
- `QueueGpsrDefs.h` - Protocol constants and messages

**Interfaces:**
- Implements `IManetRouting` (INET contract)
- Accesses queue inspector via module lookup

### `researchproject.linklayer.queue` Package

**Purpose:** MAC queue state inspection for routing decisions

**Key Components:**
- `QueueInspector.ned` - Queue monitoring interface
- `QueueInspector.cc/h` - Queue length/delay queries
- `QueueMetrics.h` - Queue state data structures

**Design Pattern:**
Follows INET's observer pattern for cross-layer information access

### `researchproject.common` Package

**Purpose:** Shared utilities and helper functions

**Key Components:**
- Metrics calculation
- Data structures
- Common constants

## Research-First Development Guidelines

### Before Any Implementation

1. **Survey INET Source**
   ```bash
   # Examine relevant INET areas
   ls inet4.5/src/inet/routing/gpsr/
   ls inet4.5/src/inet/linklayer/
   ```

2. **Check INET Examples**
   ```bash
   # Look for similar scenarios
   ls inet4.5/examples/manetrouting/
   ```

3. **Review Contracts**
   - Check `inet/routing/contract/IManetRouting.ned`
   - Review interface definitions
   - Understand gate connections

4. **Mirror Conventions**
   - Package naming: `researchproject.X` ↔ `inet.X`
   - Directory structure: match package hierarchy
   - Module types: follow INET's simple/compound patterns

### Package-Directory Mapping Rule

**Critical:** NED package names MUST match directory paths

```
Package: researchproject.routing.queuegpsr
Path:    src/researchproject/routing/queuegpsr/

Package: researchproject.linklayer.queue  
Path:    src/researchproject/linklayer/queue/
```

### Scenario Organization

**Principle:** Model code in `src/`, experiments in `simulations/`

**Scenario Structure:**
```
simulations/baseline_gpsr/
├── omnetpp.ini          # Run configurations
├── BaselineNetwork.ned  # Network topology
└── README.md           # Scenario description
```

**INI Configuration:**
```ini
[General]
network = BaselineNetwork
result-dir = ../../results
ned-path = ../../src;.
```

## Development Phases

### Phase 1: Baseline GPSR
**Goal:** Replicate INET's GPSR as starting point

**Research Steps:**
1. Study `inet/routing/gpsr/Gpsr.ned`
2. Review `inet/examples/manetrouting/gpsr/`
3. Understand neighbor table structure
4. Check position update mechanism

**Implementation:**
- Copy INET GPSR structure
- Adapt to project namespace
- Verify baseline behavior matches INET

### Phase 2: Delay Tiebreaker
**Goal:** Add transmission delay awareness

**Research Steps:**
1. Examine INET's delay measurement utilities
2. Check how other protocols handle timing
3. Review `inet/common/` for time utilities

**Implementation:**
- Extend position table with delay field
- Modify next-hop selection logic
- Add delay calculation module

### Phase 3: Queue-Aware Extension
**Goal:** Integrate MAC queue state into routing

**Research Steps:**
1. Study INET's MAC queue implementation (`inet/linklayer/`)
2. Check cross-layer access patterns in INET
3. Review observer/callback mechanisms

**Implementation:**
- Create queue inspector in `linklayer/queue/`
- Add queue-aware next-hop selection
- Implement preprocessing for queue access

### Phase 4: Two-Hop Peek
**Goal:** Extended neighbor awareness

**Research Steps:**
1. Review INET's neighbor discovery mechanisms
2. Check information dissemination patterns
3. Study beacon message extensions

**Implementation:**
- Extend beacon messages with neighbor list
- Build two-hop table structure
- Update next-hop selection with lookahead

### Phase 5: Compute-Aware Offload
**Goal:** Resource-aware routing decisions

**Research Steps:**
1. Check if INET has compute models
2. Review how to add custom node attributes
3. Study metric-based routing examples

**Implementation:**
- Add compute capability attributes
- Extend decision logic for offloading
- Balance compute vs. network metrics

## Metrics and Evaluation

### Core Performance Metrics (Verified in Phase 1)

#### 1. End-to-End Delay
**Source:** Application layer timestamps  
**Location:** `*.app[0]` modules  
**Measurement:** Packet creation time → Packet reception time  
**Access:** Vector data in `.vec` files  
**Query:** `opp_scavetool x -f 'name =~ "endToEndDelay*"' *.vec`

**Status:** ✅ Source verified in INET baseline

#### 2. Packet Delivery Ratio (PDR)
**Source:** Application layer statistics  
**Location:** `*.app[0]` modules  
**Measurement:** `packets received / packets sent`  
**Access:** Scalar data in `.sca` files  
**Scalars:**
- `packets sent` - Total transmitted
- `packets received` - Successfully delivered
- `packetReceived:count` - Signal-based count

**Status:** ✅ Source verified in INET baseline

#### 3. Routing Overhead
**Source:** GPSR module and MAC layer  
**Components:**
- Beacon packets (periodic position advertisements)
- Routing headers (added to data packets)
- Control messages

**Measurement:**
```
Overhead = (Beacons + Control_bytes) / Data_bytes
```

**Access:**
- GPSR module statistics for beacons
- MAC transmission counts
- Packet size accounting

**Status:** ✅ Sources identified, calculation formula defined

#### 4. Hop Count
**Source:** Network layer routing  
**Location:** IP TTL or routing statistics  
**Measurement:** Number of hops from source to destination  
**Access:** 
- IP header TTL decrements
- Routing path length statistics

**Status:** ⏳ To be verified in full 500s run

#### 5. Network Throughput
**Source:** Application and MAC layers  
**Measurement:** `Total_bytes_delivered / Simulation_time`  
**Access:**
- `packetReceived:sum(packetBytes)` from application
- MAC transmission statistics

**Status:** ✅ Source verified in INET baseline

### Collection Strategy (Phase 1 Validated)

**Results Directory:** `results/baseline_gpsr/`  
**File Types:**
- `.sca` - Scalar statistics (aggregates)
- `.vec` - Vector data (time series)
- `.vci` - Vector index (fast access)

**Baseline Run Configuration:**
- Repetitions: 10 (for statistical confidence)
- Seeds: 0-9 (auto-assigned by `${repetition}`)
- Run IDs: `Baseline-{run#}-{timestamp}-{pid}`

**Example:** `Baseline-0-20251106-19:17:29-33981`

### Cross-Layer Metric Access (For Future Phases)

#### MAC Queue State (Phase 3: Queue-Aware)
**Location:** `**.wlan[*].mac.queue`  
**Metrics:**
- Queue length (number of packets)
- Queue delay (time in queue)
- Drop count (buffer overflow)

**Access Pattern:**
```cpp
// From routing layer
cModule *mac = getParentModule()->getSubmodule("wlan", 0)->getSubmodule("mac");
cModule *queue = mac->getSubmodule("queue");
int queueLength = queue->par("queueLength");
```

**Status:** Access pattern documented, ready for Phase 3

#### Delay Measurement (Phase 2: Delay Tiebreaker)
**Source:** Packet timestamps and beacon timing  
**Calculation:** `delay = current_time - last_beacon_time`  
**Storage:** Extended PositionTable with delay field

**Pattern:**
```cpp
// In PositionTable
struct NeighborInfo {
    Coord position;
    simtime_t timestamp;
    simtime_t lastDelay;  // NEW for Phase 2
};
```

**Status:** Design ready, INET timing utilities researched

#### Two-Hop Information (Phase 4: Two-Hop Peek)
**Source:** Extended beacon messages  
**Current beacon:** position + address  
**Extended beacon:** position + address + neighbor_list

**Storage:** Two-hop position table mapping neighbors to their neighbors

**Status:** Beacon extension pattern identified from INET

#### Compute Metrics (Phase 5: Compute Offload)
**Source:** Custom node parameters  
**Advertisement:** In beacon messages (similar to position)  
**Example:** `**.host[*].computeCapability = 100 MFLOPS`

**Status:** Advertisement mechanism planned, follows position pattern

## Cross-Layer Access Patterns

### From Routing to Link Layer

**INET Pattern:**
```cpp
// Get interface table
IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(
    par("interfaceTableModule"), this);

// Get specific interface
InterfaceEntry *ie = ift->getInterfaceByName("wlan0");
```

**Queue Access Pattern:**
```cpp
// Find queue inspector for interface
cModule *queueInspector = ie->getSubmodule("queue")
    ->getSubmodule("inspector");
    
// Query queue state
int queueLength = queueInspector->par("currentLength");
```

## Testing Strategy

### Unit Testing
- Test individual components in isolation
- Verify queue inspector accuracy
- Check neighbor table updates

### Integration Testing
- Test module interactions
- Verify cross-layer access
- Check message flow

### Scenario Testing
- Compare against baseline
- Vary network conditions
- Statistical significance testing

## References

- INET User Guide: https://inet.omnetpp.org/docs/users-guide/
- OMNeT++ Manual: https://doc.omnetpp.org/
- GPSR Paper: Karp & Kung, "GPSR: Greedy Perimeter Stateless Routing for Wireless Networks", MobiCom 2000

## Notes

- Always check INET source before implementing
- Maintain strict package-to-directory mapping
- Keep model and scenario code separated
- Document assumptions and design decisions
- Version control configuration alongside code
