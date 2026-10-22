#include "halo2/source_renderer_state_tu15.hpp"
namespace halo2 {
void mount_native_tu15_renderer_state_data(SourceRuntimeMemory& memory) {
    // Exact cold BSS, not initialized renderer state. Original16AF0 writes its
    // selectors/scalars; original1C190 constructs every table bit itself.
    memory.mount(0x492368,std::array<std::byte,0x13f4>{},SourceRuntimeMemory::Access::ReadWrite,
        "source16AF0 cold renderer state destination");
    memory.mount(0x493aa8,std::array<std::byte,0x880>{},SourceRuntimeMemory::Access::ReadWrite,
        "source1C190 cold renderer bit-mask table");
    constexpr std::array<std::uint32_t,16> coordinates{
        0x00000000,0x00020002,0x00020000,0x00000002,
        0x00010001,0x00030003,0x00010003,0x00030001,
        0x00010000,0x00030002,0x00000003,0x00020001,
        0x00000001,0x00020003,0x00030000,0x00010002};
    std::array<std::byte,64> bytes{};
    for(unsigned i=0;i<coordinates.size();++i)for(unsigned j=0;j<4;++j)
        bytes[i*4+j]=std::byte(coordinates[i]>>(j*8));
    memory.mount(0x449248,bytes,SourceRuntimeMemory::Access::ReadOnly,
        "source1C190 authored signed word coordinate pairs");
    // 46A0E4 (3.0f) is already owned by the scalar-state operand bank.
}
}
