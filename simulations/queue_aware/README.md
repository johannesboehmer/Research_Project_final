# Queue-Aware Routing Scenario

## Status
🚧 **Not Yet Implemented** - Awaiting Phase 3 development

## Purpose
Integrate MAC queue state into routing decisions to avoid congested paths.

## Research First - Before Implementation

### INET Areas to Examine
1. **MAC queue implementations** in `inet/linklayer/ieee80211/mac/`
2. **Cross-layer access patterns** in INET routing protocols
3. **Queue interfaces** in `inet/queueing/contract/`

### Questions to Answer
- How can routing layer access MAC queue state?
- What queue metrics are available?
- How does INET handle cross-layer information?
- Are there observer patterns or direct module access?

## Planned Implementation

### Model Components
Create in `src/researchproject/linklayer/queue/`:
- Queue inspector module for state queries
- Queue metrics data structures
- Integration with routing layer

Extend in `src/researchproject/routing/queuegpsr/`:
- Queue-aware next-hop selection
- Load balancing logic

### Configuration
- Enable queue inspection
- Configure queue weight vs. geographic progress
- Test under varying loads

## Metrics to Compare
- Queue utilization distribution
- Packet loss reduction
- End-to-end delay under load
- Fairness metrics

## References
- Study INET's MAC implementations for queue access
- Review `inet/queueing/` package structure
- Check `ModuleAccess.h` for lookup patterns
