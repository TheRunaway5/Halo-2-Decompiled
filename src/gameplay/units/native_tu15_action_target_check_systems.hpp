#pragma once
// Native TU1.5 action target check, entered at 1AE500 (combat action unit):
// whether an action's object may act on a target.
//
//   action_target_check::check   1AE500(object, target, context, result*) -> AL
//
// The result byte is cleared first. The object never acts on itself. An
// object that is not of the first two types (its header's type byte +3)
// passes. Otherwise it fails while flagged (bit 2 of its byte +0x10A) or
// while its tag is (bit 21 of +0xBC). An object of kind bit 1 (its byte
// +0xAA) without a link (+0x138 NONE) leaves the decision to 1AE5E0(target,
// context, result*); any other passes, the result byte becoming whether
// 1E7810(object, context) refuses it.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_action_target_check.hpp) maps the role to the original
// instruction and owns the ABI of the children; the result pointer the
// original keeps in EBP across 1E7810 is a Kept, refreshed from the
// registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_action_target_check {
using native_object_update::at;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // 16-byte tag instances (+8 the tag's address)
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_type=3u,header_address=8u,
    tag_instance_shift=4u,instance_address=8u;
inline constexpr std::uint32_t acting_types=3u;            // types 0 and 1
namespace object {inline constexpr std::uint32_t tag=0u,kind=0xaau,flags=0x10au,link=0x138u;}
inline constexpr std::uint32_t excluded_bit=2u,tag_flags=0xbcu,tag_excluded_bit=21u,deferring_kinds=2u;
inline constexpr std::uint16_t no_link=0xffffu;

namespace action_target_check {
enum class Argument {object,target,context,result};
enum class Access {
    target_read,result_read,object_read,result_clear,headers,header_datums,type,object_address,object_tag,
    instances,tag_address,object_flags,link,tag_flags_read,kind,context_forward,target_forward,context_test,
    result_store
};
enum class Outcome {self,other_type,object_excluded,tag_excluded,deferred,tested};
struct Kept {std::uint32_t result;};   // EBP
// Frame:
//   argument(Argument)              the argument's slot
//   saved(register index)           PUSH EBP / EDI / EBX / ESI as the original does
//   compared(object, target)        CMP EDI,EAX
//   link_compared(link)             CMP SI,-1
//   defer(target, context)          1AE5E0(target, context, result*): its AL is the result
//   refuses(kept, object, context)  1E7810(EAX = object, ECX = context); refreshes kept; returns AL
//   negated(al)                     NEG AL; SBB AL,AL; INC AL: AL becomes AL == 0
template<class Memory,class Frame> Outcome check(Memory &memory,Frame &frame) {
    using enum Access;
    const auto target=memory.load32(target_read,frame.argument(Argument::target));
    frame.saved_ebp();
    Kept kept{memory.load32(result_read,frame.argument(Argument::result))};
    frame.saved_edi();
    const auto acting=memory.load32(object_read,frame.argument(Argument::object));
    frame.compared(acting,target);
    memory.store8(result_clear,kept.result,0u);
    if(acting==target)return Outcome::self;
    const auto headers_address=memory.load32(headers,globals::object_headers);
    const auto header=memory.load32(header_datums,at(headers_address,datum_data))+header_size*(acting&0xffffu);
    frame.saved_ebx();
    if(!((1u<<(memory.load8(type,at(header,header_type))&31u))&acting_types))return Outcome::other_type;
    const auto address=memory.load32(object_address,at(header,header_address));
    const auto tag_handle=memory.load32(object_tag,at(address,object::tag));
    const auto tag=memory.load32(tag_address,((tag_handle&0xffffu)<<tag_instance_shift)+
        memory.load32(instances,globals::tag_instances)+instance_address);
    const auto flags=memory.load8(object_flags,at(address,object::flags));
    frame.saved_esi();
    const auto link_index=memory.load16(link,at(address,object::link));
    if((flags>>excluded_bit)&1u)return Outcome::object_excluded;
    if((memory.load32(tag_flags_read,at(tag,tag_flags))>>tag_excluded_bit)&1u)return Outcome::tag_excluded;
    if((1u<<(memory.load8(kind,at(address,object::kind))&31u))&deferring_kinds) {
        frame.link_compared(link_index);
        if(link_index==no_link) {
            const auto context=memory.load32(context_forward,frame.argument(Argument::context));
            frame.defer(memory.load32(target_forward,frame.argument(Argument::target)),context);
            return Outcome::deferred;
        }
    }
    const auto context=memory.load32(context_test,frame.argument(Argument::context));
    const auto refused=frame.refuses(kept,address,context);
    memory.store8(result_store,kept.result,frame.negated(refused));
    return Outcome::tested;
}
} // namespace action_target_check
} // namespace halo2::native_action_target_check
