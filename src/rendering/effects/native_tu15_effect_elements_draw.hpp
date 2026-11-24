#pragma once
// Boundary adapter for the effect elements draw, 1770C0
// (native_tu15_effect_elements_draw_systems.hpp). Its callers and the
// children keep the original ABI, which the adapter owns: the first person
// byte on the stack (RET 4); 0x14 bytes of frame with EBP, EDI, EBX and ESI
// saved below them, read from the ESP each child left; 16EA60 (ECX the array,
// EDX the index), 178070 (ECX the element), 3EB10 and 158290 (EAX the unit)
// through the unit's dispatch, 17DE60 (EAX the effect, the unit pushed, RET
// 4) and 1772C0 (four pushed, RET 10) direct, every register each returns
// coming back. Each access publishes its original instruction.
// HALO2_REFERENCE_EFFECT_ELEMENTS_DRAW=1 restores the translated body.
// HALO2_VERIFY_EFFECT_ELEMENTS_DRAW=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/effects/native_tu15_effect_elements_draw_systems.hpp"

namespace halo2::native_effect_elements_draw_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_effect_elements_draw;
using Access=logic::effect_elements_draw::Access;
using Kept=logic::effect_elements_draw::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_EFFECT_ELEMENTS_DRAW");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_EFFECT_ELEMENTS_DRAW");return v;}
enum Branch : unsigned {elements, considered, owned, held, drawn_element, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Effect elements draw"};return v;}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x1770c5u,0x1770ddu,0x1770e0u,0x1770e6u,0x1770f1u,0x177100u,0x177104u,0x17710du,0x17711du,0x17712au,
    0x177149u,0x17715fu,0x177168u,0x177171u,0x17717du,0x177183u,0x177188u,0x177199u,0x17719du,0x1771c2u,0x1771cbu,0x1771e1u,0x1771efu,
    0x1771f3u,0x1771fcu,0x177208u,0x177214u,0x17721cu,0x177220u,0x177228u,0x17722cu,0x177233u,0x17723eu,0x177242u,0x17724du,0x177251u,
    0x177255u,0x177263u,0x177272u,0x177277u,0x17728au,0x17728du,0x177293u,0x17729eu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::handle_store_next)+1u);
template<class State,class Held,class Draw,class Dispatch> struct Adapter : Port<State> {
    Held &held_child;Draw &draw_child;Dispatch &dispatch;
    std::uint32_t depth{};      // bytes pushed below the saved registers (negative before EBX and ESI)
    bool first_next{true},first_defined{true},seen_flag{};
    std::uint32_t exit{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Held &h,Draw &d,Dispatch &ds):Port<State>(s,c,t,{}),held_child(h),draw_child(d),dispatch(ds){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    template<class Route> void routed(std::uint32_t target,std::uint32_t pc,std::uint32_t next,Kept &kept,Route &route) {
        auto child=[&](State &v){route(v,target,pc);};
        this->call(pc,next,target,child);
        refresh(kept);
    }
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case elements_load:case elements_next:case effect_load:gp[edi]=v;break;
        case array_size:case array_size_next:case effect_datums:case definition_again:case first_person_dword:gp[ecx]=v;break;
        case array_data:case array_data_next:case players_load:case definition_kind:case handle_argument:case index_next:gp[edx]=v;break;
        case salt_load:case salt_next:gp[ecx]=sx16(v);break;
        case handle_load:case local_unit_load:case local_unit_again:gp[esi]=v;break;
        case data_load:gp[ebx]=v;break;
        case cluster_load:set_low_word(gp[ecx],v);break;                                                 // MOV CX,word
        case flags_load:gp[eax]=v&0xffffu;break;                                                          // MOVZX EAX,word
        case effects_load:case definition_load:case active_dword:case handle_next:gp[eax]=v;break;
        case owner_load:gp[ebp]=v;break;
        case first_person_load:set_low_byte(gp[edx],v);break;
        case shown_load:set_low_byte(gp[eax],v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing() {                                                                                     // SUB ESP,14; PUSH EBP; PUSH EDI
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],logic::local::size,4);
        this->push(0x1770c3u,gp[ebp]);this->push(0x1770c4u,gp[edi]);depth=0u-8u;
    }
    void zero_index(){this->gp[edx]=this->flags.logic(0u,4);}                                           // XOR EDX,EDX
    std::uint32_t next(Kept &kept) {                                                                     // MOV ECX,EDI; CALL 16EA60
        this->gp[ecx]=kept.array_or_effect;
        const std::uint32_t pc=first_next?0x1770cfu:0x17727fu;first_next=false;
        routed(0x16ea60u,pc,pc+5u,kept,dispatch);
        return this->gp[eax];
    }
    std::uint32_t reset_owner(){return this->gp[ebp]=this->flags.logic(this->gp[ebp]|logic::none,4);}   // OR EBP,-1
    bool found(std::uint32_t index) {                                                                    // CMP EAX,EBP; MOV ECX,EBP
        auto &gp=this->gp;
        this->compare32(index,gp[ebp]);gp[ecx]=gp[ebp];
        return index!=gp[ebp];
    }
    std::uint32_t no_handle()const{return this->gp[ecx];}
    std::uint32_t element_offset(std::uint32_t element_size,std::uint32_t index){return this->gp[ecx]=this->flags.product(element_size,index);}   // IMUL ECX,EAX
    std::uint32_t handle(std::uint32_t salt,std::uint32_t index) {                                      // SHL ECX,10; OR ECX,EAX
        auto &fl=this->flags;
        return this->gp[ecx]=fl.logic(fl.shift(sx16(salt),logic::salt_shift,4)|index,4);
    }
    bool handle_compared(std::uint32_t h,std::uint32_t owner){this->compare32(h,owner);return h!=owner;}   // CMP reg,EBP
    void saving() {                                                                                      // PUSH EBX; PUSH ESI
        auto &gp=this->gp;
        this->push(0x1770fbu,gp[ebx]);this->push(0x1770fcu,gp[esi]);depth=0u;
        this->reached(Branch::elements);
    }
    std::uint32_t masked(std::uint32_t current){return this->gp[esi]=this->flags.logic(current&logic::handle_mask,4);}   // AND ESI,FFFF
    std::uint32_t element_address(std::uint32_t index,std::uint32_t data) {                             // IMUL ESI,ESI,54; ADD ESI,EBX
        auto &fl=this->flags;
        return this->gp[esi]=fl.plus(fl.product(index,logic::element::size),data,4);
    }
    std::uint32_t defined(Kept &kept) {                                                                  // MOV ECX,ESI; CALL 178070
        this->gp[ecx]=kept.held;
        routed(0x178070u,0x177118u,0x17711du,kept,dispatch);
        return this->gp[eax];
    }
    bool cluster_compared(std::uint32_t cluster,std::uint32_t owner) {                                  // CMP CX,BP
        this->flags.minus(cluster&0xffffu,owner&0xffffu,2);
        return (cluster&0xffffu)!=(owner&0xffffu);
    }
    bool always(std::uint32_t flags_value) {                                                             // SHR EAX,9; TEST AL,1
        auto &gp=this->gp;
        gp[eax]=this->flags.right(flags_value&0xffffu,logic::definition::always_bit,4,false);this->flags.logic(gp[eax]&1u,1);
        return gp[eax]&1u;
    }
    std::uint32_t cluster_bit(std::uint32_t cluster) {                                                   // MOVSX EDX,CX; MOV ECX,EDX; AND ECX,1F; MOV EAX,1; SHL EAX,CL
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=sx16(cluster);gp[ecx]=fl.logic(gp[edx]&0x1fu,4);
        return gp[eax]=fl.shift(1u,gp[ecx],4);
    }
    std::uint32_t cluster_word(std::uint32_t) {                                                          // MOV ECX,EDX; SAR ECX,5
        auto &gp=this->gp;
        return gp[ecx]=this->flags.right(gp[edx],5u,4,true);
    }
    bool visible(std::uint32_t visible_value,std::uint32_t bit){this->flags.logic(visible_value&bit,4);return visible_value&bit;}   // TEST [..],EAX
    bool drawn_anyway(Kept &kept) {                                                                      // CALL 3EB10; TEST AL,AL
        routed(0x3eb10u,0x177152u,0x177157u,kept,dispatch);
        this->test8(this->gp[eax]);
        return this->gp[eax]&0xffu;
    }
    bool parent_compared(std::uint32_t parent,std::uint32_t owner){this->compare32(parent,owner);return parent==owner;}   // CMP [ESI+4C],EBP
    bool location_compared(std::uint32_t location,std::uint32_t owner) {                                 // CMP [ESI+30],EBP
        this->compare32(location,owner);
        if(location!=owner)this->reached(Branch::considered);
        return location!=owner;
    }
    std::uint32_t defined_again(Kept &kept) {                                                            // MOV ECX,ESI; CALL 178070
        this->gp[ecx]=kept.held;
        routed(0x178070u,0x177176u,0x17717bu,kept,dispatch);
        return this->gp[eax];
    }
    bool effect_compared(std::uint32_t effect_handle,std::uint32_t owner){this->compare32(effect_handle,owner);return effect_handle!=owner;}   // CMP EDI,EBP
    std::uint32_t effect_offset(std::uint32_t effect_handle) {                                          // MOV EDX,EDI; AND EDX,FFFF; IMUL EDX,EDX,190
        auto &fl=this->flags;
        return this->gp[edx]=fl.product(fl.logic(effect_handle&logic::handle_mask,4),logic::effect::size);
    }
    std::uint32_t unmarked(){set_low_byte(this->gp[ebx],this->flags.logic(0u,1));return this->gp[ebx];}   // XOR BL,BL
    bool unit_compared(std::uint32_t unit){this->compare32(unit,logic::none);return unit!=logic::none;}   // CMP ESI,-1
    bool classified(Kept &kept) {                                                                        // MOV EAX,ESI; CALL 158290; TEST EAX,EAX
        this->gp[eax]=kept.held;
        routed(0x158290u,0x1771acu,0x1771b1u,kept,dispatch);
        this->test32(this->gp[eax]);
        return this->gp[eax]!=0u;
    }
    std::uint32_t marked(){set_low_byte(this->gp[ebx],1u);return this->gp[ebx];}                        // MOV BL,1
    bool owner_tested(std::uint32_t owner){this->compare32(owner,logic::none);return owner!=logic::none;}   // CMP EBP,-1
    bool owner_compared(std::uint32_t unit,std::uint32_t owner) {                                       // CMP ESI,EBP
        this->compare32(unit,owner);
        if(unit==owner)this->reached(Branch::owned);
        return unit==owner;
    }
    bool mark_tested(std::uint32_t mark){this->test8(mark);return mark&0xffu;}                          // TEST BL,BL
    bool effect_tested(std::uint32_t effect_handle){this->compare32(effect_handle,logic::none);return effect_handle!=logic::none;}   // CMP EDI,-1
    bool held_by(Kept &kept) {                                                                           // PUSH ESI; MOV EAX,EDI; CALL 17DE60; TEST AL,AL
        auto &gp=this->gp;
        put(0x1771d7u,kept.held);gp[eax]=kept.array_or_effect;
        this->call(0x1771dau,0x1771dfu,0x17de60u,held_child);depth-=4u;
        refresh(kept);
        this->test8(gp[eax]);
        if(gp[eax]&0xffu)this->reached(Branch::held);
        return gp[eax]&0xffu;
    }
    void seen(bool held_value) {                                                                         // MOV CL,1 / XOR CL,CL
        seen_flag=held_value;
        set_low_byte(this->gp[ecx],held_value?1u:this->flags.logic(0u,1));
    }
    bool first_person_compared(std::uint32_t first_person,std::uint32_t argument) {                     // CMP DL,[ESP+28]; SETZ AL; TEST DL,DL
        auto &gp=this->gp;
        this->flags.minus(first_person&0xffu,argument&0xffu,1);
        set_low_byte(gp[eax],this->flags.zf?1u:0u);this->test8(first_person);
        return gp[eax]&0xffu;
    }
    std::uint32_t player_offset(std::uint32_t unit){return this->gp[esi]=this->flags.product(unit,logic::player::size);}   // IMUL ESI,ESI,94
    bool view_compared(std::uint32_t view){this->flags.minus(view&0xffffu,logic::word_none,2);return (view&0xffffu)!=logic::word_none;}   // CMP word,-1
    bool shown(std::uint32_t byte) {                                                                     // SHR AL,6; AND AL,1
        auto &gp=this->gp;auto &fl=this->flags;
        set_low_byte(gp[eax],fl.logic(fl.right(byte&0xffu,logic::definition::shown_bit,1,false)&1u,1));
        return gp[eax]&0xffu;
    }
    bool active_compared(std::uint32_t word){this->flags.minus(word&0xffffu,0u,2);return (word&0xffffu)!=0u;}   // CMP word,0
    bool seen_tested(){this->test8(this->gp[ecx]);return seen_flag;}                                    // TEST CL,CL
    bool kind_compared(std::uint32_t word){this->flags.minus(word&0xffffu,logic::definition::hidden_kind,2);return (word&0xffffu)==logic::definition::hidden_kind;}   // CMP word,2
    bool draw_tested(bool){this->test8(this->gp[eax]);return this->gp[eax]&0xffu;}                     // TEST AL,AL
    void drawn(std::uint32_t element_handle,std::uint32_t flag_value,std::uint32_t active,Kept &kept) {  // PUSH 0, EAX, ECX, EDX; CALL 1772C0
        put(0x177259u,0u);put(0x17725bu,active);put(0x17725cu,flag_value);put(0x17725du,element_handle);
        this->call(0x17725eu,0x177263u,0x1772c0u,draw_child);depth-=16u;
        refresh(kept);
        this->reached(Branch::drawn_element);
    }
    void incremented(std::uint32_t index){this->gp[edx]=this->flags.increment(index,4,false);}          // INC EDX
    void restoring(bool full) {
        auto &gp=this->gp;
        if(full){gp[esi]=this->pop(0x1772a8u);gp[ebx]=this->pop(0x1772a9u);}                             // POP ESI; POP EBX
        gp[edi]=this->pop(0x1772aau);gp[ebp]=this->pop(0x1772abu);                                       // POP EDI; POP EBP
        gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);                                         // ADD ESP,14
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::effect_elements_draw::draw(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]});
        this->ret(0x1772afu,4u);this->publish(0x1772afu);
    }
};
template<class State,class Reference,class Held,class Draw,class Dispatch> bool bridge(State &s,Reference reference,Held held,Draw draw,Dispatch dispatch) {
    return run_owner(s,0x1770c0u,0x1772afu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Held,Draw,Dispatch>(v,c,t,held,draw,dispatch);});
}
} // namespace halo2::native_effect_elements_draw_detail
