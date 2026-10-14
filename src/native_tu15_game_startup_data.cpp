#include "halo2/source_game_storage_startup_tu15.hpp"
#include <array>

namespace halo2 {
void mount_native_tu15_game_startup_data(SourceRuntimeMemory& memory) {
    // Authenticated TU1.5 44AED0 + row*24, consumed by 13A766. These are
    // opaque original function identities; no host pointer or executable
    // section is copied. Source dispatch must supply each actual owner.
    constexpr std::array<std::uint32_t,68> callbacks{
        0x6de60u,0x31d400u,0x31d400u,0x148d40u,0x6e830u,0x31d400u,0x31d400u,0x31d400u,
        0x31d400u,0x21c0a0u,0x21e5d0u,0x1589d0u,0x31d400u,0x1a5410u,0x134b00u,0x22c100u,
        0x126640u,0x149050u,0x1e7710u,0x14dfe0u,0x1888a0u,0x1eecf0u,0x159f80u,0x69220u,
        0x31d400u,0x1f3830u,0x31d400u,0xb87f0u,0x10e3b0u,0x10fa30u,0x1f4700u,0x12560u,
        0x2b480u,0x31d400u,0x186cb0u,0x157e10u,0x171e10u,0x1800b0u,0x31d400u,0x17e640u,
        0x1789f0u,0x31d400u,0x31d400u,0x228260u,0x127ee0u,0x2131b0u,0x18b5c0u,0x22c440u,
        0x2293e0u,0x1563a0u,0x14a1ffu,0x1410a7u,0x31d400u,0x13e9e0u,0x2129d0u,0x2035b0u,
        0x31d400u,0xd7320u,0x1cfa40u,0x1cb130u,0x31d400u,0x31d400u,0x31d400u,0x219080u,
        0x137870u,0x31d400u,0x22c980u,0x31d400u};
    for(unsigned row=0;row<callbacks.size();++row) {
        std::array<std::byte,4> bytes{};
        for(unsigned i=0;i<4;++i)bytes[i]=std::byte(callbacks[row]>>(8*i));
        memory.mount(0x44aed0u+row*0x24u,bytes,SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 subsystem initializer identity "+std::to_string(row));
    }
    memory.mount(0x4f3218u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold game-options binding");
}
}
