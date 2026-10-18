// Byte samples and signed residuals deliberately have different storage contracts.
#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <stdexcept>
#include <algorithm>
namespace vvenc {
using Sample8 = uint8_t;
using Residual = int16_t;
using WorkingSample = int16_t; // fractional prediction/filter intermediates, not byte pixels
static_assert(sizeof(Sample8)==1 && sizeof(Residual)==2, "native sample/residual precision");

// Published only after the existing full-picture reconstruction/border barrier.
// Stride is in bytes, numerically identical to the legacy reference's sample stride.
class ReferencePlane8 {
  std::vector<Sample8> storage;
  ptrdiff_t origin = 0;
  int stride = 0;
  bool ready = false;
public:
  void reset() { ready=false; }
  void destroy() { std::vector<Sample8>().swap(storage); ready=false; }
  const Sample8* data() const { return ready ? storage.data()+origin : nullptr; }
  size_t bytes() const { return ready ? storage.size() : 0; }
  void assign(const WorkingSample* src, int pitch, int width, int height, int mx, int my) {
    if(!src || width<=0 || height<=0 || mx<0 || my<0 || pitch<width+2*mx)
      throw std::invalid_argument("invalid native8 reference geometry");
    ready=false; stride=pitch; origin=ptrdiff_t(my)*pitch+mx;
    storage.resize(size_t(pitch)*(height+2*my));
    for(int y=-my;y<height+my;y++) {
      const WorkingSample* row=src+ptrdiff_t(y)*pitch-mx;
      Sample8* dst=storage.data()+origin+ptrdiff_t(y)*pitch-mx;
      for(int x=0;x<width+2*mx;x++) {
        if(unsigned(row[x])>255) throw std::range_error("native8 reference sample outside 0..255");
        dst[x]=Sample8(row[x]);
      }
    }
    ready=true;
  }
};
inline uint64_t sad8Scalar(const Sample8* a, ptrdiff_t as, const Sample8* b, ptrdiff_t bs,
                           int w, int h, int subShift, uint64_t maximum=UINT64_MAX) {
  uint64_t sum=0; const int step=1<<subShift;
  for(int y=0;y<h;y+=step) {
    for(int x=0;x<w;x++) sum+=unsigned(a[x]>b[x]?a[x]-b[x]:b[x]-a[x]);
    if(w>=64 && (sum<<subShift)>maximum) return sum<<subShift;
    a+=as*step;b+=bs*step;
  }
  return sum<<subShift;
}
} // namespace vvenc
