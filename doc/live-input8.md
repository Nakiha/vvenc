# Packed byte input for the live path

`VVENC_LIVE_INPUT8_VERSION=1` extends `vvencYUVBuffer` and requires all clients to rebuild against the matching fork ABI. It adds three non-owning, read-only `planes8` pointers. Plane width/height and stride remain in `planes`; stride is bytes when byte input is selected. All active byte planes must be present and every corresponding legacy `planes[].ptr` must be null. Default initialization clears the new pointers and trace flag. Buffer free functions never free byte pointers.

The API accepts packed input only for the verified serial LiveCqfPoll path with input and internal precision 8. Invalid dimensions, short/negative strides, missing chroma, mixed pointers and incompatible modes fail before reading samples. The library copies active rows into its owned working pixels before returning; callers need no extra full-frame 16-bit staging allocation. Input pixels are not modified. Existing row/bottom padding stays in the copy owner. The legacy downsample branch is explicitly unavailable for byte input.

This is an input contract, not a complete byte-storage encoder. Orig/reference/residual Pel storage and existing SIMD remain 16-bit. Copying byte input currently zero-extends to that owned working storage. Splitting sample storage from signed residual/interpolation types and byte-specific kernels is subsequent work; do not change the global Pel typedef to uint8_t.

PTS, ordinary-I requests, per-picture QP offsets, pending poll and tracing use the existing picture ownership. There are no new queues, future-picture reads or IPC frame pointers. JSON support for one-pass lookahead RC is unchanged.

The parent service has a read-only mmap/API fixture comparing byte input with equivalent legacy 8-bit samples in 16-bit containers, invalid-input checks and live HEVC integration/EOF and on/off trace equivalence checks.
