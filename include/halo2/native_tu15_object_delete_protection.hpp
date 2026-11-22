#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
// B1870: campaign protection depends on the object's retained player link.
inline bool native_tu15_object_delete_protected(const SourcePhysicsContactMemory& memory,
    std::uint32_t object) {
    const auto word=[&](std::uint32_t address) {
        std::array<std::byte,4> bytes{};
        memory.read(address,bytes);
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8u);
        return value;
    };
    std::byte mode{};
    memory.read(word(0x4f3218u)+0xcu,std::span(&mode,1));
    if(mode!=std::byte{4})return false;
    const auto headers=word(word(0x4ecbd0u)+0x44u);
    const auto body=word(headers+(object&0xffffu)*12u+8u);
    return word(body+0xd4u)!=0xffffffffu;
}
}
