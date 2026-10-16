# Fork-only PTS-grid GOP4 mode

`VVENC_LIVE_PTS_GOP_VERSION == 1` adds opt-in `LivePtsGop=1`. It uses one
reserved config byte without changing `sizeof(vvenc_config)` or subsequent
field offsets. Default/upstream GOP scheduling is unchanged.

This narrow experimental mode requires GOP4, POC0IDR, CRA, picture reordering,
STA=0, MaxParallelFrames=0, a positive TicksPerSecond and RefreshSec, single
pass, no lead/trail pictures, and no future-reference MCTF. The service uses
its explicit P4/B2/B1/B3 layout with BIM=0. Initialization rejects other
layouts that refer past the preceding anchor or exceed the advertised DPB.

Input CTS must be valid and strictly increasing. The first CTS anchors the
RefreshSec grid. The first frame in a newly occupied grid interval becomes
a standalone CRA. A forward timestamp gap skips empty intervals; it does
not create multiple refresh frames. Missing, repeated, backward or overflowing
timestamps fail instead of silently changing the timeline. The nominal frame
rate and IntraPeriod still guide RC; they do not decide refresh positions.

The preprocessor plans at most four pending pictures before publishing their
GOP entries to MCTF and the encoders. It closes a group before a CRA or EOF
with P, pruning it to 1/2/3 pictures when needed. Full groups are BBBP. All
reference lists and coding numbers are assigned before downstream processing;
no published GOP entry is rewritten. Tail lists are cached and checked against
the full group's DPB limits. MCTF consumes each planned batch without waiting
for another changed queue tail. Serial RC waits for its analysis chunk before
initializing the next anchor, avoiding an unfinished picture on reentry.

This mode returns valid CTS and **invalid DTS** in vvencAccessUnit. Its matching
FFmpeg wrapper supplies DTS from a bounded FIFO of input timestamps, shifted
by two pictures (the maximum display/coding reordering for this profile).
The two initial DTS values extrapolate the nominal frame interval before the
first CTS. The wrapper checks monotonic DTS and DTS <= PTS, and rejects FIFO
overflow. Do not use the original nominal-frame DTS arithmetic with VFR.

Validation in the parent project uses independent native VVC decode, complete
frame counts, PTS interval preservation, exact occupied-grid key positions,
short tails IP/IBP/IBBP, fractional frame rates, cadence changes, jitter,
timestamp gaps, nonzero origins, UDP output, RC and latency/trace accounting.
See tests/vvenc_pts_integration.py and tests/vvenc_integration.py there.
