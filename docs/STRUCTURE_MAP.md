# Research Project Structure Map
# This file documents the INET-aligned organization

## Directory → Package Mapping

src/researchproject/                          → package researchproject
src/researchproject/routing/                  → package researchproject.routing
src/researchproject/routing/queuegpsr/        → package researchproject.routing.queuegpsr
src/researchproject/linklayer/                → package researchproject.linklayer
src/researchproject/linklayer/queue/          → package researchproject.linklayer.queue
src/researchproject/common/                   → package researchproject.common

## INET Analogies

This structure mirrors INET's organization:

inet.routing.gpsr        → researchproject.routing.queuegpsr
inet.linklayer.*         → researchproject.linklayer.queue
inet.common.*            → researchproject.common.*

## Scenario Organization

simulations/baseline_gpsr/       - Standard GPSR baseline
simulations/delay_tiebreaker/    - Delay-based next-hop selection
simulations/queue_aware/         - Queue-aware routing
simulations/two_hop_peek/        - Two-hop lookahead
simulations/compute_offload/     - Compute-aware offloading

## Results Collection

All simulation outputs directed to: results/
Configured in omnetpp.ini via: result-dir = ../../results

## Development Workflow

1. Research INET structure before implementation
2. Mirror INET conventions for consistency
3. Keep model code (src/) separate from scenarios (simulations/)
4. Package names match directory paths exactly
5. Results stay out of source tree

## Key Files to Create (Phase by Phase)

### Phase 1: Baseline GPSR
- src/researchproject/routing/queuegpsr/QueueGpsr.ned
- src/researchproject/routing/queuegpsr/QueueGpsr.h
- src/researchproject/routing/queuegpsr/QueueGpsr.cc
- simulations/baseline_gpsr/BaselineNetwork.ned
- simulations/baseline_gpsr/omnetpp.ini

### Phase 2: Queue Inspector
- src/researchproject/linklayer/queue/QueueInspector.ned
- src/researchproject/linklayer/queue/QueueInspector.h
- src/researchproject/linklayer/queue/QueueInspector.cc

### Phase 3+: Iterative Extensions
- Extend existing modules
- Add new scenarios in simulations/
- Update documentation in docs/

## Before Each Implementation Step

Check INET reference:
1. Examine inet4.5/src/inet/ for similar modules
2. Review inet4.5/examples/ for usage patterns
3. Follow naming and packaging conventions
4. Maintain OSI layer separation
