#include "halo2/native_parallel.hpp"
#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <xmmintrin.h>

namespace {
int failures=0;
void need(bool condition,const std::string& message) {
    if(!condition){++failures;std::cerr<<"FAIL: "<<message<<'\n';}
}
float divide(float a,float b){volatile float x=a,y=b;volatile float z=x/y;return z;}
std::uint16_t x87_status(){std::uint16_t word;asm volatile("fnstsw %0":"=m"(word));return word;}
long double x87_divide(long double a,long double b){volatile long double x=a,y=b;volatile long double z=x/y;return z;}
}

int main() {
    using halo2::native_parallel_for;
    // HALO2_REFERENCE_SERIAL_NATIVE runs every task inline; all semantic
    // checks below still apply.
    const bool workers=halo2::native_parallel_workers()>0;
    need(workers || std::getenv("HALO2_REFERENCE_SERIAL_NATIVE"),"host provides worker threads");

    // Ordered outputs equal inline execution, and work spreads across threads.
    std::vector<float> parallel(64),serial(64);
    std::vector<std::thread::id> owners(64);
    native_parallel_for(64,[&](std::size_t i){
        float x=float(i)+.5f;for(int k=0;k<20000;++k)x=x*1.0000001f+1e-7f;
        parallel[i]=x;owners[i]=std::this_thread::get_id();
    });
    for(std::size_t i=0;i<64;++i){float x=float(i)+.5f;for(int k=0;k<20000;++k)x=x*1.0000001f+1e-7f;serial[i]=x;}
    need(parallel==serial,"parallel results equal inline results");
    bool off_caller=false;for(const auto& id:owners)off_caller|=id!=std::this_thread::get_id();
    need(off_caller || !workers,"at least one task ran on a worker");

    // Workers inherit the caller's rounding control; inline values agree.
    const auto saved=_mm_getcsr();
    _mm_setcsr((saved&~0x6000u)|0x6000u); // round toward zero, flags kept
    std::vector<float> rz(32);
    native_parallel_for(rz.size(),[&](std::size_t i){rz[i]=divide(1.f,float(i+3));});
    for(std::size_t i=0;i<rz.size();++i)need(rz[i]==divide(1.f,float(i+3)),"worker uses caller rounding mode");
    _mm_setcsr(saved);

    // Sticky SSE flags raised only on workers reach the caller after the join.
    _mm_setcsr(saved&~0x3fu);
    native_parallel_for(16,[&](std::size_t i){
        if(i==11)(void)divide(1.f,0.f);       // ZE
        else (void)divide(1.f,3.f);           // PE
    });
    const auto flags=_mm_getcsr()&0x3fu;
    need((flags&0x04u)!=0,"divide-by-zero flag merged");
    need((flags&0x20u)!=0,"precision flag merged");
    need((flags&~0x24u)==0,"no unrelated flags appear");
    _mm_setcsr(saved);

    // x87 sticky flags merge as well.
    asm volatile("fnclex");
    native_parallel_for(8,[&](std::size_t i){if(i==5)(void)x87_divide(1.L,0.L);});
    need((x87_status()&0x04u)!=0,"x87 divide-by-zero flag merged");
    asm volatile("fnclex");

    // errno ends at the last change in task order, as inline execution would.
    errno=0;
    native_parallel_for(12,[&](std::size_t i){if(i==2)errno=EDOM;if(i==9)errno=ERANGE;});
    need(errno==ERANGE,"errno takes the last change in task order");
    errno=0;

    // The first failing task's exception is rethrown after every task runs.
    std::atomic<int> ran{0};
    try {
        native_parallel_for(20,[&](std::size_t i){
            ++ran;
            if(i==4)throw std::runtime_error("four");
            if(i==13)throw std::runtime_error("thirteen");
        });
        need(false,"exception propagated");
    } catch(const std::runtime_error& error) {
        need(std::string(error.what())=="four","first exception in task order is rethrown");
    }
    need(ran==20,"every task finished before rethrow");

    // Nested calls run inline on the calling task's thread.
    std::vector<int> nested(8,0);
    native_parallel_for(4,[&](std::size_t i){
        const auto self=std::this_thread::get_id();bool same=true;
        native_parallel_for(2,[&](std::size_t j){same&=std::this_thread::get_id()==self;nested[i*2+j]=int(i*2+j);});
        need(same,"nested tasks stay on their caller thread");
    });
    for(int i=0;i<8;++i)need(nested[i]==i,"nested outputs complete");

    if(failures)return 1;
    std::cout<<"native_parallel: all checks passed\n";
    return 0;
}
