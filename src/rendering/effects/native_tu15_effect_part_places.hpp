#pragma once
// Boundary adapter for the effect part places, 17CD10
// (native_tu15_effect_part_places_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: the effect on the stack
// (RET 4); a 0x7C-byte frame below the saved EBP and ESI (and EBX and EDI
// while parts are placed); 17DD50 (ECX the node frame, EDX the effect),
// 145560 (ECX the node frame, EDX the transform, the composed frame pushed,
// RET 4) and 159690 (the part, the ratio and the transform pushed, RET C)
// through the dispatch, every register each returns coming back. Each access
// publishes its original instruction.
// HALO2_REFERENCE_EFFECT_PART_PLACES=1 restores the translated body.
// HALO2_VERIFY_EFFECT_PART_PLACES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/effects/native_tu15_effect_part_places_systems.hpp"

namespace halo2::native_effect_part_places_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_effect_part_places;
using Access=logic::effect_part_places::Access;
using Kept=logic::effect_part_places::Kept;
using Outcome=logic::effect_part_places::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_EFFECT_PART_PLACES");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_EFFECT_PART_PLACES");return v;}
enum Branch : unsigned {no_parts, placed, scaled_part, on_node, off_node, branch_count};
static_assert(unsigned(Outcome::placed)==Branch::placed);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Effect part places"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0x17cd13u,0x17cd1au,0x17cd21u,0x17cd2cu,0x17cd30u,0x17cd38u,0x17cd3bu,0x17cd45u,0x17cd49u,
    0x17cd54u,0x17cd60u,0x17cd63u,0x17cd67u,0x17cd76u,0x17cd7fu,0x17cd83u,0x17cd87u,0x17cd8du,0x17cd91u,0x17cd9au,0x17cda0u,
    0x17cdafu,0x17cdb4u,0x17cdb8u,0u,0u,0x17ce09u,0x17ce0du,0x17ce11u,0x17ce1au,0x17ce1eu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::offset_store_again)+1u);
template<class State,class Dispatch> struct Adapter : Port<State> {
    Dispatch &dispatch;
    std::uint32_t depth{};      // bytes below the return address
    bool node_path{};           // the last location was on a node: its reloads' instructions
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,{}),dispatch(d){}
    std::uint32_t argument()const{return this->gp[esp]+depth+4u;}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        if(a==Access::next_load)return node_path?0x17cdeau:0x17cdffu;
        if(a==Access::event_load)return node_path?0x17cdeeu:0x17ce03u;
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case instances:gp[ecx]=v;put(0x17cd19u,gp[ebp]);break;                                         // PUSH EBP
        case effect_argument:gp[ebp]=v;break;
        case effect_tag:gp[eax]=fl.shift(fl.logic(v&0xffffu,4),4u,4);break;                            // AND EAX,FFFF; SHL EAX,4
        case definition_address:gp[edx]=v;break;
        case event_index:gp[ecx]=fl.product(logic::widen16(v),logic::event_size);put(0x17cd37u,gp[esi]);break;   // IMUL ECX,38; PUSH ESI
        case events:gp[esi]=v;break;
        case part_count:gp[edx]=v;gp[ecx]=fl.plus(gp[ecx],gp[esi],4);gp[eax]=fl.logic(0u,4);break;     // ADD ECX,ESI; XOR EAX,EAX
        case first_part:gp[edi]=v;break;
        case offset_load:case first_location:case next_load:gp[eax]=v;break;
        case scale:x[1]=lane0(v);break;
        case location_index:gp[eax]=logic::widen16(v);break;
        case ratio_load:gp[ebx]=v;break;
        case pool:gp[ecx]=v;break;
        case pool_data:                                                                                // AND EAX,FFFF; IMUL EAX,3C; ADD EAX,EDX; MOV ESI,EAX
            gp[edx]=v;gp[eax]=fl.plus(fl.product(fl.logic(gp[eax]&0xffffu,4),logic::location_size),v,4);gp[esi]=gp[eax];break;
        case next_location:gp[edx]=v;gp[eax]=fl.logic(0u,4);break;                                     // XOR EAX,EAX
        case node:gp[eax]=v&0xffffu;break;
        case event_load:gp[ecx]=v;break;
        case index_load:gp[eax]=v;break;
        case offset_again:gp[esi]=v;break;
        case count_again:gp[edx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void counted(std::uint32_t count){this->compare32(count,0u);}                                     // CMP EDX,EAX
    void saving(){put(0x17cd53u,this->gp[ebx]);}
    void saved(){put(0x17cd58u,this->gp[edi]);}
    // XORPS XMM0,XMM0; ADD EDI,EAX; COMISS XMM1,XMM0; (MOVSS XMM0,[EBP+60]; DIVSS XMM0,XMM1).
    void scaled(std::uint32_t scale_value,bool scaled_part,std::uint32_t ratio) {
        auto &gp=this->gp;auto &fl=this->flags;auto &x=this->s.xmm;
        x[0]=_mm_setzero_ps();gp[edi]=fl.plus(gp[edi],gp[eax],4);fl.scalar_compare(lane0(scale_value),x[0],false);
        if(scaled_part){x[0]=lane0(ratio);this->reached(Branch::scaled_part);}
    }
    void located(std::uint32_t handle) {                                                               // CMP EAX,-1
        this->compare32(handle,logic::none);
    }
    void on_node(std::uint32_t node_value) {                                                           // CMP AX,FFFF
        this->flags.minus(node_value&0xffffu,0xffffu,2);
        node_path=node_value!=0xffffu;
        this->reached(node_path?Branch::on_node:Branch::off_node);
    }
    void call_through(std::uint32_t pc,std::uint32_t target) {
        auto child=[&](State &v){dispatch(v,target,pc);};
        this->call(pc,pc+5u,target,child);
    }
    void node_frame(Kept &kept) {                                                                      // LEA ECX,[ESP+24]; MOV EDX,EBP; CALL 17DD50
        auto &gp=this->gp;
        gp[ecx]=local(logic::local::node_frame);gp[edx]=kept.effect;
        call_through(0x17cdc8u,0x17dd50u);
        refresh(kept);
    }
    void compose(std::uint32_t transform,Kept &kept) {                                                 // LEA EAX,[ESP+58]; PUSH EAX; LEA EDX,[ESI+8]; LEA ECX,[ESP+28]; CALL 145560
        auto &gp=this->gp;
        gp[eax]=local(logic::local::composed);put(0x17cdd1u,gp[eax]);gp[edx]=transform;gp[ecx]=local(logic::local::node_frame);
        call_through(0x17cdd9u,0x145560u);depth-=4u;
        refresh(kept);
    }
    void place(std::uint32_t transform,Kept &kept) {                                                   // PUSH transform; PUSH EBX; PUSH EDI; CALL 159690
        auto &gp=this->gp;
        if(node_path){gp[ecx]=transform;put(0x17cde2u,transform);put(0x17cde3u,kept.ratio);put(0x17cde4u,kept.part);call_through(0x17cde5u,0x159690u);}
        else{gp[esi]=this->flags.plus(kept.location,8u,4);put(0x17cdf7u,transform);put(0x17cdf8u,kept.ratio);put(0x17cdf9u,kept.part);call_through(0x17cdfau,0x159690u);}
        depth-=0xcu;
        refresh(kept);
    }
    void advanced(std::uint32_t index,std::uint32_t offset,std::uint32_t count) {                      // INC EAX; ADD ESI,3C; CMP EAX,EDX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.increment(index-1u,4,false);gp[esi]=fl.plus(offset-logic::part_size,logic::part_size,4);this->compare32(index,count);
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x7cu,4);depth=0x7cu;                                                 // SUB ESP,7C
        Memory<Adapter> memory{*this};
        const auto outcome=logic::effect_part_places::place(memory,*this);
        this->reached(unsigned(outcome));
        if(outcome==Outcome::placed){gp[edi]=take(0x17ce28u);gp[ebx]=take(0x17ce29u);}
        gp[esi]=take(0x17ce2au);gp[ebp]=take(0x17ce2bu);
        gp[esp]=fl.plus(gp[esp],0x7cu,4);                                                              // ADD ESP,7C
        this->ret(0x17ce2fu,4u);this->publish(0x17ce2fu);
    }
};
template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    return run_owner(s,0x17cd10u,0x17ce2fu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Dispatch>(v,c,t,dispatch);});
}
} // namespace halo2::native_effect_part_places_detail
