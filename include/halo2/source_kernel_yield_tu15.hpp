#pragma once
#include "halo2/native_kernel_yield.hpp"
#include <array>
#include <cstddef>
#include <stdexcept>

namespace halo2 {
// The retained2DBF74 wrapper owns status-to-BOOL conversion. This actual
// import boundary publishes only the host NTSTATUS and its no-argument RET.
template<class Memory,class Call,class Abi>
bool source_try_kernel_yield_tu15(const Memory&memory,const Call&call,Abi&abi) {
    if(call.instruction!=0x2dbf74u||call.target!=0x800000eeu||call.tail)return false;
    std::array<std::byte,4> raw{};const auto stack=abi.gp[4];memory.read(stack,raw);
    std::uint32_t target{};
    for(unsigned n=0;n<4;++n)target|=std::to_integer<std::uint32_t>(raw[n])<<(n*8);
    if(target!=0x2dbf7au)throw std::invalid_argument("TU1.5 kernel-yield return frame differs");
    const auto status=native_kernel_yield_status();
    abi.gp[0]=status;abi.gp[4]=stack+4u;return true;
}
} // namespace halo2
