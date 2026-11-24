#pragma once
// Native TU1.5 wake chain, entered at B9390 (scene device constructor, AI
// actor and AI frame units): an object and its parents are woken.
//
//   wake_chain::wake   B9390(EAX = object handle)
//
// Unless the object's header is already awake (flag 2) or the handle is
// NONE, the object and each of its parents (+0x14, until NONE) are marked
// awake in their headers.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_wake_chain.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_wake_chain {
using native_object_update::DataArray;
using native_object_update::ObjectHeader;
using native_object_update::at;
using native_object_update::element;
using native_object_update::handle_index;
using native_object_update::none;
namespace globals {using native_object_update::globals::object_headers;}
namespace header_flags {using native_object_update::header_flags::awake;}
inline constexpr std::uint32_t object_parent=0x14u;

namespace wake_chain {
enum class Access {headers,datums,first_flags,flags,flags_store,datums_again,object,parent};
template<class Memory> void wake(Memory &memory,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto data=memory.load32(datums,at(table,offsetof(DataArray,data)));
    const auto first=element(data,handle_index(handle),sizeof(ObjectHeader));
    if(memory.load8(first_flags,at(first,offsetof(ObjectHeader,flags)))&header_flags::awake)return;
    auto current=handle,current_data=data;
    while(current!=none) {
        const auto header=element(current_data,handle_index(current),sizeof(ObjectHeader));
        memory.store8(flags_store,at(header,offsetof(ObjectHeader,flags)),
                      memory.load8(flags,at(header,offsetof(ObjectHeader,flags)))|header_flags::awake);
        current_data=memory.load32(datums_again,at(table,offsetof(DataArray,data)));
        const auto object_data=memory.load32(object,at(element(current_data,handle_index(current),sizeof(ObjectHeader)),
                                                        offsetof(ObjectHeader,object)));
        current=memory.load32(parent,at(object_data,object_parent));
    }
}
} // namespace wake_chain
} // namespace halo2::native_wake_chain
