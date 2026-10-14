#include "halo2/native_tu15_graphics_device.hpp"
namespace halo2 {
namespace {
template<std::size_t N> auto encode(const std::array<std::uint32_t,N>& words) {
    std::array<std::byte,N*4> bytes{};
    for(unsigned i=0;i<N;++i)for(unsigned j=0;j<4;++j)bytes[4*i+j]=std::byte(words[i]>>(8*j));
    return bytes;
}
}
void mount_native_tu15_graphics_device_data(SourceRuntimeMemory& memory) {
    memory.mount(0x48c7f8,encode(std::array<std::uint32_t,53>{0x1u,0x0u,0x20000u,0x0u,0x0u,0x80000007u,0x0u,0x7bbef0u,0xcf2u,0x377101u,0xffu,0x1fffu,0x1fffu,0xffu,0x84208u,0x7ec87u,0x1f030700u,0x1f030700u,0x3030300u,0x17u,0x17u,0x1fu,0x1000u,0x1000u,0x200u,0x2000u,0x0u,0x4u,0x501502f9u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u}),SourceRuntimeMemory::Access::ReadOnly,"source128A0 authored graphics-control defaults");
    memory.mount(0x491508,std::array<std::byte,212>{},SourceRuntimeMemory::Access::ReadWrite,"source128A0 graphics-control constructor destination");
    memory.mount(0x473568,encode(std::array<std::uint32_t,2>{0xffffffffu,0xffffffffu}),SourceRuntimeMemory::Access::ReadWrite,"source authored graphics-control selection defaults");
    memory.mount(0x515e9c,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,"source128A0 cold device publication byte");
    memory.mount(0x47361c,std::array<std::byte,1>{std::byte(1)},SourceRuntimeMemory::Access::ReadWrite,"source12560 authored graphics flag");
}
}
