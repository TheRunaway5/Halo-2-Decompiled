#include "audio/native_tu15_sound_kernel.hpp"
#include <iostream>
#include <stdexcept>
namespace {
using namespace halo2;using namespace halo2::native_tu15_sound;
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
struct UnusedMemory final:M {
    void read(std::uint32_t,std::span<std::byte>)const override{throw std::runtime_error("Contract regression unexpectedly read game state");}
    void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("Contract regression unexpectedly wrote game state");}
};
std::string compare(std::uint32_t native_value,std::uint32_t original_value,std::uint8_t full,std::uint8_t word,std::uint8_t byte) {
    UnusedMemory memory;Abi abi{};abi.gp[4]=0x1000u;
    return native_tu15_sound_compare_with(memory,abi,{},[&](auto&,auto& result){result.gp[0]=native_value;},
        [&](auto&,auto& result,const auto&){result.gp[0]=original_value;},full,word,0u,0x2287b0u,byte);
}
void run() {
    // A policy regression only: these lambdas supply no game dependency or
    // original instruction result to any listener algorithm.
    check(compare(0xaabbcc01u,0x12345601u,0,0,1).empty(),"AL contract incorrectly owes dead upper bytes");
    check(compare(0xaabbcc00u,0x12345601u,0,0,1).find("gp0 low8")!=std::string::npos,"AL mismatch escaped the live shared comparator");
    check(compare(0xaabbcc01u,0x12345601u,0,1,0).find("low16")!=std::string::npos,"Existing AX contract changed");
    check(compare(0xaabbcc01u,0x12345601u,1,0,0).find("gp0 native")!=std::string::npos,"Existing full EAX contract changed");
    unsigned count{};
    for(const auto& kernel:listener_kernels()) {
        ++count;check(!kernel.calls_out,"Qualified listener escaped its closed native dependency graph");
        const auto& contract=kernel.contract;
        check((contract.gp_full&0xf8u)==0xf8u,"Listener omitted a nonvolatile/return-stack obligation");
        if(kernel.entry==0x2287b0u || kernel.entry==0x2531e0u)
            check(contract.gp_low8==1u && !(contract.gp_full&1u) && !(contract.gp_low16&1u),"Actual AL query lost its byte-width return contract");
        else check(contract.gp_low8==0u && (contract.xmm&1u),"Actual numerical listener lost its XMM0 result");
    }
    check(count==5,"Actual qualified listener table changed");
    std::cout<<"Actual five listener contracts and shared live AL comparator passed; low8, low16 and full-width obligations remain distinct\n";
}
}
int main()try{run();return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
