#pragma once
// Boundary adapter for the effect element, 17BD40
// (native_tu15_effect_element_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EAX the part index, the
// effect handle on the stack, EAX the element or 0 (RET 4); 8 bytes of
// frame with EBX, EBP, ESI and EDI saved below them, read from the ESP each
// child left; 158290 through the unit's dispatch, 17E100 (ECX the part, EDX
// the effect), 17DCE0 (3 pushed, RET C), 17AEB0 (EAX, ECX, EDX), 176E70 (4
// pushed, RET 10), 177F10 (1 pushed and EBX, RET 4) and 17BB10 (5 pushed,
// RET 14) direct, every register each returns coming back. XMM0 to XMM2 hold
// what the original's scalar moves and division leave in them. Each access
// publishes its original instruction.
// HALO2_REFERENCE_EFFECT_ELEMENT=1 restores the translated body.
// HALO2_VERIFY_EFFECT_ELEMENT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_effect_element_systems.hpp"

namespace halo2::native_effect_element_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_effect_element;
using Access=logic::effect_element::Access;
using Kept=logic::effect_element::Kept;
using Outcome=logic::effect_element::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_EFFECT_ELEMENT");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_EFFECT_ELEMENT");return v;}
enum Branch : unsigned {failed, made, marked, positive, branch_count};
static_assert(unsigned(Outcome::made)==Branch::made);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Effect element"};return v;}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0x17bd40u,0x17bd4bu,0x17bd5du,0x17bd62u,0x17bd65u,0x17bd71u,0x17bd75u,0x17bd79u,0x17bd7fu,0x17bd83u,
    0x17bdb5u,0x17bdbbu,0x17bdc5u,0x17bdc9u,0x17bdcfu,0x17bde9u,0x17be02u,0x17be06u,0x17be09u,0x17be1fu,0x17be25u,0x17be28u,0x17be3cu,
    0x17be3cu,0x17be43u,0u,0u,0x17be7cu,0x17be80u,0x17be83u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::active_load)+1u);
template<class State,class Accept,class Locate,class FrameCheck,class Handle,class Prepare,class Start,class Dispatch>
struct Adapter : Port<State> {
    Accept &accept_child;Locate &locate_child;FrameCheck &frame_child;Handle &handle_child;Prepare &prepare_child;Start &start_child;
    Dispatch &dispatch;
    std::uint32_t depth{};      // bytes pushed below the saved registers (negative before ESI and EDI)
    bool scale_positive{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Accept &a,Locate &l,FrameCheck &f,Handle &h,Prepare &p,Start &st,Dispatch &d)
        :Port<State>(s,c,t,{}),accept_child(a),locate_child(l),frame_child(f),handle_child(h),prepare_child(p),start_child(st),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        if(a==Access::one_load)return scale_positive?0x17be4au:0x17be5du;
        if(a==Access::rate_store)return scale_positive?0x17be56u:0x17be65u;
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case instances:case events_load:case elements_load:gp[edx]=v;break;
        case effect_handle:gp[ebp]=v;break;
        case effects_load:case definition_load:case owner_load:case location_load:case location_value:case tag_again:gp[eax]=v;break;
        case tag_load:case handle_again:case element_datums:case location_again:gp[ecx]=v;break;
        case event_index:gp[ecx]=sx16(v);break;
        case parts_load:gp[esi]=v;break;
        case part_word:set_low_word(gp[eax],v);break;                                                    // MOV AX,word
        case part_type:set_low_word(gp[esi],v);break;                                                    // MOV SI,word
        case event_again:gp[edx]=sx16(v);break;
        case scale_load:this->s.xmm[1]=lane0(v);break;                                                  // MOVSS XMM1
        case one_load:this->s.xmm[scale_positive?2:1]=lane0(v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing() {                                                                                     // SUB ESP,8; PUSH EBX; PUSH EBP
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],logic::local::size,4);
        this->push(0x17bd49u,gp[ebx]);this->push(0x17bd4au,gp[ebp]);depth=0u-8u;
    }
    std::uint32_t effect_offset(std::uint32_t handle) {                                                 // AND EBP,FFFF; IMUL EBP,EBP,190
        auto &fl=this->flags;
        return this->gp[ebp]=fl.product(fl.logic(handle&logic::handle_mask,4),logic::effect::size);
    }
    std::uint32_t part_index(){return this->gp[ebx]=this->gp[eax];}                                     // MOV EBX,EAX
    std::uint32_t effect_address(std::uint32_t offset,std::uint32_t datums){return this->gp[ebp]=this->flags.plus(offset,datums,4);}  // ADD EBP,[EAX+44]
    std::uint32_t tag_offset(std::uint32_t tag) {                                                       // AND ECX,FFFF; SHL ECX,4
        auto &fl=this->flags;
        static_assert((1u<<4)==logic::tag_size);
        return this->gp[ecx]=fl.shift(fl.logic(tag&logic::handle_mask,4),4u,4);
    }
    std::uint32_t event_offset(std::uint32_t index){return this->gp[ecx]=this->flags.product(sx16(index),logic::event::size);}  // IMUL ECX,ECX,38
    void saving(unsigned n) {
        auto &gp=this->gp;
        if(n==0u){this->push(0x17bd82u,gp[esi]);depth=0u-4u;}else{this->push(0x17bd87u,gp[edi]);depth=0u;}  // PUSH ESI / PUSH EDI
    }
    std::uint32_t part_address(std::uint32_t index,std::uint32_t parts) {                               // MOV EDI,EBX; IMUL EDI,EDI,38; ADD EDI,ESI
        auto &fl=this->flags;
        return this->gp[edi]=fl.plus(fl.product(index,logic::part::size),parts,4);
    }
    std::uint32_t unmarked(){return this->gp[esi]=this->flags.logic(0u,4);}                             // XOR ESI,ESI
    void owner_compared(std::uint32_t owner){this->compare32(owner,logic::none);}                      // CMP EAX,-1
    bool classified(Kept &kept) {                                                                        // CALL 158290; TEST EAX,EAX
        auto child=[&](State &v){dispatch(v,0x158290u,0x17bd96u);};
        this->call(0x17bd96u,0x17bd9bu,0x158290u,child);
        refresh(kept);
        this->test32(this->gp[eax]);
        return this->gp[eax]!=0u;
    }
    std::uint32_t marked(){this->reached(Branch::marked);return this->gp[esi]=1u;}                      // MOV ESI,1
    bool accepted(Kept &kept) {                                                                          // MOV ECX,EDI; MOV EDX,EBP; CALL 17E100; TEST AL,AL
        auto &gp=this->gp;
        gp[ecx]=kept.part_address;gp[edx]=kept.effect_address;
        this->call(0x17bda8u,0x17bdadu,0x17e100u,accept_child);
        refresh(kept);
        this->test8(gp[eax]);
        return gp[eax]&0xffu;
    }
    void location_compared(std::uint32_t location){this->compare32(location,logic::none);}             // CMP EAX,-1
    void word_cleared(){this->gp[eax]=this->flags.logic(0u,4);}                                         // XOR EAX,EAX
    std::uint32_t located(std::uint32_t word,Kept &kept) {                                              // LEA ECX,[ESP+10]; PUSH EAX, ECX, EBP; CALL 17DCE0; TEST EAX,EAX
        auto &gp=this->gp;
        gp[ecx]=local(logic::local::location_value);
        put(0x17bdd7u,word);put(0x17bdd8u,gp[ecx]);put(0x17bdd9u,kept.effect_address);
        this->call(0x17bddau,0x17bddfu,0x17dce0u,locate_child);depth-=12u;
        refresh(kept);
        this->test32(gp[eax]);
        return gp[eax];
    }
    std::uint32_t type_cleared(){return this->gp[esi]=this->flags.logic(0u,4);}                         // XOR ESI,ESI
    bool frame_accepted(std::uint32_t type,std::uint32_t frame_address,Kept &kept) {                   // LEA EDX,[EAX+30]; LEA ECX,[EBP+1C]; MOV EAX,ESI; CALL 17AEB0; TEST AL,AL
        auto &gp=this->gp;
        gp[edx]=frame_address+logic::frame::data;gp[ecx]=kept.effect_address+logic::effect::frame_base;gp[eax]=type;
        this->call(0x17bdf5u,0x17bdfau,0x17aeb0u,frame_child);
        refresh(kept);
        this->test8(gp[eax]);
        return gp[eax]&0xffu;
    }
    std::uint32_t handled(std::uint32_t event_value,std::uint32_t tag,std::uint32_t handle,Kept &kept) {   // PUSH EDX, EBX, EAX, ECX; CALL 176E70; CMP EAX,-1
        auto &gp=this->gp;
        put(0x17be0du,sx16(event_value));put(0x17be0eu,kept.index);put(0x17be0fu,tag);put(0x17be10u,handle);
        this->call(0x17be11u,0x17be16u,0x176e70u,handle_child);depth-=16u;
        refresh(kept);
        this->compare32(gp[eax],logic::none);
        return gp[eax];
    }
    std::uint32_t element_address(std::uint32_t handle,std::uint32_t datums) {                          // XORPS XMM0; AND EAX,FFFF; IMUL EAX,EAX,54; ADD EAX,ECX; MOV ESI,EAX
        auto &gp=this->gp;auto &fl=this->flags;
        this->s.xmm[0]=_mm_setzero_ps();
        gp[eax]=fl.plus(fl.product(fl.logic(handle&logic::handle_mask,4),logic::element::size),datums,4);
        return gp[esi]=gp[eax];
    }
    std::uint32_t ored(std::uint32_t v,std::uint32_t bit){return this->flags.logic((v|bit)&0xffu,1);}  // OR byte [ESI+C],1
    bool scale_compared(std::uint32_t) {                                                                 // COMISS XMM1,XMM0
        auto &x=this->s.xmm;
        this->flags.scalar_compare(x[1],x[0],false);
        scale_positive=!(this->flags.cf||this->flags.zf);
        if(scale_positive)this->reached(Branch::positive);
        return scale_positive;
    }
    void rated(bool divided,std::uint32_t rate){if(divided)this->s.xmm[2]=lane0(rate);}                // DIVSS XMM2,XMM1
    void prepared(Kept &kept) {                                                                          // LEA EAX,[EBP+FC]; LEA EBX,[EBP+100]; PUSH EAX; CALL 177F10
        auto &gp=this->gp;
        gp[eax]=kept.effect_address+logic::effect::first_list;gp[ebx]=kept.effect_address+logic::effect::second_list;
        put(0x17be76u,gp[eax]);
        this->call(0x17be77u,0x17be7cu,0x177f10u,prepare_child);depth-=4u;
        refresh(kept);
    }
    void active_compared(std::uint32_t active) {                                                         // CMP word [EDI+C],0; SETNZ DL
        this->flags.minus(active&0xffffu,0u,2);
        set_low_byte(this->gp[edx],this->flags.zf?0u:1u);
    }
    void started(Kept &kept) {                                                                           // PUSH EDX, 0, EDI, ESI, EBP; CALL 17BB10
        auto &gp=this->gp;
        put(0x17be8bu,gp[edx]);put(0x17be8cu,0u);put(0x17be8eu,kept.part_address);put(0x17be8fu,kept.held);put(0x17be90u,kept.effect_address);
        this->call(0x17be91u,0x17be96u,0x17bb10u,start_child);depth-=20u;
        refresh(kept);
    }
    std::uint32_t exit{};
    void restoring(bool made_element) {
        auto &gp=this->gp;
        if(made_element) {                                                                               // POP EDI; MOV EAX,ESI; POP ESI; POP EBP; POP EBX
            gp[edi]=this->pop(0x17be96u);gp[eax]=gp[esi];gp[esi]=this->pop(0x17be99u);gp[ebp]=this->pop(0x17be9au);
            gp[ebx]=this->pop(0x17be9bu);exit=0x17be9fu;
        } else {                                                                                         // POP EDI; POP ESI; POP EBP; XOR EAX,EAX; POP EBX
            gp[edi]=this->pop(0x17bea2u);gp[esi]=this->pop(0x17bea3u);gp[ebp]=this->pop(0x17bea4u);
            gp[eax]=this->flags.logic(0u,4);gp[ebx]=this->pop(0x17bea7u);exit=0x17beabu;
        }
        gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);                                         // ADD ESP,8
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::effect_element::make(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]});
        this->reached(unsigned(outcome));
        this->ret(exit,4u);this->publish(exit);
    }
};
template<class State,class Reference,class Accept,class Locate,class FrameCheck,class Handle,class Prepare,class Start,class Dispatch>
bool bridge(State &s,Reference reference,Accept accept,Locate locate,FrameCheck frame_check,Handle handle,Prepare prepare,Start start,
            Dispatch dispatch) {
    return run_owner(s,0x17bd40u,0x17beabu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return Adapter<State,Accept,Locate,FrameCheck,Handle,Prepare,Start,Dispatch>(v,c,t,accept,locate,frame_check,handle,prepare,start,dispatch);});
}
} // namespace halo2::native_effect_element_detail
