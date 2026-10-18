// Byte SAD implementation shared by runtime-dispatched encoder and boundary tests.
#pragma once
#include "../Native8.h"
#include <immintrin.h>
namespace vvenc {
template<bool avx2>
inline uint64_t sad8X86(const Sample8* a, ptrdiff_t as, const Sample8* b, ptrdiff_t bs,
                       int w, int h, int subShift, uint64_t maximum=UINT64_MAX)
{
  const int step=1<<subShift;
  __m128i sum=_mm_setzero_si128(); uint64_t tail=0;
#ifdef USE_AVX2
  if(avx2 && w>=32 && (w&31)==0) {
    __m256i acc=_mm256_setzero_si256();
    for(int y=0;y<h;y+=step) {
      for(int x=0;x<w;x+=32)
        acc=_mm256_add_epi64(acc,_mm256_sad_epu8(_mm256_loadu_si256((const __m256i*)(a+x)),_mm256_loadu_si256((const __m256i*)(b+x))));
      if(w>=64 && ((y>>subShift)&3)==3) {
        const __m128i both=_mm_add_epi64(_mm256_castsi256_si128(acc),_mm256_extracti128_si256(acc,1));
        const uint64_t partial=(uint64_t(_mm_cvtsi128_si64(both))+uint64_t(_mm_cvtsi128_si64(_mm_srli_si128(both,8))))<<subShift;
        if(partial>maximum)return partial;
      }
      a+=as*step;b+=bs*step;
    }
    sum=_mm_add_epi64(_mm256_castsi256_si128(acc),_mm256_extracti128_si256(acc,1));
  } else
#endif
  {
    for(int y=0;y<h;y+=step) {
      int x=0;
      for(;x+16<=w;x+=16)
        sum=_mm_add_epi64(sum,_mm_sad_epu8(_mm_loadu_si128((const __m128i*)(a+x)),_mm_loadu_si128((const __m128i*)(b+x))));
      if(x+8<=w) {
        sum=_mm_add_epi64(sum,_mm_sad_epu8(_mm_loadl_epi64((const __m128i*)(a+x)),_mm_loadl_epi64((const __m128i*)(b+x))));x+=8;
      }
      for(;x<w;x++) tail+=unsigned(a[x]>b[x]?a[x]-b[x]:b[x]-a[x]);
      if(w>=64 && ((y>>subShift)&3)==3) {
        const uint64_t partial=(uint64_t(_mm_cvtsi128_si64(sum))+uint64_t(_mm_cvtsi128_si64(_mm_srli_si128(sum,8)))+tail)<<subShift;
        if(partial>maximum)return partial;
      }
      a+=as*step;b+=bs*step;
    }
  }
  return (uint64_t(_mm_cvtsi128_si64(sum))+uint64_t(_mm_cvtsi128_si64(_mm_srli_si128(sum,8)))+tail)<<subShift;
}
}
