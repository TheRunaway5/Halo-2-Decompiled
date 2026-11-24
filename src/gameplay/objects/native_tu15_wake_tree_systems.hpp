#pragma once
// Native TU1.5 wake tree, entered at BE7E0 and BDA50 (scene device
// constructor unit): an object's tree is woken.
//
//   wake_tree::wake_children   BE7E0(object handle)
//   wake_tree::wake            BDA50(EAX = object handle)
//
// BE7E0: when the object's type (header +3) is one of the types in mask
// 0x1003 (bits 0, 1 and 12), its parent chain is woken (B9390); then each
// child (+0x10, siblings through +0xC, until NONE) has its own tree woken
// (BE7E0 again).
// BDA50: a handle whose header has flag 0x80 is first resolved (BCFB0);
// an active object (header flag 1) has its parent chain woken (B9390) and
// header flag 4 set; then its tree is woken (BE7E0).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_wake_tree.hpp) maps the role to the original instruction and
// owns the ABI of the children. The values the original keeps in
// callee-saved registers across them are a Kept, refreshed from the
// registers each child returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_wake_tree {
using native_object_update::DataArray;
using native_object_update::ObjectHeader;
using native_object_update::at;
using native_object_update::element;
using native_object_update::handle_index;
using native_object_update::none;
namespace globals {using native_object_update::globals::object_headers;}
inline constexpr std::uint32_t woken_types=0x1003u;
inline constexpr std::uint32_t first_child=0x10u,next_sibling=0xcu;
namespace header_flags {
inline constexpr std::uint32_t active=0x01u,moved=0x04u,indirect=0x80u;
}

namespace wake_children {
enum class Local {handle};
enum class Access {handle_argument,headers,datums,object,type,first_child,child_headers,child_datums,child_object,sibling};
// The header the original keeps in ESI (the object, then each child).
struct Kept {std::uint32_t header;std::uint32_t object;};
// Frame:
//   local(Local)            the argument's address now
//   wake_chain(kept)        B9390(EAX the handle); refreshes kept
//   wake_child(kept)        BE7E0(child) with ESI the child's header;
//                           refreshes kept
template<class Memory,class Frame> void wake(Memory &memory,Frame &frame) {
    using enum Access;
    const auto handle=memory.load32(handle_argument,frame.local(Local::handle));
    const auto table=memory.load32(headers,globals::object_headers);
    const auto header=element(memory.load32(datums,at(table,offsetof(DataArray,data))),handle_index(handle),sizeof(ObjectHeader));
    Kept kept{header,memory.load32(object,at(header,offsetof(ObjectHeader,object)))};
    const auto type_value=memory.load8(type,at(header,offsetof(ObjectHeader,type)));
    if((1u<<(type_value&31u))&woken_types)frame.wake_chain(kept);
    auto child=memory.load32(first_child,at(kept.object,native_wake_tree::first_child));
    while(child!=none) {
        const auto child_table=memory.load32(child_headers,globals::object_headers);
        kept.header=element(memory.load32(child_datums,at(child_table,offsetof(DataArray,data))),handle_index(child),
                            sizeof(ObjectHeader));
        frame.wake_child(kept);
        const auto child_data=memory.load32(child_object,at(kept.header,offsetof(ObjectHeader,object)));
        child=memory.load32(sibling,at(child_data,next_sibling));
    }
}
} // namespace wake_children

namespace wake {
enum class Access {headers,datums,flags,active_flags,moved_read,moved_store};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t datums;   // EBX: the header datums
    std::uint32_t handle;   // ESI
    std::uint32_t header;   // EDI
};
// Frame:
//   resolve(kept)       BCFB0(ECX the handle); returns EAX, refreshes kept
//   wake_chain(kept)    B9390(EAX the handle); refreshes kept
//   wake_children(kept) BE7E0(handle); refreshes kept
template<class Memory,class Frame> void wake(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    Kept kept{0u,handle,0u};
    const auto table=memory.load32(headers,globals::object_headers);
    kept.datums=memory.load32(datums,at(table,offsetof(DataArray,data)));
    kept.header=element(kept.datums,handle_index(kept.handle),sizeof(ObjectHeader));
    if(memory.load8(flags,at(kept.header,offsetof(ObjectHeader,flags)))&header_flags::indirect) {
        kept.handle=frame.resolve(kept);
        kept.header=element(kept.datums,handle_index(kept.handle),sizeof(ObjectHeader));
    }
    if(memory.load8(active_flags,at(kept.header,offsetof(ObjectHeader,flags)))&header_flags::active) {
        frame.wake_chain(kept);
        memory.store8(moved_store,at(kept.header,offsetof(ObjectHeader,flags)),
                      memory.load8(moved_read,at(kept.header,offsetof(ObjectHeader,flags)))|header_flags::moved);
    }
    frame.wake_children(kept);
}
} // namespace wake
} // namespace halo2::native_wake_tree
