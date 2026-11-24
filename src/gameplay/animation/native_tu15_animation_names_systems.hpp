#pragma once
// Native TU1.5 animation names, entered at 1D45A0 (physics live support, AI
// actor and AI frame units): four animation names, their gaps filled.
//
//   animation_names::resolve   1D45A0(EAX, ECX, EDX = names, ESI = out*; record*, fourth name) -> RET 8
//
// The four names are written out as given. A name given as unset (7000101)
// takes the record's (+0x70, +0x74, +0x78, +0x7C). Then a first name of NONE
// or none (7000001) becomes 6000086; a second or third that is none or NONE
// becomes 7000083 or 30000D9. 1E5810 looks the third up among the record's
// definition's names (+0x54 of its definition, through the tag instances at
// 4F0414, with the record's tag +0x68; kind 8): a found name (+4) that is
// neither NONE nor 30000D9 replaces the second unless the second is
// 400054B. Last a fourth of NONE (from the record) or none becomes 400000C.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter (native_tu15_animation_names.hpp)
// maps the role to the original instruction and owns the ABI of 1E5810. The
// values the original keeps in callee-saved registers across it are a Kept,
// refreshed from the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_animation_names {
using native_object_update::at;
using native_object_update::none;
namespace globals {inline constexpr std::uint32_t tag_instances=0x4f0414u;}
inline constexpr std::uint32_t tag_size=16u,tag_data=8u,definition_names=0x54u,lookup_kind=8u;
// String ids.
namespace names {inline constexpr std::uint32_t unset=0x7000101u,empty=0x7000001u,first_default=0x6000086u,second_default=0x7000083u,
    third_default=0x30000d9u,fourth_default=0x400000cu,fixed_second=0x400054bu;}
namespace record {inline constexpr std::uint32_t tag=0x68u,first=0x70u,second=0x74u,third=0x78u,fourth=0x7cu;}
namespace found {inline constexpr std::uint32_t name=4u;}
namespace argument {inline constexpr std::uint32_t record=0u,fourth=1u;}

namespace animation_names {
enum class Access {record_argument,fourth_argument,given,record_first,first_store,out_first,first_default,record_second,second_record,
    out_second,second_default,record_third,third_record,out_third,third_default,record_tag,third_now,instances,second_now,definition,
    found_name,second_store,record_fourth,fourth_store,fourth_none,out_fourth,fourth_default};
// The original's EBX (the record), EBP (the fourth name), ESI (out) and EDI (the second name) across 1E5810.
struct Kept {std::uint32_t record,fourth,out,second;};
// Frame:
//   argument(k), saving(n)                       the arguments' slots now; PUSH EBX, EBP, then EDI
//   compared(a, b)                               a CMP's flags
//   pushing(third, tag)                          PUSH 8; AND EAX,FFFF; PUSH EDX (the third); SHL EAX,4
//   lookup(definition, offset, kept) -> EAX      ADD EDX,54; PUSH EDX; CALL 1E5810 (RET C), refreshes kept; TEST EAX,EAX
//   second_replaced(kept), settled(kept), restoring(n)   MOV EDI,EAX; MOV ECX,400000C; POP EDI; POP EBP, EBX
template<class Memory,class Frame> void resolve(Memory &memory,Frame &frame,std::uint32_t first_in,std::uint32_t second_in,
                                                std::uint32_t third_in,Kept kept) {
    using enum Access;
    frame.compared(first_in,names::unset);
    frame.saving(0u);
    kept.record=memory.load32(record_argument,frame.argument(argument::record));
    frame.saving(1u);
    kept.fourth=memory.load32(fourth_argument,frame.argument(argument::fourth));
    memory.store32(given,kept.out,first_in);
    memory.store32(given,kept.out+4u,second_in);
    memory.store32(given,kept.out+8u,third_in);
    memory.store32(given,kept.out+0xcu,kept.fourth);
    // The first name: the record's when unset; none or NONE takes its default.
    bool replace=false;
    if(first_in==names::unset) {
        const auto value=memory.load32(record_first,at(kept.record,record::first));
        frame.compared(value,none);
        memory.store32(first_store,kept.out,value);
        replace=value==none;
    }
    if(!replace) {
        const auto value=memory.load32(out_first,kept.out);
        frame.compared(value,names::empty);
        replace=value==names::empty;
    }
    if(replace)memory.store32(first_default,kept.out,names::first_default);
    // The second and third names alike, with their defaults.
    const auto fill=[&](std::uint32_t in,std::uint32_t offset,std::uint32_t slot,Access from,Access to,Access now,Access fallback,
                        std::uint32_t name) {
        frame.compared(in,names::unset);
        if(in==names::unset)memory.store32(to,kept.out+slot,memory.load32(from,at(kept.record,offset)));
        const auto value=memory.load32(now,kept.out+slot);
        frame.compared(value,names::empty);
        if(value!=names::empty)frame.compared(value,none);
        if(value==names::empty || value==none)memory.store32(fallback,kept.out+slot,name);
    };
    fill(second_in,record::second,4u,record_second,second_record,out_second,second_default,names::second_default);
    fill(third_in,record::third,8u,record_third,third_record,out_third,third_default,names::third_default);
    // The third looked up among the definition's names: a found name replaces the second.
    const auto tag=memory.load32(record_tag,at(kept.record,record::tag));
    const auto third=memory.load32(third_now,kept.out+8u);
    const auto instances_address=memory.load32(instances,globals::tag_instances);
    frame.saving(2u);
    kept.second=memory.load32(second_now,kept.out+4u);
    frame.pushing(third,tag);
    const auto definition_value=memory.load32(definition,tag_size*(tag&0xffffu)+instances_address+tag_data);
    const auto entry=frame.lookup(definition_value,definition_names,kept);
    if(entry) {
        const auto name=memory.load32(found_name,at(entry,found::name));
        frame.compared(name,none);
        if(name!=none) {
            frame.compared(name,names::third_default);
            if(name!=names::third_default) {
                frame.compared(kept.second,names::fixed_second);
                if(kept.second!=names::fixed_second){kept.second=name;frame.second_replaced(kept);}
            }
        }
    }
    // The fourth: the record's when unset; NONE or none takes its default.
    frame.compared(kept.fourth,names::unset);
    memory.store32(second_store,kept.out+4u,kept.second);
    frame.settled(kept);
    if(kept.fourth==names::unset) {
        const auto value=memory.load32(record_fourth,at(kept.record,record::fourth));
        frame.compared(value,none);
        memory.store32(fourth_store,kept.out+0xcu,value);
        if(value==none)memory.store32(fourth_none,kept.out+0xcu,names::fourth_default);
    }
    const auto fourth=memory.load32(out_fourth,kept.out+0xcu);
    frame.compared(fourth,names::empty);
    frame.restoring(1u);
    if(fourth==names::empty)memory.store32(fourth_default,kept.out+0xcu,names::fourth_default);
}
} // namespace animation_names
} // namespace halo2::native_animation_names
