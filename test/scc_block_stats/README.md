# Focused SCC dispatch regression

This Linux GCC/Clang CTest project compiles the **real** `PelBufferOps`
constructor, x86 dispatcher and SSE41/AVX2 buffer translation units. It calls
the selected operation through the real table, not the intrinsic helper.
Runtime SCALAR is selected through the same CPU-selection primitive used by
`vvenc_set_SIMD_extension`, before initialization in a separate process. AUTO
must replace the constructor's scalar pointer; SCALAR must retain it. Repeated
initialization must preserve the selected pointer. Every mode compares 256000
sample/stride/alignment cases to the original integer formula and uses guard
pages to detect overreads at the four-sample row boundary.

Use two new build directories; never overwrite a frozen experiment:

```sh
cmake -S test/scc_block_stats -B /tmp/scc-on -DSCC_TEST_ENABLE_SIMD=ON
cmake --build /tmp/scc-on -j2
ctest --test-dir /tmp/scc-on --output-on-failure -V
cmake -S test/scc_block_stats -B /tmp/scc-off -DSCC_TEST_ENABLE_SIMD=OFF
cmake --build /tmp/scc-off -j2
ctest --test-dir /tmp/scc-off --output-on-failure -V
```

OFF omits `TARGET_SIMD_X86` and the x86 sources (the library's
`VVENC_ENABLE_X86_SIMD=OFF` mechanism) and disables compiler auto-vectorization.
It checks the actual scalar buffer constructor/call path. This is a focused
disabled-backend build, not a claim that the entire encoder was rebuilt with
SIMD disabled. The project never modifies or links a previously measured DSO.
