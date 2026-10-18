# Bounded live CQF progress (API 1)

`m_liveCqfPoll` consumes another reserved config byte without changing config
size. It is restricted to live PTS GOP CQF with no RC lookahead, frame parallel
processing or future MCTF references. All other modes retain their scheduling.

PreProcess publishes the complete planned short group before it becomes
visible downstream. Each GOP entry records that group's exact size. The final
stage may therefore run with threshold one, but initializes capped-CQF group
QP only when every member of that group is present and earlier rate updates
are retired. It does not wait for a picture from the next group as a sentinel.
Picture allocation bounds remain unchanged.

`vvenc_encode_pending` advances already submitted pictures through at most
eight serial stage sweeps and returns at most one AU. An empty AU means no
further bounded progress was obtained. No input is accepted, no end-of-stream
state is set, and partial groups are not closed or padded. Normal NULL-input
`vvenc_encode` retains full EOF drain semantics. Call only from the encoder
owner; it is not a concurrent API or a deadline guarantee.

The consumer must drain pending AUs before accepting additional pictures.
Otherwise a one-input/one-AU wrapper can retain output while continually
submitting future frames, even when RC lookahead is disabled.
