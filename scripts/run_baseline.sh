#!/bin/bash
#
# Run baseline GPSR simulation
#

# Navigate to simulation directory
cd "$(dirname "$0")/../simulations/baseline_gpsr"

# Check if network file exists
if [ ! -f "BaselineNetwork.ned" ]; then
    echo "Error: BaselineNetwork.ned not found"
    echo "Please create the network topology first"
    exit 1
fi

# Check if config exists
if [ ! -f "omnetpp.ini" ]; then
    echo "Error: omnetpp.ini not found"
    echo "Please create the configuration file first"
    exit 1
fi

# Run simulation
echo "Running baseline GPSR simulation..."
echo "Results will be saved to ../../results/"

opp_run -m -u Cmdenv \
    -c Baseline \
    -n ../../src:. \
    --result-dir=../../results \
    "$@"

echo "Simulation complete. Check results/ directory for outputs."
