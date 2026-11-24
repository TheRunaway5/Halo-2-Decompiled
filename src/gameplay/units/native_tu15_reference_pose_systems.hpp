#pragma once
// Native TU1.5 reference pose, entered at 1CF070 (physics live support
// unit): a vector and a value resolved from an animation reference.
//
//   reference_pose::resolve   1CF070(EAX = reference*, ESI = vector*, EBX = value*; argument) -> RET 4
//
// The vector and the value start at zero. Unless the reference's word +6 is
// NONE, a cleared 12-byte record on the stack is filled by 1E5DF0 (the
// definition of the reference's tag +0 and its +4 pushed, EDI the record)
// and handed to 212D40 (EDX the record, ECX the vector, EAX the value, the
// argument pushed).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_reference_pose.hpp) maps the role to the original
// instruction and owns the ABI of the children; the vector and the value
// the original keeps in ESI and EBX across 1E5DF0 are a Kept, refreshed
// from the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_reference_pose {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t tag_instances=0x4f0414u;}   // 16-byte tag instances (+8 the tag's address)
namespace reference {inline constexpr std::uint32_t tag=0u,second=4u,high=6u;}
namespace local {inline constexpr std::uint32_t value=0u,data=4u,group=8u,kind=9u;}
inline constexpr std::uint32_t zero_float=0u;

namespace reference_pose {
enum class Access {vector_clear,value_clear,high,local_value,local_data,local_group,local_kind,second,tag,tags,definition,argument};
struct Kept {std::uint32_t vector,value;};   // ESI, EBX
// Frame: reserve() the 12 bytes; local(offset) a stack slot of the record;
// record(definition, second) 1E5DF0, refreshes kept; argument_slot();
// pose(kept, argument) 212D40.
template<class Memory,class Frame> bool resolve(Memory &memory,Frame &frame,std::uint32_t reference_address,std::uint32_t vector,
                                                std::uint32_t value_out) {
    using enum Access;
    for(std::uint32_t k=0;k<3u;++k)memory.store32(vector_clear,at(vector,4u*k),zero_float);
    frame.reserve();
    memory.store32(value_clear,value_out,zero_float);
    if((memory.load16(high,at(reference_address,reference::high))&0xffffu)==0xffffu)return false;
    memory.store32(local_value,frame.local(local::value),0u);
    memory.store32(local_data,frame.local(local::data),0u);
    memory.store8(local_group,frame.local(local::group),0u);
    memory.store8(local_kind,frame.local(local::kind),0u);
    const auto second_word=memory.load32(second,at(reference_address,reference::second));
    const auto tag_handle=memory.load32(Access::tag,at(reference_address,reference::tag));
    const auto instances=memory.load32(tags,globals::tag_instances);
    Kept kept{vector,value_out};
    frame.record(kept,memory.load32(Access::definition,16u*(tag_handle&0xffffu)+instances+8u),second_word);
    frame.pose(kept,memory.load32(argument,frame.argument_slot()));
    return true;
}
} // namespace reference_pose
} // namespace halo2::native_reference_pose
