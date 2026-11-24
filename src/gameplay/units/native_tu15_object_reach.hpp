#pragma once
// Boundary adapter for the object reach, 154270
// (native_tu15_object_reach_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the object, the player and
// the result on the stack (RET 8); 0x10 bytes of frame with EBX, EBP, ESI
// and EDI saved below them, read from the ESP each child left; CFC90 (the
// unit, the object and the hands record pushed, RET C) through the unit's
// dispatch, 153E10 (EBX the action, the player pushed, RET 4) and C6920 (EDI
// the unit) direct, every register each returns coming back. XMM0 to XMM4
// hold what the original's scalar moves and arithmetic leave in them. Each
// access publishes its original instruction.
// HALO2_REFERENCE_OBJECT_REACH=1 restores the translated body.
// HALO2_VERIFY_OBJECT_REACH=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_object_reach_systems.hpp"

namespace halo2::native_object_reach_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_object_reach;
using Access=logic::object_reach::Access;
using Kept=logic::object_reach::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_OBJECT_REACH");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_OBJECT_REACH");return v;}
enum Branch : unsigned {inside, held, acted, kind, result, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Object reach"};return v;}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0x154270u,0x154279u,0x15427eu,0x154291u,0x154296u,0x154299u,0x15429fu,0x1542a5u,0x1542b8u,0x1542c6u,
    0x1542cau,0x1542d4u,0x1542d8u,0x1542eau,0x1542f1u,0x1542fau,0x154308u,0x154314u,0x154319u,0x15431eu,0x154323u,0x154328u,0x15432du,
    0x154373u,0x154377u,0x1543a2u,0x1543a6u,0x1543adu,0x1543b1u,0x1543b5u,0x1543b9u,0x1543bdu,0x1543c6u,0x1543cfu,0x1543d3u,0x1543d7u,
    0x1543e4u,0x1543e9u,0x1543efu,0x1543fbu,0x1543ffu,0x154403u,0x154407u,0x154409u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::result_second_store)+1u);
template<class State,class Act,class Veto,class Dispatch> struct Adapter : Port<State> {
    Act &act;Veto &veto;Dispatch &dispatch;
    std::uint32_t depth{};      // bytes pushed below the saved registers (negative before ESI and EDI)
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Act &a,Veto &v,Dispatch &d):Port<State>(s,c,t,{}),act(a),veto(v),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    std::uint32_t argument(unsigned n)const{return local(0x24u+4u*n);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case headers_load:case datums_load:case object_load:case hands_low:case result_second:case action_first:gp[ecx]=v;break;
        case player_argument:gp[ebp]=v;break;
        case players_load:case tag_load:case definition_load:case result_first:case game_load:case result_again:gp[eax]=v;break;
        case player_datums:case instances_load:gp[ebx]=v;break;
        case unit_handle_load:case player_again:case action_second:gp[edx]=v;break;
        case extension_load:x[0]=lane0(v);break;                                                         // MOVSS XMM0
        case unit_load:case result_argument:case unit_again:gp[edi]=v;break;
        case radius_load:x[3]=lane0(v);break;                                                            // MOVSS XMM3
        case hands_high:set_low_word(gp[edx],v);break;                                                   // MOV DX,word
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing(){this->gp[esp]=this->flags.minus(this->gp[esp],logic::local::size,4);}             // SUB ESP,10
    void saving(unsigned n) {
        auto &gp=this->gp;
        if(n==0u){this->push(0x15427cu,gp[ebx]);this->push(0x15427du,gp[ebp]);depth=0u-8u;}             // PUSH EBX; PUSH EBP
        else{this->push(0x1542b7u,gp[edi]);depth=0u;}                                                     // PUSH EDI
    }
    std::uint32_t player_offset(std::uint32_t handle) {                                                 // AND EBP,FFFF; IMUL EBP,EBP,21C
        auto &fl=this->flags;
        return this->gp[ebp]=fl.product(fl.logic(handle&logic::handle_mask,4),logic::player::size);
    }
    std::uint32_t saving_object(Kept &) {                                                               // PUSH ESI; MOV ESI,EAX
        auto &gp=this->gp;
        this->push(0x15428eu,gp[esi]);depth=0u-4u;
        return gp[esi]=gp[eax];
    }
    std::uint32_t player_address(std::uint32_t offset,std::uint32_t datums){return this->gp[ebp]=this->flags.plus(offset,datums,4);}   // ADD EBP,EBX
    std::uint32_t index(std::uint32_t handle,unsigned) {                                                // MOV EAX,reg; AND EAX,FFFF; LEA EAX,[EAX+EAX*2]
        const auto masked=this->flags.logic(handle&logic::handle_mask,4);
        this->gp[eax]=3u*masked;
        return masked;
    }
    std::uint32_t tag_offset(std::uint32_t tag) {                                                       // AND EAX,FFFF; SHL EAX,4
        auto &fl=this->flags;
        static_assert((1u<<4)==logic::tag_size);
        return this->gp[eax]=fl.shift(fl.logic(tag&logic::handle_mask,4),4u,4);
    }
    bool extended_tested(std::uint32_t word) {                                                          // CMP word [EAX+290],0; SETNZ BL; TEST BL,BL; (XORPS XMM0)
        auto &gp=this->gp;
        this->flags.minus(word&0xffffu,0u,2);
        set_low_byte(gp[ebx],(word&0xffffu)?1u:0u);this->test8(gp[ebx]);
        if(!(word&0xffffu))this->s.xmm[0]=_mm_setzero_ps();
        return word&0xffffu;
    }
    void flags_tested(std::uint32_t flags_value){this->flags.logic(flags_value&logic::object::unreachable,1);}   // TEST byte [ECX+12C],1
    void reached_out(std::uint32_t reach){this->s.xmm[3]=lane0(reach);}                                // ADDSS XMM3,XMM0; ADDSS XMM3,[456918]
    void parent_compared(std::uint32_t parent,std::uint32_t unit_handle){this->compare32(parent,unit_handle);}   // CMP [ECX+14C],EDX
    void distance(std::uint32_t dy,std::uint32_t x2,std::uint32_t distance_value,std::uint32_t reach2) {   // ...; COMISS XMM0,XMM4
        auto &x=this->s.xmm;
        x[0]=lane0(reach2);x[1]=lane0(dy);x[2]=lane0(x2);x[4]=lane0(distance_value);
        this->flags.scalar_compare(x[0],x[4],false);
        if(!logic::sse::not_above(reach2,distance_value))this->reached(Branch::inside);
    }
    bool hands_found(std::uint32_t unit_handle,Kept &kept) {                                            // LEA ECX,[ESP+10]; PUSH ECX, ESI, EDX; CALL CFC90; TEST AL,AL
        auto &gp=this->gp;
        gp[ecx]=local(logic::local::action);put(0x154363u,gp[ecx]);put(0x154364u,kept.object_handle);put(0x154365u,unit_handle);
        auto child=[&](State &v){dispatch(v,0xcfc90u,0x154366u);};
        this->call(0x154366u,0x15436bu,0xcfc90u,child);depth-=12u;
        refresh(kept);
        this->test8(gp[eax]);
        if(gp[eax]&0xffu)this->reached(Branch::held);
        return gp[eax]&0xffu;
    }
    std::uint32_t hands(std::uint32_t low,std::uint32_t high) {                                         // XOR EAX,EAX; TEST CL/DL; MOV EAX,1/2; TEST CH/DH; OR EAX,4/8
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.logic(0u,4);
        fl.logic(low&0xffu,1);
        if(low&0xffu)gp[eax]=1u;else{fl.logic(high&0xffu,1);if(high&0xffu)gp[eax]=2u;}
        fl.logic((low>>8)&0xffu,1);
        if((low>>8)&0xffu)gp[eax]=fl.logic(gp[eax]|4u,4);
        else{fl.logic((high>>8)&0xffu,1);if((high>>8)&0xffu)gp[eax]=fl.logic(gp[eax]|8u,4);}
        return gp[eax];
    }
    void negated() {                                                                                     // XOR EDX,EDX; NEG BL
        auto &gp=this->gp;
        gp[edx]=this->flags.logic(0u,4);
        set_low_byte(gp[ebx],this->flags.minus(0u,gp[ebx]&0xffu,1));
    }
    void pass(std::uint32_t v){put(0x1543bfu,v);}                                                       // PUSH EDX
    std::uint32_t action_type() {                                                                        // SBB EBX,EBX; AND EBX,6; INC EBX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ebx]=fl.increment(fl.logic(fl.borrow(gp[ebx],gp[ebx],4)&6u,4),4,false);
        return gp[ebx];
    }
    void action_address(){this->gp[ebx]=local(logic::local::action);}                                   // LEA EBX,[ESP+14]
    bool acted(Kept &kept) {                                                                             // CALL 153E10; TEST AL,AL
        auto &gp=this->gp;
        this->call(0x1543dbu,0x1543e0u,0x153e10u,act);depth-=4u;
        refresh(kept);
        this->test8(gp[eax]);
        if(gp[eax]&0xffu)this->reached(Branch::acted);
        return gp[eax]&0xffu;
    }
    void kind_compared(std::uint32_t kind_value) {                                                       // CMP [EAX+8],2
        this->compare32(kind_value,logic::game::acting_kind);
        if(kind_value==logic::game::acting_kind)this->reached(Branch::kind);
    }
    bool vetoed(Kept &kept) {                                                                            // CALL C6920; TEST AL,AL
        auto &gp=this->gp;
        this->call(0x1543f2u,0x1543f7u,0xc6920u,veto);
        refresh(kept);
        this->test8(gp[eax]);
        if(!(gp[eax]&0xffu))this->reached(Branch::result);
        return gp[eax]&0xffu;
    }
    void restoring() {                                                                                   // POP EDI; POP ESI; POP EBP; POP EBX; ADD ESP,10
        auto &gp=this->gp;
        gp[edi]=this->pop(0x15440cu);gp[esi]=this->pop(0x15440du);gp[ebp]=this->pop(0x15440eu);gp[ebx]=this->pop(0x15440fu);
        gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::object_reach::reach_for(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]});
        this->ret(0x154413u,8u);this->publish(0x154413u);
    }
};
template<class State,class Reference,class Act,class Veto,class Dispatch> bool bridge(State &s,Reference reference,Act act,Veto veto,Dispatch dispatch) {
    return run_owner(s,0x154270u,0x154413u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Act,Veto,Dispatch>(v,c,t,act,veto,dispatch);});
}
} // namespace halo2::native_object_reach_detail
