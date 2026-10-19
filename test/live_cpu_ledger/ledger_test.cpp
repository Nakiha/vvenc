#include "Utilities/NoMallocThreadPool.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
using namespace vvenc;
// This pool-only fixture never uses encoder configuration: production thread
// profiling is disabled here, so the pool merely passes the reference through.
// Keep the test independent of the hidden EncCfg constructor in libvvenc.so.
#if ENABLE_TIME_PROFILING_MT_MODE
#error The pool-only configuration stub requires thread profiling to be disabled.
#endif
namespace vvenc { VVEncCfg::VVEncCfg() {} }
static void burn(int64_t microseconds){const int64_t start=ledgerCpuNow();volatile unsigned value=1;while(ledgerCpuNow()-start<microseconds)for(int i=0;i<100;i++)value=value*1664525u+1013904223u;}
struct Task {std::atomic<unsigned> attempts{0};bool retry=false;std::atomic<bool>* ready=nullptr;};
static bool ready(int,Task* t){return !t->ready||t->ready->load();}
static bool work(int,Task* t){burn(500);return !(t->retry&&t->attempts.fetch_add(1)==0);}
int main(int argc,char** argv){
 const bool enabled=argc==2&&!std::strcmp(argv[1],"enabled");
 if(enabled)setenv("VVENC_LIVE_CPU_LEDGER","1",1);else unsetenv("VVENC_LIVE_CPU_LEDGER");
 setenv("BUSY_WAIT_TIME","0",1);
 VVEncCfg config{};NoMallocThreadPool pool(4,"ledger-test",&config);
 std::this_thread::sleep_for(std::chrono::milliseconds(100));
 auto idle0=pool.liveCpuSnapshot();std::this_thread::sleep_for(std::chrono::milliseconds(200));auto idle1=pool.liveCpuSnapshot();
 WaitCounter count;std::vector<Task> tasks(40);std::atomic<bool> gate{false};
 for(unsigned i=0;i<tasks.size();i++){
   tasks[i].retry=(i%5)==0;tasks[i].ready=&gate;
   pool.addBarrierTask<Task>(work,&tasks[i],&count,nullptr,{},ready,LiveCpuTask(i%4));
 }
 std::this_thread::sleep_for(std::chrono::milliseconds(20));gate.store(true);count.wait();
 pool.shutdown(true);auto snapshot=pool.liveCpuSnapshot();if(!snapshot.valid)return 2;
 const auto& c=snapshot.value;
 if(!enabled){for(auto v:c)if(v)return 3;std::puts("disabled: zero ledger counters");return 0;}
 if(c[LC_TASKS]!=c[LC_MCTF_ME]+c[LC_MCTF_FILTER]+c[LC_CTU]+c[LC_OTHER])return 4;
 if(c[LC_TOTAL]<c[LC_SCAN]+c[LC_YIELD]+c[LC_LOCK]+c[LC_TASKS])return 5;
 if(c[LC_TASK_CALLS]!=48||!c[LC_READY_FAILED]||!c[LC_MCTF_ME]||!c[LC_MCTF_FILTER]||!c[LC_CTU]||!c[LC_OTHER])return 6;
 std::printf("enabled: tasks=%lld total_cpu_us=%lld task_cpu_us=%lld idle_200ms_cpu_us=%lld residual_cpu_us=%lld\n",(long long)c[LC_TASK_CALLS],(long long)c[LC_TOTAL],(long long)c[LC_TASKS],(long long)(idle1.value[LC_TOTAL]-idle0.value[LC_TOTAL]),(long long)(c[LC_TOTAL]-c[LC_SCAN]-c[LC_YIELD]-c[LC_LOCK]-c[LC_TASKS]));
 return 0;
}
