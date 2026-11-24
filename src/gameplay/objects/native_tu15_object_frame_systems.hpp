#pragma once
// Native TU1.5 object frame, entered at BC190 (scene device constructor
// unit): an object's frame in the world.
//
//   object_frame::of   BC190(EAX = object handle; frame*) -> EAX = frame, RET 4
//
// The object's own position, forward and up (+0x64, +0x70, +0x7C) make the
// frame (144BF0); an object with a parent (+0x14, NONE for none) is then
// composed with the parent's node (145560): the 0x34-byte node its node
// index (signed byte +0x18) picks among the parent's nodes (at the parent
// plus its int16 +0x116).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_object_frame.hpp) maps the role to the original instruction
// and owns the ABI of the children; the object table, the object and the
// frame, which the original keeps in EBP, EDI and EBX across them, are a
// Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_frame {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen8;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t objects=0x4ecbd0u;}   // DataArray* of 12-byte rows (+8 the object)
inline constexpr std::uint32_t datum_data=0x44u,row_size=12u,row_object=8u,node_size=0x34u;
namespace object {inline constexpr std::uint32_t parent=0x14u,parent_node=0x18u,position=0x64u,forward=0x70u,up=0x7cu,nodes=0x116u;}

namespace object_frame {
enum class Access {frame_argument,objects,object_data,object,parent,object_data_again,parent_object,parent_node,nodes};
enum class Outcome {root,child};
struct Kept {std::uint32_t objects,object,frame;};   // EBP, EDI, EBX
// Frame:
//   argument()                    the frame argument's slot
//   own(kept)                     144BF0 from the object's position, forward and up
//   parented(kept, node)          145560 with the parent's node
template<class Memory,class Frame> Outcome of(Memory &memory,Frame &frame,std::uint32_t handle,std::uint32_t &result) {
    using enum Access;
    const auto out=memory.load32(frame_argument,frame.argument());
    const auto objects_array=memory.load32(objects,globals::objects);
    Kept kept{objects_array,0u,out};
    kept.object=memory.load32(object,memory.load32(object_data,at(objects_array,datum_data))+row_size*(handle&0xffffu)+row_object);
    frame.own(kept);
    const auto parent_handle=memory.load32(parent,at(kept.object,object::parent));
    auto outcome=Outcome::root;
    if(parent_handle!=none) {
        const auto parent_address=memory.load32(parent_object,
            memory.load32(object_data_again,at(kept.objects,datum_data))+row_size*(parent_handle&0xffffu)+row_object);
        const auto node=widen8(memory.load8(parent_node,at(kept.object,object::parent_node)));
        const auto first_node=widen16(memory.load16(nodes,at(parent_address,object::nodes)));
        frame.parented(kept,first_node+(node_size*node+parent_address));
        outcome=Outcome::child;
    }
    result=kept.frame;
    return outcome;
}
} // namespace object_frame
} // namespace halo2::native_object_frame
