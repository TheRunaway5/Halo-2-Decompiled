#pragma once
// Native TU1.5 small queries: five leaves of their units.
//
//   reference_clear::clear    1E5A50(EAX = reference)          (sandbox special unit)
//     A 0x14-byte object reference made empty: its handles (+0, +4, +0xC,
//     +0x10) NONE, its index (+8) NONE and its count (+0xA) zero.
//   shadow_option::enabled    335B0() -> AL                     (shadow frame unit)
//     With the state at [51DF50] whose byte +5 is set, the option byte at
//     515EF5; otherwise one.
//   seat_player::of           150990(ECX = seat) -> EAX         (combat action, sandbox special units)
//     A local seat's player ([4F54F0] +0xC by seat); NONE for no seat.
//   word_change::changed      25BC70(word a; word b) -> AL      (multiplayer engines unit)
//     Whether two words differ.
//   cluster_match::matches    226B0() -> AL                     (shadow frame unit)
//     Whether the location ([4ECC14]) has clusters (+0x80 positive) and a
//     present cluster (+0x84, whose +0x1C is not NONE) that is the
//     structure's ([4ECC18]): the cluster's +4 against the structure's +8.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapters
// (native_tu15_small_queries.hpp) map the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_small_queries {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
inline constexpr std::uint32_t no_index=0xffffu;

namespace reference_clear {
enum class Access {handle,owner,index,count,parent,next};
namespace reference {inline constexpr std::uint32_t handle=0u,owner=4u,index=8u,count=0xau,parent=0xcu,next=0x10u;}
template<class Memory,class Frame> void clear(Memory &memory,Frame &,std::uint32_t reference_address) {
    using enum Access;
    memory.store32(handle,at(reference_address,reference::handle),none);
    memory.store32(owner,at(reference_address,reference::owner),none);
    memory.store16(index,at(reference_address,reference::index),no_index);
    memory.store16(count,at(reference_address,reference::count),0u);
    memory.store32(parent,at(reference_address,reference::parent),none);
    memory.store32(next,at(reference_address,reference::next),none);
}
} // namespace reference_clear

namespace shadow_option {
inline constexpr std::uint32_t state=0x51df50u,option=0x515ef5u,gate=5u;
enum class Access {state,gate,option};
template<class Memory,class Frame> std::uint32_t enabled(Memory &memory,Frame &) {
    const auto state_value=memory.load32(Access::state,state);
    if(!state_value)return 1u;
    const bool gated=memory.load8(Access::gate,at(state_value,gate))&0xffu;
    const auto option_value=memory.load8(Access::option,option)&0xffu;
    return gated?option_value:1u;
}
} // namespace shadow_option

namespace seat_player {
inline constexpr std::uint32_t seats=0x4f54f0u,seat_players=0xcu;
enum class Access {seats,player};
template<class Memory,class Frame> std::uint32_t of(Memory &memory,Frame &,std::uint32_t seat) {
    if(seat==none)return none;
    return memory.load32(Access::player,memory.load32(Access::seats,seats)+seat_players+4u*seat);
}
} // namespace seat_player

namespace word_change {
enum class Access {first,second};
// Frame: argument(k) the k-th argument's slot.
template<class Memory,class Frame> bool changed(Memory &memory,Frame &frame) {
    const auto first=memory.load16(Access::first,frame.argument(1u));
    return first!=memory.load16(Access::second,frame.argument(2u));
}
} // namespace word_change

namespace cluster_match {
namespace globals {inline constexpr std::uint32_t location=0x4ecc14u,structure=0x4ecc18u;}
namespace place {inline constexpr std::uint32_t clusters=0x80u,cluster=0x84u;}
namespace cluster {inline constexpr std::uint32_t name=4u,present=0x1cu;}
namespace structure_record {inline constexpr std::uint32_t cluster_name=8u;}
enum class Access {location,clusters,structure,cluster,present,name,structure_name};
template<class Memory,class Frame> bool matches(Memory &memory,Frame &) {
    using enum Access;
    const auto place_address=memory.load32(Access::location,globals::location);
    if(!place_address || signed_value(memory.load32(clusters,at(place_address,place::clusters)))<=0)return false;
    const auto structure_address=memory.load32(Access::structure,globals::structure);
    if(!structure_address)return false;
    const auto cluster_address=memory.load32(Access::cluster,at(place_address,place::cluster));
    if(memory.load32(present,at(cluster_address,cluster::present))==none)return false;
    const auto cluster_name=memory.load32(name,at(cluster_address,cluster::name));
    return cluster_name==memory.load32(Access::structure_name,at(structure_address,structure_record::cluster_name));
}
} // namespace cluster_match
} // namespace halo2::native_small_queries
