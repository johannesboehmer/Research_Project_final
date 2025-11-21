# Research First: INET Investigation Guide

## Before Implementing Anything

### 1. Locate Analogous INET Modules

```bash
# Navigate to INET source
cd ../inet4.5/src/inet

# For routing logic
ls routing/gpsr/         # Original GPSR implementation
ls routing/base/         # Base routing classes
ls routing/contract/     # Routing interfaces

# For link layer access
ls linklayer/ethernet/
ls linklayer/ieee80211/
ls linklayer/contract/

# For common utilities
ls common/
ls common/packet/
ls common/geometry/
```

### 2. Study INET Examples

```bash
cd ../inet4.5/examples

# GPSR examples
ls manetrouting/gpsr/
cat manetrouting/gpsr/omnetpp.ini
cat manetrouting/gpsr/GPSRNetwork.ned

# Wireless examples
ls wireless/
ls adhoc/
```

### 3. Check Module Interfaces

Key INET interfaces to understand:

**Routing:**
- `inet/routing/contract/IManetRouting.ned` - MANET routing interface
- `inet/routing/base/RoutingProtocolBase.h` - Base class

**Network:**
- `inet/networklayer/contract/IRoutingTable.h`
- `inet/networklayer/contract/IInterfaceTable.h`

**Link Layer:**
- `inet/linklayer/contract/IMacProtocol.ned`
- Queue interfaces in specific MAC implementations

### 4. Review NED Package Structure

```bash
# Check how INET organizes packages
grep -r "package inet" ../inet4.5/src/inet --include="*.ned" | head -20

# Example output shows:
# inet/package.ned:          package inet;
# inet/routing/gpsr/Gpsr.ned: package inet.routing.gpsr;
```

**Rule:** Package name = directory path under src/

## Phase-by-Phase Research Checklist

### Phase 1: Baseline GPSR

**Before coding, examine:**

1. **INET GPSR Module**
   ```bash
   # Read the source
   cat ../inet4.5/src/inet/routing/gpsr/Gpsr.ned
   cat ../inet4.5/src/inet/routing/gpsr/Gpsr.h
   
   # Check message definitions
   cat ../inet4.5/src/inet/routing/gpsr/Gpsr.msg
   ```

2. **Position Table Implementation**
   ```bash
   cat ../inet4.5/src/inet/routing/gpsr/PositionTable.h
   cat ../inet4.5/src/inet/routing/gpsr/PositionTable.cc
   ```

3. **GPSR Example Network**
   ```bash
   cat ../inet4.5/examples/manetrouting/gpsr/GPSRNetwork.ned
   cat ../inet4.5/examples/manetrouting/gpsr/omnetpp.ini
   ```

**Questions to answer:**
- How does GPSR access position information?
- What is the neighbor discovery mechanism?
- How are beacons structured and sent?
- What is the position table update logic?
- How does next-hop selection work?

**Mirror these patterns in:** `researchproject.routing.queuegpsr`

### Phase 2: Delay Awareness

**Before coding, examine:**

1. **INET Timing Utilities**
   ```bash
   grep -r "simtime" ../inet4.5/src/inet/common --include="*.h"
   grep -r "delay" ../inet4.5/src/inet/common --include="*.h"
   ```

2. **Other Delay-Aware Protocols**
   ```bash
   # Check AODV or DSR for timing
   ls ../inet4.5/src/inet/routing/aodv/
   grep -r "latency\|delay" ../inet4.5/src/inet/routing/aodv/
   ```

**Questions to answer:**
- How does INET measure transmission delay?
- Where are timestamps stored in packets?
- How do other protocols handle delay metrics?
- What time units and precision are used?

### Phase 3: Queue-Aware Routing

**Before coding, examine:**

1. **MAC Queue Implementations**
   ```bash
   # Check IEEE 802.11 MAC queues
   ls ../inet4.5/src/inet/linklayer/ieee80211/mac/
   grep -r "queue" ../inet4.5/src/inet/linklayer/ieee80211/mac/ --include="*.h"
   
   # Check Ethernet queues
   ls ../inet4.5/src/inet/linklayer/ethernet/
   ```

2. **Cross-Layer Access Patterns**
   ```bash
   # How do modules access other layers?
   grep -r "getModuleFromPar" ../inet4.5/src/inet/routing/ --include="*.cc" | head -10
   grep -r "findModuleByPath" ../inet4.5/src/inet/routing/ --include="*.cc" | head -10
   ```

3. **Queue Interfaces**
   ```bash
   ls ../inet4.5/src/inet/queueing/
   cat ../inet4.5/src/inet/queueing/contract/IPacketQueue.ned
   ```

**Questions to answer:**
- How can routing layer access MAC queue state?
- What queue metrics are available (length, delay, drops)?
- How does INET handle cross-layer information exchange?
- Are there observer patterns or direct access?
- What is the proper module lookup method?

**Create in:** `researchproject.linklayer.queue`

### Phase 4: Two-Hop Peek

**Before coding, examine:**

1. **Neighbor Discovery in INET**
   ```bash
   # Check how protocols share neighbor info
   grep -r "neighbor" ../inet4.5/src/inet/routing/gpsr/ --include="*.cc"
   cat ../inet4.5/src/inet/routing/gpsr/PositionTable.cc
   ```

2. **Beacon Message Extensions**
   ```bash
   # How to extend messages
   cat ../inet4.5/src/inet/routing/gpsr/Gpsr.msg
   grep -r "beacon" ../inet4.5/src/inet/routing/ --include="*.msg"
   ```

**Questions to answer:**
- How are beacons structured in GPSR?
- Can we piggyback neighbor lists?
- What is the overhead of extended beacons?
- How to maintain two-hop table efficiently?

### Phase 5: Compute-Aware Offload

**Before coding, examine:**

1. **Node Capabilities in INET**
   ```bash
   # Check how nodes advertise capabilities
   ls ../inet4.5/src/inet/node/
   grep -r "capability\|resource" ../inet4.5/src/inet/node/ --include="*.ned"
   ```

2. **Application-Layer Metrics**
   ```bash
   ls ../inet4.5/src/inet/applications/
   # See how apps interact with network layer
   ```

**Questions to answer:**
- How to model compute resources in INET?
- Where to store node capability information?
- How to balance compute vs. network metrics?
- Should this be in routing layer or separate?

## Cross-Reference Workflow

For each new feature:

1. **Identify INET analog** - Find similar functionality in INET
2. **Study implementation** - Read source and examples
3. **Check interfaces** - Understand contracts and base classes
4. **Mirror structure** - Follow INET's patterns
5. **Adapt naming** - Use `researchproject.*` packages
6. **Test integration** - Verify with INET modules

## Key INET Files to Reference

### Routing
- `src/inet/routing/contract/IManetRouting.ned`
- `src/inet/routing/base/RoutingProtocolBase.h`
- `src/inet/routing/gpsr/Gpsr.ned`
- `src/inet/routing/gpsr/Gpsr.h`
- `src/inet/routing/gpsr/PositionTable.h`

### Link Layer
- `src/inet/linklayer/ieee80211/mac/*`
- `src/inet/linklayer/contract/IMacProtocol.ned`
- `src/inet/queueing/contract/IPacketQueue.ned`

### Network
- `src/inet/networklayer/contract/IInterfaceTable.h`
- `src/inet/networklayer/ipv4/Ipv4.ned`

### Common
- `src/inet/common/ModuleAccess.h` - Module lookup utilities
- `src/inet/common/packet/Packet.h` - Packet structure
- `src/inet/common/geometry/common/Coord.h` - Position coordinates

### Examples
- `examples/manetrouting/gpsr/` - GPSR scenarios
- `examples/wireless/` - Wireless network examples
- `examples/adhoc/` - Ad-hoc network patterns

## Documentation Sources

1. **INET User Guide:** https://inet.omnetpp.org/docs/users-guide/
2. **INET API Docs:** https://doc.omnetpp.org/inet/api-current/
3. **OMNeT++ Manual:** https://doc.omnetpp.org/omnetpp/manual/
4. **INET Source Code:** Best documentation is the code itself!

## Remember

> "Before writing any code, spend time reading INET source code in the analogous area. 
> The patterns you find there are battle-tested and align with OMNeT++ best practices."

Always mirror:
- Directory structure
- Package naming
- Interface implementations
- Module organization
- Parameter conventions
- Gate naming patterns
- Message definitions
- Documentation style
