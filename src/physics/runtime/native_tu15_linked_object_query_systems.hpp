#pragma once
// Native TU1.5 linked object query, entered at 10B0F0 (physics live support
// unit): whether an object's linked object's handle passes 15C7F0.
//
//   linked_object_query::accepted   10B0F0(EAX = object handle) -> AL
//
// Only in game mode 2 (the game globals [4F3218] +8), and only for an object
// flagged by bit 0 of its byte +0x12C: the object links another (+0x154),
// which holds a handle (+0x13C); with neither link NONE, 15C7F0(handle)
// decides. Every other case is 0.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_linked_object_query.hpp) maps the role to the original
// instruction and owns the ABI of 15C7F0.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_linked_object_query {
using native_object_update::at;
using native_object_update::none;
namespace globals {
inline constexpr std::uint32_t game=0x4f3218u;             // GameGlobals*
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
}
inline constexpr std::uint32_t game_mode=8u,queried_mode=2u;
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
namespace object {inline constexpr std::uint32_t flags=0x12cu,link=0x154u,handle=0x13cu;}
inline constexpr std::uint32_t queried_flag=1u;

namespace linked_object_query {
enum class Access {game,mode,headers,header_datums,object_address,flags,link,linked_address,handle};
enum class Outcome {other_mode,unflagged,no_link,no_handle,asked};
// Frame: ask(handle) 15C7F0(handle pushed); returns AL.
template<class Memory,class Frame> Outcome query(Memory &memory,Frame &frame,std::uint32_t object_handle,std::uint32_t &result) {
    using enum Access;
    result=0;
    const auto game_address=memory.load32(game,globals::game);
    if(memory.load32(mode,at(game_address,game_mode))!=queried_mode)return Outcome::other_mode;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto address=memory.load32(object_address,datums+header_size*(object_handle&0xffffu)+header_address);
    if(!(memory.load8(flags,at(address,object::flags))&queried_flag))return Outcome::unflagged;
    const auto linked=memory.load32(link,at(address,object::link));
    if(linked==none)return Outcome::no_link;
    const auto linked_object=memory.load32(linked_address,datums+header_size*(linked&0xffffu)+header_address);
    const auto linked_handle=memory.load32(handle,at(linked_object,object::handle));
    if(linked_handle==none)return Outcome::no_handle;
    result=frame.ask(linked_handle)!=0;
    return Outcome::asked;
}
} // namespace linked_object_query
} // namespace halo2::native_linked_object_query
