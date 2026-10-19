/* -----------------------------------------------------------------------------
The copyright in this software is being made available under the Clear BSD
License, included below. No patent rights, trademark rights and/or
other Intellectual Property Rights other than the copyrights concerning
the Software are granted under this license.

The Clear BSD License

Copyright (c) 2019-2026, Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V. & The VVenC Authors.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted (subject to the limitations in the disclaimer below) provided that
the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

     * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.

     * Neither the name of the copyright holder nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.


------------------------------------------------------------------------------------------- */
// Exact 4x4 mean/deviation primitive used by PreProcess screen-content detection.
// No thresholds, samples, tools or future-picture dependencies are changed.
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdlib>

#if ENABLE_SIMD_OPT && defined(__SSE2__)
#include <emmintrin.h>
#endif

namespace vvenc { namespace detail {

static inline int sccBlockDeviationScalar( const int16_t* src, ptrdiff_t stride, int& mean )
{
  mean = 0;
  for( int y = 0; y < 4; ++y )
    for( int x = 0; x < 4; ++x ) mean += src[y * stride + x];
  int deviation = 0;
  for( int y = 0; y < 4; ++y )
    for( int x = 0; x < 4; ++x ) deviation += std::abs( mean - ( int(src[y * stride + x]) << 4 ) );
  return deviation;
}

#if ENABLE_SIMD_OPT && defined(__SSE2__)
static inline int sccSum4( __m128i value )
{
  value = _mm_add_epi32( value, _mm_srli_si128( value, 8 ) );
  value = _mm_add_epi32( value, _mm_srli_si128( value, 4 ) );
  return _mm_cvtsi128_si32( value );
}

static inline __m128i sccAbsDeviation( __m128i row, __m128i mean )
{
  __m128i difference = _mm_sub_epi32( mean, _mm_slli_epi32( row, 4 ) );
  __m128i sign = _mm_srai_epi32( difference, 31 );
  return _mm_sub_epi32( _mm_xor_si128( difference, sign ), sign );
}

static inline int sccBlockDeviationSse2( const int16_t* src, ptrdiff_t stride, int& mean )
{
  // Exactly four Pel values per row: no alignment or padding requirement.
  // Keep unsigned input samples in 32-bit lanes so 10/12-bit scaled
  // deviations cannot overflow signed 16-bit arithmetic.
  const __m128i zero = _mm_setzero_si128();
  const __m128i a = _mm_unpacklo_epi16( _mm_loadl_epi64( reinterpret_cast<const __m128i*>(src) ), zero );
  const __m128i b = _mm_unpacklo_epi16( _mm_loadl_epi64( reinterpret_cast<const __m128i*>(src + stride) ), zero );
  const __m128i c = _mm_unpacklo_epi16( _mm_loadl_epi64( reinterpret_cast<const __m128i*>(src + 2 * stride) ), zero );
  const __m128i d = _mm_unpacklo_epi16( _mm_loadl_epi64( reinterpret_cast<const __m128i*>(src + 3 * stride) ), zero );
  mean = sccSum4( _mm_add_epi32( _mm_add_epi32(a, b), _mm_add_epi32(c, d) ) );
  const __m128i broadcast = _mm_set1_epi32( mean );
  return sccSum4( _mm_add_epi32(
    _mm_add_epi32( sccAbsDeviation(a, broadcast), sccAbsDeviation(b, broadcast) ),
    _mm_add_epi32( sccAbsDeviation(c, broadcast), sccAbsDeviation(d, broadcast) ) ) );
}
#endif

static inline int sccBlockDeviation( const int16_t* src, ptrdiff_t stride, int& mean )
{
#if ENABLE_SIMD_OPT && defined(__SSE2__)
  return sccBlockDeviationSse2( src, stride, mean );
#else
  return sccBlockDeviationScalar( src, stride, mean );
#endif
}

} }
