#include "halo2/source_runtime_memory.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace halo2;
namespace {
using Words=std::array<std::uint32_t,4>;
using Memory=SourcePhysicsContactMemory;
void need(bool value,const char *message){if(!value)throw std::runtime_error(message);}
#if defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
Words original(const Memory &memory,std::uint32_t address) {
    Words result{};
    for(unsigned lane=0;lane<4;++lane)result[lane]=physics_step_detail::r(memory,address+4u*lane);
    return result;
}
#if defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
Words actual(const Memory &memory,std::uint32_t address) {
    const auto vector=physics_step_detail::v(memory,address);
    Words result;std::memcpy(result.data(),&vector,sizeof(result));return result;
}
struct Outcome {Words words{};std::string error;friend bool operator==(const Outcome&,const Outcome&)=default;};
template<class Read> Outcome outcome(Read read,const Memory &memory,std::uint32_t address) {
    try{return {read(memory,address),{}};}catch(const std::exception &e){return {{},e.what()};}
}
unsigned comparisons{};
void compare(const Memory &memory,std::uint32_t address) {
    need(outcome(original,memory,address)==outcome(actual,memory,address),
         "Vector adapter changed complete bits or first fault");++comparisons;
}
void retained_banks() {
    SourceRuntimeMemory bank;using Access=SourceRuntimeMemory::Access;
    std::vector<std::byte> bytes(0x3007);
    for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::byte((i*173u+29u)&255u);
    bank.mount(0x3ff003,bytes,Access::ReadWrite,"unaligned retained vector data");
    for(unsigned i=0;i<bytes.size()-15;++i)compare(bank,0x3ff003+i);
    for(unsigned i=0;i<20;++i)compare(bank,0x3ff003+std::uint32_t(bytes.size())-i);
    // Adjacent owners, a gap, and reads spanning more than two owners.
    for(unsigned i=0;i<12;++i)
        bank.mount(0x1000+4*i,std::span(bytes).subspan(i*4,4),Access::ReadOnly,"individual scalar owner");
    bank.mount(0x1040,std::span(bytes).first(16),Access::ReadOnly,"owner beyond a gap");
    for(unsigned i=0;i<80;++i)compare(bank,0x1000+i);
    bank.reserve(0x8000,0x3000,0x1000,"partially committed vector bank");
    bank.commit(0x8000,std::span(bytes).first(0x1000));
    bank.commit(0xa000,std::span(bytes).first(0x1000));
    for(unsigned i=0;i<40;++i){compare(bank,0x8fe0+i);compare(bank,0x9fe0+i);}
    // Four scalar source addresses wrap independently at the 32-bit boundary.
    bank.mount(0xfffffff0,std::span(bytes).first(16),Access::ReadOnly,"last source words");
    bank.mount(0,std::span(bytes).subspan(16,16),Access::ReadOnly,"first source words");
    for(unsigned i=0;i<32;++i)compare(bank,0xffffffe0+i);
    const auto before=bank.checkpoint();
    for(unsigned i=0;i<32;++i)compare(bank,0x400000+i);
    need(bank.checkpoint()==before,"Vector reads mutated retained source state");
    bank.release(0x3ff003);compare(bank,0x400000);
    bank.mount(0x3ff003,std::vector<std::byte>(bytes.size(),std::byte{0xa5}),Access::ReadWrite,"replacement source owner");
    compare(bank,0x400000);
}
struct Device final:Memory {
    mutable std::vector<std::pair<std::uint32_t,std::size_t>> reads;
    unsigned fail_at=4;
    void read(std::uint32_t address,std::span<std::byte> bytes)const override {
        reads.emplace_back(address,bytes.size());
        if(reads.size()==fail_at)throw std::runtime_error("device read fault");
        for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::byte((address+i+reads.size())&255u);
    }
    void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("unexpected device write");}
};
void device_order() {
    for(unsigned fail:{1u,2u,3u,4u,5u})for(auto address:{0x123u,0xfffffff8u}) {
        Device a,b;a.fail_at=b.fail_at=fail;
        const auto expected=outcome(original,a,address),observed=outcome(actual,b,address);
        need(expected==observed && a.reads==b.reads,"Device-backed vector changed ordered scalar reads/faults");
        for(const auto &[p,n]:b.reads){(void)p;need(n==4,"Device-backed vector widened a scalar read");}
        ++comparisons;
    }
}
double measure(Words(*read)(const Memory&,std::uint32_t),const Memory &memory) {
    const auto begin=std::clock();std::uint64_t checksum{};
    constexpr unsigned iterations=600000;
    for(unsigned i=0;i<iterations;++i) {
        const auto result=read(memory,0x60000000u+((i*17u)&0x3fffu)*4u);
        checksum+=result[i&3];
#if defined(__GNUC__) || defined(__clang__)
        asm volatile("" : "+r"(checksum) :: "memory");
#endif
    }
    need(checksum!=0,"Vector benchmark did not consume actual source bytes");
    return double(std::clock()-begin)*1.e9/(CLOCKS_PER_SEC*double(iterations));
}
void benchmark(bool require_gain) {
    SourceRuntimeMemory bank;std::vector<std::byte> bytes(65536+16);
    for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::byte(i*19u+7u);
    bank.mount(0x60000000,bytes,SourceRuntimeMemory::Access::ReadWrite,"translated vector benchmark owner");
    std::array<double,7> reference{},optimized{};
    for(unsigned i=0;i<reference.size();++i) {
        if(i&1){optimized[i]=measure(actual,bank);reference[i]=measure(original,bank);}
        else{reference[i]=measure(original,bank);optimized[i]=measure(actual,bank);}
        std::cout<<"pair="<<i<<" original_cpu_ns="<<reference[i]<<" actual_cpu_ns="<<optimized[i]<<'\n';
    }
    std::sort(reference.begin(),reference.end());std::sort(optimized.begin(),optimized.end());
    std::cout<<"median_original_cpu_ns="<<reference[3]<<" median_actual_cpu_ns="<<optimized[3]
             <<" component_speedup="<<reference[3]/optimized[3]<<'\n';
    if(require_gain)need(optimized[3]*1.15<reference[3],"Vector path has not achieved the component 1.15x performance gate");
}
}
int main(int argc,char **argv)try {
    retained_banks();device_order();
    std::cout<<"PASS "<<comparisons<<" complete vector bits/faults, owners, commitment, wrap and device read order\n";
    if(argc>1)benchmark(std::string_view(argv[1])=="--require-speedup");
    return 0;
}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
