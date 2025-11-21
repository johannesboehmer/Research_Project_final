#!/bin/bash

#
# Run script for Research_project simulations
# Usage: ./run.sh [CONFIG_NAME] [SIM_TIME_LIMIT]
#

set -e

CONFIG=${1:-Baseline}
SIM_TIME=${2:-500s}

cd "$(dirname "$0")/simulations/baseline_gpsr"

echo "Running configuration: $CONFIG"
echo "Simulation time limit: $SIM_TIME"

opp_run -u Cmdenv \
    -l ../../../inet4.5/src/libINET.dylib \
    -n .:../../src:../../../inet4.5/src \
    -c "$CONFIG" \
    --sim-time-limit="$SIM_TIME"

echo ""
echo "Simulation complete. Results in simulations/baseline_gpsr/results/"
