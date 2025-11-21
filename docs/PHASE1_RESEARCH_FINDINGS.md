# Phase 1: Baseline GPSR Research Findings

**Date:** November 6, 2025  
**Status:** Research Complete, Ready for Baseline Verification  

## INET GPSR Structure Analysis

### Module Hierarchy

```
inet.routing.gpsr.Gpsr (simple module)
  ↓ implements
inet.routing.contract.IManetRouting (interface)
  ↓ extends
inet.routing.base.RoutingProtocolBase (base class)
```

### Key Files Examined

1. **`inet/routing/gpsr/Gpsr.ned`** - Module definition
   - Simple module implementing IManetRouting
   - Gates: `ipIn`, `ipOut`
   - Parameters: beaconInterval, neighborValidityInterval, planarizationMode, etc.

2. **`inet/routing/gpsr/Gpsr.h`** - Class interface
   - Extends `RoutingProtocolBase`
   - Implements `cListener` and `NetfilterBase::HookBase`
   - Uses `PositionTable` for neighbor tracking
   - Key components: beacon timer, purge timer, position table

3. **`inet/routing/gpsr/PositionTable.h`** - Neighbor table
   - Maps L3Address → (timestamp, Coord)
   - Methods: getPosition, setPosition, removeOldPositions
   - Simple std::map based storage

4. **`inet/node/gpsr/GpsrRouter.ned`** - Convenience node type
   - Extends `AdhocHost`
   - Includes `gpsr` submodule
   - Connections to traffic node (tn)

### Parameters to Mirror

```ned
// Context parameters
string interfaceTableModule;
string routingTableModule = default("^.ipv4.routingTable");
string networkProtocolModule = default("^.ipv4.ip");
string outputInterface = default("wlan0");

// GPSR algorithm parameters
string planarizationMode @enum("", "GG", "RNG") = default("GG");
string interfaces = default("*");
double beaconInterval @unit(s) = default(10s);
double maxJitter @unit(s) = default(0.5 * beaconInterval);
double neighborValidityInterval @unit(s) = default(4.5 * beaconInterval);
int positionByteLength @unit(B) = default(2 * 4B);
bool displayBubbles = default(false);
```

### Gates Structure

```ned
gates:
    input ipIn;    // From network layer
    output ipOut;  // To network layer
```

### Key Mechanisms Identified

#### 1. Beaconing
- Periodic beacons sent at `beaconInterval` with `maxJitter`
- Beacons carry position (Coord) and address
- Used for neighbor discovery

#### 2. Position Table
- Stores neighbor positions with timestamps
- Purged periodically based on `neighborValidityInterval`
- Simple map structure: Address → (time, position)

#### 3. Next-Hop Selection
- Greedy mode: Choose neighbor closest to destination
- Perimeter mode: Planarization (GG or RNG) for recovery
- Uses position information only (no queue, delay, etc.)

#### 4. Message Flow
- Packets come from network layer via `ipIn`
- GPSR adds routing header/option
- Packets sent to network layer via `ipOut`
- Beacons sent periodically as UDP packets

### INET Example Configuration

From `inet/examples/manetrouting/gpsr/omnetpp.ini`:

```ini
# Key settings
**.wlan[*].bitrate = 2Mbps
**.wlan[*].mac.dcf.channelAccess.cwMin = 7
**.wlan[*].radio.transmitter.power = 2mW

# Mobility constraints
**.mobility.constraintAreaMaxX = 1000m
**.mobility.constraintAreaMaxY = 1000m

# Random number generators
num-rngs = 3
**.mobility.rng-0 = 1
**.wlan[*].mac.rng-0 = 2
```

### Metrics Available in INET GPSR

Based on INET structure, the following metrics are available:

#### From Network Layer
- **End-to-end delay**: Application layer timestamps
- **Packet delivery ratio**: Sent vs. received packets
- **Hop count**: Incremented in routing headers

#### From GPSR Module
- **Routing overhead**: Beacon count vs. data packets
- **Mode switches**: Greedy ↔ Perimeter transitions
- **Position table size**: Neighbor count over time

#### From Link Layer
- **MAC queue length**: Available from wlan[*].mac.queue
- **Transmission attempts**: MAC statistics
- **Collisions**: MAC statistics

## What to Mirror in `researchproject.routing.queuegpsr`

### Phase 1: Initial QueueGpsr Module

For baseline equivalence, create:

1. **`QueueGpsr.ned`** - Initially identical to INET's Gpsr.ned
   - Same parameters
   - Same gates
   - Implements IManetRouting

2. **`QueueGpsr.h`** - Extend RoutingProtocolBase
   - Same class structure as Gpsr
   - Keep PositionTable usage
   - No modifications yet

3. **`QueueGpsr.cc`** - Copy INET logic
   - Same beaconing mechanism
   - Same next-hop selection
   - Verify behavior matches INET

### Phase 2+: Extensions

Once baseline verified, extend with:

- **Delay measurement** in PositionTable
- **Queue inspection** via linklayer.queue package
- **Two-hop information** in beacons
- **Compute metrics** in neighbor table

## Package Structure to Use

```
researchproject.routing.queuegpsr     (package)
├── QueueGpsr.ned                     (module definition)
├── QueueGpsr.h                       (class interface)
├── QueueGpsr.cc                      (implementation)
├── QueuePositionTable.h              (extended neighbor table)
├── QueuePositionTable.cc             
├── QueueGpsr.msg                     (message definitions)
└── package.ned                       (package declaration) ✓ Already exists
```

This mirrors:
```
inet.routing.gpsr                     (package)
├── Gpsr.ned
├── Gpsr.h
├── Gpsr.cc
├── PositionTable.h
├── PositionTable.cc
└── Gpsr.msg
```

## Baseline Scenario Validation

The prepared `simulations/baseline_gpsr/` uses:
- **INET's standard Gpsr** (not custom yet)
- **AdhocHost** nodes with GPSR routing
- **Configuration** matches INET examples

### Expected Behavior

1. Nodes beacon every 10s
2. Position table updated from beacons
3. Greedy routing toward destinations
4. Perimeter mode if greedy fails
5. Metrics collected in `results/baseline_gpsr/`

## Next Steps for Phase 1

1. ✅ Research complete - documented above
2. ⏳ Run baseline with INET GPSR
3. ⏳ Verify metrics collection
4. ⏳ Validate result paths
5. ⏳ Test multiple seeds
6. ⏳ Document observations
7. ⏳ Mark Phase 1 complete

## References

- INET GPSR source: `inet4.5/src/inet/routing/gpsr/`
- INET examples: `inet4.5/examples/manetrouting/gpsr/`
- GPSR paper: Karp & Kung, MobiCom 2000
- IManetRouting interface: `inet4.5/src/inet/routing/contract/`

## Notes

- INET uses global position table (KLUDGE noted in source)
- Position piggybacking not yet implemented in INET
- Promiscuous mode not implemented in INET
- Our extensions will add queue-awareness, delay metrics, etc.
- Must maintain compatibility with IManetRouting interface
