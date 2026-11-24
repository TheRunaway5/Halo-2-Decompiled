#pragma once
// Native TU1.5 object function scale, entered at C0700 (shadow frame unit):
// the scale one of an object's tag functions gives it.
//
//   function_scale::scale   C0700(EDX = object handle, ECX = index) -> XMM0
//
// The scale starts at one (46A0D0). The object's tag (+0, through the tag
// instances at 4F0414) lists 24-byte functions (+0x98); when the indexed one
// names a function (+0x10), BCB70 evaluates it for the object into the
// scale. When the tag's flag bit 9 (word +2) is set, the object's own scale
// (+0xA0) multiplies the result.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_function_scale.hpp) maps the role to the original
// instruction and owns the ABI of the child BCB70. The tag and the object
// the original keeps in ESI and EDI across it are a Kept, refreshed from the
// registers it returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_function_scale {
using native_object_update::at;
using native_object_update::object_of;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t tag_instances=0x4f0414u;
inline constexpr std::uint32_t one=0x46a0d0u;   // float
}
inline constexpr std::uint32_t tag_address=8u,instance_shift=4u,tag_functions=0x98u,function_size=24u,function_index=0x10u,
    tag_flags=2u,scaled_bit=9u,object_scale=0xa0u;

namespace function_scale {
enum class Local {scale};
enum class Access {one,object_table,object_datums,object_address,object_tag,instances,tag,functions,scale_init,
    function,scale_read,flags,object_scale};
struct Kept {std::uint32_t tag;std::uint32_t object;};   // ESI, EDI
// Frame:
//   local(Local)              the scale slot's address now
//   evaluate(function, kept)  BCB70(object handle, function, scale*);
//                             refreshes kept
//   scaled(value)             the result in XMM0
template<class Memory,class Frame> std::uint32_t scale(Memory &memory,Frame &frame,std::uint32_t handle,std::uint32_t index) {
    using enum Access;
    const auto one_value=memory.load32(one,globals::one);
    Kept kept{};
    kept.object=object_of(memory,handle,object_table,object_datums,object_address);
    const auto tag_handle=memory.load32(object_tag,kept.object);
    const auto instances_address=memory.load32(instances,globals::tag_instances);
    kept.tag=memory.load32(tag,at(((tag_handle&0xffffu)<<instance_shift)+instances_address,tag_address));
    const auto functions_address=memory.load32(functions,at(kept.tag,tag_functions));
    memory.store32(scale_init,frame.local(Local::scale),one_value);
    auto value=one_value;
    const auto function_value=memory.load32(function,at(functions_address+index*function_size,function_index));
    if(function_value!=0) {
        frame.evaluate(function_value,kept);
        value=memory.load32(scale_read,frame.local(Local::scale));
    }
    if((memory.load16(flags,at(kept.tag,tag_flags))>>scaled_bit)&1u)
        value=sse::multiply(memory.load32(object_scale,at(kept.object,native_function_scale::object_scale)),value);
    frame.scaled(value);
    return value;
}
} // namespace function_scale
} // namespace halo2::native_function_scale
