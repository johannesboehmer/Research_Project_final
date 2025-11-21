# Research Project Development Workflow

## Phase-by-Phase Implementation

This document outlines the step-by-step workflow following the research-first approach.

---

## Phase 1: Baseline GPSR ✅ (Verification In Progress)

### Status
- ✅ Project structure created
- ✅ Package hierarchy established
- ✅ Baseline scenario configured
- ✅ INET GPSR research complete (see PHASE1_RESEARCH_FINDINGS.md)
- ✅ Baseline simulation running successfully
- ✅ Metrics sources identified (see PHASE1_METRICS_INVENTORY.md)
- ⏳ Full 500s run pending
- ⏳ Baseline statistics extraction pending

### Completed Research
**Date:** November 6, 2025

Documented in `docs/PHASE1_RESEARCH_FINDINGS.md`:
- INET GPSR module structure analyzed
- PositionTable implementation understood
- Beaconing mechanism documented
- Next-hop selection logic reviewed
- IManetRouting interface verified
- GpsrRouter node type examined

### Baseline Run Results

**Test Run (10s):**
- Configuration: Baseline
- Repetitions: 10 (seeds 0-9)
- Network: 20 GpsrRouter nodes
- Status: ✅ Successful
- Results: `results/baseline_gpsr/`
- Files: 30 total (10× .sca, 10× .vec, 10× .vci)

**Observations:**
- Simulation executes cleanly with INET's GpsrRouter
- Results properly directed to results/ directory
- Multiple seeds working correctly
- Significant variation in transmission counts (211-1287) across seeds
- Ready for extended 500s runs

### Metrics Inventory Completed

Documented in `docs/PHASE1_METRICS_INVENTORY.md`:

1. **✅ Packet Delivery Ratio** - Application layer (packets sent/received)
2. **✅ End-to-End Delay** - Vector data (requires longer run)
3. **✅ Routing Overhead** - GPSR beacons + transmissions
4. **✅ Throughput** - Byte counts from application layer
5. **✅ Hop Count** - IP/routing layer (to be verified in 500s run)

**Cross-layer sources identified for future phases:**
- MAC queue state access points
- Delay measurement timing sources
- Beacon extension possibilities
- Compute metric advertisement locations

### Next Steps to Complete Phase 1

1. **Run full 500s baseline** - Extended time for meaningful metrics
   ```bash
   cd simulations/baseline_gpsr
   opp_run -l ../../../inet4.5/src/libINET.dylib \
           -n ../../../inet4.5/src:.:../../src \
           -u Cmdenv -c Baseline \
           --result-dir=../../results/baseline_gpsr
   ```

2. **Extract baseline statistics** - Create summary tables
   ```bash
   cd results/baseline_gpsr
   opp_scavetool s -o baseline_summary.csv Baseline-*.sca
   ```

3. **Verify all 5 core metrics** - Confirm each metric is measurable

4. **Document seed mapping** - Record run IDs for reproducibility

5. **Create baseline results doc** - `docs/BASELINE_RESULTS.md` with tables

6. **Mark Phase 1 complete** - Update WORKFLOW.md status

### Phase 1 Completion Criteria

- [x] INET GPSR thoroughly understood and documented
- [x] Baseline scenario configured with INET's GpsrRouter  
- [x] Test run (10s) successful with 10 repetitions
- [x] Metrics sources identified and documented
- [ ] Full run (500s) completed with all seeds
- [ ] All 5 core metrics verified and extracted
- [ ] Baseline results summarized in documentation
- [ ] Results directory structure validated
- [ ] Ready for Phase 2 extensions

---

## Phase 2: Delay Tiebreaker 🚧

### Prerequisites
- Phase 1 baseline complete
- Understanding of INET timing utilities

### Research Tasks
- [ ] Study `inet/common/` timing mechanisms
- [ ] Review delay measurement in AODV/DSR
- [ ] Understand packet timestamp patterns

### Implementation Tasks
- [ ] Create delay measurement module
- [ ] Extend neighbor table with delay field
- [ ] Modify next-hop selection logic
- [ ] Create scenario in `simulations/delay_tiebreaker/`
- [ ] Run comparative evaluation

---

## Phase 3: Queue-Aware Routing 🚧

### Prerequisites
- Phase 2 complete
- Understanding of INET MAC queue access

### Research Tasks
- [ ] Study `inet/linklayer/ieee80211/mac/` queue impl
- [ ] Review cross-layer access patterns
- [ ] Check `inet/queueing/contract/` interfaces
- [ ] Understand `ModuleAccess.h` utilities

### Implementation Tasks
- [ ] Create `QueueInspector` module in `linklayer/queue/`
- [ ] Implement queue state queries
- [ ] Integrate with routing layer
- [ ] Add preprocessing for queue access
- [ ] Create scenario in `simulations/queue_aware/`
- [ ] Evaluate under load

---

## Phase 4: Two-Hop Peek 🚧

### Prerequisites
- Phase 3 complete
- Understanding of GPSR beaconing

### Research Tasks
- [ ] Study GPSR beacon structure
- [ ] Review neighbor list piggybacking
- [ ] Analyze overhead implications
- [ ] Check position table implementation

### Implementation Tasks
- [ ] Extend beacon messages
- [ ] Build two-hop position table
- [ ] Implement lookahead logic
- [ ] Create scenario in `simulations/two_hop_peek/`
- [ ] Measure overhead vs. benefit

---

## Phase 5: Compute-Aware Offload 🚧

### Prerequisites
- Phase 4 complete
- Understanding of resource modeling

### Research Tasks
- [ ] Study INET node capability patterns
- [ ] Review application-layer integration
- [ ] Research multi-objective routing

### Implementation Tasks
- [ ] Model compute resources in `common/`
- [ ] Add capability advertisement
- [ ] Implement compute-aware routing
- [ ] Create scenario in `simulations/compute_offload/`
- [ ] Evaluate offloading scenarios

---

## General Workflow for Each Phase

### 1. Research (Before Any Coding)
```bash
# Navigate to INET
cd ../inet4.5/src/inet

# Examine relevant areas
ls routing/
ls linklayer/
ls common/

# Study examples
cd ../examples
ls manetrouting/
ls wireless/
```

**Document findings in `docs/` before proceeding!**

### 2. Design
- Sketch module structure
- Plan package placement
- Design NED files
- Define message formats
- Specify parameters

### 3. Implement
- Follow INET conventions strictly
- Mirror directory structure
- Match naming patterns
- Implement incrementally
- Test frequently

### 4. Configure
- Create scenario in `simulations/`
- Write `omnetpp.ini` configuration
- Define network topology in NED
- Set `result-dir = ../../results`

### 5. Test
- Run baseline comparison
- Verify expected behavior
- Check metric collection
- Run multiple seeds

### 6. Evaluate
- Analyze results in `results/`
- Generate statistics
- Compare against baseline
- Document findings

### 7. Document
- Update `docs/DESIGN.md`
- Add scenario README
- Document key decisions
- Note issues/improvements

---

## Critical Rules

### Package-Directory Mapping
```
Package: researchproject.routing.queuegpsr
Path:    src/researchproject/routing/queuegpsr/
```
**Always match exactly!**

### Model vs. Scenario Separation
- Model code → `src/`
- Experiments → `simulations/`
- Results → `results/`
- Never mix!

### Research-First Principle
> "Before implementing ANY feature, examine how INET handles similar functionality."

### INET Convention Adherence
- Module naming
- Parameter conventions
- Gate naming patterns
- Message definitions
- Documentation style

---

## Progress Tracking

### Completed
- [x] Project structure
- [x] Package hierarchy
- [x] Documentation framework
- [x] Baseline scenario setup

### In Progress
- [ ] Phase 1: Baseline GPSR implementation
- [ ] Phase 1: Baseline verification

### Planned
- [ ] Phase 2: Delay tiebreaker
- [ ] Phase 3: Queue-aware routing
- [ ] Phase 4: Two-hop peek
- [ ] Phase 5: Compute-aware offload

---

## Testing Strategy

### Per-Phase Testing
1. **Unit tests** - Individual module behavior
2. **Integration tests** - Module interactions
3. **Scenario tests** - Full network simulation
4. **Comparison tests** - Against baseline/previous phase

### Statistical Validation
- Multiple random seeds (≥10)
- Confidence intervals
- Statistical significance tests
- Variance analysis

### Performance Profiling
- Execution time
- Memory usage
- Scalability tests

---

## Logging and Instrumentation Design

### Overview
Custom logging tokens for routing decision analysis and cross-configuration comparison. Designed to be parseable into CSV for statistical analysis.

**Status**: Design-only (implementation in Phase 2+)

---

### Token 1: SCORE_COMPONENTS

**Purpose**: Log individual scoring components for next-hop selection to quantify decision factors.

**Format**:
```
[SCORE_COMPONENTS] pkt=<pkt_id> candidate=<neighbor_addr> \
  progress=<progress_score> delay=<delay_score> queue=<queue_score> \
  total=<final_score> selected=<0|1>
```

**Fields**:
- `pkt`: Packet sequence number (for tracing specific packets)
- `candidate`: L3Address of candidate neighbor
- `progress`: Geographic progress score (distance reduction to destination)
- `delay`: Delay-based score (from delay tiebreaker mechanism)
- `queue`: Queue-based score (from queue-aware routing logic)
- `total`: Final combined score (weighted sum or product)
- `selected`: 1 if chosen as next-hop, 0 otherwise

**Example Log Entries**:
```
[SCORE_COMPONENTS] pkt=42 candidate=10.0.0.5 progress=0.85 delay=0.90 queue=1.00 total=0.85 selected=1
[SCORE_COMPONENTS] pkt=42 candidate=10.0.0.7 progress=0.92 delay=0.60 queue=0.30 total=0.61 selected=0
[SCORE_COMPONENTS] pkt=42 candidate=10.0.0.3 progress=0.78 delay=0.95 queue=0.95 total=0.78 selected=0
```

**Intended Location**:
- QueueGpsr::selectNextHop() or equivalent decision function
- Log BEFORE final selection (emit for all candidates, mark selected)

**Parsing Strategy** (Python):
```python
import re
import pandas as pd

log_pattern = r'\[SCORE_COMPONENTS\] pkt=(\d+) candidate=([\d.]+) progress=([\d.]+) delay=([\d.]+) queue=([\d.]+) total=([\d.]+) selected=([01])'

def parse_score_components(log_file):
    records = []
    with open(log_file) as f:
        for line in f:
            if '[SCORE_COMPONENTS]' in line:
                match = re.search(log_pattern, line)
                if match:
                    records.append({
                        'pkt_id': int(match.group(1)),
                        'candidate': match.group(2),
                        'progress': float(match.group(3)),
                        'delay': float(match.group(4)),
                        'queue': float(match.group(5)),
                        'total': float(match.group(6)),
                        'selected': int(match.group(7))
                    })
    return pd.DataFrame(records)

# Analysis
df = parse_score_components('results/baseline.log')
# Compare selected vs. non-selected candidates
selected = df[df['selected'] == 1]
rejected = df[df['selected'] == 0]
print(f"Avg progress (selected): {selected['progress'].mean()}")
print(f"Avg queue (selected): {selected['queue'].mean()}")
```

**Comparison Value**:
- **Baseline GPSR**: Only `progress` component varies (delay=1.0, queue=1.0 constant)
- **Queue-Aware GPSR**: `queue` component varies, shows tradeoff with `progress`
- **Delay Tiebreaker**: `delay` component breaks ties when `progress` similar

---

### Token 2: ROUTING_DECISION

**Purpose**: Log final routing decision (mode, next-hop, reason) for high-level flow analysis.

**Format**:
```
[ROUTING_DECISION] pkt=<pkt_id> src=<src_addr> dst=<dst_addr> \
  mode=<GREEDY|PERIMETER> nexthop=<neighbor_addr> \
  reason=<PROGRESS|DELAY_TIE|QUEUE_AWARE|PERIMETER_FACE>
```

**Fields**:
- `pkt`: Packet sequence number
- `src`: Source L3Address (packet origin)
- `dst`: Destination L3Address (packet target)
- `mode`: Routing mode (GREEDY or PERIMETER)
- `nexthop`: Chosen next-hop L3Address
- `reason`: Decision rationale (variant-specific)

**Example Log Entries**:
```
[ROUTING_DECISION] pkt=42 src=10.0.0.0 dst=10.0.0.15 mode=GREEDY nexthop=10.0.0.5 reason=PROGRESS
[ROUTING_DECISION] pkt=43 src=10.0.0.0 dst=10.0.0.15 mode=GREEDY nexthop=10.0.0.7 reason=DELAY_TIE
[ROUTING_DECISION] pkt=44 src=10.0.0.0 dst=10.0.0.15 mode=GREEDY nexthop=10.0.0.3 reason=QUEUE_AWARE
[ROUTING_DECISION] pkt=45 src=10.0.0.5 dst=10.0.0.15 mode=PERIMETER nexthop=10.0.0.8 reason=PERIMETER_FACE
```

**Intended Location**:
- QueueGpsr::routeDatagram() AFTER next-hop selection
- Log once per packet forwarding event

**Parsing Strategy** (Python):
```python
import re
import pandas as pd

log_pattern = r'\[ROUTING_DECISION\] pkt=(\d+) src=([\d.]+) dst=([\d.]+) mode=(\w+) nexthop=([\d.]+) reason=(\w+)'

def parse_routing_decisions(log_file):
    records = []
    with open(log_file) as f:
        for line in f:
            if '[ROUTING_DECISION]' in line:
                match = re.search(log_pattern, line)
                if match:
                    records.append({
                        'pkt_id': int(match.group(1)),
                        'src': match.group(2),
                        'dst': match.group(3),
                        'mode': match.group(4),
                        'nexthop': match.group(5),
                        'reason': match.group(6)
                    })
    return pd.DataFrame(records)

# Analysis
df = parse_routing_decisions('results/queue_aware.log')
# Count routing modes
print(df['mode'].value_counts())
# Count decision reasons
print(df['reason'].value_counts())
# Perimeter mode rate
perimeter_rate = (df['mode'] == 'PERIMETER').sum() / len(df)
print(f"Perimeter mode: {perimeter_rate:.2%}")
```

**Comparison Value**:
- **Baseline GPSR**: reason=PROGRESS (always), mode=GREEDY (mostly)
- **Queue-Aware GPSR**: reason=QUEUE_AWARE (when congestion detected)
- **Delay Tiebreaker**: reason=DELAY_TIE (when progress scores tied)
- **All variants**: Track perimeter mode frequency (greedy failure rate)

---

### Integration with Existing Metrics

**Cross-reference with METRICS_MAP.md**:
- `SCORE_COMPONENTS` explains WHY a path was chosen (decision factors)
- `ROUTING_DECISION` explains WHICH path was chosen (flow-level view)
- `METRICS_MAP.md` metrics explain OUTCOME (TCR, delay, overhead)

**Example Combined Analysis**:
1. Extract `ROUTING_DECISION` logs → identify flows with high perimeter mode rate
2. Extract `SCORE_COMPONENTS` for those flows → check if low progress/queue scores
3. Query `METRICS_MAP.md` E2E delay → correlate perimeter mode with higher delay
4. Conclusion: Queue-aware routing reduces perimeter mode → lower delay

---

### Logging Control (Configuration)

**Enable/disable via omnetpp.ini**:
```ini
# Phase 2+: Enable detailed logging for analysis
**.gpsr.logScoreComponents = true
**.gpsr.logRoutingDecisions = true

# Production runs: Disable logging (performance)
**.gpsr.logScoreComponents = false
**.gpsr.logRoutingDecisions = false
```

**Alternative: Conditional logging**:
```cpp
// In QueueGpsr.cc
#ifdef ENABLE_DETAILED_LOGGING
EV_INFO << "[SCORE_COMPONENTS] pkt=" << pkt->getId() << " ... " << endl;
#endif
```

**Rationale**:
- Detailed logging for debugging and analysis (Phases 2-4)
- Disable for large-scale statistical runs (Phase 5)
- Avoid performance overhead in production evaluation

---

### Log File Management

**Separate log files per configuration**:
```ini
[Config Baseline]
result-dir = results/baseline_gpsr
cmdenv-output-file = results/baseline_gpsr/baseline.log

[Config QueueAware]
result-dir = results/queue_aware
cmdenv-output-file = results/queue_aware/queue_aware.log
```

**Post-processing script**:
```bash
# scripts/parse_all_logs.sh
for config in baseline_gpsr queue_aware delay_tiebreaker two_hop_peek; do
    python scripts/parse_logs.py results/$config/*.log -o results/$config/decisions.csv
done

# Merge all CSVs for comparison
python scripts/merge_decisions.py results/*/decisions.csv -o results/all_decisions.csv
```

---

### Summary: Logging Tokens

| Token | Purpose | Location | Fields | Use Case |
|-------|---------|----------|--------|----------|
| **SCORE_COMPONENTS** | Next-hop scoring details | selectNextHop() | progress, delay, queue, total | Analyze decision factors |
| **ROUTING_DECISION** | Final routing choice | routeDatagram() | mode, nexthop, reason | Track routing mode and flow paths |

**Implementation Status**: Design complete, implementation in Phase 2+

**References**:
- METRICS_MAP.md: Outcome metrics (TCR, delay, overhead)
- BASELINE_READINESS_CHECKLIST.md: Verification before logging added
- PHASE1_RESEARCH_FINDINGS.md: INET GPSR structure for logging integration

---

## Next Immediate Actions

1. **Verify baseline can run**
   ```bash
   cd simulations/baseline_gpsr
   # Check if INET GPSR works with current config
   ```

2. **Study INET GPSR source**
   ```bash
   cd ../inet4.5/src/inet/routing/gpsr/
   # Read and understand implementation
   ```

3. **Plan Phase 1 implementation**
   - Document INET GPSR structure
   - Design QueueGpsr module skeleton
   - Plan incremental development

4. **Set up build system**
   ```bash
   cd Research_project
   make makemake
   make
   ```

---

## Resources

### Documentation
- `README.md` - Project overview
- `docs/DESIGN.md` - Architecture details
- `docs/INET_RESEARCH_GUIDE.md` - INET study guide
- `docs/STRUCTURE_MAP.md` - Directory organization

### Key INET Files
- `inet/routing/gpsr/` - GPSR implementation
- `inet/examples/manetrouting/gpsr/` - GPSR examples
- `inet/routing/contract/` - Routing interfaces

### Scripts
- `scripts/run_baseline.sh` - Run baseline simulation
- `scripts/run_all_simulations.sh` - Run all scenarios
- `scripts/analyze_results.py` - Process results
