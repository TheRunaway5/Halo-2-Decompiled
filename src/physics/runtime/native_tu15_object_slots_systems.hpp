#pragma once
// Native TU1.5 object slot queries (physics live support, AI frame and AI
// actor units):
//
//   slot_handle::of     CE230(EAX = object handle, CX = slot index) -> EAX
//                       the handle in the object's slot (+0x218, by the
//                       signed index); NONE for a NONE index
//   both_slots::set     CFB40(EAX = object handle) -> AL
//                       whether both slot indices (+0x212, +0x213) are set
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapters
// (native_tu15_slot_handle.hpp, native_tu15_both_slots.hpp) map the role to
// the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_slots {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
namespace object {inline constexpr std::uint32_t slot_indices=0x212u,slots=0x218u;}
inline constexpr std::uint32_t no_index16=0xffffu,no_index8=0xffu;

namespace slot_handle {
enum class Access {headers,header_datums,object_address,slot};
template<class Memory> std::uint32_t of(Memory &memory,std::uint32_t handle,std::uint32_t index) {
    using enum Access;
    if((index&0xffffu)==no_index16)return none;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto address=memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address);
    return memory.load32(slot,address+object::slots+4u*widen16(index&0xffffu));
}
} // namespace slot_handle

namespace both_slots {
enum class Access {headers,header_datums,object_address,first,second};
template<class Memory> bool set(Memory &memory,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto address=memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address);
    if((memory.load8(first,at(address,object::slot_indices))&0xffu)==no_index8)return false;
    return (memory.load8(second,at(address,object::slot_indices+1u))&0xffu)!=no_index8;
}
} // namespace both_slots
} // namespace halo2::native_object_slots
