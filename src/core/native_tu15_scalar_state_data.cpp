#include "halo2/native_tu15_scalar_state_data.hpp"
#include <algorithm>
namespace halo2 {
namespace {
std::array<std::byte,4> encode(std::uint32_t value) {
    std::array<std::byte,4> bytes{};
    for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));
    return bytes;
}
}
void mount_native_tu15_scalar_state_data(SourceRuntimeMemory& memory) {
    memory.mount(0x450268,encode(0x40857af5),SourceRuntimeMemory::Access::ReadOnly,"source1F3830 first scalar operand");
    memory.mount(0x450270,encode(0x3a902de0),SourceRuntimeMemory::Access::ReadOnly,"source1F3830 third scalar operand");
    memory.mount(0x474d04,encode(0x44abec),SourceRuntimeMemory::Access::ReadOnly,"source1F3830 vector identity");
    memory.mount(0x44abec,std::array<std::byte,12>{},SourceRuntimeMemory::Access::ReadOnly,"source1F3830 authored three-element vector");
    // Loader-zero destinations for original new-map row42 and row24.
    // Complete219300 and11E550/219F90 produce the actual values later.
    memory.mount(0x505e84u,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
        "source219300 cold contrail scalar destinations");
    memory.mount(0x505e90u,std::vector<std::byte>(0xd0c),SourceRuntimeMemory::Access::ReadWrite,
        "source11E550 cold complete contrail noise destination");
    memory.mount(0x46a168u,encode(0x3d8f5c29u),SourceRuntimeMemory::Access::ReadOnly,
        "source21A2A0 authored contrail interpolation operand");
    memory.mount(0x46a170u,encode(0x4b000000u),SourceRuntimeMemory::Access::ReadOnly,
        "source21A2A0 authored contrail quantization operand");
    memory.mount(0x46a2fcu,encode(0x47e7aaacu),SourceRuntimeMemory::Access::ReadOnly,
        "source219300 authored contrail scalar conversion operand");
    memory.mount(0x46a468u,encode(0xbe99999au),SourceRuntimeMemory::Access::ReadOnly,
        "source18F7A0 authored lower speech-clock delta bound");
    memory.mount(0x46a5d0u,encode(0x380a697bu),SourceRuntimeMemory::Access::ReadOnly,
        "source110350 authored facial phase fade increment");
    memory.mount(0x52be68,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"source1F3830 scalar-state binding");
    const auto one=encode(0x3f800000);bool present=false;
    try {
        const auto old=memory.view(0x46a0d0,4);present=true;
        if(!memory.is_read_only(0x46a0d0,4) || !std::ranges::equal(old,one))
            throw std::invalid_argument("Source1F3830 shared scalar conflicts with its immutable owner");
    }catch(const std::out_of_range&){}
    if(!present)memory.mount(0x46a0d0,one,SourceRuntimeMemory::Access::ReadOnly,"source shared authored scalar one");
}
}
