#pragma once
#include "halo2/physics_contact_record.hpp"
#include <stdexcept>

namespace halo2 {
using SourceBspBreakableVisibilityTu15=std::array<std::byte,0x4204>;
// Actual186CB0 publishes this allocation at4F9B50;186D30 resets it and
//186ED0/1870C0 consume/update it for damaged terrain and instances. Read the
// current complete bank for this view. A constructor-time all-one copy cannot
// represent later breakage, and no source pointer or borrowed span escapes.
inline SourceBspBreakableVisibilityTu15 source_bsp_breakable_visibility_tu15(
    const SourcePhysicsContactMemory &memory) {
    std::array<std::byte,4> pointer;memory.read(0x4f9b50u,pointer);
    std::uint32_t address{};
    for(unsigned byte=0;byte<4;++byte)
        address|=std::to_integer<std::uint32_t>(pointer[byte])<<(byte*8u);
    if(!address || address==0xffffffffu)
        throw std::runtime_error("TU1.5 BSP visibility requires its actual breakable allocation");
    SourceBspBreakableVisibilityTu15 result;memory.read(address,result);return result;
}
}
