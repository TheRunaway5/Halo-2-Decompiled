#pragma once
// Native TU1.5 pose blend, entered at 1D5970 (physics live support unit): a
// pose vector and value from three scaled references, picked or blended.
//
//   pose_blend::evaluate   1D5970(EAX = object, EBX = references*, ESI = vector*; value*) -> RET 4
//
// The references are three 0x20-byte scaled references (1CEFB0) at +0, +0x20
// and +0x40, with a weight (+0x80). An object (through the object headers at
// [4ECBD0]) with its byte +0xB3 set has the zero vector ([474D04] points at
// it) and the value 0. Otherwise, with overrides on (byte [47C1B5]) a usable
// third reference (+0 not NONE, word +6 not NONE) is the pose; with blending
// on (byte [47C1B4]) a usable second reference is the pose at a weight of
// [46A188] or more, and between [46A0EC] and [46A188] (exclusive) the pose is
// the second and first references' poses blended by 1D61B0's factor (in
// XMM0): first + (second - first) * factor, held in the argument's slot over
// the value pointer. Otherwise a usable first reference is the pose, or the
// zero vector and 0. The single-precision arithmetic is the original's
// scalar SSE, in its order, on the live MXCSR.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (the two poses
// blended and the factor); the boundary adapter (native_tu15_pose_blend.hpp)
// maps the role to the original instruction and owns the ABI of the children.
// The values the original keeps in callee-saved registers across them are a
// Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_pose_blend {
using native_object_update::at;
using native_object_update::none;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,overrides=0x47c1b5u,blending=0x47c1b4u,full=0x46a188u,
    least=0x46a0ecu,zero_vector=0x474d04u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,vector_words=3u,zero_float=0u;
namespace object {inline constexpr std::uint32_t frozen=0xb3u;}
namespace references {inline constexpr std::uint32_t first=0u,second=0x20u,third=0x40u,weight=0x80u;}
namespace reference {inline constexpr std::uint32_t handle=0u,high=6u;}
// The frame's slots, by offset from the return address: the two poses blended (value, vector) and the factor, in
// the argument's slot.
namespace local {inline constexpr std::int32_t second_value=-0x20,first_value=-0x1c,second_vector=-0x18,first_vector=-0xc,factor=4;}

namespace pose_blend {
enum class Access {value_pointer,headers,datums,object_address,frozen_byte,overrides,third_handle,third_high,blending,second_handle,
    full,second_high,second_weight,second_again,second_high_again,least,low_weight,high_weight,factor_store,second_read,factor_load,
    first_read,vector_store,value_store,first_handle,first_high,zero_pointer,zero_read,zero_write};
enum class Outcome {frozen,third,second,blended,first,zero};
// The original's EBX (the references), EBP (the value pointer) and ESI (the vector) across the children.
struct Kept {std::uint32_t references,value,vector;};
// Frame:
//   none_tested(object), saving(n), argument()      OR EDX,-1; CMP; PUSH EBP (then EDI); the argument's slot now
//   indexing(object), frozen_tested(byte), flag_tested(flag), handle_tested(handle), high_tested(high)
//   weight_compared(weight, bound)                  COMISS (JAE: at or above the bound)
//   pose(reference, kept)                           PUSH EBP; MOV EDI,ESI; 1CEFB0(ECX = reference), refreshes kept
//   factor(kept) -> (factor, ECX)                   1D61B0(EAX = references): XMM0, and the ECX it returned
//   pushing(n, value slot, vector slot)             LEA; PUSH the value slot; LEA EDI the vector slot
//   blend_pose(n, reference, kept)                  1CEFB0 into the frame, refreshes kept
//   lerped(n, result), zero_copied(word, value), clearing(frozen)   the blend's and the zero copy's registers; POP EDI
template<class Memory,class Frame> void evaluate(Memory &memory,Frame &frame,std::uint32_t object_handle,Kept kept,Outcome &outcome) {
    using enum Access;
    frame.none_tested(object_handle);
    frame.saving(0u);
    kept.value=memory.load32(value_pointer,frame.argument());
    // The zero vector and 0.
    const auto zero=[&](bool frozen) {
        const auto source=memory.load32(zero_pointer,globals::zero_vector);
        for(std::uint32_t word=0;word<vector_words;++word) {
            const auto value=memory.load32(zero_read,source+4u*word);
            frame.zero_copied(word,value);
            memory.store32(zero_write,kept.vector+4u*word,value);
        }
        frame.clearing(frozen);
        memory.store32(value_store,kept.value,zero_float);
    };
    if(object_handle!=none) {
        const auto datums_address=memory.load32(datums,at(memory.load32(headers,globals::object_headers),datum_data));
        frame.indexing(object_handle);
        const auto object_address=memory.load32(Access::object_address,datums_address+header_size*(object_handle&0xffffu)+header_address);
        const auto frozen=memory.load8(frozen_byte,at(object_address,object::frozen));
        frame.frozen_tested(frozen);
        if(frozen){zero(true);outcome=Outcome::frozen;return;}
    }
    const auto overrides_on=memory.load8(Access::overrides,globals::overrides);
    frame.flag_tested(overrides_on);
    frame.saving(1u);
    if(overrides_on) {
        const auto third=kept.references+references::third;
        const auto handle=memory.load32(third_handle,at(third,reference::handle));
        frame.handle_tested(handle);
        if(handle!=none) {
            const auto high=memory.load16(third_high,at(third,reference::high));
            frame.high_tested(high);
            if(high!=0xffffu){frame.pose(third,kept);outcome=Outcome::third;return;}
        }
    }
    const auto blending_on=memory.load8(Access::blending,globals::blending);
    frame.flag_tested(blending_on);
    if(blending_on) {
        const auto second=kept.references+references::second;
        const auto handle=memory.load32(second_handle,at(second,reference::handle));
        frame.handle_tested(handle);
        const auto full_weight=memory.load32(Access::full,globals::full);
        if(handle!=none) {
            const auto high=memory.load16(second_high,at(second,reference::high));
            frame.high_tested(high);
            if(high!=0xffffu) {
                const auto weight=memory.load32(second_weight,at(kept.references,references::weight));
                frame.weight_compared(weight,full_weight);
                if(!sse::below(weight,full_weight)){frame.pose(second,kept);outcome=Outcome::second;return;}
            }
            const auto again=memory.load32(second_again,at(second,reference::handle));
            frame.handle_tested(again);
            const auto high_again=again!=none?memory.load16(second_high_again,at(second,reference::high)):0xffffu;
            if(again!=none)frame.high_tested(high_again);
            if(high_again!=0xffffu) {
                const auto least_weight=memory.load32(Access::least,globals::least);
                const auto low=memory.load32(low_weight,at(kept.references,references::weight));
                frame.weight_compared(least_weight,low);
                const bool above_least=sse::below(least_weight,low);
                const auto high_value=above_least?memory.load32(high_weight,at(kept.references,references::weight)):0u;
                if(above_least)frame.weight_compared(high_value,full_weight);
                if(above_least && sse::below(high_value,full_weight)) {
                    // Blended: the second and first references' poses into the frame, by the factor.
                    // 1D61B0 keeps ECX (the second reference); the original passes on the ECX it returned.
                    const auto [factor,blended_reference]=frame.factor(kept);
                    frame.pushing(0u,local::second_value,local::second_vector);
                    memory.store32(factor_store,frame.local(local::factor),factor);
                    frame.blend_pose(0u,blended_reference,kept);
                    frame.pushing(1u,local::first_value,local::first_vector);
                    frame.blend_pose(1u,kept.references+references::first,kept);
                    // first + (second - first) * factor, word by word, then the value.
                    const auto lerp=[&](std::uint32_t second_part,std::uint32_t first_part,std::uint32_t f) {
                        return sse::add(sse::multiply(sse::subtract(second_part,first_part),f),first_part);
                    };
                    auto from=memory.load32(first_read,frame.local(local::first_vector));
                    const auto factor_value=memory.load32(factor_load,frame.local(local::factor));
                    auto result=lerp(memory.load32(second_read,frame.local(local::second_vector)),from,factor_value);
                    frame.lerped(0u,result);
                    for(std::uint32_t word=1;word<=vector_words;++word) {
                        const bool last=word==vector_words;
                        from=memory.load32(first_read,last?frame.local(local::first_value):frame.local(local::first_vector)+4u*word);
                        memory.store32(vector_store,kept.vector+4u*(word-1u),result);
                        result=lerp(memory.load32(second_read,last?frame.local(local::second_value):frame.local(local::second_vector)+4u*word),
                            from,factor_value);
                        frame.lerped(word,result);
                    }
                    frame.clearing(false);
                    memory.store32(value_store,kept.value,result);
                    outcome=Outcome::blended;
                    return;
                }
            }
        }
    }
    const auto first=kept.references+references::first;
    const auto handle=memory.load32(first_handle,at(first,reference::handle));
    frame.handle_tested(handle);
    if(handle!=none) {
        const auto high=memory.load16(first_high,at(first,reference::high));
        frame.high_tested(high);
        if(high!=0xffffu){frame.pose(first,kept);outcome=Outcome::first;return;}
    }
    zero(false);
    outcome=Outcome::zero;
}
} // namespace pose_blend
} // namespace halo2::native_pose_blend
