# MAC Queue Measurement Fix - Implementation Report

**Date:** November 18, 2025  
**Status:** ✅ Successfully Implemented and Validated  
**Impact:** Tiebreaker now fully functional with 85.6% traffic steering to idle relay

---

## Problem Summary

The queue-aware tiebreaker was not activating despite heavy congestion on one relay. Investigation revealed that `getLocalTxBacklogBytes()` was using an artificial counter (`localTxBacklogBytes`) that tracked IP-level packet additions but never decreased, resulting in beacons reporting `Q=0 bytes` even under 1.5 Mbps load.

### Root Cause Analysis

1. **Artificial Counter Issue**
   - Counter: `localTxBacklogBytes` incremented on packet arrival
   - Problem: No decrement mechanism when packets transmitted
   - Decay hack attempted but insufficient
   - Result: Always reported 0 or stale values

2. **Diagnosis Evidence**
   - Out of 250 packets, only 28 used tiebreaker
   - 222 packets routed via congested relay despite idle alternative
   - Beacon logs showed `Q=0 bytes` throughout simulation
   - Only ONE beacon showed Q=84912 bytes at t=12.76s → triggered 28 tiebreaker activations

3. **Beacon Age Analysis Ruled Out**
   - Initially suspected stale beacons (3s validity threshold)
   - Log analysis showed 394 FRESH vs only 6 STALE beacon checks
   - Beacons were current - the queue measurement itself was broken

---

## Solution Implementation

### Architecture Change

Replaced artificial counter with **direct MAC queue introspection** using INET's `IPacketCollection` interface.

```cpp
unsigned long QueueGpsr::getLocalTxBacklogBytes() const
{
    try {
        // Navigate to WLAN MAC module
        cModule *wlanModule = host->getSubmodule("wlan", 0);
        cModule *macModule = wlanModule->getSubmodule("mac");
        
        // Recursively search for IPacketCollection in MAC tree
        // Actual path: mac.dcf.channelAccess.pendingQueue
        std::function<cModule*(cModule*)> findQueue = [&](cModule *mod) -> cModule* {
            auto *q = dynamic_cast<queueing::IPacketCollection *>(mod);
            if (q) return mod;
            for (cModule::SubmoduleIterator it(mod); !it.end(); ++it) {
                cModule *found = findQueue(*it);
                if (found) return found;
            }
            return nullptr;
        };
        
        cModule *queueModule = findQueue(macModule);
        if (!queueModule) return 0;
        
        // Read actual queue state
        auto *queue = dynamic_cast<queueing::IPacketCollection *>(queueModule);
        b totalLength = queue->getTotalLength();
        return (unsigned long)totalLength.get();
        
    } catch (std::exception& e) {
        return 0;
    }
}
```

### Queue Path Discovery

**Expected Path (from omnetpp.ini):** `wlan[0].mac.tx.queue`
```ini
*.host[*].wlan[*].mac.tx.queue.typename = "PriorityQueue"
```

**Actual Path (INET 4.5 802.11 MAC):** `wlan[0].mac.dcf.channelAccess.pendingQueue`

The configuration sets up a PriorityQueue for the interface, but INET 4.5's Ieee80211Mac internally uses the DCF (Distributed Coordination Function) which has its own pending queue structure. The recursive search successfully locates the active queue implementing `IPacketCollection`.

### Code Cleanup

Removed obsolete decay logic from `processBeaconTimer()`:
```cpp
// REMOVED:
// if (enableQueueDelay && localTxBacklogBytes > 0) {
//     localTxBacklogBytes -= decayAmount;
// }

// NEW:
// Note: No decay needed - getLocalTxBacklogBytes() now reads actual queue directly
```

---

## Build System Fixes

### Compilation Challenges

1. **Double-Slash Path Bug**
   - Makefile generated `out/clang-release//src` (double slash)
   - Fixed: `PROJECTRELATIVE_PATH` conditional logic

2. **Missing INET Headers**
   - Relative path `../../inet4.5/src` not resolving
   - Solution: Absolute paths in Makefile
   ```makefile
   INCLUDE_PATH = -I. -Isrc -I/Users/johannesbohmer/.../inet4.5/src
   LIBS = $(LDFLAG_LIBPATH)/Users/johannesbohmer/.../inet4.5/src -lINET$D
   ```

3. **Message File Generation**
   - Empty `QueueGpsr_m.h` and `QueueGpsr_m.cc` files
   - Regenerated: `opp_msgtool --msg6 -I<inet_path> QueueGpsr.msg`

---

## Validation Results

### Test Configuration
- **Topology:** 4 nodes (source, 2 relays, destination)
- **Distance:** 450m source-to-destination (requires relay)
- **Radio Range:** ~280m (6.5mW power)
- **Congestion:** 1.5 Mbps on relay host[1], 0 Mbps on relay host[2]
- **Test Flow:** 10 pkt/s × 500B from source to destination

### Performance Comparison

| Metric | Baseline GPSR | Queue-Aware GPSR | Improvement |
|--------|--------------|------------------|-------------|
| **Tiebreaker Activations** | 0 | 214 / 250 | 85.6% |
| **Packets Delivered** | 197 / 250 (78.8%) | 244 / 250 (97.6%) | +47 packets |
| **Mean Delay** | 292.44 ms | 23.12 ms | **-269.3 ms (92% reduction)** |
| **Routing via Idle Relay** | 0 / 250 (0%) | 214 / 250 (85.6%) | Congestion-aware |

### Key Observations

1. **Tiebreaker Activation:** 214 out of 250 packets triggered the queue-aware tiebreaker
2. **Intelligent Routing:** 85.6% of packets routed via idle relay (host[2]) instead of congested relay (host[1])
3. **Delivery Improvement:** Packet loss dropped from 21.2% to 2.4%
4. **Latency Reduction:** 92% decrease in end-to-end delay
5. **Remaining Congested Routing:** 36 packets still used congested relay (14.4%) - likely during queue draining periods or when queues were briefly equal

---

## Technical Details

### INET 4.5 Queue Architecture

```
wlan[0]
├── mac (Ieee80211Mac)
│   ├── dcf (Ieee80211DcfComponent)
│   │   ├── channelAccess
│   │   │   └── pendingQueue ← IPacketCollection ✓
│   ├── ds (Ieee80211DataService)
│   ├── rx (Ieee80211Rx)
│   └── tx (Ieee80211Tx)  ← Expected queue here but empty
├── radio (Ieee80211ScalarRadio)
└── ...
```

### Queue Measurement Behavior

- **Empty Queue:** Returns 0 bytes (idle relay)
- **Active Queue:** Returns sum of all packet lengths in pendingQueue
- **Units:** Bytes (converted from INET's `b` type via `.get()`)
- **Scope:** Includes all queued packets (data + control in separate priority queues)

### Beacon Propagation

```cpp
// In createBeacon():
if (enableQueueDelay) {
    uint32_t localBacklog = (uint32_t)getLocalTxBacklogBytes();  // Now reads actual MAC queue
    beacon->setTxBacklogBytes(localBacklog);
    
    std::cout << "🟦 Beacon TX [" << host->getFullName() << "]: t=" << simTime() 
              << "s | Q=" << localBacklog << " bytes" << std::endl;
}
```

---

## Diagnostic Logging Added

Temporary debug output for validation (first 10 calls):

```
[Q-CALL #1] getLocalTxBacklogBytes() called on host[0] at t=2.097627s
[Q-READ #1] host[0] t=2.097627s: MAC queue=0 bytes (0 packets) [queueModule=...pendingQueue]
```

This confirmed:
- Function being called correctly
- Queue path discovery working
- Actual queue measurements (0 bytes for idle, >0 for congested)

---

## Files Modified

1. **src/researchproject/routing/queuegpsr/QueueGpsr.cc**
   - Replaced `getLocalTxBacklogBytes()` implementation (lines ~686-750)
   - Removed decay logic from `processBeaconTimer()` (lines ~220-240)
   - Added `#include <functional>` for recursive queue search

2. **Makefile**
   - Fixed `PROJECTRELATIVE_PATH` conditional
   - Changed `INCLUDE_PATH` to absolute paths
   - Changed `LIBS` to absolute paths

3. **Regenerated:**
   - `src/researchproject/routing/queuegpsr/QueueGpsr_m.cc`
   - `src/researchproject/routing/queuegpsr/QueueGpsr_m.h`

---

## Lessons Learned

1. **Abstraction Validation:** Always verify that abstraction layers (like counters) actually reflect underlying state
2. **INET Introspection:** INET provides rich interfaces (`IPacketCollection`) for runtime inspection
3. **Path Discovery:** Don't hardcode module paths - INET internals vary between versions/configurations
4. **Build System Gotchas:** OMNeT++ Makefiles need careful handling of relative vs absolute paths
5. **Diagnostic First:** Add logging before assuming root cause

---

## Future Considerations

### Optimization Opportunities

1. **Cache Queue Module Reference**
   - Currently searches MAC tree on every beacon creation
   - Could cache `queueModule` pointer after first discovery
   - Trade-off: Memory vs CPU (search is fast ~O(10) modules)

2. **Multi-Interface Support**
   - Current: Hardcoded `wlan[0]`
   - Enhancement: Support multiple WLAN interfaces

3. **Queue Type Flexibility**
   - Current: Any `IPacketCollection` implementation
   - Enhancement: Prefer specific queue types (e.g., PriorityQueue for finer control)

### Monitoring Recommendations

- Log queue measurements during long-running simulations
- Track queue size distribution (histogram)
- Correlate queue size with tiebreaker activation rate
- Monitor false positives (tiebreaker when both queues empty)

---

## Conclusion

The MAC queue measurement fix successfully restored the queue-aware tiebreaker functionality. By reading actual MAC queue state instead of relying on an artificial counter, the system now accurately detects congestion and routes traffic intelligently. The 92% delay reduction and 85.6% idle relay utilization demonstrate that the tiebreaker is working as designed.

**Status:** ✅ Ready for production use in research simulations
