# UnitDisk Reachability Plan

**Purpose**: Document INET UnitDisk radio models for predictable Step 0 connectivity testing, with migration path to 802.11 realism.

**Status**: Research-only (no configuration changes yet)

---

## UnitDisk Radio Models in INET

### Core Components

1. **UnitDiskRadioMedium** (`inet.physicallayer.wireless.unitdisk.UnitDiskRadioMedium`)
   - Simple, fast, predictable physical layer
   - Analog model: `UnitDiskAnalogModel`
   - Must be used with compatible UnitDisk radios
   - Location: `inet4.5/src/inet/physicallayer/wireless/unitdisk/UnitDiskRadioMedium.ned`

2. **UnitDiskRadio** (`inet.physicallayer.wireless.unitdisk.UnitDiskRadio`)
   - Basic unit disk radio implementation
   - Transmitter: `UnitDiskTransmitter`
   - Receiver: `UnitDiskReceiver`
   - Distance-based success/failure (no interference modeling)
   - Location: `inet4.5/src/inet/physicallayer/wireless/unitdisk/UnitDiskRadio.ned`

3. **Ieee80211UnitDiskRadio** (`inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211UnitDiskRadio`)
   - Extends Ieee80211Radio with UnitDisk analog representation
   - Transmitter: `Ieee80211UnitDiskTransmitter`
   - Receiver: `Ieee80211UnitDiskReceiver`
   - Provides 802.11 framing with UnitDisk propagation
   - Location: `inet4.5/src/inet/physicallayer/wireless/ieee80211/packetlevel/Ieee80211UnitDiskRadio.ned`

---

## INET Example Configurations

### Pattern 1: AckingWirelessInterface with UnitDiskRadio
**Source**: `inet4.5/examples/adhoc/idealwireless/omnetpp.ini` and `inet4.5/examples/aodv/omnetpp.ini`

```ini
# Radio Medium
*.radioMedium.typename = "UnitDiskRadioMedium"

# Interface Configuration
**.wlan[*].typename = "AckingWirelessInterface"
**.wlan[*].radio.typename = "UnitDiskRadio"
**.wlan[*].radio.transmitter.communicationRange = 200m
**.wlan[*].radio.transmitter.interferenceRange = 0m  # No interference modeling
**.wlan[*].radio.receiver.ignoreInterference = true

# MAC layer: no MAC protocol, immediate forwarding
**.wlan[*].mac.typename = "AckingMac"
**.wlan[*].mac.fullDuplex = false
**.wlan[*].mac.useAck = true
**.wlan[*].mac.ackTimeout = 100us
```

**Characteristics**:
- **Deterministic**: Packet delivered if distance ≤ communicationRange
- **No MAC contention**: AckingMac bypasses CSMA/CA
- **Immediate acknowledgments**: No backoff, no collisions
- **Zero interference modeling**: ignoreInterference = true

**Use case**: Baseline routing protocol verification without PHY/MAC complexity.

### Pattern 2: Full 802.11 with UnitDiskRadio
**Source**: INET wireless/scaling examples (not common in MANET examples)

```ini
# Radio Medium
*.radioMedium.typename = "UnitDiskRadioMedium"

# Interface Configuration
**.wlan[*].typename = "Ieee80211Interface"
**.wlan[*].radio.typename = "Ieee80211UnitDiskRadio"
**.wlan[*].radio.transmitter.communicationRange = 250m

# MAC layer: full CSMA/CA
**.wlan[*].mac.typename = "Ieee80211Mac"
**.wlan[*].mac.dcf.channelAccess.cwMin = 15
**.wlan[*].mac.dcf.channelAccess.cwMax = 1023
```

**Characteristics**:
- **Predictable range**: UnitDisk physics, but with MAC layer complexity
- **CSMA/CA active**: Realistic contention, backoff, RTS/CTS
- **Interference possible**: MAC-level collisions can occur
- **802.11 framing**: Full MAC headers, beacons, management frames

**Use case**: Test routing with MAC contention but predictable radio propagation.

---

## Current Baseline Configuration

**File**: `simulations/baseline_gpsr/omnetpp.ini`

```ini
# Current Configuration (NOT UnitDisk)
*.radioMedium.typename = "Ieee80211ScalarRadioMedium"

**.wlan[*].typename = "Ieee80211Interface"
**.wlan[*].radio.typename = "Ieee80211ScalarRadio"
**.wlan[*].radio.transmitter.power = 2mW
**.wlan[*].radio.transmitter.headerLength = 96b
**.wlan[*].radio.transmitter.bitrate = 2Mbps

**.wlan[*].mac.typename = "Ieee80211Mac"
**.wlan[*].mac.dcf.typename = "Dcf"
```

**Issues for Step 0**:
- **Path loss modeling**: ScalarRadioMedium uses log-distance path loss, signal attenuation
- **Non-deterministic**: Packet success depends on SNR, interference, fading
- **MAC complexity**: Full CSMA/CA adds contention, backoff, collisions
- **Hard to isolate routing issues**: PHY/MAC failures mask routing problems

---

## Migration Plan for Baseline

### Step 0: Switch to UnitDisk for Predictability

**Goal**: Establish known-good connectivity to isolate routing/application issues.

**Configuration changes** (to be applied later):
```ini
# Replace radioMedium
*.radioMedium.typename = "UnitDiskRadioMedium"

# Replace interface with AckingWirelessInterface
**.wlan[*].typename = "AckingWirelessInterface"
**.wlan[*].radio.typename = "UnitDiskRadio"
**.wlan[*].radio.transmitter.communicationRange = 300m  # Ensure multi-hop
**.wlan[*].radio.transmitter.interferenceRange = 0m
**.wlan[*].radio.receiver.ignoreInterference = true

# Use AckingMac (no CSMA/CA)
**.wlan[*].mac.typename = "AckingMac"
**.wlan[*].mac.fullDuplex = false
**.wlan[*].mac.useAck = true
**.wlan[*].mac.ackTimeout = 100us
```

**Rationale**:
- Node spacing: 1000m × 1000m area, 20 nodes → ~200m average spacing
- communicationRange = 300m ensures single-hop neighbors while requiring multi-hop routing
- AckingMac eliminates MAC-layer packet loss, focusing on routing layer
- Matches AODV example pattern for routing verification

### Step 1: Verify Baseline with UnitDisk

**Expected outcomes**:
1. ✅ All beacons delivered deterministically within communicationRange
2. ✅ GPSR neighbor discovery completes reliably
3. ✅ Application packets routed successfully (packetReceived > 0)
4. ✅ Hop counts stable and predictable across seeds

**Verification commands**:
```bash
cd simulations/baseline_gpsr
opp_scavetool query -f 'module =~ *.host[*].app[0] AND name =~ packet*' results/*.sca
opp_scavetool query -f 'module =~ *.host[*].gpsr AND name =~ beacon*' results/*.sca
```

### Step 2: Switch Back to 802.11 Scalar for Realism

**After baseline verification**, re-enable realistic PHY/MAC:
```ini
# Restore 802.11 Scalar Radio
*.radioMedium.typename = "Ieee80211ScalarRadioMedium"
**.wlan[*].typename = "Ieee80211Interface"
**.wlan[*].radio.typename = "Ieee80211ScalarRadio"
**.wlan[*].radio.transmitter.power = 2mW  # Adjust for 300m range
**.wlan[*].mac.typename = "Ieee80211Mac"
```

**Rationale**:
- UnitDisk validates routing logic is correct
- 802.11 Scalar adds realistic PHY/MAC challenges
- Compare metrics: UnitDisk (ideal) vs. 802.11 (realistic) to quantify MAC impact

---

## Example NED Changes (Conceptual - Not Applied Yet)

### Current: BaselineNetwork.ned (excerpt)
```ned
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.gpsr.GpsrRouter;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;

network BaselineNetwork {
    parameters:
        @display("bgb=1000,1000");
        int numHosts = default(20);
    submodules:
        radioMedium: Ieee80211ScalarRadioMedium;  // <-- To be changed
        configurator: Ipv4NetworkConfigurator;
        host[numHosts]: GpsrRouter;  // <-- GpsrRouter already has wlan[*]
}
```

### Proposed: UnitDisk variant (conceptual)
```ned
// No NED changes needed! Only omnetpp.ini changes required:
// **.radioMedium.typename assignment overrides NED type
```

**Key insight**: INET's typename assignment in .ini files allows switching radio models without editing NED files.

---

## Hardcoded Radio Examples in INET

### AODV Example (UnitDisk)
**File**: `inet4.5/examples/aodv/omnetpp.ini`
- Uses: `AckingWirelessInterface` + `UnitDiskRadio`
- communicationRange: 250m
- Topology: 20 nodes, similar to our baseline

### GPSR Example (802.11 Scalar)
**File**: `inet4.5/examples/manetrouting/gpsr/omnetpp.ini`
- Uses: `Ieee80211Interface` + `Ieee80211ScalarRadio`
- No UnitDisk variant provided
- Uses PingApp (not UdpBasicApp)

### Adhoc Idealwireless Example (UnitDisk)
**File**: `inet4.5/examples/adhoc/idealwireless/omnetpp.ini`
- Uses: `AckingWirelessInterface` + `UnitDiskRadio`
- communicationRange: 200m
- Demonstrates "ideal" connectivity for protocol testing

---

## Decision: UnitDisk for Step 0 Only

**Why UnitDisk first**:
1. **Isolate variables**: Remove PHY/MAC uncertainty from routing verification
2. **INET precedent**: AODV example uses UnitDisk for baseline validation
3. **Fast debugging**: Deterministic connectivity → routing issues easier to diagnose
4. **Known-good baseline**: Establish working configuration before adding complexity

**Why return to 802.11 Scalar**:
1. **Realistic evaluation**: QueueGPSR performance under real channel conditions
2. **Fair comparison**: Standard GPSR also evaluated with 802.11 Scalar
3. **Publication validity**: Research requires realistic MAC/PHY modeling

**Migration timing**:
- Phase 1: UnitDisk baseline verification (current)
- Phase 2-5: 802.11 Scalar for all variant testing (delay_tiebreaker, queue_aware, etc.)

---

## Summary

**UnitDisk Models**:
- `UnitDiskRadioMedium` + `UnitDiskRadio` + `AckingWirelessInterface` → simplest, deterministic
- `UnitDiskRadioMedium` + `Ieee80211UnitDiskRadio` + `Ieee80211Interface` → 802.11 MAC with deterministic PHY

**Current Status**:
- Baseline uses: `Ieee80211ScalarRadioMedium` + `Ieee80211ScalarRadio` (realistic, non-deterministic)

**Next Steps** (after diagnosis):
1. Switch to UnitDisk for Step 0 verification
2. Verify baseline metrics with deterministic connectivity
3. Return to 802.11 Scalar for Phase 2+ evaluation

**References**:
- AODV example: `inet4.5/examples/aodv/omnetpp.ini` (lines 1-80)
- UnitDiskRadio NED: `inet4.5/src/inet/physicallayer/wireless/unitdisk/UnitDiskRadio.ned`
- Ieee80211UnitDiskRadio NED: `inet4.5/src/inet/physicallayer/wireless/ieee80211/packetlevel/Ieee80211UnitDiskRadio.ned`
