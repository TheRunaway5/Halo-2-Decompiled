#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <array>
#include <vector>

namespace halo2 {
// TU1.5 45CD0 owns only these caller-packet stores. Preserve both the literal
// byte widths and their order, including the instance plane's second writes.
struct NativeTu15PartPacketStore {
    std::uint32_t instruction{},offset{},value{};
    unsigned bytes{4};
};
struct NativeTu15PartPacket45cd0 {
    bool accepted{};
    std::array<std::byte,44> bytes{};
    std::vector<NativeTu15PartPacketStore> stores;
};
// Whole 45CD0 plus its read-only palette, material-visibility and transparent
// plane hash callees. Reads the actual populated table; never rebuilds it from
// cache metadata or substitutes a plane lookup result. No World writes occur.
NativeTu15PartPacket45cd0 native_tu15_command_part_prepare_45cd0(
    const SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t part,
    std::uint32_t matrix,const std::array<std::byte,44>&,NativeTu15PhysicsFrameFp&);
}
