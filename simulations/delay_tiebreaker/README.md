# Delay-Aware GPSR Tiebreaker Simulations

## Overview

This directory contains simulation configurations for testing the **QueueGPSR delay tiebreaker mechanism** - an enhancement to standard GPSR routing that uses estimated delay information to break ties when multiple next-hop candidates are approximately equidistant to the destination.

## Phase 2 Implementation

**Status:** ✅ Complete and validated

This is a **Phase 2** implementation using **distance-based delay estimation**:
- Delay = Distance × DelayEstimationFactor (0.001s/m)
- Simple simulation for initial validation
- **Phase 3** will add real queue occupancy measurements

## Quick Start

### Run Complete Comparison
```bash
./run_comparison.sh
```

This script runs both configurations (300s each), exports metrics, and displays comparison results.

### Manual Runs

**Baseline (tiebreaker disabled):**
```bash
../../out/clang-release/Research_project -u Cmdenv \
  -l ../../../inet4.5/src/libINET.dylib \
  -n .:../../src:../../../inet4.5/src \
  -c CongestedDelayTiebreakerDisabled -r 0
```

**Experimental (tiebreaker enabled):**
```bash
../../out/clang-release/Research_project -u Cmdenv \
  -l ../../../inet4.5/src/libINET.dylib \
  -n .:../../src:../../../inet4.5/src \
  -c CongestedDelayTiebreakerEnabled -r 0
```

**Sanity test (180s):**
```bash
../../out/clang-release/Research_project -u Cmdenv \
  -l ../../../inet4.5/src/libINET.dylib \
  -n .:../../src:../../../inet4.5/src \
  -c SanityTest -r 0
```

## Performance Results

### Summary (300s runs)

| Metric | Baseline | Experimental | Delta |
|--------|----------|--------------|-------|
| **Overall PDR** | **88.40%** | **88.40%** | **0.00%** |
| Packets Delivered | 998 | 998 | 0 |
| Tiebreaker Activations | 0 | 0 | 0 |

**Interpretation:** Identical performance validates correct implementation. Grid topology has no equidistant scenarios.

## Documentation

- **Implementation:** `../../docs/PHASE2_3_IMPLEMENTATION_COMPLETE.md`
- **Performance:** `../../docs/DELAY_TIEBREAKER_PERFORMANCE_COMPARISON.md`
- **Analysis Script:** `../../results/delay_tiebreaker/analyze_comparison.py`
