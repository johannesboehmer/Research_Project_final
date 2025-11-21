# Light Line Scenario Specification

**Purpose**: Sparse node density, low mobility, minimal contention. Tests routing efficiency in uncongested conditions.

**Status**: Documentation-only (NED/ini not implemented yet)

---

## Topology Overview

### Node Configuration
- **Node count**: 10 hosts
- **Topology**: Linear chain along 1000m line
- **Node spacing**: 100m (9 gaps between 10 nodes)
- **Communication range**: 250m (ensures 2-3 hop connectivity)
- **Area**: 1000m × 200m (narrow corridor)

### Mobility
- **Model**: `StationaryMobility` (static nodes)
- **Justification**: Focus on routing efficiency without link breakage

### Traffic Pattern
- **Source**: host[0] (leftmost)
- **Destination**: host[9] (rightmost)
- **Flow count**: 1 flow
- **Packet size**: 512 bytes
- **Packet interval**: exponential(1s) mean
- **Duration**: 500s

---

## Comparable INET Example

**Reference**: `inet4.5/examples/adhoc/idealwireless/omnetpp.ini`
- **Similarity**: Linear topology, static nodes, single flow
- **Difference**: INET example uses 4 nodes, we use 10 for multi-hop routing

**Configuration pattern to mirror**:
```ini
# From INET idealwireless example
**.numHosts = 10
**.mobility.typename = "StationaryMobility"
**.mobility.initFromDisplayString = false
**.mobility.initialX = replaceIfExists(index * 100m)  # Linear spacing
**.mobility.initialY = 100m  # Centered vertically
**.mobility.initialZ = 0m
```

---

## Metrics Focus

### Expected Behavior
- **TCR (Task Completion Rate)**: High (>0.95)
  - Low contention → minimal packet loss
  - Static topology → stable routes
  
- **E2E Delay**: Low (0.01-0.02s)
  - Short queue delays due to low traffic
  - Path length = 9 hops (host[0] → host[9])
  
- **PDR (Packet Delivery Ratio)**: High (>0.95)
  - Same as TCR (minimal loss)
  
- **Average Hops**: ~9 hops
  - Linear topology → deterministic path length
  - Greedy forwarding only (no perimeter routing)
  
- **Control Overhead**: Moderate (0.1-0.2)
  - 10 nodes × 50 beacons (500s / 10s interval) = 500 beacons total
  - Low relative to data traffic in uncongested scenario
  
- **MAC Queue Bytes**: Very low (<0.5)
  - Single flow → no contention
  - Packets forwarded immediately

### Comparison Value
- **Baseline**: Quantify GPSR performance in ideal conditions
- **Variants**: Show improvement is NOT due to favorable topology (i.e., variants must also excel here)

---

## Seed Configuration

### Random Number Generators
```ini
seed-set = ${repetition}
num-rngs = 3
**.mobility.rng-0 = 1  # Not used (static mobility)
**.mac.rng-0 = 2        # MAC backoff
**.app[*].rng-0 = 0     # Traffic generation
```

### Repetitions
- **Count**: 10 repetitions
- **Seeds**: 0-9
- **Purpose**: Statistical confidence intervals for metrics

---

## Result Directory

```ini
[Config LightLine]
result-dir = results/light_line
```

### Paired Comparisons
- `results/light_line/Baseline-*.sca` (Standard GPSR)
- `results/light_line/DelayTiebreaker-*.sca` (Variant 1)
- `results/light_line/QueueAware-*.sca` (Variant 2)
- `results/light_line/TwoHopPeek-*.sca` (Variant 3)

---

## Scenario Rationale

### Why Light Line?
1. **Baseline reference**: Establishes upper bound on routing efficiency
2. **No MAC interference**: Isolates routing algorithm performance
3. **Predictable paths**: Greedy forwarding always succeeds (no perimeter mode)
4. **Stress test for queue-aware logic**: Queue-aware GPSR should NOT degrade here

### Expected Outcome
- All variants perform similarly (>0.95 TCR, ~9 hops, <0.02s delay)
- Demonstrates variants do NOT harm performance in ideal conditions
- Validates implementation correctness before testing congested scenarios

---

## Future NED Implementation

### Network Module (conceptual)
```ned
// File: simulations/light_line/LightLineNetwork.ned
package researchproject.simulations.light_line;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.gpsr.GpsrRouter;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;

network LightLineNetwork {
    parameters:
        @display("bgb=1000,200");
        int numHosts = default(10);
    submodules:
        radioMedium: Ieee80211ScalarRadioMedium;
        configurator: Ipv4NetworkConfigurator;
        host[numHosts]: GpsrRouter {
            @display("p=$index*100,100");
        }
}
```

### Configuration File (conceptual)
```ini
# File: simulations/light_line/omnetpp.ini
[Config LightLine]
network = LightLineNetwork
sim-time-limit = 500s
result-dir = results/light_line

# Mobility: Static linear positions
**.mobility.typename = "StationaryMobility"
**.host[*].mobility.initialX = index * 100m
**.host[*].mobility.initialY = 100m
**.host[*].mobility.initialZ = 0m

# Traffic: Single flow (host[0] → host[9])
**.numApps = 1
**.app[0].typename = "UdpBasicApp"
**.app[0].destAddresses = ""
**.app[0].localPort = 5000
**.app[0].destPort = 5000
**.app[0].messageLength = 512B
**.app[0].sendInterval = exponential(1s)
**.app[0].startTime = uniform(0s, 10s)
**.app[0].stopTime = -1s

**.host[0].app[0].destAddresses = "host[9]"  # Only source sends

# Radio: UnitDisk for Step 0, then 802.11 Scalar
**.wlan[*].typename = "AckingWirelessInterface"
**.wlan[*].radio.typename = "UnitDiskRadio"
**.wlan[*].radio.transmitter.communicationRange = 250m
**.wlan[*].radio.transmitter.interferenceRange = 0m
**.wlan[*].radio.receiver.ignoreInterference = true

# GPSR: Standard parameters
**.gpsr.beaconInterval = 10s
**.gpsr.maxJitter = 5s
**.gpsr.neighborValidityInterval = 45s
**.gpsr.interfaces = "*"

# Seeds
seed-set = ${repetition}
repeat = 10
```

---

## Summary

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| **Nodes** | 10 | Multi-hop routing (9 hops) |
| **Spacing** | 100m | Requires 250m range for 2-hop connectivity |
| **Mobility** | Static | Isolate routing algorithm performance |
| **Flows** | 1 (host[0] → host[9]) | Minimal contention |
| **Duration** | 500s | 50 beacons per node, statistically significant |
| **Seeds** | 10 | Confidence intervals for metrics |

**Next Steps** (after Phase 1):
1. Implement `LightLineNetwork.ned`
2. Configure `omnetpp.ini` with above parameters
3. Run baseline GPSR and all variants
4. Collect metrics using `METRICS_MAP.md` queries
5. Verify all variants achieve >0.95 TCR

---

**References**:
- INET idealwireless example: `inet4.5/examples/adhoc/idealwireless/omnetpp.ini`
- METRICS_MAP.md: Full collection strategy
- UNITDISK_REACHABILITY_PLAN.md: Radio configuration details
