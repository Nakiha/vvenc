# Live per-picture QP delta (fork API 1)

`VVENC_LIVE_QP_OFFSET_VERSION=1` extends vvencYUVBuffer with `liveQpOffset`
(-63..63, default zero), and vvencAccessUnit with `liveSliceQp` (slice QP
before CTU adaptation). Recompile clients against this pinned ABI.
A nonzero delta is accepted only in the validated live CQF poll path.
The delta is copied into PicShared on reuse and applied to that picture's
base QP before the existing temporal/QPA cascade and lambda calculation.
No global encoder config is changed while other pictures are pending.
This API is not a rate controller or an encoder reconfiguration operation.
The caller owns causal budgets, predictions, pending reservations and actual
AU bit feedback; zero preserves the original quality calculation.
