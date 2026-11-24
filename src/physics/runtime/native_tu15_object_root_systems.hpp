#pragma once
// Native TU1.5 object root queries: an object's root (its parent chain
// followed by +0x14 until NONE) and what the root holds.
//
//   root_words::copy   BC330(ECX = object handle, EAX = out) (physics live
//                      support and sandbox special units): the root's words
//                      +0x28 and +0x2C copied out
//   root_flag::of      BBD50(EAX = object handle) -> EAX (physics live
//                      support): bit 6 of the root's header flags (+2)
//
// A NONE handle has no chain to follow: it is its own root, as in the
// original (whose header lookup then takes index 0xFFFF).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapters (native_tu15_root_words.hpp,
// native_tu15_root_flag.hpp) map the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_root {
using native_object_update::at;
using native_object_update::none;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_flags=2u,header_address=8u,parent=0x14u;
inline constexpr std::uint32_t root_flag_bit=6u;

// The root of a handle's parent chain. Frame: stepped(handle) before each step.
template<class Memory,class Frame,class Access>
std::uint32_t root_of(Memory &memory,Frame &frame,std::uint32_t handle,std::uint32_t datums,Access object_role,Access parent_role) {
    auto root=none;
    for(auto current=handle;current!=none;) {
        frame.stepped(current);
        root=current;
        current=memory.load32(parent_role,at(memory.load32(object_role,datums+header_size*(current&0xffffu)+header_address),parent));
    }
    return root;
}

namespace root_words {
enum class Access {headers,header_datums,chain_object,chain_parent,datums_again,root_object,first,first_store,second,second_store};
template<class Memory,class Frame> void copy(Memory &memory,Frame &frame,std::uint32_t handle,std::uint32_t out) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    auto root=none;
    if(handle!=none)root=root_of(memory,frame,handle,memory.load32(header_datums,at(table,datum_data)),chain_object,chain_parent);
    frame.walked(root);
    const auto datums=memory.load32(datums_again,at(table,datum_data));
    const auto object=memory.load32(root_object,datums+header_size*(root&0xffffu)+header_address);
    memory.store32(first_store,at(out,0u),memory.load32(first,at(object,0x28u)));
    memory.store32(second_store,at(out,4u),memory.load32(second,at(object,0x2cu)));
}
} // namespace root_words

namespace root_flag {
enum class Access {headers,header_datums,chain_object,chain_parent,datums_again,root_flags};
template<class Memory,class Frame> std::uint32_t of(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    auto root=none;
    if(handle!=none)root=root_of(memory,frame,handle,memory.load32(header_datums,at(table,datum_data)),chain_object,chain_parent);
    frame.walked(root);
    const auto datums=memory.load32(datums_again,at(table,datum_data));
    return (memory.load8(root_flags,datums+header_size*(root&0xffffu)+header_flags)>>root_flag_bit)&1u;
}
} // namespace root_flag
} // namespace halo2::native_object_root
