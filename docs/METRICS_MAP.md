# Metrics Map: Cross-Configuration Collection Plan

**Purpose**: Document exact INET module paths, statistic names, and collection formulas for all metrics across baseline and variant configurations.

**Status**: Ready for implementation (collection queries defined, no changes needed)

---

## Core Metrics Overview

| Metric | Source Layer | Purpose | Collection Method |
|--------|-------------|---------|------------------|
| **TCR** (Task Completion Rate) | Application | Success rate of delivered tasks/packets | app[0] packetReceived/packetSent |
| **E2E Delay** | Application | End-to-end latency | app[0] endToEndDelay |
| **PDR** (Packet Delivery Ratio) | Application | Overall delivery success | app[0] packetReceived/packetSent |
| **Average Hops** | Network/Routing | Path length efficiency | gpsr hopCount (or network-layer TTL delta) |
| **Control Overhead** | Routing | Protocol efficiency | (beaconBytes + controlBytes) / deliveredDataBytes |
| **MAC Queue Bytes** | Link Layer | Congestion indicator | mac pendingQueue length / backlog |

---

## 1. TCR (Task Completion Rate)

### Definition
Fraction of application-layer tasks (packets) successfully delivered from source to destination.

### INET Module Path
```
**.host[*].app[0]
```

### Statistics
- **Sent packets**: `packetSent:count`
- **Received packets**: `packetReceived:count`

### Formula
```
TCR = Σ(packetReceived) / Σ(packetSent)
```
where Σ is across all source-destination pairs.

### Collection Query
```bash
opp_scavetool query -f 'module =~ *.host[*].app[0] AND (name =~ packetSent:count OR name =~ packetReceived:count)' results/*.sca
```

### Expected Output Format
```
run     module              name                value
Baseline-0  host[0].app[0]  packetSent:count    10
Baseline-0  host[0].app[0]  packetReceived:count 0
Baseline-0  host[15].app[0] packetReceived:count 0
```

### Analysis Script (Python)
```python
import pandas as pd

def compute_tcr(sca_files):
    sent = query_scalar(sca_files, 'packetSent:count')
    received = query_scalar(sca_files, 'packetReceived:count')
    return received.sum() / sent.sum()
```

---

## 2. E2E Delay (End-to-End Delay)

### Definition
Time elapsed from application-layer send to application-layer receive, averaged across all delivered packets.

### INET Module Path

### Statistics
- **Statistic name**: `endToEndDelay:vector` or `endToEndDelay:histogram`
- **Alternative**: `rcvdPkLifetime:vector` (for received packets only)

### Formula
```
E2E Delay (mean) = mean(endToEndDelay) across all received packets
```

### Collection Query
```bash
# For vector data (detailed)
opp_scavetool query -f 'module =~ *.host[*].app[0] AND name =~ endToEndDelay:vector' results/*.vec

# For scalar statistics (mean/max)
opp_scavetool query -f 'module =~ *.host[*].app[0] AND name =~ endToEndDelay:mean' results/*.sca
```

### Expected Output Format
```
run     module              name                    value
Baseline-0  host[15].app[0]  endToEndDelay:mean      0.015  # seconds
Baseline-0  host[16].app[0]  endToEndDelay:mean      0.023
```

### Analysis Script (Python)
```python
def compute_e2e_delay(vec_files):
    delays = query_vector(vec_files, 'endToEndDelay:vector')
### Notes
- **Includes**: Queueing delay, transmission delay, propagation delay, routing overhead
- **Alternative metric**: `rcvdPkLifetime` (packet age at reception)

---

## 3. PDR (Packet Delivery Ratio)

Same as TCR (application-layer success rate). Included separately for clarity in routing literature.

### INET Module Path
**.host[*].app[0]

### Statistics
- **Sent packets**: `packetSent:count`
- **Received packets**: `packetReceived:count`

### Formula
PDR = Σ(packetReceived) / Σ(packetSent)
```

### Collection Query
Same as TCR (see above).

### Notes
- **TCR vs PDR**: Identical at application layer. TCR emphasizes task-oriented view (compute offload context), PDR emphasizes network performance.

---

## 4. Average Hops (Path Length)

### Definition

```
**.host[*].gpsr
```

- **Statistic name**: `hopCount:vector` or `hopCount:mean`
- **Alternative**: Network-layer `hopLimit` delta (initial TTL - final TTL)

### Formula
```
Average Hops = mean(hopCount) across all delivered packets

### Collection Query
```bash
# GPSR hop count (if available)
opp_scavetool query -f 'module =~ *.host[*].gpsr AND name =~ hopCount:mean' results/*.sca
```

### Expected Output Format
Baseline-0  host[15].gpsr  hopCount:mean      2.3
Baseline-0  host[16].gpsr  hopCount:mean      3.1
```

### Analysis Script (Python)
```python
def compute_avg_hops(sca_files):
    hops = query_scalar(sca_files, 'hopCount:mean')
    return hops.mean()  # Average across all destinations
```

### Notes
- **GPSR limitation**: GPSR may not natively record hopCount in INET 4.5
- **Workaround**: Log TTL at send (e.g., 64) and TTL at receive (e.g., 62) → hops = 2
- **Future logging**: Add `hopCount` field to GPSR data packet for explicit tracking

---
## 5. Control Overhead

### Definition
Ratio of control bytes (beacons, route discovery) to delivered data bytes. Measures routing protocol efficiency.

### INET Module Paths

#### Beacon Overhead (GPSR)
```
**.host[*].gpsr
```
- **Statistic**: `beaconBytesSent:count` (beacon payload + headers)

#### Data Delivered (Application)
```
**.host[*].app[0]
```
- **Statistic**: `packetReceived:sum(packetBytes)` (delivered data payload)

### Formula
```
Control Overhead = Σ(beaconBytesSent + controlBytesSent) / Σ(deliveredDataBytes)
```

### Collection Query
```bash
# Beacon overhead
opp_scavetool query -f 'module =~ *.host[*].gpsr AND name =~ *Bytes*' results/*.sca

# Delivered data
opp_scavetool query -f 'module =~ *.host[*].app[0] AND name =~ packetReceived:sum(packetBytes)' results/*.sca
```

### Expected Output Format
```
run     module          name                        value
Baseline-0  host[0].gpsr   beaconBytesSent:sum       12000  # 100 beacons * 120 bytes
Baseline-0  host[15].app[0] packetReceived:sum(packetBytes) 5120  # 10 packets * 512 bytes
```

### Analysis Script (Python)
```python
def compute_control_overhead(sca_files):
    beacon_bytes = query_scalar(sca_files, 'beaconBytesSent:sum').sum()
    control_bytes = query_scalar(sca_files, 'controlBytesSent:sum').sum()
    data_bytes = query_scalar(sca_files, 'packetReceived:sum(packetBytes)').sum()
    return (beacon_bytes + control_bytes) / data_bytes
```

### Notes
- **Beacon size**: GPSR beacon = L3Address + Coord + UDP header (~40-60 bytes)
- **Beacon interval**: 10s → ~50 beacons per node in 500s simulation
- **Expected range**: 0.1-1.0 (beacon overhead dominates in low-traffic scenarios)

---

## 6. MAC Queue Bytes (Congestion Indicator)

### Definition
Average or maximum size of MAC layer queue, indicating link-layer congestion.

### INET Module Path
```
**.host[*].wlan[*].mac.dcf.channelAccess.pendingQueue
```
or
```
**.host[*].wlan[*].mac.queue
```

### Statistics
- **Queue length**: `queueLength:vector` or `queueLength:mean`
- **Queue bytes**: `queueingTime:vector` (inferred from delay)
- **Alternative**: `droppedPkBytes:sum` (dropped due to queue overflow)

### Formula
```
MAC Queue Bytes (mean) = mean(queueLength * packetSize) across time
```

### Collection Query
```bash
# Queue length (packets)
opp_scavetool query -f 'module =~ *.host[*].wlan[*].mac.dcf.channelAccess.pendingQueue AND name =~ queueLength:mean' results/*.sca

# Dropped packets (overflow indicator)
opp_scavetool query -f 'module =~ *.host[*].wlan[*].mac AND name =~ droppedPkBytes:sum' results/*.sca
```

### Expected Output Format
```
run     module                              name                    value
Baseline-0  host[0].wlan[0].mac.dcf.channelAccess.pendingQueue queueLength:mean  1.2
Baseline-0  host[1].wlan[0].mac             droppedPkBytes:sum      0
```

### Analysis Script (Python)
```python
def compute_mac_queue_congestion(sca_files):
    queue_length = query_scalar(sca_files, 'queueLength:mean')
    return queue_length.mean()  # Average across all nodes
```

### Notes
- **INET 4.5 queue module**: `DropTailQueue` or `PriorityQueue` depending on configuration
- **Threshold**: queueLength > 5 indicates congestion
- **Alternative metric**: `queueingTime:mean` (time packets wait in queue)

---

## Cross-Configuration Collection Strategy

### Simulation Runs
Each configuration produces results in separate directories:

| Configuration | Result Directory | Reps |
|--------------|------------------|------|
| Baseline GPSR | `results/baseline_gpsr/` | 10 |
| Delay Tiebreaker | `results/delay_tiebreaker/` | 10 |
| Queue-Aware GPSR | `results/queue_aware/` | 10 |
| Two-Hop Peek | `results/two_hop_peek/` | 10 |
| Compute Offload | `results/compute_offload/` | 10 |

### Unified Analysis Script
**File**: `scripts/collect_all_metrics.py`

```python
import pandas as pd
from opp_scavetool import query_scalar, query_vector

configs = [
    'baseline_gpsr',
    'delay_tiebreaker',
    'queue_aware',
    'two_hop_peek',
    'compute_offload'
]

metrics = {}

for config in configs:
    result_dir = f'results/{config}/'
    
    # TCR
    metrics[config]['TCR'] = compute_tcr(result_dir)
    
    # E2E Delay
    metrics[config]['E2E_Delay'] = compute_e2e_delay(result_dir)
    
    # PDR (same as TCR)
    metrics[config]['PDR'] = metrics[config]['TCR']
    
    # Average Hops
    metrics[config]['Avg_Hops'] = compute_avg_hops(result_dir)
    
    # Control Overhead
    metrics[config]['Control_Overhead'] = compute_control_overhead(result_dir)
    
    # MAC Queue
    metrics[config]['MAC_Queue_Mean'] = compute_mac_queue_congestion(result_dir)

# Export to CSV
df = pd.DataFrame(metrics).T
df.to_csv('results/metrics_comparison.csv')
print(df)
```

**Output**:
```csv
config,TCR,E2E_Delay,PDR,Avg_Hops,Control_Overhead,MAC_Queue_Mean
baseline_gpsr,0.85,0.023,0.85,2.3,0.15,1.2
delay_tiebreaker,0.87,0.021,0.87,2.2,0.16,1.1
queue_aware,0.92,0.019,0.92,2.4,0.15,0.8
two_hop_peek,0.90,0.020,0.90,2.3,0.18,0.9
compute_offload,0.88,0.022,0.88,2.5,0.17,1.0
```

---

## Scenario-Specific Metrics

### Light Line Scenario (Sparse, low mobility)
**Expected**:
- TCR: High (>0.9) due to low contention
- E2E Delay: Low (<0.02s) due to short paths
- Control Overhead: Moderate (0.1-0.2) due to beaconing
- MAC Queue: Low (<1.0) due to low traffic

### Congested Cross Scenario (Dense, high traffic)
**Expected**:
- TCR: Lower (0.7-0.8) due to collisions
- E2E Delay: Higher (0.03-0.05s) due to queueing
- Control Overhead: Higher (0.2-0.3) due to frequent beaconing
- MAC Queue: High (2-5) indicating congestion

### Short Pass-By Scenario (High mobility)
**Expected**:
- TCR: Variable (0.6-0.9) due to link breakage
- E2E Delay: Moderate (0.02-0.03s)
- Control Overhead: High (0.3-0.5) due to frequent neighbor changes
- Average Hops: Higher (3-4) due to perimeter routing

---

## Summary Table: Collection Commands

| Metric | Query Command | Output Format |
|--------|--------------|---------------|
| **TCR** | `opp_scavetool query -f 'module =~ *.host[*].app[0] AND name =~ packet*:count' results/*.sca` | Scalar (count) |
| **E2E Delay** | `opp_scavetool query -f 'module =~ *.host[*].app[0] AND name =~ endToEndDelay:mean' results/*.sca` | Scalar (seconds) |
| **PDR** | Same as TCR | Scalar (count) |
| **Avg Hops** | `opp_scavetool query -f 'module =~ *.host[*].gpsr AND name =~ hopCount:mean' results/*.sca` | Scalar (hops) |
| **Control Overhead** | `opp_scavetool query -f 'module =~ *.host[*].gpsr AND name =~ *Bytes*' results/*.sca` | Scalar (bytes) |
| **MAC Queue** | `opp_scavetool query -f 'module =~ *.host[*].wlan[*].mac* AND name =~ queueLength:mean' results/*.sca` | Scalar (packets) |

---

## Baseline Readiness Check

Before running full 500s simulations, verify metrics collection on 10s test run:

```bash
cd simulations/baseline_gpsr

# Check TCR metrics exist
opp_scavetool query -f 'module =~ *.host[*].app[0] AND name =~ packetSent:count' results/Baseline-#0.sca

# Check E2E delay vectors exist
opp_scavetool query -f 'name =~ endToEndDelay:vector' results/Baseline-#0.vec

# Check GPSR statistics exist
opp_scavetool query -f 'module =~ *.host[*].gpsr' results/Baseline-#0.sca

# Check MAC queue statistics exist
opp_scavetool query -f 'module =~ *.host[*].wlan[*].mac AND name =~ queue*' results/Baseline-#0.sca
```

**Expected**: All queries return results. If any are missing, adjust NED/ini configuration to enable statistic recording.

---

**References**:
- INET UdpBasicApp: `inet4.5/src/inet/applications/udpapp/UdpBasicApp.ned`
- INET Gpsr: `inet4.5/src/inet/routing/gpsr/Gpsr.ned`
- INET Ieee80211Mac: `inet4.5/src/inet/linklayer/ieee80211/mac/Ieee80211Mac.ned`
- opp_scavetool: OMNeT++ User Guide Chapter 13
