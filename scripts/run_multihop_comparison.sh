#!/bin/bash

echo "═══════════════════════════════════════════════════════════════════"
echo "  MULTI-HOP PERFORMANCE COMPARISON"
echo "  Purpose: Measure end-to-end impact when relays are actually used"
echo "═══════════════════════════════════════════════════════════════════"
echo ""
echo "Configuration: 10mW radio power (range ~300m)"
echo "Source-Destination: 450m (requires relay)"
echo "Relays: host[1] congested (2.28 Mbps), host[2] idle"
echo ""

# Ensure logs directory exists at project root
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$PROJECT_ROOT/logs"

cd simulations/delay_tiebreaker

echo "[1/2] Running Baseline GPSR (no tiebreaker)..."
../../Research_project -u Cmdenv -c MultiHopPerformanceBaseline \
    -n ../../src:../../../inet4.5/src:. \
    --sim-time-limit=40s \
    --result-dir=../../results \
    > ../../logs/multihop_baseline.log 2>&1

if [ $? -eq 0 ]; then
    echo "✓ Baseline complete"
else
    echo "✗ Baseline failed - check logs/multihop_baseline.log"
    tail -20 ../../logs/multihop_baseline.log
    exit 1
fi

echo ""
echo "[2/2] Running Queue-Aware GPSR (with tiebreaker)..."
../../Research_project -u Cmdenv -c MultiHopPerformanceEnhanced \
    -n ../../src:../../../inet4.5/src:. \
    --sim-time-limit=40s \
    --result-dir=../../results \
    > ../../logs/multihop_enhanced.log 2>&1

if [ $? -eq 0 ]; then
    echo "✓ Enhanced complete"
else
    echo "✗ Enhanced failed - check logs/multihop_enhanced.log"
    tail -20 ../../logs/multihop_enhanced.log
    exit 1
fi

cd ../..

echo ""
echo "═══════════════════════════════════════════════════════════════════"
echo "  RESULTS ANALYSIS"
echo "═══════════════════════════════════════════════════════════════════"
echo ""

# Extract delivery statistics
echo "[1] PACKET DELIVERY"
echo "─────────────────────────────────────────────────────────────────"

baseline_sent=$(grep "scalar.*host\[0\].*app\[0\].*packetSent:count" results/MultiHopPerformanceBaseline-#0.sca | awk '{print $NF}')
baseline_rcvd=$(grep "scalar.*host\[3\].*app\[1\].*packetReceived:count" results/MultiHopPerformanceBaseline-#0.sca | awk '{print $NF}')
baseline_delivery=$(echo "scale=2; $baseline_rcvd * 100 / $baseline_sent" | bc 2>/dev/null || echo "N/A")

enhanced_sent=$(grep "scalar.*host\[0\].*app\[0\].*packetSent:count" results/MultiHopPerformanceEnhanced-#0.sca | awk '{print $NF}')
enhanced_rcvd=$(grep "scalar.*host\[3\].*app\[1\].*packetReceived:count" results/MultiHopPerformanceEnhanced-#0.sca | awk '{print $NF}')
enhanced_delivery=$(echo "scale=2; $enhanced_rcvd * 100 / $enhanced_sent" | bc 2>/dev/null || echo "N/A")

echo "Baseline GPSR (no tiebreaker):"
echo "  Sent:     $baseline_sent"
echo "  Received: $baseline_rcvd"
echo "  Ratio:    ${baseline_delivery}%"
echo ""
echo "Queue-Aware GPSR (with tiebreaker):"
echo "  Sent:     $enhanced_sent"
echo "  Received: $enhanced_rcvd"
echo "  Ratio:    ${enhanced_delivery}%"
echo ""

if [ "$baseline_rcvd" != "$enhanced_rcvd" ]; then
    improvement=$(echo "$enhanced_rcvd - $baseline_rcvd" | bc)
    echo "Improvement: +${improvement} packets delivered"
else
    echo "Delivery ratio: Same for both (expected with light flow)"
fi

echo ""
echo "[2] ROUTING BEHAVIOR"
echo "─────────────────────────────────────────────────────────────────"

enhanced_via_congested=$(grep "\[ROUTE\].*host\[0\].*nextHop=10.0.0.2" multihop_enhanced.log | wc -l | tr -d ' ')
enhanced_via_idle=$(grep "\[ROUTE\].*host\[0\].*nextHop=10.0.0.3" multihop_enhanced.log | wc -l | tr -d ' ')
baseline_via_congested=$(grep "\[ROUTE\].*host\[0\].*nextHop=10.0.0.2" ../../logs/multihop_baseline.log | wc -l | tr -d ' ')
baseline_via_idle=$(grep "\[ROUTE\].*host\[0\].*nextHop=10.0.0.3" ../../logs/multihop_baseline.log | wc -l | tr -d ' ')

enhanced_via_congested=$(grep "\[ROUTE\].*host\[0\].*nextHop=10.0.0.2" ../../logs/multihop_enhanced.log | wc -l | tr -d ' ')
enhanced_via_idle=$(grep "\[ROUTE\].*host\[0\].*nextHop=10.0.0.3" ../../logs/multihop_enhanced.log | wc -l | tr -d ' ')

echo "Baseline GPSR routing decisions:"
echo "  Via congested relay (host[1]):  $baseline_via_congested"
echo "  Via idle relay (host[2]):       $baseline_via_idle"
if [ "$baseline_via_idle" -gt 0 ] && [ "$baseline_via_congested" -gt 0 ]; then
    baseline_idle_pct=$(echo "scale=1; $baseline_via_idle * 100 / ($baseline_via_congested + $baseline_via_idle)" | bc)
    echo "  → ${baseline_idle_pct}% toward idle relay"
fi
echo ""
echo "Queue-Aware GPSR routing decisions:"
echo "  Via congested relay (host[1]):  $enhanced_via_congested"
echo "  Via idle relay (host[2]):       $enhanced_via_idle"
if [ "$enhanced_via_idle" -gt 0 ] && [ "$enhanced_via_congested" -gt 0 ]; then
    enhanced_idle_pct=$(echo "scale=1; $enhanced_via_idle * 100 / ($enhanced_via_congested + $enhanced_via_idle)" | bc)
    echo "  → ${enhanced_idle_pct}% toward idle relay"
elif [ "$enhanced_via_idle" -gt 0 ]; then
    echo "  → 100% toward idle relay (perfect congestion avoidance)"
fi

echo ""
echo "[3] TIEBREAKER ACTIVATIONS"
echo "─────────────────────────────────────────────────────────────────"

baseline_tiebreakers=$(grep "scalar.*host\[0\].*routing.*tiebreakerActivations[^:]" results/MultiHopPerformanceBaseline-#0.sca | awk '{print $NF}')
enhanced_tiebreakers=$(grep "scalar.*host\[0\].*routing.*tiebreakerActivations[^:]" results/MultiHopPerformanceEnhanced-#0.sca | awk '{print $NF}')

echo "Baseline: $baseline_tiebreakers (should be 0 - disabled)"
echo "Enhanced: $enhanced_tiebreakers (activations during congestion)"

echo ""
echo "[4] END-TO-END DELAY"
echo "─────────────────────────────────────────────────────────────────"

# Extract delay statistics
baseline_delay=$(grep "host\[3\].*app\[1\].*endToEndDelay:histogram" results/MultiHopPerformanceBaseline-#0.sca -A 3 | grep "field mean" | awk '{print $3}')
enhanced_delay=$(grep "host\[3\].*app\[1\].*endToEndDelay:histogram" results/MultiHopPerformanceEnhanced-#0.sca -A 3 | grep "field mean" | awk '{print $3}')

baseline_delay_ms=$(echo "scale=2; $baseline_delay * 1000" | bc 2>/dev/null || echo "N/A")
enhanced_delay_ms=$(echo "scale=2; $enhanced_delay * 1000" | bc 2>/dev/null || echo "N/A")

echo "Baseline GPSR (all via congested relay):"
echo "  Mean delay: ${baseline_delay_ms} ms"
echo ""
echo "Queue-Aware GPSR (mixed routing):"
echo "  Mean delay: ${enhanced_delay_ms} ms"

if [ ! -z "$baseline_delay" ] && [ ! -z "$enhanced_delay" ]; then
    delay_reduction=$(echo "scale=2; ($baseline_delay - $enhanced_delay) * 1000" | bc)
    delay_reduction_pct=$(echo "scale=1; ($baseline_delay - $enhanced_delay) * 100 / $baseline_delay" | bc 2>/dev/null || echo "N/A")
    
    if (( $(echo "$delay_reduction > 0" | bc -l) )); then
        echo ""
        echo "Improvement: -${delay_reduction} ms (${delay_reduction_pct}% reduction)"
    elif (( $(echo "$delay_reduction < 0" | bc -l) )); then
        delay_increase=$(echo "$delay_reduction * -1" | bc)
        echo ""
        echo "Change: +${delay_increase} ms (delay increased)"
    else
        echo ""
        echo "No change in delay"
    fi
fi

echo ""
echo "═══════════════════════════════════════════════════════════════════"
echo ""
echo "Results saved to:"
echo "  - results/MultiHopPerformanceBaseline-#0.sca"
echo "  - results/MultiHopPerformanceEnhanced-#0.sca"
echo "  - logs/multihop_baseline.log"
echo "  - logs/multihop_enhanced.log"
echo ""
