#include "halo2/source_runtime_host_physical_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <pthread.h>
#include <stdexcept>

namespace {
thread_local bool count_mutexes{};
thread_local std::size_t mutexes{};
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
template<class F>void rejected(F f){bool failed=false;try{f();}catch(const std::runtime_error&){failed=true;}
    check(failed,"Protected source write was accepted");}
}
extern "C" int __real_pthread_mutex_lock(pthread_mutex_t *);
extern "C" int __wrap_pthread_mutex_lock(pthread_mutex_t *mutex) {
    if(count_mutexes)++mutexes;
    return __real_pthread_mutex_lock(mutex);
}
int main(int argc,char **)try {
    if(argc>1)::setenv("HALO2_REFERENCE_ALL_PHYSICAL_WRITE_GUARDS","1",1);
    using namespace halo2;
    SourceRuntimeMemory memory;SourceRuntimeHostPhysicalMemory physical(memory);
    const std::array<std::byte,4096> zero{};
    memory.mount(0x1000,zero,SourceRuntimeMemory::Access::ReadWrite,"translated ordinary stores");
    memory.mount(0x7ffff000u,zero,SourceRuntimeMemory::Access::ReadWrite,"last ordinary page");
    memory.mount(0x3000,zero,SourceRuntimeMemory::Access::ReadOnly,"ordinary read-only resource");
    const auto base=physical.allocate({0x3000,0,0x2fff,4096,4});
    check(base==0x80000000u,"Physical adapter lost its upper-half token namespace");
    const std::array value{std::byte{0xa7}};
    std::size_t observed{};int owner;
    source_runtime_register_write_observer(memory,&owner,[&](auto,auto){++observed;});
    const auto begin=std::chrono::steady_clock::now();
    count_mutexes=true;
    constexpr unsigned stores=2000000;
    for(unsigned i=0;i<stores;++i)memory.write(0x1000u+(i&255u),value);
    count_mutexes=false;
    const auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
    check(observed==stores,"Ordinary store auditing was skipped");
    memory.write(0x7fffffffu,value); // End exactly at the physical boundary.
    physical.protect(base,4096,2);
    const auto old=memory.view(0x7fffffffu,1)[0];const auto before=observed;
    const std::array crossing{std::byte{0x12},std::byte{0x34}};
    rejected([&]{memory.write(0x7fffffffu,crossing);});
    check(memory.view(0x7fffffffu,1)[0]==old,"Rejected cross-boundary write changed its ordinary prefix");
    rejected([&]{memory.write(base,value);});
    rejected([&]{memory.write(memory.native_address(base,1),value);});
    rejected([&]{(void)memory.mutable_view(base,1);});
    rejected([&]{memory.validate_mutable_view(base,1);});
    rejected([&]{memory.write(0x3000,value);});
    check(observed==before,"Rejected protected write reached observers");
    memory.write(base+4096,value);
    check(memory.view(base+4096,1)[0]==value[0],"Writable physical page changed");
    physical.protect(base,4096,4);memory.write(base,value);
    source_runtime_unregister_write_observer(memory,&owner);
    std::cout<<"stores="<<stores<<" elapsed_ms="<<elapsed<<" ordinary_physical_mutex_locks="<<mutexes<<'\n';
    check(!mutexes,"Ordinary translated writes still acquire the physical-page protection mutex");
    std::cout<<"PASS physical guards, native aliases, boundary rejection, unchanged ordinary permissions and observers\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
