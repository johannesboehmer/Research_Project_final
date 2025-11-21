# Baseline Readiness Checklist

**Purpose**: Ordered verification steps for baseline GPSR configuration before full 500s runs.

**Status**: Diagnostic phase (test run complete, zero packet reception issue identified)

---

## Current Status Summary

### Test Run Results (10s, 10 seeds)
- ✅ Simulation completes without errors
- ✅ Results properly directed to `results/baseline_gpsr/`
- ✅ 30 files generated (10× .sca, 10× .vec, 10× .vci)
- ❌ **Application layer**: packetSent > 0, but packetReceived = 0 (ALL hosts)
- ✅ Transmission activity observed (201-1287 transmissions per seed)
- ❌ **Zero packet delivery**: Indicates routing/connectivity failure

---

## Diagnostic Checklist (Execute in Order)

### ☐ Check 1: Verify Traffic Endpoints and Address Resolution

**Goal**: Confirm UdpBasicApp can resolve destination addresses and bind UDP sockets.

#### INET Module to Consult
- **UdpBasicApp**: `inet4.5/src/inet/applications/udpapp/UdpBasicApp.{ned,cc}`
- **L3AddressResolver**: `inet4.5/src/inet/networklayer/common/L3AddressResolver.cc`

#### Verification Steps

1. **Check address resolution format**:
   ```bash
   # Current baseline configuration
   grep "destAddresses" simulations/baseline_gpsr/omnetpp.ini
   ```
   - **Current**: `**.host[0].app[0].destAddresses = "host[15]"`
   - **INET GPSR example**: `**.host[0].app[0].destAddr = "host[1](ipv4)"`
   - **Issue**: UdpBasicApp may require protocol hint for resolution

2. **Inspect initialization logs**:
   ```bash
   # Run single-seed simulation with verbose logging
   cd simulations/baseline_gpsr
   ../../inet4.5/src/run_inet -u Cmdenv -c Baseline --repeat=0 --sim-time-limit=10s --cmdenv-express-mode=false
   ```
   - **Expected log**: "UdpBasicApp: resolved host[15] to 10.0.0.15"
   - **Failure log**: "UdpBasicApp: cannot resolve host[15]" → address resolution failure

3. **Compare with INET PingApp pattern**:
   - INET GPSR example uses `PingApp` with `destAddr = "host[1](ipv4)"`
   - PingApp explicitly hints protocol for L3AddressResolver
   - **Hypothesis**: UdpBasicApp needs similar format or configurator support

#### Potential Fixes (Not Applied Yet)
```ini
# Option A: Add protocol hint
**.host[0].app[0].destAddresses = "host[15](ipv4)"

# Option B: Use IP address directly (after configurator assigns)
**.host[0].app[0].destAddresses = "10.0.0.15"

# Option C: Switch to PingApp for baseline verification
**.app[0].typename = "PingApp"
**.host[0].app[0].destAddr = "host[15](ipv4)"
```

#### Reference
- L3AddressResolver code: `inet4.5/src/inet/networklayer/common/L3AddressResolver.cc` (lines 50-100)
- UdpBasicApp initialization: `inet4.5/src/inet/applications/udpapp/UdpBasicApp.cc::initialize()`

---

### ☐ Check 2: Verify Routing Activation and IP Assignment

**Goal**: Confirm GPSR module receives packets from network layer and has valid routes.

#### INET Module to Consult
- **Gpsr**: `inet4.5/src/inet/routing/gpsr/Gpsr.{ned,cc}`
- **Ipv4NetworkConfigurator**: `inet4.5/src/inet/networklayer/configurator/ipv4/Ipv4NetworkConfigurator.ned`

#### Verification Steps

1. **Check IP address assignment**:
   ```bash
   # Query IP addresses assigned by configurator
   opp_scavetool query -f 'module =~ *.host[*].ipv4.ip AND name =~ assignedAddress' results/Baseline-#0.sca
   ```
   - **Expected**: Each host has unique IP (e.g., 10.0.0.0 - 10.0.0.19)
   - **Failure**: Unspecified address (0.0.0.0) → configurator not running

2. **Check GPSR registration with network layer**:
   ```bash
   # Extract GPSR initialization logs
   grep "Gpsr::initialize" results/*.log
   ```
   - **Expected log**: "GPSR: registered with network protocol IPv4"
   - **Failure log**: "GPSR: no network protocol found" → hook registration failed

3. **Check routing table entries**:
   ```bash
   # Query routing statistics
   opp_scavetool query -f 'module =~ *.host[*].ipv4.routingTable' results/Baseline-#0.sca
   ```
   - **Expected**: No static routes (GPSR uses geographic routing, not table-based)
   - **Verification**: GPSR should intercept packets via INetfilter hook

4. **Inspect network configurator settings**:
   ```ini
   # Current configuration (from omnetpp.ini)
   **.configurator.config = xmldoc("network-config.xml", "config.xml")
   ```
   - **Check**: If XML file missing, configurator may fail silently
   - **Alternative**: Use auto-assignment (remove config parameter)

#### Potential Fixes (Not Applied Yet)
```ini
# Option A: Ensure configurator auto-assigns IPs
**.configurator.config = ""  # Use default auto-assignment
**.configurator.addStaticRoutes = false  # GPSR handles routing

# Option B: Explicitly set IP range
**.configurator.assignAddresses = true
**.configurator.assignDisjunctSubnetAddresses = false
```

#### Reference
- Gpsr hook registration: `inet4.5/src/inet/routing/gpsr/Gpsr.cc::initialize()` (lines 85-103)
- Ipv4NetworkConfigurator: `inet4.5/src/inet/networklayer/configurator/ipv4/Ipv4NetworkConfigurator.ned`

---

### ☐ Check 3: Verify Neighbor Discovery via Beacons

**Goal**: Confirm GPSR nodes send, receive, and process beacons to populate neighbor tables.

#### INET Module to Consult
- **Gpsr beacon mechanism**: `inet4.5/src/inet/routing/gpsr/Gpsr.cc` (lines 206-242)
- **PositionTable**: `inet4.5/src/inet/routing/gpsr/PositionTable.{h,cc}`

#### Verification Steps

1. **Check beacon sent vs. received**:
   ```bash
   # Query beacon statistics
   opp_scavetool query -f 'module =~ *.host[*].gpsr AND name =~ beacon*' results/Baseline-#0.sca
   ```
   - **Expected**: beaconsSent ≈ (10s / 10s interval) = 1 beacon per node
   - **Expected**: beaconsReceived > 0 (neighbors within range receive)
   - **Failure**: beaconsSent > 0 but beaconsReceived = 0 → multicast delivery failure

2. **Check multicast group membership**:
   ```bash
   # Extract multicast join logs
   grep "joinMulticastGroup" results/*.log
   grep "224.0.0.109" results/*.log
   ```
   - **Expected log**: Each host joins `224.0.0.109` on `wlan[0]`
   - **Failure log**: No join messages → interface mismatch or timing issue

3. **Check neighbor table population**:
   ```bash
   # Query neighbor count over time
   opp_scavetool query -f 'module =~ *.host[*].gpsr AND name =~ numNeighbors:vector' results/Baseline-#0.vec
   ```
   - **Expected**: numNeighbors > 0 after first beacon (t≈10s)
   - **Failure**: numNeighbors = 0 throughout → beacons not received or processed

4. **Check GPSR interfaces parameter**:
   ```ini
   # Current configuration
   **.gpsr.interfaces = "wlan0"
   ```
   - **Potential issue**: GpsrRouter uses `wlan[*]` array, actual interface name may be "wlan[0]" not "wlan0"
   - **INET default**: `interfaces = "*"` (matches all)

#### Potential Fixes (Not Applied Yet)
```ini
# Option A: Match all interfaces (INET default)
**.gpsr.interfaces = "*"

# Option B: Use explicit array notation
**.gpsr.interfaces = "wlan[0]"

# Option C: Enable verbose beacon logging
**.gpsr.displayBubbles = true  # Visual beacon transmission indicators
```

#### Reference
- GPSR beacon sending: `inet4.5/src/inet/routing/gpsr/Gpsr.cc::sendBeacon()` (lines 219-234)
- Multicast group join: `inet4.5/src/inet/routing/gpsr/Gpsr.cc::configureInterfaces()` (lines 276-284)
- Multicast address: `inet4.5/src/inet/networklayer/contract/ipv4/Ipv4Address.cc:39` (224.0.0.109)

---

### ☐ Check 4: Verify Multi-Hop Radio Reachability

**Goal**: Confirm radio propagation allows multi-hop connectivity (nodes can communicate beyond direct neighbors).

#### INET Module to Consult
- **Ieee80211ScalarRadioMedium**: `inet4.5/src/inet/physicallayer/wireless/ieee80211/packetlevel/Ieee80211ScalarRadioMedium.ned`
- **Path loss model**: `inet4.5/src/inet/physicallayer/wireless/common/pathloss/*`

#### Verification Steps

1. **Check radio transmission statistics**:
   ```bash
   # Query transmission counts
   opp_scavetool query -f 'module =~ *.host[*].wlan[*].radio AND name =~ transmission*' results/Baseline-#0.sca
   ```
   - **Current**: transmissionCount = 201-1287 per seed (HIGH activity)
   - **Issue**: Transmissions occurring but not being received at application layer

2. **Check reception statistics**:
   ```bash
   # Query reception counts
   opp_scavetool query -f 'module =~ *.host[*].wlan[*].radio AND name =~ reception*' results/Baseline-#0.sca
   ```
   - **Expected**: receptionCount > 0 (neighbors receive transmissions)
   - **Failure**: receptionCount = 0 → radio medium not delivering packets

3. **Verify path loss configuration**:
   ```ini
   # Current configuration (inferred, not explicit in omnetpp.ini)
   **.radioMedium.pathLoss.typename = "TwoRayGroundReflection"  # INET default
   ```
   - **Issue**: ScalarRadio uses SNR-based reception → packets may fail due to weak signal
   - **Alternative**: UnitDiskRadio uses deterministic range-based delivery

4. **Calculate expected range**:
   - **Transmit power**: 2mW
   - **Bitrate**: 2Mbps
   - **Path loss**: Two-ray ground reflection (distance-dependent)
   - **Expected range**: ~200-300m (depends on receiver sensitivity)

5. **Check node spacing vs. range**:
   ```bash
   # Extract node positions
   opp_scavetool query -f 'module =~ *.host[*].mobility AND name =~ position*' results/Baseline-#0.vec
   ```
   - **Baseline topology**: 1000m × 1000m, 20 nodes → ~223m average spacing
   - **Issue**: If range < spacing, network may be disconnected (no multi-hop paths)

#### Potential Fixes (Not Applied Yet)
```ini
# Option A: Switch to UnitDisk for deterministic connectivity (Step 0)
*.radioMedium.typename = "UnitDiskRadioMedium"
**.wlan[*].typename = "AckingWirelessInterface"
**.wlan[*].radio.typename = "UnitDiskRadio"
**.wlan[*].radio.transmitter.communicationRange = 300m
**.wlan[*].radio.transmitter.interferenceRange = 0m
**.wlan[*].radio.receiver.ignoreInterference = true

# Option B: Increase transmit power for 802.11 Scalar
**.wlan[*].radio.transmitter.power = 20mW  # 10× increase for ~400m range

# Option C: Reduce node area for denser topology
network.bgb = "500,500"  # Smaller area → closer nodes
```

#### Reference
- UnitDiskRadio documentation: `UNITDISK_REACHABILITY_PLAN.md`
- Ieee80211ScalarRadio: `inet4.5/src/inet/physicallayer/wireless/ieee80211/packetlevel/Ieee80211ScalarRadio.ned`
- Path loss models: `inet4.5/src/inet/physicallayer/wireless/common/pathloss/`

---

### ☐ Check 5: Verify GPSR Packet Forwarding

**Goal**: Confirm GPSR receives data packets and attempts forwarding (greedy or perimeter).

#### INET Module to Consult
- **Gpsr datagram forwarding**: `inet4.5/src/inet/routing/gpsr/Gpsr.cc` (lines 400-600)
- **INetfilter hook**: `inet4.5/src/inet/networklayer/contract/INetfilter.h`

#### Verification Steps

1. **Check GPSR packet statistics**:
   ```bash
   # Query GPSR data packet handling
   opp_scavetool query -f 'module =~ *.host[*].gpsr AND name =~ data*' results/Baseline-#0.sca
   ```
   - **Expected**: dataPacketsForwarded > 0 (intermediate hops)
   - **Expected**: dataPacketsReceived > 0 (destination hosts)
   - **Failure**: All zero → GPSR not receiving packets from network layer

2. **Check greedy vs. perimeter mode**:
   ```bash
   # Query routing mode statistics
   opp_scavetool query -f 'module =~ *.host[*].gpsr AND name =~ *Mode*' results/Baseline-#0.sca
   ```
   - **Expected**: greedyModeCount > 0 (normal forwarding)
   - **Optional**: perimeterModeCount = 0 (no greedy failures in sparse topology)
   - **Failure**: All zero → no packets entering GPSR module

3. **Check packet drops**:
   ```bash
   # Query drop statistics
   opp_scavetool query -f 'module =~ *.host[*].gpsr AND name =~ *drop*' results/Baseline-#0.sca
   ```
   - **Potential**: packetsDroppedNoRoute > 0 (destination unreachable)
   - **Potential**: packetsDroppedNoNextHop > 0 (neighbor table empty)

#### Potential Fixes (Not Applied Yet)
- If GPSR never receives packets → Check 1 (address resolution) likely failed
- If GPSR drops all packets → Check 3 (neighbor discovery) likely failed
- If GPSR forwards but destination never receives → Check 4 (radio range) likely failed

#### Reference
- GPSR packet handling: `inet4.5/src/inet/routing/gpsr/Gpsr.cc::routeDatagram()`
- INetfilter hook: `inet4.5/src/inet/networklayer/contract/INetfilter.h`

---

## Recommended Diagnostic Sequence

### Step 1: Quick Wins (Interface and Address Format)
Execute **Check 3** first (easiest fix):
```ini
# Change in omnetpp.ini
**.gpsr.interfaces = "*"  # Was "wlan0"
```

Then execute **Check 1** (address format):
```ini
# Change in omnetpp.ini
**.host[0].app[0].destAddresses = "host[15](ipv4)"  # Add protocol hint
```

**Run 10s test** → If still zero reception, proceed to Step 2.

---

### Step 2: Radio Determinism (UnitDisk Switch)
Execute **Check 4** (deterministic connectivity):
```ini
# Replace radio configuration
*.radioMedium.typename = "UnitDiskRadioMedium"
**.wlan[*].typename = "AckingWirelessInterface"
**.wlan[*].radio.typename = "UnitDiskRadio"
**.wlan[*].radio.transmitter.communicationRange = 300m
**.wlan[*].radio.transmitter.interferenceRange = 0m
**.wlan[*].radio.receiver.ignoreInterference = true
```

**Run 10s test** → If packets delivered, issue is radio propagation (not routing).

---

### Step 3: Deep Dive (Beacon and IP Verification)
Execute **Check 2** and **Check 3** (requires log analysis):
1. Run with `--cmdenv-express-mode=false` for verbose logs
2. Inspect logs for:
   - IP address assignment
   - Multicast group join messages
   - Beacon transmission/reception events
   - GPSR initialization success

**Identify failure point** → Apply targeted fix.

---

## Configuration Stability Check

### Result Directory
```ini
[Config Baseline]
result-dir = results/baseline_gpsr
```
- ✅ **Verified**: Results properly isolated in `baseline_gpsr/` subdirectory

### Seed Configuration
```ini
seed-set = ${repetition}
repeat = 10
```
- ✅ **Verified**: 10 seeds properly configured (0-9)
- ✅ **Reproducibility**: seed-set=${repetition} ensures deterministic RNG

### Simulation Time
```ini
sim-time-limit = 10s  # Test run
# sim-time-limit = 500s  # Full run (after verification)
```
- ✅ **Current**: 10s test run appropriate for diagnosis
- ⏳ **Next**: Switch to 500s after passing all checks

---

## Pass Criteria for Full Run

Before running full 500s simulations, verify:

1. ✅ **Application layer**: packetReceived > 0 (at least one flow delivers packets)
2. ✅ **GPSR neighbor table**: numNeighbors > 0 (neighbor discovery successful)
3. ✅ **GPSR forwarding**: dataPacketsForwarded > 0 (multi-hop routing active)
4. ✅ **Beacon delivery**: beaconsReceived > 0 (multicast working)
5. ✅ **Radio reception**: receptionCount > 0 (radio medium delivering packets)

**Only proceed to 500s run when all criteria pass.**

---

## Summary: Ordered Execution

| Check | Focus | Effort | Impact | Execute When |
|-------|-------|--------|--------|--------------|
| **Check 3** | Interfaces | Low | High | 1st (quick fix) |
| **Check 1** | Addresses | Low | High | 2nd (quick fix) |
| **Check 4** | Radio range | Medium | High | 3rd (UnitDisk switch) |
| **Check 2** | IP/Routing | High | Medium | 4th (log analysis) |
| **Check 5** | Forwarding | High | Low | 5th (after others pass) |

**Recommended**: Execute Checks 3, 1, 4 first (simple ini changes), then run test. If still failing, execute Checks 2 and 5 (log analysis).

---

**References**:
- GPSR_BEACON_MULTICAST_REQUIREMENTS.md: Multicast group join details
- UNITDISK_REACHABILITY_PLAN.md: UnitDisk radio configuration
- METRICS_MAP.md: Statistics collection queries
- INET GPSR example: `inet4.5/examples/manetrouting/gpsr/omnetpp.ini`
