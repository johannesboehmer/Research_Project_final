# Queue-Aware GPSR Routing Research Project

## Overview
This project implements and evaluates queue-aware extensions to the Greedy Perimeter Stateless Routing (GPSR) protocol for wireless ad-hoc networks. The implementation follows INET framework conventions and OMNeT++ best practices.

## Project Structure (INET-aligned)

This project follows the research-first approach with INET-aligned organization:

### `/src/researchproject/` - Model Code
Model implementation organized by OSI layer, mirroring INET's `src/inet/` structure:

- **`routing/queuegpsr/`** - QueueGPSR protocol implementation
  - Core routing logic and extensions
  - Mirrors `inet.routing.gpsr` structure
  - Contains C++/NED for protocol behavior

- **`linklayer/queue/`** - Queue inspection utilities
  - MAC queue state access
  - Queue length monitoring
  - Mirrors `inet.linklayer` conventions

- **`common/`** - Shared utilities
  - Helper functions and data structures
  - Metrics calculation utilities
  - Mirrors `inet.common` for project-wide code

### `/simulations/` - Experiment Configurations
Scenario-specific networks and run configurations, separated from model code:

- **`baseline_gpsr/`** - Standard GPSR baseline
- **`delay_tiebreaker/`** - Delay-based tiebreaking
- **`queue_aware/`** - Queue-aware next-hop selection
- **`two_hop_peek/`** - Two-hop lookahead
- **`compute_offload/`** - Compute-aware offloading

Each contains:
- `omnetpp.ini` - Run configurations
- `*Network.ned` - Scenario network topology
- Scenario-specific parameters

### `/results/` - Simulation Outputs
Centralized output directory for all simulation results:
- `.sca` scalar files
- `.vec` vector files
- Configured via `result-dir = ../results` in INI files

### `/scripts/` - Automation Tools
- Batch runners for multiple configurations
- Result processing and analysis scripts
- Command-line execution helpers

### `/docs/` - Documentation
- Design notes and assumptions
- Metrics definitions
- Literature references

## Research-First Workflow

Before implementing any feature:

1. **Research INET structure** - Examine relevant `inet4.5/src/inet/` modules
2. **Mirror conventions** - Follow naming, packaging, and separation patterns
3. **Check examples** - Look at `inet4.5/examples/` for similar scenarios
4. **Maintain separation** - Keep model code in `src/`, scenarios in `simulations/`

## NED Package Mapping

Package names mirror directory paths under NED roots:

```
researchproject                     → src/researchproject/
researchproject.routing             → src/researchproject/routing/
researchproject.routing.queuegpsr   → src/researchproject/routing/queuegpsr/
researchproject.linklayer.queue     → src/researchproject/linklayer/queue/
researchproject.common              → src/researchproject/common/
```

This follows INET's rule: `inet.routing.gpsr` → `src/inet/routing/gpsr/`

## Development Phases

### Phase 1: Baseline GPSR ✅ COMPLETE
- Standard GPSR behavior replicated
- Testing methodology established
- Baseline metrics collection working

### Phase 2: Delay Tiebreaker ✅ COMPLETE
- Delay-based next-hop selection implemented
- Distance-only delay estimation working

### Phase 3: Queue-Aware Routing ✅ COMPLETE
- MAC queue state integrated via beacons
- Direct MAC queue introspection via `IPacketCollection` interface
- Queue measurement from actual MAC queue at `mac.dcf.channelAccess.pendingQueue`
- Q/R queueing delay calculation implemented
- Tiebreaker selects path based on total delay (distance + queueing)
- **Critical Fix:** Replaced artificial counter with real-time MAC queue measurement
- **Validation: 214/250 activations (85.6%), 92% delay reduction, 97.6% delivery**

### Phase 4: Two-Hop Peek 🚧 PLANNED
- Extended neighbor information
- Two-hop lookahead logic

## Quick Start

### Run Validation Test

The project includes an automated validation test that demonstrates the queue-aware tiebreaker:

```bash
scripts/run_multihop_comparison.sh
```

This will:
1. Run baseline GPSR (no tiebreaker) and queue-aware GPSR simulations
2. Compare routing decisions between congested and idle relays
3. Display tiebreaker activations and delay comparison
4. Verify queue measurement from actual MAC queue
5. Report comprehensive performance metrics

**Expected result:** 214/250 packets (85.6%) routed via idle relay, achieving 92% delay reduction (292ms → 23ms) and 97.6% delivery rate (vs 78.8% baseline).

Note: generated logs and run outputs are now saved under the `logs/` directory.

### Build and Run Manually

```bash
# Build
make

# Run validation config
./out/clang-release/Research_project \
    -u Cmdenv \
    -n ../../src:simulations:src:../inet4.5/src \
    -f simulations/delay_tiebreaker/omnetpp.ini \
    -c QueueAwareTiebreakerValidation \
    --sim-time-limit=40s
```

## Implementation Details

### Queue-Aware Tiebreaker

When GPSR finds multiple next-hop candidates at equal distance to the destination (within `distanceEqualityThreshold`), the tiebreaker compares total estimated delay:

```
Total Delay = Distance × DelayFactor + Queue / Bitrate
            = (distance in meters × 0.001 s/m) + (queue bytes × 8 / link bps)
```

**Key Components:**
- **Beacon Exchange**: Nodes broadcast queue backlog in beacons every 2s
- **MAC Queue Measurement**: Direct introspection via INET's `IPacketCollection` interface
- **Queue Path**: `wlan[0].mac.dcf.channelAccess.pendingQueue` (INET 4.5 802.11 MAC)
- **Bitrate Lookup**: Dynamically reads link rate from `wlan[0].radio.transmitter.bitrate`
- **Q/R Calculation**: Computes queueing delay = (queue_bytes × 8) / bitrate_bps
- **Delay Comparison**: Selects neighbor with lower total delay

**Critical Implementation Fix (Nov 18, 2025):**
- Replaced artificial counter with recursive search for `IPacketCollection` in MAC tree
- Queue measurement now reflects actual MAC queue state in real-time
- Result: 85.6% of packets correctly routed via idle relay (vs <12% before fix)

**Files:**
- `src/researchproject/routing/queuegpsr/QueueGpsr.{h,cc,ned}` - Main implementation
- `simulations/delay_tiebreaker/omnetpp.ini` - Validation configuration
- `docs/MAC_QUEUE_FIX_IMPLEMENTATION.md` - Detailed fix documentation
- `run_multihop_comparison.sh` - Automated validation script

## Results

See `docs/MAC_QUEUE_FIX_IMPLEMENTATION.md` for complete validation report showing:
- ✅ Tie detection working (equidistant relays)
- ✅ Queue measurement accurate (direct MAC queue introspection)
- ✅ Queue path discovered: `mac.dcf.channelAccess.pendingQueue`
- ✅ Bitrate lookup functional (2.0 Mbps)
- ✅ Tiebreaker activations: 214/250 packets (85.6%)
- ✅ Optimal path selection: 92% delay reduction (292ms → 23ms)
- ✅ Delivery improvement: 78.8% → 97.6% (+47 packets)

**Key Finding:** Direct MAC queue measurement via `IPacketCollection` interface is essential for accurate congestion detection. Previous artificial counter approach failed to reflect actual queue state.

### Phase 5: Compute-Aware Offload
- Compute resource awareness
- Offloading decision logic

## Dependencies

- OMNeT++ 6.1
- INET Framework 4.5
- Standard OMNeT++ build tools

## Building

```bash
cd Research_project
make
```

## Running Simulations

From the project root:

```bash
# Run specific configuration
cd simulations/baseline_gpsr
opp_run -m -u Cmdenv -c BaselineConfig -n ../../src:. --result-dir=../../results

# Or use scripts
cd scripts
./run_baseline.sh
```

## Results Analysis

Results are collected in `results/` directory and can be analyzed using:
- OMNeT++ IDE Analysis Tool
- Python scripts in `scripts/`
- Custom analysis tools

## License

LGPL-3.0-or-later

## References

- INET Framework: https://inet.omnetpp.org/
- GPSR Paper: Karp & Kung, MobiCom 2000
- OMNeT++ Manual: https://doc.omnetpp.org/
