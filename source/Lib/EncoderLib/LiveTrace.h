// Fork opt-in telemetry. No I/O, allocation, ready-check clocks or scheduling changes.
#pragma once
#include "vvenc/vvenc.h"
#include <atomic>
#include <chrono>
#include <ctime>
namespace vvenc {
inline int64_t liveTraceNow() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
inline int64_t liveTraceCpu() {
#ifdef CLOCK_THREAD_CPUTIME_ID
  timespec t; if(!clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t))return int64_t(t.tv_sec)*1000000+t.tv_nsec/1000;
#endif
  return 0;
}
struct LiveTrace {
  vvencLiveFrameTrace data{};
  std::atomic<int64_t> cpu[5],wall,calls,first,last;
  LiveTrace(){ reset(false); }
  void reset(bool enabled) {
    data={}; for(auto& c:cpu)c.store(0,std::memory_order_relaxed);
    wall.store(0,std::memory_order_relaxed);calls.store(0,std::memory_order_relaxed);first.store(0);last.store(0);
    if(enabled)data.input_us=liveTraceNow();
  }
  vvencLiveFrameTrace snapshot() const {
    auto d=data;d.task_first_us=first.load(); d.ctu_cpu_us=cpu[0].load(); d.deblock_cpu_us=cpu[1].load();
    d.sao_cpu_us=cpu[2].load();d.alf_cpu_us=cpu[3].load();d.finish_cpu_us=cpu[4].load();
    d.task_wall_us=wall.load();d.task_calls=calls.load();return d;
  }
};
struct LiveTaskTimer {
  LiveTrace* trace; int kind;int64_t wall=0,cpu=0;
  LiveTaskTimer(LiveTrace& t,int state):trace(t.data.input_us?&t:nullptr),kind(state==0?0:state<=2?1:state==3?2:state<=9?3:4) {
    if(trace){
      wall=liveTraceNow();int64_t old=trace->first.load();
      while((!old||wall<old)&&!trace->first.compare_exchange_weak(old,wall)){}
      cpu=liveTraceCpu();
    }
  }
  ~LiveTaskTimer(){if(trace){trace->cpu[kind].fetch_add(liveTraceCpu()-cpu,std::memory_order_relaxed);int64_t end=liveTraceNow();trace->wall.fetch_add(end-wall,std::memory_order_relaxed);trace->calls.fetch_add(1,std::memory_order_relaxed);
    int64_t old=trace->last.load();while(old<end&&!trace->last.compare_exchange_weak(old,end)){}}}
};
}
