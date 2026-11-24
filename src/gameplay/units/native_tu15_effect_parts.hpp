#pragma once
// Boundary adapter for the effect event parts, entered at 17BEB0
// (native_tu15_effect_parts_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns:
//   - three stack arguments, RET C; SUB ESP,C makes the frame (the owner
//     mark, the part index and the active byte at [ESP+C], [ESP+10] and
//     [ESP+14] once EBX, EBP and EDI are saved; ESI is saved once the parts
//     are walked); the arguments and locals are addressed from the live
//     ESP, so a child that leaves the stack unbalanced moves them as it does
//     for the original;
//   - 158290 (through the unit's route) gets EAX the owner; 17BD40 gets EAX
//     the part index and the effect handle; 17BB10 gets (effect, element,
//     part, location, active) and XMM0 the scale; every register a child
//     returns comes back;
//   - the flags of the last instruction that sets them (ADD ESP,C at the
//     exit);
//   - the cursor: each access publishes its original instruction.
// HALO2_REFERENCE_EFFECT_PARTS=1 restores the translated body.
// HALO2_VERIFY_EFFECT_PARTS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_effect_parts_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_effect_parts_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_effect_parts;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_EFFECT_PARTS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_EFFECT_PARTS");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    no_owner, owner, marked, unmarked, no_parts, parts, environment_applies, environment_skips, mode_checked,
    unmarked_mode, unmarked_skip, unmarked_applies, marked_mode, marked_skip, marked_applies, any_mode, slot_found,
    next_slot, slot_missing, element_failed, element_made, no_type, typed, next_part, last_part, branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Effect parts verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge parts_edges[]{
    {0x17befbu,0x17bf12u,no_owner},{0x17befbu,0x17bf01u,owner},
    {0x17bf01u,0x17bf0au,marked},{0x17bf01u,0x17bf12u,unmarked},
    {0x17bf15u,0x17c011u,no_parts},{0x17bf15u,0x17bf1fu,parts},
    {0x17bf5eu,0x17bfa0u,environment_applies},{0x17bf5eu,0x17bff9u,environment_skips},{0x17bf5eu,0x17bf6cu,mode_checked},
    {0x17bf6cu,0x17bf76u,unmarked_mode},{0x17bf76u,0x17bff9u,unmarked_skip},{0x17bf76u,0x17bfa0u,unmarked_applies},
    {0x17bf6cu,0x17bf83u,marked_mode},{0x17bf83u,0x17bff9u,marked_skip},{0x17bf83u,0x17bfa0u,marked_applies},
    {0x17bf6cu,0x17bfa0u,any_mode},
    {0x17bfa0u,0x17bfafu,slot_found},{0x17bfa0u,0x17bfa0u,next_slot},{0x17bfa0u,0x17bfc9u,slot_missing},
    {0x17bfd2u,0x17bff9u,element_failed},{0x17bfd2u,0x17bfdbu,element_made},
    {0x17bfdbu,0x17bff9u,no_type},{0x17bfdbu,0x17bfe1u,typed},
    {0x17c006u,0x17bf20u,next_part},{0x17c006u,0x17c010u,last_part},
};
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
template<class State,class Route,class Make,class Create> struct Parts : Port<State> {
    using Access=logic::effect_parts::Access;
    using Local=logic::effect_parts::Local;
    using Kept=logic::effect_parts::Kept;
    Route &route;Make &make_child;Create &create_child;
    // The frame's bytes below the return address: it sits at ESP+depth.
    std::uint32_t depth{};bool saved_esi{};
    Parts(State &s,std::uint32_t *c,Trace<State> *t,Route &r,Make &m,Create &k)
        :Port<State>(s,c,t,parts_edges),route(r),make_child(m),create_child(k){}
    std::uint32_t local(Local l)const {
        const auto entry=this->gp[esp]+depth;
        switch(l) {
        case Local::handle:return entry+4u;
        case Local::scale:return entry+8u;
        case Local::location:return entry+0xcu;
        case Local::owner_mark:return entry-0xcu;
        case Local::index:return entry-8u;
        case Local::active:return entry-4u;
        }
        return 0;
    }
    void save(std::uint32_t pc,std::uint32_t value){this->push(pc,value);depth+=4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case effects:gp[esp]=this->flags.minus(gp[esp],0xcu,4);depth=0xcu;return 0x17beb3u;       // SUB ESP,C
        case effects_data:return 0x17bebau;
        case tag_instances:return 0x17bebdu;
        case handle_argument:return 0x17bec4u;
        case effect_tag:return 0x17bed4u;
        case effect_event:return 0x17bedau;
        case tag_address:return 0x17beeau;
        case events:return 0x17beeeu;
        case owner:return 0x17bef1u;
        case owner_mark_init:return 0x17befbu;
        case owner_mark_set:return 0x17bf0au;
        case part_count:return 0x17bf12u;
        case index_init:return 0x17bf15u;
        case parts:if(!saved_esi){saved_esi=true;save(0x17bf1fu,gp[esi]);}return 0x17bf20u;
        case effect_flags:return 0x17bf23u;
        case part_environment:return 0x17bf26u;
        case part_active:return 0x17bf2du;
        case active_store:return 0x17bf3bu;
        // CMP AX,1 (the effect's environment flag) or CMP AX,2.
        case players:this->flags.minus(gp[eax]&0xffffu,gp[edx]&1u?1u:2u,2);return 0x17bf4bu;
        case player_count:return 0x17bf5eu;
        case part_mode:return 0x17bf6cu;
        case owner_mark_unmarked:return 0x17bf76u;
        case owner_mark_marked:return 0x17bf83u;
        case slot_part:return 0x17bfa0u;
        case slot_handle:return 0x17bfafu;
        case elements:return 0x17bfb2u;
        case element_data:return 0x17bfb8u;
        case handle_again:this->test32(gp[eax]);return 0x17bfc9u;                                  // TEST EAX,EAX
        case index_for_element:return 0x17bfcdu;
        case part_type:return 0x17bfdbu;
        case active_argument:return 0x17bfe1u;
        case location_argument:return 0x17bfe5u;
        case scale_argument:return 0x17bfe9u;
        case index_read:return 0x17bff9u;
        case part_count_again:return 0x17bffdu;
        case index_store:return 0x17c006u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case effects:gp[eax]=v;save(0x17beb8u,gp[ebx]);save(0x17beb9u,gp[ebp]);break;            // PUSH EBX; PUSH EBP
        case effects_data:gp[ebp]=v;break;
        case tag_instances:gp[edx]=v;save(0x17bec3u,gp[edi]);break;                               // PUSH EDI
        // AND EDI,FFFF; IMUL EDI,EDI,190.
        case handle_argument:gp[edi]=this->flags.product(this->flags.logic(v&0xffffu,4),logic::effect_size);break;
        case effect_tag:gp[ecx]=v;gp[edi]=this->flags.plus(gp[edi],gp[ebp],4);break;               // ADD EDI,EBP
        // MOVSX EBP; IMUL EBP,EBP,38; AND ECX,FFFF; SHL ECX,4.
        case effect_event:
            gp[ebp]=this->flags.product(logic::widen16(v),logic::event_size);
            gp[ecx]=this->flags.shift(this->flags.logic(gp[ecx]&0xffffu,4),logic::tag_instance_shift,4);break;
        case tag_address:case index_for_element:case slot_handle:case index_read:gp[eax]=v;break;
        case events:case owner_mark_unmarked:case location_argument:case handle_again:gp[edx]=v;
            if(a==owner_mark_unmarked)this->test32(v);
            break;
        // XOR EBX,EBX; ADD EBP,EDX; CMP EAX,-1.
        case owner:
            gp[eax]=v;gp[ebx]=0;gp[ebp]=this->flags.plus(gp[ebp],gp[edx],4);this->compare32(v,logic::none);break;
        case part_count:this->compare32(v,gp[ebx]);break;
        case parts:gp[esi]=v;break;
        case effect_flags:set_low_byte(gp[edx],v);break;
        case part_environment:set_low_word(gp[eax],v);gp[esi]=this->flags.plus(gp[esi],gp[ebx],4);break;  // ADD ESI,EBX
        // CMP word,0; SETNE CL; SHR DL,5; TEST DL,1.
        case part_active:
            this->flags.minus(v&0xffffu,0u,2);set_low_byte(gp[ecx],(v&0xffffu)!=0?1u:0u);
            set_low_byte(gp[edx],this->flags.right(gp[edx]&0xffu,logic::effect::environment_flag,1,false));
            this->test8(gp[edx]&1u);break;
        // SETNE AL; TEST AL,AL; MOV ECX,1; SETNE AL.
        case players: {
            const auto differs=(gp[eax]&0xffffu)!=(gp[edx]&1u?1u:2u);
            gp[edx]=v;set_low_byte(gp[eax],differs?1u:0u);this->test8(gp[eax]);gp[ecx]=1u;break;
        }
        // CMP word,CX; TEST AL,AL.
        case player_count:this->flags.minus(v&0xffffu,gp[ecx]&0xffffu,2);this->test8(gp[eax]);break;
        case part_mode:set_low_word(gp[eax],v);this->flags.minus(v&0xffffu,2u,2);break;            // CMP AX,2
        case owner_mark_marked:this->compare32(v,gp[ecx]);break;
        // CMP [ECX],ESI; (INC EDX; ADD ECX,8; CMP EDX,10).
        case slot_part:
            this->compare32(v,gp[esi]);
            if(v!=gp[esi]) {
                gp[edx]=this->flags.increment(gp[edx],4,false);gp[ecx]=this->flags.plus(gp[ecx],logic::slot_size,4);
                this->compare32(gp[edx],logic::slot_count);
            }
            break;
        case elements:gp[ecx]=v;break;
        // AND EAX,FFFF; IMUL EAX,EAX,54; ADD EAX,EDX.
        case element_data:
            gp[edx]=v;gp[eax]=this->flags.plus(this->flags.product(this->flags.logic(gp[eax]&0xffffu,4),logic::element_size),v,4);
            break;
        case part_type:this->compare32(v,logic::none);break;
        case active_argument:gp[ecx]=v;break;
        case scale_argument:this->s.xmm[0]=lane0_only(v);break;
        // INC EAX; ADD EBX,38; CMP EAX,ECX.
        case part_count_again:
            gp[ecx]=v;gp[eax]=this->flags.increment(gp[eax],4,false);gp[ebx]=this->flags.plus(gp[ebx],logic::part_size,4);
            this->compare32(gp[eax],gp[ecx]);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void returned(Kept &kept)const{kept={this->gp[edi],this->gp[ebp],this->gp[ebx],this->gp[esi]};}
    std::uint32_t classify_owner(std::uint32_t,Kept &kept) {
        this->call_routed(route,0x17bf01u,0x17bf06u,0x158290u);
        returned(kept);this->test32(this->gp[eax]);
        return this->gp[eax];
    }
    // XOR EAX,EAX; XOR EDX,EDX; LEA ECX,[EDI+110].
    void search_begins() {
        auto &gp=this->gp;
        gp[eax]=this->flags.logic(0u,4);gp[edx]=this->flags.logic(0u,4);gp[ecx]=gp[edi]+logic::effect::slots;
    }
    std::uint32_t make_element(Kept &kept) {
        save(0x17bfd1u,this->gp[edx]);
        this->call(0x17bfd2u,0x17bfd7u,0x17bd40u,make_child);depth-=4u;
        returned(kept);this->test32(this->gp[eax]);
        return this->gp[eax];
    }
    void create_part(std::uint32_t,Kept &kept) {
        auto &gp=this->gp;
        save(0x17bfefu,gp[ecx]);save(0x17bff0u,gp[edx]);save(0x17bff1u,gp[esi]);save(0x17bff2u,gp[eax]);
        save(0x17bff3u,gp[edi]);
        this->call(0x17bff4u,0x17bff9u,0x17bb10u,create_child);depth-=0x14u;
        returned(kept);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Parts> memory{*this};
        if(logic::effect_parts::create(memory,*this)==logic::effect_parts::Exit::walked)gp[esi]=this->pop(0x17c010u);
        gp[edi]=this->pop(0x17c011u);gp[ebp]=this->pop(0x17c012u);gp[ebx]=this->pop(0x17c013u);
        gp[esp]=this->flags.plus(gp[esp],0xcu,4);
        this->ret(0x17c017u,0xcu);this->publish(0x17c017u);
    }
};
template<class State,class Reference,class Route,class Make,class Create>
bool bridge(State &s,Reference reference,Route route,Make make,Create create) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x17beb0u,0x17c017u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Parts<State,Route,Make,Create>(v,c,t,route,make,create);});
    return true;
}
} // namespace halo2::native_effect_parts_detail
