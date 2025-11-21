#!/bin/bash
#
# Run Full Baseline GPSR Simulation (500s, 10 repetitions)
# Phase 1: Baseline Verification - Complete Run
#

set -e  # Exit on error

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_ROOT/simulations/baseline_gpsr"

echo "========================================"
echo "Phase 1: Full Baseline GPSR Run"
echo "========================================"
echo ""
echo "Configuration: Baseline"
echo "Simulation Time: 500 seconds"
echo "Repetitions: 10 (seeds 0-9)"
echo "Network: 20 GpsrRouter nodes"
echo "Results: ../../results/baseline_gpsr/"
echo ""
echo "This will take approximately 30-60 minutes."
echo ""
read -p "Press Enter to start..."

# Run simulation
echo ""
echo "Starting simulation..."
echo ""

opp_run -l ../../../inet4.5/src/libINET.dylib \
        -n ../../../inet4.5/src:.:../../src \
        -u Cmdenv \
        -c Baseline \
        --result-dir=../../results/baseline_gpsr

echo ""
echo "========================================"
echo "Simulation Complete!"
echo "========================================"
echo ""
echo "Results saved to: $PROJECT_ROOT/results/baseline_gpsr/"
echo ""
echo "Next steps:"
echo "  1. Extract statistics: cd $PROJECT_ROOT/scripts && ./analyze_baseline.sh"
echo "  2. Document results: Create docs/BASELINE_RESULTS.md"
echo "  3. Mark Phase 1 complete: Update docs/WORKFLOW.md"
echo ""
