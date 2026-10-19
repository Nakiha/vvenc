#include "CommonLib/Unit.h"
#if ENABLE_SIMD_OPT_BUFFER && defined(TARGET_SIMD_X86)
#include "CommonLib/x86/CommonDefX86.h"
#endif
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <sys/mman.h>
#include <unistd.h>

static int reference(const vvenc::Pel* p,ptrdiff_t stride,int& mean)
{
  mean=0;for(int y=0;y<4;y++)for(int x=0;x<4;x++)mean+=p[y*stride+x];
  int variance=0;for(int y=0;y<4;y++)for(int x=0;x<4;x++)variance+=std::abs(mean-(int(p[y*stride+x])<<4));
  return variance;
}
int main(int argc,char** argv)
{
  if(argc!=2)return 2;
#if ENABLE_SIMD_OPT_BUFFER && defined(TARGET_SIMD_X86)
  const auto detected=vvenc::read_x86_extension_flags();
  const bool scalar=std::strcmp(argv[1],"SCALAR")==0;
  // This is the actual runtime selector used by vvenc_set_SIMD_extension.
  vvenc::read_x86_extension_flags(scalar?vvenc::SCALAR:detected);
#else
  if(std::strcmp(argv[1],"DISABLED"))return 3;
#endif
  vvenc::PelBufferOps ops;
  const auto constructorScalar=ops.SccBlockDeviation;
  if(!constructorScalar)return 4;
#if ENABLE_SIMD_OPT_BUFFER && defined(TARGET_SIMD_X86)
  ops.initPelBufOpsX86();
  if(!ops.isInitX86Done)return 5;
  if(scalar && (vvenc::read_x86_extension_flags()!=vvenc::SCALAR || ops.SccBlockDeviation!=constructorScalar))return 6;
  if(!scalar && detected>=vvenc::SSE41 && ops.SccBlockDeviation==constructorScalar)return 7;
  const auto selected=ops.SccBlockDeviation;
  ops.initPelBufOpsX86();if(ops.SccBlockDeviation!=selected)return 8;
#endif
  std::mt19937 random(0x517cc1);std::array<vvenc::Pel,4*97+16> src;unsigned checked=0;
  for(int depth:{8,10,12,15})for(int stride:{4,5,17,97})for(int offset=0;offset<8;offset++)for(int pattern=0;pattern<2000;pattern++)
  {
    const int maximum=(1<<depth)-1;
    for(auto& p:src)p=pattern==0?0:pattern==1?maximum:pattern==2?maximum/2:pattern==3?(&p-src.data())%2*maximum:random()&maximum;
    int a,b;const int av=reference(src.data()+offset,stride,a);
    const int bv=ops.SccBlockDeviation(src.data()+offset,stride,b);
    if(a!=b||av!=bv){std::fprintf(stderr,"dispatch mismatch depth=%d stride=%d offset=%d pattern=%d\n",depth,stride,offset,pattern);return 9;}
    checked++;
  }
  const int page=sysconf(_SC_PAGESIZE);
  char* memory=(char*)mmap(nullptr,page*8,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
  if(memory==MAP_FAILED)return 10;
  for(int y=0;y<4;y++)
  {
    if(mprotect(memory+2*y*page,page,PROT_READ|PROT_WRITE))return 11;
    auto row=(vvenc::Pel*)(memory+(2*y+1)*page-8);
    for(int x=0;x<4;x++)row[x]=(y*4+x)*257;
  }
  const auto start=(const vvenc::Pel*)(memory+page-8);int a,b;
  const int av=reference(start,page,a),bv=ops.SccBlockDeviation(start,page,b);
  if(av!=bv||a!=b)return 12;
  munmap(memory,page*8);
  std::printf("mode=%s actual_PelBufferOps=passed cases=%u short_rows=passed\n",argv[1],checked);
  return 0;
}
