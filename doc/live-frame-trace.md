# Per-picture internal telemetry (fork API 2)

`VVENC_LIVE_FRAME_TRACE_VERSION=2` extends YUVBuffer with `liveTrace` (default
false) and AccessUnit with a plain `vvencLiveFrameTrace` snapshot. Recompile all
clients with the matching ABI. Opt-in is restricted to the live CQF poll path.

Each PicShared resets on reuse. Steady-clock microsecond events cover input copy,
last required group input, group formation, per-picture QPA preprocessing, MCTF,
GOP dispatch, compress submission return, first/last successful CTU task,
finalize callback entry/end, reconstruction
and actual AU construction. They change no readiness checks or coding choices.
Input/group timestamps must be correlated by CTS/POC, not output order.

Only successfully ready CTU work is timed: the readiness probe takes no clocks
and does no trace atomics. Per-picture relaxed atomic aggregates collect thread
CPU for CTU search/quantization, deblock/reshape, SAO, ALF/CCALF, and finishing;
worker wall and call counts are separate. Concurrent CPU sums are not elapsed
latency. Main dispatch/finalize and preprocessing CPU are separate and can overlap
worker execution. MCTF wall includes parallel worker waits; this first API does
not collect total MCTF worker CPU. No I/O, logging, allocation or new task queues
occur in CTU tracing. Compare diagnostic on/off; this is not a free profiler.
Linux CLOCK_THREAD_CPUTIME_ID and CLOCK_MONOTONIC are the validated clock domain.

API 2 adds GOP admission, slice initialization begin/end/thread CPU, and the
immediately preceding serial picture's exact reconstruction publication clock.
This clock is published atomically by the existing finalize callback and is never
used to decide readiness. In the supported non-FPP live CQF path it splits the
admission-to-dispatch residence into overlap with unfinished serial predecessor,
available-but-not-yet-initialized residence, initialization, and post-init residence.
Overlap measures a necessary serial constraint, not an exclusive wait on a mutex;
the remaining available interval includes polling cadence and untimed checks.
The application auditor verifies predecessor identity against actual coded order.

GOP visit counts and rejected coding-order, pre-analysis, rate-cap, reference,
APS, picture-encoder and RC checks are integer counters, not time estimates. The
original short-circuit checks run once; diagnostics add no reference probes and
no clocks to readiness probes. Unvisited pictures behind the coding-order head
do not acquire inferred rejection counts. Clients must rebuild for the new ABI.
