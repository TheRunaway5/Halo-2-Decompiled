#pragma once
// Native TU1.5 deferred action target check, entered at 1AE5E0 (combat
// action unit): the decision the action target check (1AE500) leaves to an
// object's riders and its parent.
//
//   action_target_defer::check   1AE5E0(EDI = object; target, context, result*) -> AL, RET C
//
// The result byte is cleared first. An object whose tag carries bit 21 of
// its flags (+0x1EC) passes. An object riding (bit 26 of its flags +4) on a
// parent (+0x14) of the first two types (its header's type byte +3) leaves
// the decision to 1AE500 for the parent (the parent, the target, the
// context, the result). Otherwise 1AE6F0 (ECX the object; the context, the
// result) passes it, or else passes the first of its children (+0x10, then
// each one's sibling +0xC) that rides and is of the first two kinds (its
// byte +0xAA); without one it fails.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the arguments it rereads; the boundary
// adapter (native_tu15_action_target_defer.hpp) maps the role to the
// original instruction and owns the ABI of the children (1AE500, 1AE6F0).
// The values the original keeps in callee-saved registers across 1AE6F0 are
// a Kept, refreshed from the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_action_target_defer {
using native_object_update::at;
inline constexpr std::uint32_t none=0xffffffffu;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // 16-byte tag instances (+8 the tag's address)
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_type=3u,header_address=8u,tag_size=16u,instance_address=8u;
namespace object {inline constexpr std::uint32_t tag=0u,flags=4u,next_sibling=0xcu,first_child=0x10u,parent=0x14u,kind=0xaau;}
inline constexpr std::uint32_t riding_bit=26u,tag_flags=0x1ecu,tag_passing_bit=21u,acting_types=3u;
namespace argument {inline constexpr std::uint32_t target=0u,context=1u,result=2u;}

namespace action_target_defer {
enum class Access {headers,header_datums,context_argument,result_argument,object_address,object_tag,instances,tag_address,result_clear,
    tag_flags_read,parent,object_flags,parent_datums,parent_type,context_forward,target_forward,context_reload,first_child,
    headers_again,child_datums,child_address,child_flags,child_kind,sibling};
enum class Outcome {passing_tag,parent_decides,passed,child_passed,failed};
// The original's ESI, EBP and EBX across 1AE6F0: the object (or child) it
// asked about, the result and the context.
struct Kept {std::uint32_t address,result,context;};
// Frame:
//   argument(k)                                the argument's slot now
//   parent_decides(parent, target, context, result) -> AL   1AE500
//   rider_passes(object, kept) -> AL           1AE6F0(ECX the object; the context, the result), refreshes kept
template<class Memory,class Frame> Outcome check(Memory &memory,Frame &frame,std::uint32_t object_handle) {
    using enum Access;
    const auto headers_address=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(headers_address,datum_data));
    Kept kept{};
    kept.context=memory.load32(context_argument,frame.argument(argument::context));
    kept.result=memory.load32(result_argument,frame.argument(argument::result));
    kept.address=memory.load32(object_address,datums+header_size*(object_handle&0xffffu)+header_address);
    const auto tag_handle=memory.load32(object_tag,at(kept.address,object::tag));
    const auto tag=memory.load32(tag_address,tag_size*(tag_handle&0xffffu)+memory.load32(instances,globals::tag_instances)+instance_address);
    memory.store8(result_clear,kept.result,0u);
    if((memory.load32(tag_flags_read,at(tag,tag_flags))>>tag_passing_bit)&1u)return Outcome::passing_tag;
    const auto parent_handle=memory.load32(parent,at(kept.address,object::parent));
    if(parent_handle!=none && ((memory.load32(object_flags,at(kept.address,object::flags))>>riding_bit)&1u)) {
        const auto parent_datums=memory.load32(Access::parent_datums,at(headers_address,datum_data));
        if((1u<<(memory.load8(parent_type,parent_datums+header_size*(parent_handle&0xffffu)+header_type)&31u))&acting_types) {
            const auto context=memory.load32(context_forward,frame.argument(argument::context));
            const auto target=memory.load32(target_forward,frame.argument(argument::target));
            frame.parent_decides(parent_handle,target,context,kept.result);
            return Outcome::parent_decides;
        }
        kept.context=memory.load32(context_reload,frame.argument(argument::context));
    }
    if(frame.rider_passes(object_handle,kept)&0xffu)return Outcome::passed;
    auto child=memory.load32(first_child,at(kept.address,object::first_child));
    while(child!=none) {
        const auto child_headers=memory.load32(child_datums,at(memory.load32(headers_again,globals::object_headers),datum_data));
        kept.address=memory.load32(child_address,child_headers+header_size*(child&0xffffu)+header_address);
        if(((memory.load32(child_flags,at(kept.address,object::flags))>>riding_bit)&1u) &&
           ((1u<<(memory.load8(child_kind,at(kept.address,object::kind))&31u))&acting_types) &&
           (frame.rider_passes(child,kept)&0xffu))
            return Outcome::child_passed;
        child=memory.load32(sibling,at(kept.address,object::next_sibling));
    }
    return Outcome::failed;
}
} // namespace action_target_defer
} // namespace halo2::native_action_target_defer
