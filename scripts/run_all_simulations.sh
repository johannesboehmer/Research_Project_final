#!/bin/bash
#
# Run all simulation configurations sequentially
#

PROJECT_ROOT="$(dirname "$0")/.."
cd "$PROJECT_ROOT"

echo "======================================"
echo "Queue-Aware GPSR Simulation Runner"
echo "======================================"
echo ""

# Array of simulation directories
SIMULATIONS=(
    "baseline_gpsr"
    "delay_tiebreaker"
    "queue_aware"
    "two_hop_peek"
    "compute_offload"
)

# Check if build exists
if [ ! -d "out" ]; then
    echo "Error: Project not built. Please build first:"
    echo "  make makemake && make"
    exit 1
fi

# Run each simulation
for sim in "${SIMULATIONS[@]}"; do
    echo ""
    echo "--------------------------------------"
    echo "Running: $sim"
    echo "--------------------------------------"
    
    if [ ! -d "simulations/$sim" ]; then
        echo "Warning: Simulation directory not found: $sim"
        echo "Skipping..."
        continue
    fi
    
    if [ ! -f "simulations/$sim/omnetpp.ini" ]; then
        echo "Warning: No omnetpp.ini found in $sim"
        echo "Skipping..."
        continue
    fi
    
    cd "simulations/$sim"
    
    opp_run -m -u Cmdenv \
        -n ../../src:. \
        --result-dir=../../results \
        all
    
    cd "$PROJECT_ROOT"
    
    echo "Completed: $sim"
done

echo ""
echo "======================================"
echo "All simulations complete!"
echo "Results saved to: results/"
echo "======================================"
