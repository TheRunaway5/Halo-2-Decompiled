#pragma once
#include "halo2/physics_contact_record.hpp"
#include <array>
#include <cstdint>
namespace halo2::native_unit_animation_keys_detail {
// Complete111D10 logical lookup shared by unit actions and actor navigation.
// Each output is published before the next input is read, as in the source.
template<class Mode,class State> std::uint8_t lookup(const SourcePhysicsContactMemory &memory,
    std::uint32_t unit,Mode mode,State state) {
    const auto read=[&](std::uint32_t p,unsigned width=4) {
        std::array<std::byte,4> bytes{};memory.read(p,std::span(bytes).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
    };
    constexpr auto none=0xffffffffu;
    const auto pool=read(0x4ecbd0u),rows=read(pool+0x44u),body=read(rows+(unit&65535u)*12u+8u);
    const auto manager=body+std::uint32_t(std::int32_t(std::int16_t(read(body+0x12au,2))));
    if(read(manager+0x68u)!=none && read(manager)!=none && read(manager+6u,2)!=65535u) {
        mode(0x111d48u,read(manager+0x70u));state(0x111d4du,read(manager+0x7cu));return 1;
    }
    mode(0x111d53u,none);state(0x111d59u,none);return 0;
}
}
