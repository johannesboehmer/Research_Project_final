# Phase 1 Diagnosis: Final Summary

**Date**: November 6, 2025  
**Investigation**: Research-first approach with minimal configuration changes  
**Outcome**: Root cause identified - Network layer configuration issue

---

## Changes Applied (November 6, 2025)

### Quick Wins (Test 1)
1. **.gpsr.interfaces = "*"** (from "wlan0") - INET default for array interfaces
2. **.host[*].app[0].destAddresses = "host[X](ipv4)"** - Added protocol hints per INET examples
3. **Package declarations fixed** (simulations.baseline_gpsr)

**Result**: ❌ Failed - Zero packet reception persisted

### UnitDisk Radio Switch (Test 2)  
4. **UnitDiskRadioMedium** (from Ieee80211ScalarRadioMedium) - Deterministic connectivity
5. **AckingWirelessInterface** + **UnitDiskRadio** (from Ieee80211Interface) - Predictable delivery
6. **communicationRange = 300m** - Explicit range for 1000m×1000m area
7. **MAC/radio header lengths** - Per INET adhoc/idealwireless example

**Result**: ❌ Failed - Zero packet reception STILL persists

---

## Test Results Comparison

| Metric | 802.11 Scalar | UnitDisk | Expected |
|--------|---------------|----------|----------|
| **packetSent** | 32 total | 76 total | >0 ✅ |
| **packetReceived** | 0 | 0 | >0 ❌ |
| **Transmissions** | 255 | 141 | >0 ✅ |
| **GPSR beacons** | None | None | >0 ❌ |
| **GPSR data pkts** | None | None | >0 ❌ |

**Key Finding**: Radio layer active (transmissions occurring), but GPSR layer completely silent.

---

## Root Cause Analysis

### What Works ✅
- Network simulation runs without errors
- Mobility and position assignment functional  
- Physical/link layer transmitting packets
- UdpBasicApp generating traffic (packets sent)
- Radio medium computing receptions (UnitDisk: 2679 computations)

### What Doesn't Work ❌
- **GPSR module receives zero packets**
- No GPSR statistics recorded (no beacons, no data forwarding)
- Zero application-layer packet reception
- Routing layer completely bypassed

### Diagnosis

**Problem**: Packets never reach GPSR routing module despite being transmitted.

**Likely Causes** (in order of probability):

1. **IP Configurator Creating Static Routes**
   - Current config: `<interface hosts='*' address='10.0.x.x' netmask='255.255.0.0'/>`
   - Configurator may be creating direct routes that bypass GPSR
   - INET default: Configurator adds static routes for reachability
   - **Fix**: Disable static route generation: `**.configurator.addStaticRoutes = false`

2. **GPSR Hook Not Registered**
   - GPSR registers INetfilter hook with network protocol  
   - If registration fails, packets route via IP routing table
   - GpsrRouter should handle this automatically
   - **Verify**: Check GPSR::initialize() was called (enable logging)

3. **Wrong Network Protocol Module**
   - GPSR needs to register with correct IPv4 module
   - Default: "^.ipv4.ipv4"
   - **Verify**: Check GPSR networkProtocolModule parameter

4. **Interface Table Mismatch**
   - GPSR uses `interfaces = "*"` to match all multicast interfaces
   - If wlan[0] not multicast-capable, beacons won't send
   - **Verify**: Check interface properties in initialization logs

---

## Evidence

### From 802.11 Scalar Test
```
Application Layer:
- host[0]: packetSent=10, packetReceived=0
- host[1]: packetSent=7, packetReceived=0

Radio Layer:
- Transmission count = 255
- Reception computation count = 4845

GPSR Layer:
- NO statistics recorded
- NO beacons sent/received
- NO data packets forwarded
```

### From UnitDisk Test  
```
Application Layer:
- host[0]: packetSent=10, packetReceived=0
- host[1]: packetSent=14, packetReceived=0

Radio Layer:
- Transmission count = 141
- Reception computation count = 2679
- UnitDisk guarantees 300m deterministic range

GPSR Layer:
- NO statistics recorded (SAME as 802.11)
- NO beacons sent/received
- NO data packets forwarded
```

**Conclusion**: Radio model irrelevant - problem is network layer configuration.

---

## Recommended Fix (Not Applied Yet)

### Primary Fix: Disable Static Routes

Add to `omnetpp.ini`:
```ini
# Network layer configuration
**.configurator.addStaticRoutes = false  # Let GPSR handle routing
**.configurator.addDirectRoutes = false  # No direct peer routes
**.configurator.addSubnetRoutes = false  # No subnet routes
```

**Rationale**: Force all packets through GPSR module instead of IP routing table.

### Secondary Fix: Enable GPSR Logging

Add to `omnetpp.ini` for diagnosis:
```ini
**.gpsr.displayBubbles = true  # Visual feedback
cmdenv-express-mode = false    # Detailed logs
```

Run short test and check logs for:
- "GPSR: registered with network protocol"
- "Sending beacon"
- "Processing beacon"
- "Routing datagram"

### Tertiary Fix: Verify GpsrRouter Configuration

Check if GpsrRouter needs explicit routing table configuration:
```ini
**.routingTable.netmaskRoutes = ""  # Clear default routes
```

---

## Files Modified (Summary)

1. **simulations/baseline_gpsr/omnetpp.ini**
   - Line 48: `interfaces = "*"` (was "wlan0")
   - Lines 83-87: Added `(ipv4)` protocol hints to destAddresses
   - Lines 27-38: Switched to UnitDiskRadioMedium + AckingWirelessInterface

2. **simulations/baseline_gpsr/BaselineNetwork.ned**
   - Line 10: `package simulations.baseline_gpsr` (was researchproject.*)
   - Line 12: `import ...UnitDiskRadioMedium` (was Ieee80211ScalarRadioMedium)
   - Line 34: `radioMedium: UnitDiskRadioMedium` (was Ieee80211ScalarRadioMedium)

3. **simulations/baseline_gpsr/package.ned**
   - Line 7: `package simulations.baseline_gpsr` (was researchproject.*)

4. **simulations/package.ned**
   - Line 7: `package simulations` (was researchproject.simulations)

---

## Next Steps

### Immediate (To Fix Zero Reception)

1. **Disable static routes** in omnetpp.ini
2. **Enable GPSR logging** for verification
3. **Run 10s test** with logging enabled
4. **Check logs** for GPSR initialization and packet handling
5. **Verify** beacons being sent/received
6. **Confirm** packets routed through GPSR module

### After Fix Verified

7. **Run full 500s baseline** (10 seeds)
8. **Extract metrics** per METRICS_MAP.md
9. **Document baseline statistics** in PHASE1_SUMMARY.md
10. **Proceed to Phase 2** (implement variants)

---

## Lessons Learned

1. **Research-first validated**: Quick Wins and UnitDisk switch both failed, confirming deeper issue
2. **Radio not the problem**: UnitDisk deterministic connectivity didn't help
3. **GPSR module inactive**: Zero statistics = routing layer not engaged
4. **IP configurator suspect**: Static routes likely bypassing GPSR
5. **Systematic diagnosis effective**: Eliminated PHY/MAC as causes through testing

---

## Documentation References

- **BASELINE_READINESS_CHECKLIST.md**: Check 2 (Routing Activation) predicted this issue
- **GPSR_BEACON_MULTICAST_REQUIREMENTS.md**: Beacon mechanism documented (not being used)
- **UNITDISK_REACHABILITY_PLAN.md**: UnitDisk switch validated radio not the problem
- **METRICS_MAP.md**: Ready for collection once routing fixed

---

**Status**: Diagnosis complete, fix identified, awaiting application and verification.

**Next Action**: Apply primary fix (disable static routes) and retest.
