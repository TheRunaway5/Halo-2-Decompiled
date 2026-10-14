#include "halo2/source_renderer_pool_tu15.hpp"
namespace halo2 {
void mount_native_tu15_renderer_pool_data(SourceRuntimeMemory& memory) {
    // Source13FC50 passes this literal allocator identity to its embedded
    // datum-array constructor; it is not a native address or initialized pool.
    memory.mount(0x474cbc,std::array<std::byte,4>{std::byte{0x38},std::byte{0x60},std::byte{0x48},std::byte{}},
        SourceRuntimeMemory::Access::ReadOnly,"source13FC50 authored embedded-array allocator identity");
    memory.mount(0x52df98,std::array<std::byte,0x38>{},SourceRuntimeMemory::Access::ReadWrite,
        "source1C7F0 cold renderer cache counters and selectors");
    memory.mount(0x494328,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "source1C7F0 cold pool constructor publication");
    constexpr std::array<char,32> name{"vertex shader lruv cache"};
    memory.mount(0x45c468,std::as_bytes(std::span(name)),SourceRuntimeMemory::Access::ReadOnly,
        "source1C7F0 authored pool name and alignment");
}
}
