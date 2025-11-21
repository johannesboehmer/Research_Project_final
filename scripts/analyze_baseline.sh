#!/bin/bash
#
# Analyze Baseline Results
# Extract statistics and generate summary
#

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RESULTS_DIR="$PROJECT_ROOT/results/baseline_gpsr"

cd "$RESULTS_DIR"

echo "========================================"
echo "Baseline Results Analysis"
echo "========================================"
echo ""

# Check if results exist
if [ ! -f "Baseline-0.sca" ]; then
    echo "Error: No results found in $RESULTS_DIR"
    echo "Run the simulation first: scripts/run_baseline_full.sh"
    exit 1
fi

echo "Extracting statistics..."
echo ""

# Summary statistics
echo "1. Creating summary..."
opp_scavetool s -o baseline_summary.txt Baseline-*.sca

# Application metrics
echo "2. Extracting application metrics..."
opp_scavetool q -f 'module =~ "*.app[0]"' Baseline-*.sca > app_metrics.txt

# GPSR metrics
echo "3. Extracting GPSR metrics..."
opp_scavetool q -f 'module =~ "*.gpsr"' Baseline-*.sca > gpsr_metrics.txt 2>/dev/null || echo "No GPSR-specific scalars found"

# MAC metrics
echo "4. Extracting MAC metrics..."
opp_scavetool q -f 'module =~ "*.wlan[*].mac"' Baseline-*.sca > mac_metrics.txt 2>/dev/null || echo "No MAC-specific scalars found"

# Export to CSV
echo "5. Exporting to CSV..."
opp_scavetool x -o baseline_results.csv -F CSV Baseline-*.sca

echo ""
echo "========================================"
echo "Analysis Complete!"
echo "========================================"
echo ""
echo "Generated files:"
echo "  - baseline_summary.txt    : Overall statistics"
echo "  - app_metrics.txt         : Application layer metrics"
echo "  - gpsr_metrics.txt        : GPSR routing metrics"
echo "  - mac_metrics.txt         : MAC layer metrics"
echo "  - baseline_results.csv    : Full dataset in CSV"
echo ""
echo "View summary:"
echo "  cat baseline_summary.txt"
echo ""
echo "Next step: Document in docs/BASELINE_RESULTS.md"
echo ""
