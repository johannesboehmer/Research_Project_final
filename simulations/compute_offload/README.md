# Compute-Aware Offload Scenario

## Status
🚧 **Not Yet Implemented** - Awaiting Phase 5 development

## Purpose
Route traffic considering compute resource availability for offloading scenarios.

## Research First - Before Implementation

### INET Areas to Examine
1. **Node capabilities** in `inet/node/`
2. **Application layer** interactions in `inet/applications/`
3. **Resource modeling** patterns in INET

### Questions to Answer
- How to model compute resources in INET nodes?
- Where to store capability information?
- How to balance compute vs. network metrics?
- Should this be routing-layer or separate service?

## Planned Implementation

### Model Extensions
Create in `src/researchproject/common/`:
- Compute resource model
- Capability advertisement mechanism

Extend in `src/researchproject/routing/queuegpsr/`:
- Compute-aware routing decisions
- Multi-objective optimization (latency + compute)

### Configuration
- Define heterogeneous node capabilities
- Configure offloading policies
- Test edge computing scenarios

## Metrics to Compare
- Compute resource utilization
- Task completion time
- Network overhead for offloading
- Load balancing across capable nodes

## References
- Research edge computing models in INET
- Study multi-objective routing approaches
- Review capability advertisement patterns
