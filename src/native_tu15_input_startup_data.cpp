#include "halo2/source_input_startup_tu15.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
namespace halo2 {
void mount_native_tu15_input_startup_data(SourceRuntimeMemory& m) {
    constexpr std::array<std::byte,8> constants{std::byte{0},std::byte{0},std::byte{0xf0},std::byte{0x42},std::byte{0},std::byte{0},std::byte{0x70},std::byte{0x42}};
    bool shared_constants{};
    // The normal native world already owns these same two independent
    // immutable words for biped conversion. Validate both before mounting any
    // new storage, preserving their original owner boundaries and provenance.
    for(const auto& region:m.checkpoint().regions) {
        const auto end=std::uint64_t(region.address)+region.bytes.size();
        if(region.address<0x455a98u&&end>0x455a90u)shared_constants=true;
        for(const auto [p,n]:std::array<std::pair<std::uint32_t,std::uint32_t>,3>{{
            {0x4f2970u,0x2c8u},{0x52d4a8u,0x1bcu},{0x44ad30u,16u}}})
            if(region.address<std::uint64_t(p)+n&&end>p)
                throw std::invalid_argument("Input constructor destination conflicts with its cold owner");
    }
    if(shared_constants)for(unsigned i=0;i<2;++i) {
        const auto p=0x455a90u+i*4u;
        if(!m.is_read_only(p,4)||!std::ranges::equal(m.view(p,4),std::span(constants).subspan(i*4,4)))
            throw std::invalid_argument("Input preset scalar conflicts with its immutable shared owner");
    }
    // Constructor destinations, not preconstructed bindings or input samples.
    m.mount(0x4f2970,std::vector<std::byte>(0x2c8),SourceRuntimeMemory::Access::ReadWrite,"source controller feedback, bindings and input constructor destinations");
    m.mount(0x52d4a8,std::vector<std::byte>(0x1bc),SourceRuntimeMemory::Access::ReadWrite,"source input preset and presence constructor destinations");
    constexpr std::array<std::byte,16> maps{std::byte{0},std::byte{1},std::byte{2},std::byte{3},std::byte{4},std::byte{5},std::byte{6},std::byte{7},std::byte{1},std::byte{2},std::byte{4},std::byte{8},std::byte{16},std::byte{32},std::byte{64},std::byte{128}};
    m.mount(0x44ad30,maps,SourceRuntimeMemory::Access::ReadOnly,"authored analog index and digital mask tables");
    if(!shared_constants)m.mount(0x455a90,constants,SourceRuntimeMemory::Access::ReadOnly,"authored input preset floating point constants");
}
}
