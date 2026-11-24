#pragma once
// Boundary adapter for the aim offset, E06E0
// (native_tu15_aim_offset_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the unit (RET); a 0x9C-byte
// frame below the saved EBP, ESI and EDI (and EBX around the marker and the
// unit's frame); 111F00 (EAX the unit), BAD60 (three pushed, CL 0, EBX the
// marker frame, ESI the node, RET C), CDF30 (EAX the unit, two pushed, RET 8)
// and E05D0 (ESI, EDI, ECX, EDX) direct, every register each returns coming
// back. Each access publishes its original instruction.
// HALO2_REFERENCE_AIM_OFFSET=1 restores the translated body.
// HALO2_VERIFY_AIM_OFFSET=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_aim_offset_systems.hpp"

namespace halo2::native_aim_offset_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_aim_offset;
using Access=logic::aim_offset::Access;
using Kept=logic::aim_offset::Kept;
using Outcome=logic::aim_offset::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_AIM_OFFSET");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_AIM_OFFSET");return v;}
enum Branch : unsigned {no_parent, held, started, smoothed, fallback_marker, clamped_low, clamped_high, branch_count};
static_assert(unsigned(Outcome::smoothed)==Branch::smoothed);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Aim offset"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0xe06e0u,0xe06e6u,0xe06fcu,0xe0704u,0xe0723u,0xe0753u,0xe075cu,0xe0767u,0xe0769u,0xe076cu,
    0xe076fu,0xe0775u,0xe077eu,0xe0783u,0xe0789u,0xe0792u,0xe0797u,0xe07a1u,0xe07a8u,0xe07aeu,0xe07cbu,0xe07d1u,0xe07d7u,0xe07ddu,
    0xe07e3u,0xe07f7u,0xe0801u,0xe080fu,0xe0815u,0xe081bu,0xe0821u,0xe082eu,0xe0836u,0xe0841u,0xe0858u,0xe0864u,0xe086cu,0xe0886u,
    0xe0892u,0xe089au,0xe08b7u,0xe08c1u,0xe08d1u,0xe08d5u,0xe08dfu,0xe08e1u,0xe08e5u,0xe08e8u,0xe08ebu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::started_store)+1u);
template<class State,class Held,class Marker,class Facing,class Axes> struct Adapter : Port<State> {
    Held &held_child;
    Marker &marker_child;
    Facing &facing_child;
    Axes &axes_child;
    std::uint32_t depth{};      // bytes below the return address
    unsigned markers{},pushes{};
    bool popped{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Held &h,Marker &m,Facing &f,Axes &a)
        :Port<State>(s,c,t,{}),held_child(h),marker_child(m),facing_child(f),axes_child(a){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case headers:gp[ecx]=v;break;
        case datums:gp[edx]=v;break;
        case object_address:gp[ebp]=v;gp[esi]=gp[edx]+4u*gp[eax]+logic::header_address;break;          // LEA ESI,[EDX+EAX*4+8]
        case node_offset:gp[esi]=logic::widen16(v);break;
        case marker_x:case marker_y:case marker_z:x[0]=lane0(v);break;
        case facing_x:gp[ecx]=v;break;
        case facing_y:gp[edx]=v;break;
        case facing_z:gp[eax]=v;break;
        case offset_y:x[3]=lane0(v);break;
        case forward_y:x[0]=lane0(v);break;
        case forward_x:x[1]=lane0(v);break;
        case offset_x:x[2]=lane0(v);break;
        case started:set_low_byte(gp[eax],v);this->test8(v&0xffu);break;                             // MOV AL; TEST AL,AL
        case first_x:case first_z:gp[eax]=v;break;
        case low:x[2]=lane0(v);break;                                                                // MOVSS XMM2,[46A558]
        case high:x[4]=lane0(v);break;                                                               // MOVSS XMM4,[46A18C]
        case first_y:gp[ecx]=v;gp[edx]=gp[ebp]+logic::unit::offset;break;                            // LEA EDX,[EBP+3CC]
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing(std::uint32_t unit_handle) {                                                          // SUB ESP,9C; PUSH EBP, ESI, EDI; MOV EDI,EAX; AND; LEA
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x9cu,4);depth=0x9cu;
        put(0xe06efu,gp[ebp]);put(0xe06f0u,gp[esi]);put(0xe06f1u,gp[edi]);
        gp[edi]=unit_handle;gp[eax]=3u*fl.logic(unit_handle&0xffffu,4);
    }
    void parent_tested(std::uint32_t parent){this->compare32(parent,logic::none);}                    // CMP [EBP+13C],-1
    std::uint32_t held(Kept &kept) {                                                                   // MOV EAX,EDI; CALL 111F00; TEST AL,AL
        auto &gp=this->gp;
        gp[eax]=kept.unit;
        this->call(0xe0713u,0xe0718u,0x111f00u,held_child);
        refresh(kept);
        this->test8(gp[eax]&0xffu);
        return gp[eax];
    }
    void saving(){put(0xe0720u,this->gp[ebx]);this->gp[eax]=this->gp[ebp];}                          // PUSH EBX; MOV EAX,EBP
    // PUSH 1; PUSH name; PUSH EDI; XOR CL,CL; (LEA EBX; ADD ESI,EAX); CALL BAD60; (TEST AX,AX).
    std::uint32_t marker(std::uint32_t name,Kept &kept) {
        auto &gp=this->gp;auto &fl=this->flags;
        const bool first=markers++==0u;
        if(first){put(0xe072au,logic::marker_mode);put(0xe072cu,name);put(0xe0731u,kept.unit);}
        else{put(0xe0744u,logic::marker_mode);put(0xe0746u,name);put(0xe074bu,kept.unit);this->reached(Branch::fallback_marker);}
        set_low_byte(gp[ecx],fl.logic(0u,1));
        if(first){gp[ebx]=kept.marker_frame;gp[esi]=fl.plus(kept.node-kept.object,kept.object,4);}
        this->call(first?0xe073au:0xe074eu,first?0xe073fu:0xe0753u,0xbad60u,marker_child);depth-=0xcu;
        refresh(kept);
        if(first)fl.logic(gp[eax]&0xffffu,2);
        return gp[eax];
    }
    void pushing(std::uint32_t value) {                                                                // PUSH 0; (LEA ECX); PUSH ECX
        if(pushes++==0u)put(0xe079bu,value);
        else{this->gp[ecx]=value;put(0xe07a5u,value);}
    }
    void facing(std::uint32_t dz,Kept &kept) {                                                         // MOV EAX,EDI; CALL CDF30
        auto &gp=this->gp;
        gp[eax]=kept.unit;this->s.xmm[0]=lane0(dz);
        this->call(0xe07b2u,0xe07b7u,0xcdf30u,facing_child);depth-=8u;
        refresh(kept);
    }
    void axes(Kept &kept) {                                                                            // LEA ESI,[EBP+7C]; LEA EDI; LEA ECX; LEA EDX; CALL E05D0
        auto &gp=this->gp;
        gp[esi]=kept.object+logic::unit::frame_source;gp[edi]=local(logic::local::left);gp[ecx]=local(logic::local::forward);
        gp[edx]=local(logic::local::facing);
        this->call(0xe07c6u,0xe07cbu,0xe05d0u,axes_child);
        refresh(kept);
    }
    void local_offset(std::uint32_t x_value,std::uint32_t y_value,std::uint32_t z_value,std::uint32_t dx,std::uint32_t) {
        auto &x=this->s.xmm;
        x[0]=lane0(x_value);x[1]=lane0(y_value);x[2]=lane0(dx);x[3]=lane0(z_value);
    }
    void restoring(){this->gp[ebx]=take(0xe0827u);}
    void started_tested(std::uint32_t){}
    // One axis: its delta in XMM0 (XMM1, XMM3 then XMM0), COMISS against the low step (XMM2), then the high (XMM4).
    void clamped(unsigned axis,std::uint32_t delta,bool below_low,bool above_high,std::uint32_t step) {
        auto &x=this->s.xmm;auto &fl=this->flags;
        const unsigned reg=axis==1u?1u:0u;
        if(axis==2u)x[3]=lane0(delta);
        x[reg]=lane0(delta);
        fl.scalar_compare(x[2],x[reg],false);
        if(!below_low)fl.scalar_compare(x[reg],x[4],false);
        x[reg]=lane0(step);
        if(below_low)this->reached(Branch::clamped_low);
        if(above_high)this->reached(Branch::clamped_high);
    }
    void moved(unsigned axis,std::uint32_t value) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        if(axis==0u)x[5]=lane0(value);
        else x[0]=lane0(value);
        if(axis==2u){gp[edi]=take(0xe08bfu);gp[esi]=take(0xe08c0u);popped=true;}                      // POP EDI; POP ESI (before the store)
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::aim_offset::update(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[eax]});
        this->reached(unsigned(outcome));
        if(outcome==Outcome::smoothed){gp[ebp]=take(0xe08c9u);gp[esp]=fl.plus(gp[esp],0x9cu,4);this->ret(0xe08d0u,0u);this->publish(0xe08d0u);return;}
        gp[edi]=take(0xe08f2u);gp[esi]=take(0xe08f3u);gp[ebp]=take(0xe08f4u);
        gp[esp]=fl.plus(gp[esp],0x9cu,4);
        this->ret(0xe08fbu,0u);this->publish(0xe08fbu);
    }
};
template<class State,class Reference,class Held,class Marker,class Facing,class Axes>
bool bridge(State &s,Reference reference,Held held,Marker marker,Facing facing,Axes axes) {
    return run_owner(s,0xe06e0u,0xe08fbu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Held,Marker,Facing,Axes>(v,c,t,held,marker,facing,axes);});
}
} // namespace halo2::native_aim_offset_detail
