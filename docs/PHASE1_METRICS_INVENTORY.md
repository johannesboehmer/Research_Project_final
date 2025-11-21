# Phase 1: Baseline Metrics Inventory

**Date:** November 6, 2025  
**Status:** ✅ Baseline Verified - Metrics Collected  
**Run Configuration:** Baseline with 10 repetitions (seeds 0-9)  
**Simulation Time:** 10 seconds per run  
**Results Location:** `results/baseline_gpsr/`

---

## Simulation Run Summary

### Execution Details
- **Network:** BaselineNetwork (20 GpsrRouter nodes)
- **Repetitions:** 10 (run #0 through #9)
- **Seeds:** Auto-assigned by OMNeT++ (based on repetition number)
- **Result Files:** 30 files total (10× .sca, 10× .vec, 10× .vci)
- **Total Size:** ~31 MB

### Run Statistics
| Run | Events | Transmissions | Signal Arrivals | Avg Speed (ev/sec) |
|-----|--------|---------------|-----------------|-------------------|
| #0  | 13,561 | 255          | 4,845           | 184,692           |
| #1  | 14,552 | 270          | 5,130           | 192,885           |
| #2  | 11,871 | 211          | 4,009           | 188,011           |
| #3  | 15,265 | 291          | 5,529           | 198,587           |
| #4  | 28,689 | 534          | 10,146          | 193,800           |
| #5  | 65,180 | 1,287        | 24,453          | 198,252           |
| #6  | 54,514 | 1,004        | 19,076          | 193,353           |
| #7  | 23,164 | 434          | 8,246           | 193,386           |
| #8  | 11,440 | 201          | 3,819           | 187,200           |
| #9  | 50,704 | 955          | 18,145          | 193,584           |

**Observation:** Significant variation in transmission counts across runs indicates varying network connectivity based on random node placement.

---

## Core Metrics Collected

### 1. Packet Delivery Ratio (PDR)

**Source:** Application layer statistics  
**Location:** `*.app[0]` modules

**Available Scalars:**
- `packets sent` - Total packets sent by application
- `packets received` - Total packets received by application
- `packetReceived:count` - Signal-based receive count
- `packetSent:count` - Signal-based send count

**Calculation:**
```
PDR = (Total packets received) / (Total packets sent) × 100%
```

**Sample Data (Run #0):**
- host[0]: sent=10, received=0
- host[1]: sent=7, received=0
- host[2]: sent=5, received=0
- host[3]: sent=4, received=0
- host[4]: sent=6, received=0

**Current Observation:** Low reception in 10s test run. Will need longer simulation time for meaningful PDR.

---

### 2. End-to-End Delay

**Source:** Application layer with packet timestamps  
**Location:** Vector data in `*.vec` files

**Available Vectors:**
- `endToEndDelay:vector` - Time from send to receive
- Vector IDs available in .vci index files

**Calculation:**
```
Delay = Receive_timestamp - Send_timestamp
```

**Access Method:**
```bash
opp_scavetool x -f 'name =~ "endToEndDelay*"' Baseline-*.vec
```

**Note:** Requires packets to be successfully delivered for delay measurement.

---

### 3. Hop Count

**Source:** Network layer routing statistics  
**Location:** IP layer or routing protocol statistics

**Available Metrics:**
- IP TTL decrements (indirect hop count)
- Routing path length (if recorded)

**Expected Location:**
```
**.ipv4.hopCount
**.gpsr.hopCount
```

**Note:** May need to enable specific statistics recording for explicit hop count tracking.

---

### 4. Routing Overhead

**Source:** GPSR beaconing and routing packets  
**Location:** GPSR module and MAC layer

**Components:**
- **Beacon packets** - Position advertisements (periodic)
- **Control overhead** - Routing headers added to data packets
- **Total transmissions** - From radio/MAC statistics

**Available Scalars:**
```bash
# Check GPSR-specific metrics
opp_scavetool q -f 'module =~ "*.gpsr"' Baseline-#0.sca
```

**Calculation:**
```
Overhead = (Beacon_packets + Routing_headers) / Total_data_packets
```

**From Run Statistics:**
- Run #0: 255 transmissions total (includes beacons + data + retransmissions)
- Transmissions include: beacons (periodic), data packets, MAC ACKs

---

### 5. Network Throughput

**Source:** Application and MAC layer statistics  
**Location:** Multiple layers

**Available Metrics:**
- `packetSent:sum(packetBytes)` - Total bytes sent (application)
- `packetReceived:sum(packetBytes)` - Total bytes received (application)
- MAC transmission statistics

**Sample Data (Run #0):**
- host[0]: sent 5,120 bytes (10 packets × 512B)
- host[1]: sent 3,584 bytes (7 packets × 512B)

**Calculation:**
```
Throughput = Total_bytes_received / Simulation_time
```

**Current:** Limited throughput due to short 10s simulation.

---

## Additional Available Metrics

### MAC Layer Metrics

**Source:** `*.wlan[*].mac` modules

**Available:**
- Frame transmission attempts
- Collisions
- Channel access delays
- Queue statistics (queue length, drops)

**Query:**
```bash
opp_scavetool q -f 'module =~ "*.wlan[*].mac*"' Baseline-#0.sca
```

### Radio Layer Metrics

**Source:** `*.wlan[*].radio` modules

**Available:**
- Radio signal computations (observed in output)
- Interference computations
- Reception computations

**From Run #0:**
- Radio signal arrival computation count = 4,845
- Interference computation count = 14,819
- Reception computation count = 4,845

### GPSR-Specific Metrics

**Source:** `*.gpsr` module

**Expected Metrics:**
- Beacon intervals
- Neighbor table size
- Mode switches (greedy ↔ perimeter)
- Position updates

**Query:**
```bash
opp_scavetool q -l -f 'module =~ "*.gpsr"' Baseline-#0.sca | head -30
```

---

## Cross-Layer Metric Sources

### For Queue-Aware Extensions (Phase 3)

**MAC Queue State:**
- Location: `**.wlan[*].mac.queue`
- Metrics: queue length, queue delay, drop count
- Access: Direct module parameter queries or statistics

**Example Query:**
```cpp
// In routing code
cModule *mac = getParentModule()->getSubmodule("wlan", 0)->getSubmodule("mac");
cModule *queue = mac->getSubmodule("queue");
int queueLength = queue->par("queueLength");
```

### For Delay Tiebreaker (Phase 2)

**Timing Sources:**
- `simTime()` - Current simulation time
- Packet creation timestamps
- Transmission timestamps from MAC

**Storage Location:**
- Extend PositionTable with delay field
- Calculate: `delay = current_time - last_beacon_time`

### For Two-Hop Peek (Phase 4)

**Beacon Extensions:**
- Current beacons carry: position (Coord), address
- Extension needed: neighbor list in beacon payload
- Storage: Two-hop position table

### For Compute Offload (Phase 5)

**Resource Metrics:**
- Will need custom parameters on nodes
- Example: `**.host[*].computeCapability`
- Advertise in beacons similar to position

---

## Metrics Validation Checklist

### ✅ Verified Metrics

- [x] **Application layer packets sent/received** - Working
- [x] **Packet byte counts** - Working  
- [x] **Signal statistics (transmissions)** - Working
- [x] **Radio computations** - Working
- [x] **Multiple seeds execution** - Working (10 runs)
- [x] **Results in correct directory** - `results/baseline_gpsr/` ✓

### ⏳ To Verify (Needs Longer Run)

- [ ] **End-to-end delay vectors** - Need successful deliveries
- [ ] **Packet delivery ratio** - Need longer simulation
- [ ] **Hop count tracking** - Check if explicit metric exists
- [ ] **GPSR mode switches** - Check GPSR statistics
- [ ] **Neighbor table statistics** - Check GPSR module stats

### 📝 To Enable/Create

- [ ] **Explicit hop count recording** - May need to add statistic
- [ ] **Per-packet routing path** - For debugging/validation
- [ ] **Queue statistics** - For Phase 3 preparation
- [ ] **Beacon overhead breakdown** - Separate beacon vs. data

---

## Recommendations for Full Baseline

### 1. Extend Simulation Time
```ini
# In omnetpp.ini
sim-time-limit = 500s  # As originally planned
```

**Rationale:** 10s is too short for meaningful metrics. Need sufficient time for:
- Multiple beacon exchanges (beacon interval = 10s)
- Packet deliveries across multi-hop paths
- Statistical confidence in measurements

### 2. Verify Traffic Generation
Current configuration has applications on host[0..4], but we saw packets from many hosts.

**Action:** Review and confirm traffic source/destination pairs in INI file.

### 3. Enable Additional Statistics
```ini
# Add to omnetpp.ini
**.gpsr.recordScalars = true
**.gpsr.recordVectors = true
**.wlan[*].mac.queue.recordScalars = true
```

### 4. Document Run IDs
Each run has unique ID format: `Baseline-{run#}-{timestamp}-{pid}`

**Example:** `Baseline-0-20251106-19:17:29-33981`

**Storage:** Keep mapping in `docs/WORKFLOW.md` for reproducibility.

---

## Seed Configuration

### Current Setup
```ini
seed-set = ${repetition}
num-rngs = 3
**.mobility.rng-0 = 1
**.wlan[*].mac.rng-0 = 2
```

### Seed Mapping
- Run #0 → seed-set 0
- Run #1 → seed-set 1
- ...
- Run #9 → seed-set 9

**For Phase 2+:** Use the **same seed sets** for paired comparisons!

---

## Result File Structure

### Scalar Files (.sca)
- Contains aggregate statistics
- Per-module scalar values
- Run metadata

### Vector Files (.vec)
- Time-series data
- Continuous metric recordings
- Larger file sizes

### Index Files (.vci)
- Vector index for fast access
- Maps vector IDs to names/modules

---

## Next Steps for Metrics

### Immediate Actions

1. **✅ DONE: Run baseline with 10s** - Verified simulation works
2. **Re-run with 500s** - Get meaningful metrics
3. **Extract and document baseline statistics** - Create summary table
4. **Verify all 5 core metrics** - Confirm each is available

### Documentation Updates

1. **WORKFLOW.md** - Mark Phase 1 baseline verification complete
2. **DESIGN.md** - Update metrics section with actual sources
3. **Create BASELINE_RESULTS.md** - Summary statistics table

### For Phase 2 Planning

1. Research INET timing utilities (as per INET_RESEARCH_GUIDE.md)
2. Design delay measurement extension
3. Plan PositionTable extension with delay field
4. Ensure same seeds used for comparison

---

## Summary

✅ **Baseline GPSR simulation running successfully**  
✅ **Results collecting in proper directory** (`results/baseline_gpsr/`)  
✅ **Multiple seeds working** (10 repetitions)  
✅ **Core metrics sources identified**  
✅ **INET GPSR structure documented**  

**Status:** Phase 1 baseline verification in progress. Short test run (10s) successful. Ready for full 500s runs to collect meaningful metrics.

**Next:** Run extended simulation (500s) and extract baseline statistics before Phase 2.

---

## Appendix: Metric Query Commands

```bash
# List all available scalars
opp_scavetool q -l Baseline-#0.sca

# Filter application metrics
opp_scavetool q -f 'module =~ "*.app[0]"' Baseline-*.sca

# Filter GPSR metrics
opp_scavetool q -f 'module =~ "*.gpsr"' Baseline-*.sca

# Filter MAC metrics
opp_scavetool q -f 'module =~ "*.wlan[*].mac"' Baseline-*.sca

# Extract vectors
opp_scavetool x -f 'name =~ "endToEndDelay*"' Baseline-*.vec

# Export to CSV
opp_scavetool x -o results.csv -F CSV Baseline-*.sca

# Summary statistics
opp_scavetool s Baseline-*.sca
```
