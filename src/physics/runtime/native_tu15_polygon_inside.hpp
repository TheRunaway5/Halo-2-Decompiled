#pragma once
// Boundary adapter for the polygon inside, entered at 243510
// (native_tu15_polygon_inside_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: EBX the point, EDX the vertices, the count
// and the tolerance on the stack (RET 8); EBP, ESI and EDI saved; AL the
// answer (EAX the next index's mask over it), ECX the last edge's index;
// XMM0 the last cross product, XMM1 its second product, XMM2 and XMM3 the
// edge, XMM4 the floor; the flags of the last test. Each access publishes
// its original instruction.
// HALO2_REFERENCE_POLYGON_INSIDE=1 restores the translated body.
// HALO2_VERIFY_POLYGON_INSIDE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_polygon_inside_systems.hpp"

namespace halo2::native_polygon_inside_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_polygon_inside;
using Access=logic::polygon_inside::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_POLYGON_INSIDE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_POLYGON_INSIDE");return v;}
enum Branch : unsigned {empty, inside, outside, branch_count};
static_assert(unsigned(logic::polygon_inside::Outcome::outside)==Branch::outside);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Polygon inside"};return v;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
template<class State> struct Adapter : Port<State> {
    std::uint32_t depth{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u*k;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case count:return 0x243513u;
        case tolerance:return 0x24351fu;
        case count_again:return 0x243530u;
        case point_x:return 0x243535u;
        case point_y:return 0x243539u;
        case vertex_y:gp[ecx]=native_object_update::widen16(gp[ebp]&0xffffu);return 0x243541u;           // MOVSX ECX,BP
        case vertex_x:return 0x243547u;
        // XOR EAX,EAX; LEA ESI,[ECX+1]; CMP ESI,EDI; SETGE AL; LEA EDI,[EDX+ECX*8+4]; DEC EAX; AND ESI,EAX.
        case next_x:{
            gp[esi]=gp[ecx]+1u;
            const bool wraps=std::int32_t(gp[esi])>=std::int32_t(gp[edi]);
            gp[edi]=gp[edx]+8u*gp[ecx]+4u;gp[eax]=wraps?0u:0xffffffffu;gp[esi]&=gp[eax];
            return 0x24355du;
        }
        case next_y:return 0x243562u;
        case edge_x:return 0x243568u;
        case edge_y:return 0x24356du;
        case count_check:gp[ebp]=this->flags.increment(gp[ebp],4,false);return 0x243583u;               // INC EBP
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case count:this->flags.minus(v,0u,2);set_low_byte(gp[eax],1u);break;                           // CMP word [ESP+8],BP; MOV AL,1
        // XORPS XMM4; SUBSS XMM4,[ESP+C]; PUSH ESI; PUSH EDI.
        case tolerance:this->push(0x243525u,gp[esi]);this->push(0x243526u,gp[edi]);depth=12u;break;
        case count_again:gp[edi]=native_object_update::widen16(v&0xffffu);break;
        case point_x:x[1]=lane0_only(v);break;
        case point_y:x[0]=lane0_only(v);break;
        case count_check:this->flags.minus(gp[ebp]&0xffffu,v,2);break;                                  // CMP BP,[ESP+10]
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // SUBSS ×4; MULSS ×2; SUBSS; COMISS XMM4,XMM0.
    void crossed(std::uint32_t,const logic::polygon_inside::Cross &c) {
        auto &x=this->s.xmm;
        x[0]=lane0_only(c.cross);x[1]=lane0_only(c.product);x[2]=lane0_only(c.ex);x[3]=lane0_only(c.ey);
        this->flags.scalar_compare(x[4],x[0],false);
    }
    void floored(std::uint32_t floor_value){this->s.xmm[4]=lane0_only(floor_value);}
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::polygon_inside::Outcome;
        this->push(0x243510u,gp[ebp]);gp[ebp]=this->flags.logic(0u,4);                                  // PUSH EBP; XOR EBP,EBP
        depth=4u;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::polygon_inside::contains(memory,*this,gp[ebx],gp[edx]);
        this->reached(unsigned(outcome));
        if(outcome==Outcome::inside) {
            gp[edi]=this->pop(0x24358au);gp[esi]=this->pop(0x24358bu);set_low_byte(gp[eax],1u);
            gp[ebp]=this->pop(0x24358eu);this->ret(0x24358fu,8u);this->publish(0x24358fu);
            return;
        }
        if(outcome==Outcome::outside) {
            gp[edi]=this->pop(0x243592u);set_low_byte(gp[eax],this->flags.logic(0u,1));gp[esi]=this->pop(0x243595u);   // XOR AL,AL
        }
        gp[ebp]=this->pop(0x243596u);this->ret(0x243597u,8u);this->publish(0x243597u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x243510u,0x243597u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_polygon_inside_detail
