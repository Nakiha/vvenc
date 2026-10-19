#include "Utilities/LiveCpuLedger.h"
#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>

using namespace vvenc;

static bool coherent(const LiveCpuSnapshot& s) {
  const auto& v=s.value;
  return v[LC_TOTAL]==v[LC_SCAN]+v[LC_YIELD]+v[LC_LOCK]+v[LC_TASKS]
      && v[LC_TASKS]==v[LC_MCTF_ME]+v[LC_MCTF_FILTER]+v[LC_CTU]+v[LC_OTHER]
      && v[LC_YIELD]==2*v[LC_SCAN] && v[LC_LOCK]==3*v[LC_SCAN];
}

int main() {
  LiveCpuLedger ledger;
  std::atomic<bool> start{false}, stop{false};
  std::vector<std::thread> writers;
  for(unsigned tid=0;tid<4;tid++) {
    writers.emplace_back([&,tid] {
      while(!start.load(std::memory_order_acquire)) std::this_thread::yield();
      int64_t generation=0;
      while(!stop.load(std::memory_order_acquire)) {
        ++generation;
        LiveCpuAmounts v{};
        v[LC_SCAN]=generation;
        v[LC_YIELD]=2*generation;
        v[LC_LOCK]=3*generation;
        v[LC_TASKS]=4*generation;
        v[LC_TOTAL]=10*generation;
        v[LC_MCTF_ME]=generation;
        v[LC_MCTF_FILTER]=generation;
        v[LC_CTU]=generation;
        v[LC_OTHER]=generation;
        ledger.publish(tid,v);
        std::this_thread::yield();
      }
    });
  }
  start.store(true,std::memory_order_release);
  unsigned accepted=0;
  bool consistent=true;
  for(int i=0;i<50000;i++) {
    const auto s=ledger.snapshot();
    if(s.valid) {
      ++accepted;
      consistent &= coherent(s);
    }
  }
  stop.store(true,std::memory_order_release);
  for(auto& t:writers) t.join();
  const auto last=ledger.snapshot();
  std::printf("concurrent coherent snapshots=%u final_valid=%d final_coherent=%d concurrent_coherent=%d\n",
              accepted,int(last.valid),int(coherent(last)),int(consistent));
  if(!last.valid||!coherent(last)||!consistent||accepted<10) return 1;
  return 0;
}
