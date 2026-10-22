#include "halo2/native_tu15_observer_data.hpp"
#include <array>
#include <utility>

namespace halo2 {
namespace {
template<std::size_t N>
auto encode(const std::array<std::uint32_t,N>& words) {
    std::array<std::byte,N*4> bytes{};
    for(std::size_t i=0;i<N;++i)for(unsigned j=0;j<4;++j)
        bytes[i*4+j]=std::byte(words[i]>>(j*8));
    return bytes;
}
}
void mount_native_tu15_observer_data(SourceRuntimeMemory& memory) {
    // Authenticated .rdata operands from complete observer validation,
    // interpolation, world-placement and collision owners. Keep their exact
    // disjoint extents; adjacent source fields have independent owners.
    for(const auto [address,bits]:std::array<std::pair<std::uint32_t,std::uint32_t>,7>{{
        {0x44ea9cu,0xc7435000u},{0x44eaacu,0x45610000u},
        {0x45f854u,0x3e322d0eu},{0x46a124u,0x41400000u},
        {0x46a128u,0x41000000u},{0x46a2b8u,0x40e00000u},
        {0x46a514u,0x3faaaaabu}}})
        memory.mount(address,encode(std::array{bits}),SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 observer authored scalar operand");
    // Actual1730BD walks six binary32 caps through the exclusive44EAD0 end.
    memory.mount(0x44eab8u,encode(std::array{
        0x44bb8000u,0x44bb8000u,0x44bb8000u,
        0x47c35000u,0x47c35000u,0x47c35000u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 observer six acceleration caps");
    memory.mount(0x46aa20u,encode(std::array{0xe0000000u,0xc7efffffu}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 observer authored binary64 comparison operand");

    // These operands are initialized writable .data in the original loader,
    // unlike the immutable constants above. Actual1720E0 multiplies its
    // incoming delta by475288 and publishes the result to4F64A0 itself.
    memory.mount(0x475288u,encode(std::array{0x3f800000u}),
        SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 observer initialized frame-delta multiplier");
    constexpr std::array<std::uint8_t,24> widths{
        3,0,3,0,2,0,1,0,1,0,6,0,
        3,0,3,0,2,0,1,0,1,0,3,0};
    memory.mount(0x475290u,std::as_bytes(std::span(widths)),
        SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 observer initialized packet and motion widths");

    // Complete loader-zeroed static owners: one delta, four358-byte
    // observers and three independent policy bytes. Original171F40/171EF0
    // invoke172310 for reset;171E10 only allocates51DF70's separate state.
    memory.mount(0x4f64a0u,std::array<std::byte,4>{},
        SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 observer cold completed frame delta");
    memory.mount(0x4f64a4u,std::array<std::byte,4*0x358>{},
        SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 observer cold complete four-view bank");
    memory.mount(0x4f7204u,std::array<std::byte,3>{},
        SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 observer cold reset and policy flags");
}
}
