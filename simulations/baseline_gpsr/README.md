# Baseline GPSR Scenario

## Purpose
This scenario establishes the baseline performance of standard GPSR routing for comparison with queue-aware extensions.

## Configuration

### Network Topology
- **Nodes:** 20 mobile ad-hoc hosts
- **Area:** 1000m x 1000m
- **Radio:** IEEE 802.11, 2mW transmit power, 2Mbps bitrate
- **Routing:** Standard INET GPSR with Greedy-GG planarization

### Mobility Models
1. **Stationary** (`Baseline` config) - Nodes remain fixed
2. **Random Waypoint** (`BaselineMobile` config) - Nodes move at 5-15 m/s

### Traffic Pattern
- **Type:** UDP application traffic
- **Flows:** 5 concurrent source-destination pairs
- **Load:** Exponential inter-packet time (1s mean)
- **Packet Size:** 512 bytes
- **Duration:** 500 seconds

## Metrics Collected

### Primary Metrics
- **End-to-end delay** - Time from source to destination
- **Packet delivery ratio** - Successfully delivered / sent packets
- **Routing overhead** - Control packets vs. data packets

### Secondary Metrics
- **Hop count** - Average path length
- **Route stability** - Link/route change frequency
- **Network throughput** - Aggregate data delivery

## Running the Simulation

### Using Script
```bash
cd scripts
./run_baseline.sh
```

### Manually
```bash
cd simulations/baseline_gpsr
opp_run -m -u Cmdenv -c Baseline -n ../../src:. --result-dir=../../results
```

### Multiple Configurations
```bash
# Run all configs in this scenario
opp_run -m -u Cmdenv -c Baseline,BaselineMobile,BaselineHighLoad -n ../../src:. --result-dir=../../results
```

## Expected Results

Results will be saved to `../../results/baseline_gpsr/`:
- `*.sca` - Scalar statistics (means, counts, etc.)
- `*.vec` - Vector data (time series)

## Comparison Usage

These baseline results serve as reference for:
1. Delay tiebreaker extension
2. Queue-aware next-hop selection
3. Two-hop peeking
4. Compute-aware offloading

## Notes

- Uses standard INET GPSR without modifications
- Multiple repetitions (10) for statistical confidence
- Scenarios designed to stress different aspects (mobility, load)

## Related Files
- `BaselineNetwork.ned` - Network topology
- `omnetpp.ini` - Configuration parameters
- `../../src/researchproject/routing/queuegpsr/` - Future extensions will be here
