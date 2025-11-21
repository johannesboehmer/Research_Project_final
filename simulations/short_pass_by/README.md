# Short Pass-By Scenario Specification

**Purpose**: High mobility, frequent topology changes, link breakage. Tests routing resilience and perimeter-mode handling.

**Status**: Documentation-only (NED/ini not implemented yet)

---

## Topology Overview

### Node Configuration
- **Node count**: 20 hosts
- **Topology**: Two groups moving in opposite directions
- **Initial spacing**: 100m within groups
- **Communication range**: 250m
- **Area**: 1500m × 500m (elongated corridor)

### Group Layout
```
Group A: host[0..9]  moving EAST  (left → right)
Group B: host[10..19] moving WEST (right → left)
Initial positions: Groups start at opposite ends, pass through center
```

### Mobility
- **Model**: `LinearMobility`
- **Speed**: 15 m/s (~54 km/h, highway speed)
- **Direction**: Group A @ 0° (east), Group B @ 180° (west)
- **Pass-by duration**: ~50s (groups pass completely)
- **Connection window**: ~17s (250m range / 15m/s relative speed)

### Traffic Pattern
- **Inter-group flows**: host[0,2,4,6,8] → host[15,16,17,18,19] (5 flows)
- **Intra-group flows**: None (focus on link breakage stress)
- **Packet size**: 512 bytes
- **Packet interval**: exponential(0.5s) mean (high rate to capture short window)
- **Duration**: 500s (multiple pass-by events)

---

## Comparable INET Example

**Reference**: `inet4.5/examples/manetrouting/multiradio/MultiRadio.ned`
- **Similarity**: Mobile MANET with vehicular speeds
- **Difference**: INET example uses grid mobility, we use linear pass-by

**Configuration pattern to mirror**:
```ini
# Vehicular mobility
**.mobility.typename = "LinearMobility"
**.mobility.speed = 15mps
**.mobility.constraintAreaMinX = 0m
**.mobility.constraintAreaMaxX = 1500m
**.mobility.constraintAreaMinY = 0m
**.mobility.constraintAreaMaxY = 500m
**.mobility.updateInterval = 0.1s  # Fine-grained position updates

# Group A: Moving east
**.host[0..9].mobility.initialX = (index % 10) * 100m
**.host[0..9].mobility.initialY = 250m
**.host[0..9].mobility.initialMovementHeading = 0deg  # East

# Group B: Moving west
**.host[10..19].mobility.initialX = 1500m - ((index - 10) % 10) * 100m
**.host[10..19].mobility.initialY = 250m
**.host[10..19].mobility.initialMovementHeading = 180deg  # West
```

---

## Metrics Focus

### Expected Behavior
- **TCR (Task Completion Rate)**: Low-Moderate (0.6-0.75)
  - Short connection window (17s per pass-by)
  - Link breakage mid-route → packet drops
  - Perimeter routing failures (face changes during mobility)
  
- **E2E Delay**: Variable (0.02-0.10s)
  - Fast delivery during connection window
  - Long delay if perimeter routing invoked
  - Dropped packets not measured (only successful ones)
  
- **PDR (Packet Delivery Ratio)**: Low-Moderate (0.6-0.75)
  - Same as TCR (high loss due to mobility)
  
- **Average Hops**: High (3-6 hops)
  - Perimeter routing frequently triggered
  - Face changes increase path length
  
- **Control Overhead**: Very high (0.4-0.6)
  - Frequent neighbor table updates (beacons every 10s, neighbors change constantly)
  - High beacon-to-data ratio due to low delivery
  
- **MAC Queue Bytes**: Low-Moderate (1-3 packets)
  - High traffic but short connection time → moderate queuing

### Comparison Value
- **Baseline GPSR**: Struggles with link breakage (low TCR, high delay variance)
- **Two-Hop Peek**: Better anticipates link breakage → higher TCR
- **Delay Tiebreaker**: Avoids slow perimeter paths → lower E2E delay

---

## Seed Configuration

### Random Number Generators
```ini
seed-set = ${repetition}
num-rngs = 3
**.mobility.rng-0 = 1  # Mobility timing (critical for pass-by alignment)
**.mac.rng-0 = 2        # MAC backoff
**.app[*].rng-0 = 0     # Traffic generation
```

### Repetitions
- **Count**: 10 repetitions
- **Seeds**: 0-9
- **Purpose**: High variance due to timing-sensitive pass-by events

---

## Result Directory

```ini
[Config ShortPassBy]
result-dir = results/short_pass_by
```

### Paired Comparisons
- `results/short_pass_by/Baseline-*.sca` (Standard GPSR → link breakage failures)
- `results/short_pass_by/TwoHopPeek-*.sca` (Anticipates link breakage → higher TCR)
- `results/short_pass_by/DelayTiebreaker-*.sca` (Avoids perimeter delays → lower E2E)

---

## Scenario Rationale

### Why Short Pass-By?
1. **Mobility stress test**: Worst-case link breakage (high speed, short window)
2. **Perimeter routing validation**: Forces perimeter mode → tests face-change handling
3. **Neighbor discovery stress**: Frequent beacon updates under high dynamics
4. **Realistic V2V pattern**: Models highway overtaking, urban drive-bys

### Expected Outcome
- **Baseline GPSR**: TCR ~0.65 (many packets dropped during link transitions)
- **Two-Hop Peek**: TCR ~0.75 (anticipates next-hop mobility → better decisions)
- **Delay Tiebreaker**: E2E delay ~0.05s (avoids slow perimeter paths)
- **Queue-Aware**: TCR ~0.68 (queue info less relevant under mobility)

---

## Pass-By Timeline

### Initial State (t=0s)
```
Group A: [H0 H1 H2 ... H9] at X=0-900m, moving east at 15m/s
Group B: [H10 H11 ... H19] at X=600-1500m, moving west at 15m/s
Distance between groups: ~300m (no connectivity yet)
```

### Connection Window (t=10s - t=27s)
```
t=10s: Groups ~150m apart → connection established
t=18s: Groups overlap → maximum connectivity
t=27s: Groups ~150m apart → connection lost
Connection duration: ~17 seconds
```

### Post Pass-By (t>27s)
```
Groups diverge, no inter-group connectivity
Packets in-flight dropped if route breaks
Intra-group connectivity remains stable
```

### Multiple Pass-Bys (t=100s+)
```
Groups loop back (constrained area) → periodic pass-by events
Repeats connection window pattern
Statistical averaging over multiple events
```

---

## Link Breakage Mechanisms

### Neighbor Timeout
- **Beacon interval**: 10s
- **Neighbor validity**: 45s
- **Problem**: At 15m/s, node moves 150m in 10s (half of comm range)
- **Result**: Neighbors may expire before next beacon received

### Greedy Failure → Perimeter Mode
- **Trigger**: Next-hop moves out of range before packet arrives
- **Response**: GPSR enters perimeter mode (face routing)
- **Problem**: Face graph changes during mobility → routing loops

### Mid-Route Drops
- **Scenario**: Packet queued at intermediate node, next-hop moves away
- **Result**: Packet dropped (no route to destination)
- **Baseline GPSR**: No anticipation of link breakage

---

## Future NED Implementation

### Network Module (conceptual)
```ned
// File: simulations/short_pass_by/ShortPassByNetwork.ned
package researchproject.simulations.short_pass_by;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.gpsr.GpsrRouter;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;

network ShortPassByNetwork {
    parameters:
        @display("bgb=1500,500");
        int numHosts = default(20);
    submodules:
        radioMedium: Ieee80211ScalarRadioMedium;
        configurator: Ipv4NetworkConfigurator;
        host[numHosts]: GpsrRouter;
}
```

### Configuration File (conceptual)
```ini
# File: simulations/short_pass_by/omnetpp.ini
[Config ShortPassBy]
network = ShortPassByNetwork
sim-time-limit = 500s
result-dir = results/short_pass_by

# Mobility: Linear pass-by
**.mobility.typename = "LinearMobility"
**.mobility.speed = 15mps
**.mobility.constraintAreaMinX = 0m
**.mobility.constraintAreaMaxX = 1500m
**.mobility.constraintAreaMinY = 0m
**.mobility.constraintAreaMaxY = 500m
**.mobility.updateInterval = 0.1s

# Group A: Moving east (host[0..9])
**.host[0..9].mobility.initialX = (index % 10) * 100m
**.host[0..9].mobility.initialY = 250m
**.host[0..9].mobility.initialMovementHeading = 0deg

# Group B: Moving west (host[10..19])
**.host[10..19].mobility.initialX = 1500m - ((index - 10) % 10) * 100m
**.host[10..19].mobility.initialY = 250m
**.host[10..19].mobility.initialMovementHeading = 180deg

# Traffic: Inter-group flows (5 flows)
**.numApps = 1
**.app[0].typename = "UdpBasicApp"
**.app[0].destAddresses = ""
**.app[0].localPort = 5000
**.app[0].destPort = 5000
**.app[0].messageLength = 512B
**.app[0].sendInterval = exponential(0.5s)  # High rate to capture short window
**.app[0].startTime = uniform(0s, 5s)
**.app[0].stopTime = -1s

# Source flows (Group A → Group B)
**.host[0].app[0].destAddresses = "host[15]"
**.host[2].app[0].destAddresses = "host[16]"
**.host[4].app[0].destAddresses = "host[17]"
**.host[6].app[0].destAddresses = "host[18]"
**.host[8].app[0].destAddresses = "host[19]"

# Radio: 802.11 Scalar
**.wlan[*].typename = "Ieee80211Interface"
**.wlan[*].radio.typename = "Ieee80211ScalarRadio"
**.wlan[*].radio.transmitter.power = 2mW
**.wlan[*].radio.transmitter.communicationRange = 250m
**.wlan[*].mac.typename = "Ieee80211Mac"

# GPSR: Faster beaconing for mobility
**.gpsr.beaconInterval = 5s  # More frequent updates under mobility
**.gpsr.maxJitter = 2s
**.gpsr.neighborValidityInterval = 20s  # Shorter timeout
**.gpsr.interfaces = "*"

# Seeds
seed-set = ${repetition}
repeat = 10
```

---

## Validation Checks

### Before Full Run
1. **Verify pass-by timing**:
   - Run 10s test simulation
   - Check positions at t=10s: groups should be ~150m apart
   - Verify movement directions (Group A: X increasing, Group B: X decreasing)

2. **Verify connection window**:
   ```bash
   # Extract neighbor table size over time
   opp_scavetool query -f 'module =~ *.host[0].gpsr AND name =~ numNeighbors:vector' results/*.vec
   ```
   - Expected: numNeighbors peaks at t=18s (connection window)

3. **Check perimeter mode activation**:
   ```bash
   opp_scavetool query -f 'module =~ *.gpsr AND name =~ perimeterMode*' results/*.sca
   ```
   - Expected: perimeterModeCount > 0 (greedy failures occur)

### Expected Metrics
- **Connection window**: ~17s per pass-by event
- **TCR**: 0.6-0.75 (high loss due to mobility)
- **Perimeter mode**: 20-40% of packets (greedy failures common)

---

## Summary

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| **Nodes** | 20 (2 groups of 10) | High mobility stress |
| **Speed** | 15 m/s (54 km/h) | Vehicular scenario |
| **Direction** | Opposite (pass-by) | Short connection window |
| **Range** | 250m | 17s connection time |
| **Flows** | 5 inter-group | Focus on link breakage |
| **Interval** | exponential(0.5s) | Capture short window |
| **Beacon** | 5s (faster) | Track high mobility |
| **Duration** | 500s | Multiple pass-by events |
| **Seeds** | 10 | High variance due to timing |

**Next Steps** (after Phase 1):
1. Implement `ShortPassByNetwork.ned`
2. Configure `omnetpp.ini` with linear mobility
3. Run 10s test → verify pass-by timing
4. Run baseline GPSR → expect TCR ~0.65
5. Run two-hop peek variant → expect TCR ~0.75 (link anticipation)
6. Analyze perimeter mode statistics

---

**References**:
- INET multiradio MANET example: `inet4.5/examples/manetrouting/multiradio/omnetpp.ini`
- INET LinearMobility: `inet4.5/src/inet/mobility/single/LinearMobility.ned`
- METRICS_MAP.md: E2E delay and TCR collection
