#pragma once
// Native TU1.5 scaled reference pose, entered at 1CEFB0 (physics live support
// unit): a reference's pose scaled by its factor.
//
//   scaled_pose::compute   1CEFB0(ECX = reference*, EDI = vector*; value*) -> RET 4
//
// The vector starts as the zero vector ([474D04] points at it) and the
// value at zero.
// Unless the reference's word +6 is NONE, its +0x1C (truncated to an
// integer) goes to 1CF070 with a 12-byte vector and a value on the stack,
// and those, times the reference's +0x18 scaled by [46A20C], become the
// vector and the value. The original keeps both floats in its argument
// slot (over the value pointer, already read) and reads them back from
// there; the single-precision arithmetic is the original's scalar SSE.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_scaled_pose.hpp) maps the role to the original instruction
// and owns the ABI of 1CF070; the value pointer and the vector the original
// keeps in EBP and EDI across it are a Kept, refreshed from the registers
// it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include <cstdint>

namespace halo2::native_scaled_pose {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t zero_vector=0x474d04u;   // a pointer to the zero hkVector4
inline constexpr std::uint32_t factor_scale=0x46a20cu;  // float
}
namespace reference {inline constexpr std::uint32_t high=6u,factor=0x18u,rate=0x1cu;}
inline constexpr std::uint32_t zero_float=0u;
// The pose's slots on the stack, as offsets from the reserved 16 bytes.
namespace local {inline constexpr std::uint32_t value=0u,vector=4u;}

namespace scaled_pose {
enum class Access {base,value_pointer,copy,copy_store,value_clear,high,rate,rate_store,rate_reload,factor,scale,factor_store,
    factor_reload,pose_vector,vector_store,pose_value,value_store};
struct Kept {std::uint32_t value_out,vector;};   // EBP, EDI
// Frame: argument_slot() the value pointer's slot (then the floats');
// local(offset) a slot of the reserved 16 bytes; component(k) before each
// component; saved() PUSH EBX; truncated(count) the integer pushed;
// pose(kept, reference, count) 1CF070, refreshes kept; scaled(k, product) each
// product as it goes to XMM1.
template<class Memory,class Frame> bool compute(Memory &memory,Frame &frame,std::uint32_t reference_address,std::uint32_t out) {
    using enum Access;
    const auto source=memory.load32(base,globals::zero_vector);
    Kept kept{memory.load32(value_pointer,frame.argument_slot()),out};
    for(std::uint32_t k=0;k<3u;++k) {
        frame.component(k);
        memory.store32(copy_store,at(kept.vector,4u*k),memory.load32(copy,at(source,4u*k)));
    }
    memory.store32(value_clear,kept.value_out,zero_float);
    if((memory.load16(high,at(reference_address,reference::high))&0xffffu)==0xffffu)return false;
    const auto rate_value=memory.load32(rate,at(reference_address,reference::rate));
    frame.saved();
    memory.store32(rate_store,frame.argument_slot(),rate_value);
    const auto count=std::uint32_t(_mm_cvttss_si32(sse::scalar(memory.load32(rate_reload,frame.argument_slot()))));
    const auto unscaled=memory.load32(factor,at(reference_address,reference::factor));
    const auto factor_value=sse::multiply(unscaled,memory.load32(scale,globals::factor_scale));
    frame.truncated(count);
    memory.store32(factor_store,frame.argument_slot(),factor_value);
    frame.pose(kept,reference_address,count);
    const auto applied=memory.load32(factor_reload,frame.argument_slot());
    for(std::uint32_t k=0;k<3u;++k) {
        frame.component(k);
        const auto product=sse::multiply(memory.load32(pose_vector,frame.local(local::vector+4u*k)),applied);
        frame.scaled(product,applied);
        memory.store32(vector_store,at(kept.vector,4u*k),product);
    }
    const auto product=sse::multiply(memory.load32(pose_value,frame.local(local::value)),applied);
    frame.scaled(product,applied);
    memory.store32(value_store,kept.value_out,product);
    return true;
}
} // namespace scaled_pose
} // namespace halo2::native_scaled_pose
