# Congested Cross Scenario Specification

**Purpose**: Dense node density, high traffic load, cross-traffic interference. Tests routing performance under congestion.

**Status**: Documentation-only (NED/ini not implemented yet)

---

## Topology Overview

### Node Configuration
- **Node count**: 40 hosts
- **Topology**: Cross-shaped deployment (20 nodes per axis)
- **Node spacing**: 50m (high density)
- **Communication range**: 150m (3-hop connectivity across intersection)
- **Area**: 1000m × 1000m with cross pattern

### Cross Pattern Layout
```
Vertical axis:   host[0..19]  @ (500m, 0..950m) spaced 50m
Horizontal axis: host[20..39] @ (0..950m, 500m) spaced 50m
Intersection:    Multiple nodes share (500m, 500m) region → high contention
```

### Mobility
- **Model**: `StationaryMobility` (static nodes)
- **Justification**: Focus on congestion, not mobility-induced failures

### Traffic Pattern
- **Vertical flows**: host[0,2,4,6,8] → host[19,17,15,13,11] (5 flows)
- **Horizontal flows**: host[20,22,24,26,28] → host[39,37,35,33,31] (5 flows)
- **Total flows**: 10 concurrent flows
- **Packet size**: 512 bytes
- **Packet interval**: exponential(0.5s) mean (2× denser than baseline)
- **Duration**: 500s

---

## Comparable INET Example

**Reference**: `inet4.5/examples/wireless/scaling/ScalingTopology.ned`
- **Similarity**: High node density, multiple concurrent flows
- **Difference**: INET example uses grid, we use cross for directed congestion

**Configuration pattern to mirror**:
```ini
# High-density wireless with contention
**.numHosts = 40
**.mobility.typename = "StationaryMobility"

# Vertical axis placement
**.host[0..19].mobility.initialX = 500m
**.host[0..19].mobility.initialY = index * 50m

# Horizontal axis placement
**.host[20..39].mobility.initialX = (index - 20) * 50m
**.host[20..39].mobility.initialY = 500m
```

---

## Metrics Focus

### Expected Behavior
- **TCR (Task Completion Rate)**: Moderate (0.7-0.85)
  - High contention at intersection → packet drops
  - MAC collisions frequent
  
- **E2E Delay**: High (0.03-0.06s)
  - Long MAC queue delays
  - Backoff contention at intersection
  
- **PDR (Packet Delivery Ratio)**: Moderate (0.7-0.85)
  - Same as TCR (MAC-layer drops)
  
- **Average Hops**: Short (2-4 hops)
  - Cross pattern → most flows cross intersection directly
  - Greedy forwarding sufficient (no perimeter routing)
  
- **Control Overhead**: High (0.25-0.40)
  - 40 nodes × 50 beacons = 2000 beacons total
  - High beacon collision rate at intersection
  
- **MAC Queue Bytes**: Very high (3-8 packets)
  - Intersection nodes bottleneck
  - Queue buildup due to CSMA/CA contention

### Comparison Value
- **Baseline GPSR**: Suffers under congestion (lower TCR, higher delay)
- **Queue-Aware GPSR**: Routes around congested intersection → higher TCR
- **Delay Tiebreaker**: Avoids delayed paths → lower E2E delay

---

## Seed Configuration

### Random Number Generators
```ini
seed-set = ${repetition}
num-rngs = 3
**.mobility.rng-0 = 1  # Not used (static mobility)
**.mac.rng-0 = 2        # MAC backoff (critical for collisions)
**.app[*].rng-0 = 0     # Traffic generation
```

### Repetitions
- **Count**: 10 repetitions
- **Seeds**: 0-9
- **Purpose**: Statistical confidence (high variance due to collisions)

---

## Result Directory

```ini
[Config CongestedCross]
result-dir = results/congested_cross
```

### Paired Comparisons
- `results/congested_cross/Baseline-*.sca` (Standard GPSR → poor TCR)
- `results/congested_cross/QueueAware-*.sca` (Should show improvement)
- `results/congested_cross/DelayTiebreaker-*.sca` (Should show lower delay)

---

## Scenario Rationale

### Why Congested Cross?
1. **Stress test**: Worst-case congestion at intersection point
2. **Queue-awareness validation**: Proves queue-aware routing reduces congestion
3. **Differentiation**: Clear separation between baseline and variants
4. **Realistic pattern**: Models urban intersections, data center aggregation points

### Expected Outcome
- **Baseline GPSR**: TCR ~0.75, E2E delay ~0.05s (intersection bottleneck)
- **Queue-Aware GPSR**: TCR ~0.85, E2E delay ~0.04s (routes around congestion)
- **Delay Tiebreaker**: TCR ~0.80, E2E delay ~0.035s (prefers low-delay paths)
- **Two-Hop Peek**: TCR ~0.80, E2E delay ~0.04s (avoids congested second hops)

---

## Congestion Mechanisms

### Intersection Bottleneck
- **Location**: (500m, 500m) region
- **Node count**: ~4 nodes within 150m range of center
- **Traffic load**: All 10 flows pass through or near intersection
- **MAC collisions**: High CSMA/CA contention (hidden terminal problem)

### MAC Queue Buildup
- **Bottleneck nodes**: host[10], host[30] (near intersection)
- **Queue overflow**: Packets dropped when queue full
- **Backoff cascade**: Failed transmissions increase contention window

### Routing Response
- **Baseline GPSR**: Always greedy → always routes through intersection
- **Queue-Aware GPSR**: Detects high queue → routes around via longer path
- **Result**: Queue-aware trades hop count for lower congestion

---

## Future NED Implementation

### Network Module (conceptual)
```ned
// File: simulations/congested_cross/CongestedCrossNetwork.ned
package researchproject.simulations.congested_cross;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.gpsr.GpsrRouter;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;

network CongestedCrossNetwork {
    parameters:
        @display("bgb=1000,1000");
        int numHosts = default(40);
    submodules:
        radioMedium: Ieee80211ScalarRadioMedium;
        configurator: Ipv4NetworkConfigurator;
        host[numHosts]: GpsrRouter;
}
```

### Configuration File (conceptual)
```ini
# File: simulations/congested_cross/omnetpp.ini
[Config CongestedCross]
network = CongestedCrossNetwork
sim-time-limit = 500s
result-dir = results/congested_cross

# Mobility: Static cross pattern
**.mobility.typename = "StationaryMobility"

# Vertical axis (host[0..19])
**.host[0..19].mobility.initialX = 500m
**.host[0..19].mobility.initialY = (index % 20) * 50m
**.host[0..19].mobility.initialZ = 0m

# Horizontal axis (host[20..39])
**.host[20..39].mobility.initialX = ((index - 20) % 20) * 50m
**.host[20..39].mobility.initialY = 500m
**.host[20..39].mobility.initialZ = 0m

# Traffic: 10 concurrent flows
**.numApps = 1
**.app[0].typename = "UdpBasicApp"
**.app[0].destAddresses = ""
**.app[0].localPort = 5000
**.app[0].destPort = 5000
**.app[0].messageLength = 512B
**.app[0].sendInterval = exponential(0.5s)  # 2× denser than baseline
**.app[0].startTime = uniform(0s, 10s)
**.app[0].stopTime = -1s

# Vertical flows (5 flows)
**.host[0].app[0].destAddresses = "host[19]"
**.host[2].app[0].destAddresses = "host[17]"
**.host[4].app[0].destAddresses = "host[15]"
**.host[6].app[0].destAddresses = "host[13]"
**.host[8].app[0].destAddresses = "host[11]"

# Horizontal flows (5 flows)
**.host[20].app[0].destAddresses = "host[39]"
**.host[22].app[0].destAddresses = "host[37]"
**.host[24].app[0].destAddresses = "host[35]"
**.host[26].app[0].destAddresses = "host[33]"
**.host[28].app[0].destAddresses = "host[31]"

# Radio: 802.11 Scalar (realistic MAC contention)
**.wlan[*].typename = "Ieee80211Interface"
**.wlan[*].radio.typename = "Ieee80211ScalarRadio"
**.wlan[*].radio.transmitter.power = 2mW
**.wlan[*].radio.transmitter.communicationRange = 150m  # Short range for density
**.wlan[*].mac.typename = "Ieee80211Mac"
**.wlan[*].mac.dcf.channelAccess.cwMin = 15
**.wlan[*].mac.dcf.channelAccess.cwMax = 1023

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

## Validation Checks

### Before Full Run
1. **Verify intersection placement**:
   - Inspect `host[10].mobility.initialX/Y` (should be ~500m, ~500m)
   - Inspect `host[30].mobility.initialX/Y` (should be ~500m, ~500m)

2. **Verify flow crossing**:
   - Vertical flows: host[0..8] → host[11..19] (cross Y=500m)
   - Horizontal flows: host[20..28] → host[31..39] (cross X=500m)

3. **MAC queue statistics enabled**:
   ```bash
   opp_scavetool query -f 'module =~ *.host[10].wlan[*].mac AND name =~ queueLength' results/*.sca
   ```

### Expected Metrics
- **Intersection node (host[10])**: queueLength:mean > 5 (bottleneck)
- **Edge node (host[0])**: queueLength:mean < 2 (no contention)

---

## Summary

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| **Nodes** | 40 (20 per axis) | High density |
| **Spacing** | 50m | Dense packing → 3 hops per axis |
| **Mobility** | Static | Focus on congestion, not mobility |
| **Flows** | 10 concurrent | Overload intersection point |
| **Interval** | exponential(0.5s) | 2× higher load than baseline |
| **Range** | 150m | Short range → more collisions |
| **Duration** | 500s | Statistical significance |
| **Seeds** | 10 | High variance due to collisions |

**Next Steps** (after Phase 1):
1. Implement `CongestedCrossNetwork.ned`
2. Configure `omnetpp.ini` with traffic flows
3. Run baseline GPSR → expect TCR ~0.75
4. Run queue-aware variant → expect TCR improvement to ~0.85
5. Analyze MAC queue statistics to confirm bottleneck

---

**References**:
- INET wireless scaling example: `inet4.5/examples/wireless/scaling/omnetpp.ini`
- METRICS_MAP.md: MAC queue collection strategy
- UNITDISK_REACHABILITY_PLAN.md: 802.11 Scalar configuration
