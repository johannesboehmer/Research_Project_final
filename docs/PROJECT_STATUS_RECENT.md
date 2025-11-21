````markdown
# Project Status — Recent Standing

**Date:** 21 November 2025

This document summarizes the current standing of the Research_Project after the recent MAC queue measurement fixes, delay tiebreaker validation, repository cleanup and archival work.

**Quick summary:**
- Delay-aware tiebreaker: implemented and validated (distance+queue delay estimate).
- MAC queue measurement: fixed by reading actual MAC `IPacketCollection` queues (no longer using the artificial counter).
- Beacons: carry a compact `txBacklogBytes` field and are used for neighbor queue state exchange.
- Diagnostics: extensive audit and logging helpers added for validation; can be gated off.
- Repo: `simulations/queuegpsr_test` archived to `docs/archive/queuegpsr_test.tar.gz` and changes pushed to remote.

**Where to look (key files)**
- Routing and tiebreaker: `src/researchproject/routing/queuegpsr/QueueGpsr.cc` / `.h`
- Beacon & msg definition: `src/researchproject/routing/queuegpsr/QueueGpsr.msg` (field `txBacklogBytes`)
- MAC introspection diagnostics: `QueueGpsr::auditMacQueues()` and `getLocalTxBacklogBytes()` in `QueueGpsr.cc`
- Recent implementation report: `docs/MAC_QUEUE_FIX_IMPLEMENTATION.md`
- Archive location: `docs/archive/queuegpsr_test.tar.gz`

**What is implemented (Phase 1–3 items covered)**
- Beacon metric: `GpsrBeacon` includes `uint32_t txBacklogBytes` (set when `enableQueueDelay` is true).
- Accurate local queue reading: `getLocalTxBacklogBytes()` recursively finds modules implementing `queueing::IPacketCollection` under the node's `wlan[0].mac` subtree and sums their `getTotalLength()`.
- Beacon reception: `processBeacon()` stores neighbor `txBacklogBytes` with a timestamp in `neighborTxBacklogBytes`.
- Delay tiebreaker: `estimateNeighborDelay()` computes delay = distance*factor + queueDelay (if fresh) and `findGreedyRoutingNextHop()` uses this in equidistant tie cases.
- Freshness/aging: queue entries are aged and discarded when older than `beaconInterval * 3`.
- Gates: `enableDelayTiebreaker` and `enableQueueDelay` parameters exist to toggle behavior.

**What is present but fragile / partial (gaps to close for Phase 3 completeness)**
1. No dedicated reusable `QueueInspector` module. Current queue-aggregation logic is inline in `QueueGpsr::getLocalTxBacklogBytes()`; this works but is not modular or reusable by other modules.
2. `localTxBacklogBytes` counter exists (incremented on local out) but is not decremented on actual MAC transmit — it is legacy and should be removed or fixed to avoid confusion.
3. Bitrate probing is brittle: code attempts to read `wlan[0].radio.transmitter.bitrate` on neighbor hosts — this may fail for different radio models or INET versions. There is no robust fallback other than skipping Q/R when bitrate missing.
4. No on-demand RPC for freshest queue measurements; the system relies on beacons only (passive, low-overhead).
5. Beacon payload is intentionally compact (single uint32). If per-AC or normalized metrics are needed, beacon format must be extended and compatibility handled.

**Recent repository actions**
- Archived `simulations/queuegpsr_test` to `docs/archive/queuegpsr_test.tar.gz` and removed it from tree (committed).
- Committed the queue-measurement fixes and diagnostic additions; pushed to remote `main` branch.

**Validation highlights**
- Micro test (4-node relay test): tiebreaker activations increased substantially (e.g., 214/250) and overall delivery and delay improved (delivery up to ~97%, mean delay greatly reduced).
- Audit logs confirm `pendingQueue` under `mac.dcf.channelAccess` as the active `IPacketCollection` in INET 4.5 and the aggregated byte counts are non-zero on congested relays.

**Recommended immediate next steps**
1. Add a small reusable `QueueInspector` helper (C++ utility under `src/researchproject/linklayer/queue/`) that encapsulates the recursive `IPacketCollection` discovery and optionally caches the queue module pointer after first successful find. This improves maintainability and reuse.
2. Fix or remove `localTxBacklogBytes`: either implement proper decrement on transmit (hook into MAC tx completion) or delete the counter to prevent misuse.
3. Add a `nominalRadioBitrate` fallback parameter (global or per-node) read from `omnetpp.ini` and used when dynamic bitrate detection fails — this will make Q/R-based delay estimates robust across radio models.
4. Optionally: prototype a lightweight `QueueQuery`/`QueueReply` control message for on-demand fresh measurements (trade-off: extra messages vs fresh data).
5. Reduce diagnostic verbosity behind a `queueInspectorAudit`/`debugMode` flag so production runs are clean.
6. Implement `congested_cross` NED + `omnetpp.ini` (from `simulations/congested_cross/README.md`) to stress-test queue-aware routing at scale.

**Repro/quick commands**
Run the multihop comparison (writes logs to `logs/`):

```bash
cd Research_project
scripts/run_multihop_comparison.sh 2>&1 | tee logs/run_multihop_comparison.latest.out
```

Run a single scenario manually (example):

```bash
./out/clang-release/Research_project -u Cmdenv \
  -l ../../inet4.5/src/libINET.dylib \
  -n .:src:../../inet4.5/src \
  -c CongestedDelayTiebreakerEnabled -r 0
```

**Where artifacts live**
- Logs: `logs/` (latest run in `logs/run_multihop_comparison.latest.out`)
- Archived scenario: `docs/archive/queuegpsr_test.tar.gz`
- Docs & notes: `docs/MAC_QUEUE_FIX_IMPLEMENTATION.md`, `docs/PROJECT_STATUS_RECENT.md` (this file)

**Who/what to contact for next actions**
- If you want me to implement the `QueueInspector` helper and the `nominalRadioBitrate` fallback, say so and I will: (a) add the helper, (b) refactor `QueueGpsr` to use it, (c) add an `omnetpp.ini` parameter and short sanity build/run.

**Status verdict**
- Core Phase 3 objective — accurate MAC queue measurement + beacon propagation — is functionally implemented and validated in the micro test. The remaining work is mainly code hygiene, robustness (bitrate fallback), modularization (`QueueInspector`), and extending scenarios for large-scale validation.

````
