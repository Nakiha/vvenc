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



#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#if defined(__linux__)
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace vvenc {
inline int64_t ledgerCpuNow() {
#ifdef CLOCK_THREAD_CPUTIME_ID
  timespec t{}; return clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t)==0 ? int64_t(t.tv_sec)*1000000+t.tv_nsec/1000 : 0;
#else
  return 0;
#endif
}
enum LiveCpuField { LC_TOTAL, LC_SCAN, LC_YIELD, LC_LOCK, LC_TASKS, LC_MCTF_ME,
 LC_MCTF_FILTER, LC_CTU, LC_OTHER, LC_SCAN_CALLS, LC_READY_CALLS,
 LC_READY_FAILED, LC_EMPTY_SCANS, LC_TASK_CALLS, LC_COUNT };
enum LiveCpuTask { LCT_OTHER=0, LCT_MCTF_ME=1, LCT_MCTF_FILTER=2, LCT_CTU=3 };
using LiveCpuAmounts=std::array<int64_t,LC_COUNT>;
struct LiveCpuSnapshot { LiveCpuAmounts value{}; bool valid=true; };
// Fixed-size, per-thread single-writer slots. Snapshot retries are bounded.
// Every category is CPU time, including the CPU spent entering a blocking lock;
// no blocked wall time is charged. TOTAL includes unclassified instrumentation.
class LiveCpuLedger {
 struct Slot { std::atomic<uint64_t> seq{0}; std::array<std::atomic<int64_t>,LC_COUNT> values; Slot(){for(auto& v:values)v.store(0,std::memory_order_seq_cst);} };
 std::array<Slot,64> slots;
public:
 const bool enabled;
 LiveCpuLedger():enabled([]{const char* p=std::getenv("VVENC_LIVE_CPU_LEDGER");return p&&std::strcmp(p,"1")==0;}()){}
 void publish(unsigned tid, const LiveCpuAmounts& values) {
  // All operations participate in one C++ total order. If the reader sees
  // the same even sequence before and after the payload, no publication can
  // be interleaved with those reads. A 64-bit sequence avoids practical wrap.
  auto& s=slots[tid]; s.seq.fetch_add(1,std::memory_order_seq_cst);
  for(unsigned i=0;i<LC_COUNT;i++)s.values[i].store(values[i],std::memory_order_seq_cst);
  s.seq.fetch_add(1,std::memory_order_seq_cst);
 }
 LiveCpuSnapshot snapshot() const {
  LiveCpuSnapshot result;
  for(const auto& s:slots){bool copied=false;
   for(int retry=0;retry<3&&!copied;retry++){
    uint64_t a=s.seq.load(std::memory_order_seq_cst);if(a&1)continue;
    LiveCpuAmounts current{};for(unsigned i=0;i<LC_COUNT;i++)current[i]=s.values[i].load(std::memory_order_seq_cst);
    uint64_t b=s.seq.load(std::memory_order_seq_cst);if(a!=b)continue;
    for(unsigned i=0;i<LC_COUNT;i++) result.value[i]+=current[i];
    copied=true;
   }
   if(!copied)result.valid=false;
  }
  return result;
 }
};
}
