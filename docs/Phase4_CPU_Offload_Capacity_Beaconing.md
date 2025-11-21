# Phase 4: CPU Offload Capacity Beaconing

**Status:** ✅ Implemented and Validated  
**Date:** November 21, 2025

## Overview

This phase extends the GPSR beacon protocol to advertise per-node CPU offload capacity. Each UAV beacons an **effective CPU capacity** available for offloading tasks, allowing neighbors to make informed offloading decisions in later phases.

## Implementation Details

### 1. Message Extension (`QueueGpsr.msg`)

Extended `GpsrBeacon` with two new fields:

```cpp
class GpsrBeacon extends FieldsChunk
{
    L3Address address;
    Coord position;
    uint32_t txBacklogBytes;              // Phase 3: TX queue backlog
    double cpuOffloadHz = 0;              // Phase 4: effective CPU capacity (Hz)
    double cpuOffloadBacklogCycles = 0;   // Phase 4: current CPU backlog (cycles)
}
```

- **`cpuOffloadHz`**: Effective CPU capacity available for offloading, in cycles/second (Hz)
- **`cpuOffloadBacklogCycles`**: Current backlog of already-accepted offloaded work (for future queueing delay estimation)

### 2. NED Parameters (`QueueGpsr.ned`)

Added three configuration parameters:

```ned
double cpuTotalHz = default(2e9);        // total CPU capacity (e.g., 2 GHz)
double offloadShareMin = default(0.2);   // minimum fraction available for offloading
double offloadShareMax = default(0.6);   // maximum fraction available for offloading
```

### 3. Initialization Logic (`QueueGpsr.cc`)

In `initialize(INITSTAGE_LOCAL)`:

```cpp
// CPU offload capacity (Phase 4)
cpuTotalHz = par("cpuTotalHz");
offloadShareMin = par("offloadShareMin");
offloadShareMax = par("offloadShareMax");

// Draw random per-node fraction η ∈ [offloadShareMin, offloadShareMax]
double eta = uniform(offloadShareMin, offloadShareMax);
cpuOffloadHz = eta * cpuTotalHz;
cpuOffloadBacklogCycles = 0;  // no backlog initially
```

**Key Features:**
- Each UAV draws a **random per-node offload share** η from the configured range
- Provides heterogeneous CPU capacity across the network (realistic scenario)
- Randomness is repeatable per simulation run (uses OMNeT++ RNG)

### 4. Beacon Transmission (`createBeacon()`)

Updated beacon creation to include CPU capacity:

```cpp
beacon->setCpuOffloadHz(cpuOffloadHz);
beacon->setCpuOffloadBacklogCycles(cpuOffloadBacklogCycles);
```

Beacon chunk length updated to account for new fields:
```cpp
// address + position + txBacklogBytes (4) + cpuOffloadHz (8) + cpuOffloadBacklogCycles (8)
B beaconLength = B(addressLength + positionByteLength + 4 + 8 + 8);
```

### 5. Beacon Reception (`processBeacon()`)

Added neighbor CPU capacity tracking:

```cpp
// Phase 4: store neighbor CPU offload capacity
try {
    NeighborCpuInfo cpuInfo;
    cpuInfo.cpuOffloadHz = beacon->getCpuOffloadHz();
    cpuInfo.cpuOffloadBacklogCycles = beacon->getCpuOffloadBacklogCycles();
    cpuInfo.lastUpdate = simTime();
    neighborCpuCapacity[beacon->getAddress()] = cpuInfo;
    
    EV_INFO << "Neighbor CPU capacity updated: " << beacon->getAddress() 
            << " cpuOffloadHz=" << cpuInfo.cpuOffloadHz << " Hz" << endl;
}
catch (...) {
    // older beacons may not contain CPU fields; ignore
}
```

**Data Structure:**
```cpp
struct NeighborCpuInfo {
    double cpuOffloadHz;
    double cpuOffloadBacklogCycles;
    simtime_t lastUpdate;
};
std::map<L3Address, NeighborCpuInfo> neighborCpuCapacity;
```

### 6. Validation Logging

Added comprehensive logging for validation:

**Initialization (STEP 1 AUDIT):**
```
cpuOffloadHz: 8.39051e+08 Hz (41.9525% of total 2e+09 Hz)
```

**Beacon Transmission:**
```
🟦 Beacon TX [host[0]]: t=2.205s | Q=0 bytes | CPU=0.839051 GHz | nextBeacon≈t=4.20553s
```

**Beacon Reception:**
```
🔵 Beacon RX [host[0]]: t=2.090s from 10.0.0.3 | Q=0 bytes (IDLE) | CPU=0.972151 GHz
```

## Validation Results

### Test Configuration: `CpuOffloadBeaconValidation`

**Network:** 4 nodes (source, 2 relays, destination) in hexagonal topology  
**Duration:** 20 seconds (beacon-only, no data traffic)  
**CPU Parameters:**
- Base: 2 GHz (2e9 Hz)
- Offload share range: 20% - 60%
- Beacon interval: 2s

### Per-Node CPU Capacity (Sample Run)

| Host | CPU Offload Hz | Percentage of Total | GHz Equivalent |
|------|----------------|---------------------|----------------|
| host[0] | 8.39051e+08 | 41.95% | 0.839 GHz |
| host[1] | 8.74276e+08 | 43.71% | 0.874 GHz |
| host[2] | 9.72151e+08 | 48.61% | 0.972 GHz |
| host[3] | 1.07541e+09 | 53.77% | 1.075 GHz |

### Beacon Exchange Validation

✅ **Heterogeneous capacity:** Each node has different CPU offload capacity (20-60% range)  
✅ **Beacon transmission:** All nodes periodically beacon their `cpuOffloadHz`  
✅ **Beacon reception:** Neighbors correctly receive and store CPU capacity  
✅ **Neighbor table:** `neighborCpuCapacity` map correctly populated  
✅ **Logging:** Clear visibility into beaconed and received values

### Sample Beacon Trace

```
t=2.090s: host[0] receives beacon from 10.0.0.3 → CPU=0.972151 GHz
t=2.695s: host[0] receives beacon from 10.0.0.4 → CPU=1.07541 GHz
t=2.716s: host[0] receives beacon from 10.0.0.2 → CPU=0.874276 GHz
```

All three neighbors' CPU capacities are correctly advertised and stored.

## Next Steps (Phase 5+)

This implementation provides the **foundation for compute-aware offloading decisions**. The beaconed CPU capacity can now be used for:

### 1. Processing Time Estimation

For a task of size L bits with complexity X cycles/bit, processing time at neighbor k:

```
T_proc,k = (L × X) / cpuOffloadHz_k
```

### 2. CPU Queueing Delay (Future)

When `cpuOffloadBacklogCycles` is tracked:

```
T_queue,CPU,k = cpuOffloadBacklogCycles_k / cpuOffloadHz_k
```

### 3. Joint Transmission + Processing Delay

Combine Phase 3 (queue-aware routing) with Phase 4 (CPU capacity):

```
T_total,k = T_tx,k + T_proc,k + T_queue,CPU,k

where:
  T_tx,k = txBacklogBytes_k / bitrate_k  (from Phase 3)
  T_proc,k = (L × X) / cpuOffloadHz_k    (from Phase 4)
  T_queue,CPU,k = cpuOffloadBacklogCycles_k / cpuOffloadHz_k  (future)
```

### 4. Offloading Decision Logic

Extend `findGreedyRoutingNextHop()` to:
1. Filter neighbors by GPSR greedy progress
2. Apply distance-based tiebreaker (Phase 2)
3. **NEW:** Consider `T_total,k` when selecting offload target
4. Route to neighbor with minimum total delay (transmission + processing)

### 5. Dynamic CPU Backlog Tracking

Future enhancement: update `cpuOffloadBacklogCycles` when:
- Accepting an offload task: increment by task CPU cycles
- Completing a task: decrement by task CPU cycles
- Beacon the current backlog for neighbor awareness

## Files Modified

1. **`src/researchproject/routing/queuegpsr/QueueGpsr.msg`**  
   - Extended `GpsrBeacon` with `cpuOffloadHz` and `cpuOffloadBacklogCycles`

2. **`src/researchproject/routing/queuegpsr/QueueGpsr.ned`**  
   - Added CPU capacity parameters (`cpuTotalHz`, `offloadShareMin/Max`)

3. **`src/researchproject/routing/queuegpsr/QueueGpsr.h`**  
   - Added CPU capacity member variables and `NeighborCpuInfo` struct

4. **`src/researchproject/routing/queuegpsr/QueueGpsr.cc`**  
   - Initialized CPU capacity with random per-node share
   - Updated `createBeacon()` to include CPU fields
   - Updated `processBeacon()` to store neighbor CPU capacity
   - Enhanced logging for validation

5. **`simulations/delay_tiebreaker/omnetpp.ini`**  
   - Added `CpuOffloadBeaconValidation` config for testing

## Testing Instructions

Run CPU offload beacon validation:

```bash
cd /path/to/Research_project/simulations/delay_tiebreaker

../../Research_project -u Cmdenv -c CpuOffloadBeaconValidation \
  -n ../../src:../../../inet4.5/src:. \
  --sim-time-limit=20s \
  --result-dir=../../results
```

**Expected output:**
- STEP 1 AUDIT shows different `cpuOffloadHz` for each host (20-60% of 2 GHz)
- Beacon TX logs show CPU capacity in transmitted beacons
- Beacon RX logs show received neighbor CPU capacities
- No errors or warnings

## Summary

✅ **Phase 4 Complete:** CPU offload capacity beaconing is implemented and validated  
✅ **Heterogeneous capacity:** Each UAV has realistic per-node CPU availability  
✅ **Beacon protocol:** Extended with minimal overhead (16 bytes)  
✅ **Neighbor tracking:** CPU capacity stored alongside position and queue state  
✅ **Ready for integration:** Foundation in place for compute-aware offloading decisions

**Next milestone:** Integrate CPU capacity into offloading decision logic and demonstrate joint optimization of transmission delay + processing delay.
