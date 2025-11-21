# Two-Hop Peek Scenario

## Status
🚧 **Not Yet Implemented** - Awaiting Phase 4 development

## Purpose
Extend neighbor awareness to two hops for better next-hop decisions.

## Research First - Before Implementation

### INET Areas to Examine
1. **Neighbor discovery** in `inet/routing/gpsr/`
2. **Beacon message** structure and extensions
3. **Position table** implementation

### Questions to Answer
- How are beacons structured in GPSR?
- Can we piggyback neighbor lists efficiently?
- What is the overhead of extended beacons?
- How to maintain two-hop table?

## Planned Implementation

### Model Extensions
Extend in `src/researchproject/routing/queuegpsr/`:
- Extended beacon messages with neighbor lists
- Two-hop position table
- Lookahead next-hop selection

### Configuration
- Enable two-hop beaconing
- Configure beacon payload limits
- Test impact of beacon overhead

## Metrics to Compare
- Routing decision quality
- Overhead of extended beacons
- Route stability improvement
- Dead-end avoidance

## References
- Study GPSR's `PositionTable` implementation
- Review beacon message definitions
- Check overhead impact in different densities
