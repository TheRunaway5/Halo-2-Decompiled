#include "halo2/native_tu15_renderer_palette.hpp"
#include "halo2/texture.hpp"
namespace halo2 {
void mount_native_tu15_renderer_palette_data(SourceRuntimeMemory& memory) {
    memory.mount(0x474da8,tu15_bump_palette(),SourceRuntimeMemory::Access::ReadOnly,
        "source12560 authored fixed palette, identical in MCC and TU1.5");
    memory.mount(0x4915dc,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "source12560 cold palette resource output");
    memory.mount(0x4915f0,std::array<std::byte,384>{},SourceRuntimeMemory::Access::ReadWrite,
        "source12560 cold post-palette renderer cache clear");
}
}
