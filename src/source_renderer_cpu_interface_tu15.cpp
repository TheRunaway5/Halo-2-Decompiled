#include "halo2/source_renderer_tail_storage_tu15.hpp"
#include <stdexcept>
namespace halo2 {
void source_renderer_cpu_interface_initialize_tu15(SourceRuntimeMemory &memory,
    const SourceRendererCpuOriginalCall &call) {
    const auto word=[&](auto at) {
        const auto bytes=memory.view(at,4);std::uint32_t value{};
        for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(bytes[n])<<(n*8);
        return value;
    };
    if(!call || !word(0x515ec0u) || word(0x515e2cu) || word(0x515ec4u))
        throw std::runtime_error("Renderer CPU interface requires its original cold arena/storage owners");
    // Literal12822..12840 zeroes the query bank, then initializes511 slots.
    const std::array<std::byte,0x40c0> empty{};memory.write(0x52dfd8u,empty);
    for(unsigned n=0;n<511;++n)call(0x20e30u,n);
    // Literal12842..12873 allocates0x34 and publishes its actual arena token.
    // Complete126850 retains the same cursor/CRC operations for this size.
    const auto pointer=call(0x126850u,0x34u);memory.view(pointer,0x34);
    memory.write(0x515e2cu,std::as_bytes(std::span(&pointer,1)));
    call(0x20e90u,{});
}
}
