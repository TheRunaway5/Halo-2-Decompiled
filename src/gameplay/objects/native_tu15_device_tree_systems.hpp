#pragma once
// Native TU1.5 device tree, entered at 10C5D0 (scene device constructor
// unit): an object's devices, its own and its descendants', marked.
//
//   device_tree::mark   10C5D0(object handle) -> RET 4
//
// A device (object type byte +0xAA one of 7..9: machine, control, light
// fixture) gets flag bit 2 (+0x12C) and B9390 (EAX the handle). Each child
// (the first +0x10, then each next +0xC, NONE ending them) is marked the
// same way by 10C5D0 itself.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_device_tree.hpp) maps the role to the original instruction
// and owns the ABI of the children; the object and then each child's
// handle, which the original keeps in ESI across them, are refreshed from
// the register each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_device_tree {
using native_object_update::at;
using native_object_update::none;
namespace globals {inline constexpr std::uint32_t objects=0x4ecbd0u;}   // DataArray* of 12-byte rows (+8 the object)
inline constexpr std::uint32_t datum_data=0x44u,row_size=12u,row_object=8u;
inline constexpr std::uint32_t device_types=0x380u,marked=4u;   // types 7..9; flag bit 2
namespace object {inline constexpr std::uint32_t next=0xcu,first_child=0x10u,type=0xaau,flags=0x12cu;}

namespace device_tree {
enum class Access {handle,objects,object_data,object,type,flags,flags_store,first_child,objects_again,object_data_again,child_object,next};
enum class Outcome {other,device};
// Frame:
//   argument()                    the handle's slot
//   marked(object, handle) -> ESI  B9390, after the flag
//   descend(child) -> ESI          10C5D0(child)
template<class Memory,class Frame> std::uint32_t object_of(Memory &memory,Access objects_role,Access data_role,Access object_role,std::uint32_t handle) {
    const auto data=memory.load32(data_role,at(memory.load32(objects_role,globals::objects),datum_data));
    return memory.load32(object_role,data+row_size*(handle&0xffffu)+row_object);
}
template<class Memory,class Frame> Outcome mark(Memory &memory,Frame &frame) {
    using enum Access;
    const auto handle_value=memory.load32(handle,frame.argument());
    auto held=object_of<Memory,Frame>(memory,objects,object_data,object,handle_value);
    auto outcome=Outcome::other;
    if((1u<<(memory.load8(type,at(held,object::type))&31u))&device_types) {
        memory.store32(flags_store,at(held,object::flags),memory.load32(flags,at(held,object::flags))|marked);
        held=frame.marked(held,handle_value);
        outcome=Outcome::device;
    }
    for(auto child=memory.load32(first_child,at(held,object::first_child));child!=none;) {
        held=frame.descend(child);
        child=memory.load32(next,at(object_of<Memory,Frame>(memory,objects_again,object_data_again,child_object,held),object::next));
    }
    return outcome;
}
} // namespace device_tree
} // namespace halo2::native_device_tree
