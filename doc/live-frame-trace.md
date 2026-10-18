# Per-picture internal telemetry (fork API 1)

`VVENC_LIVE_FRAME_TRACE_VERSION=1` extends YUVBuffer with `liveTrace` (default
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
