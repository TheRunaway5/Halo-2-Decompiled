#pragma once
// Boundary adapter for the light gate, entered at 31500
// (native_tu15_light_gate_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: EAX the handle (RET); EBX, EDI (and
// ESI around the child) saved; 3E9B0 (through the unit's dispatch) gets EAX
// the light and ESI its source and answers AL, every register it returns
// coming back; AL the answer (BL, zero, where blocked) over the flags, ECX
// the inside answer, EDX the tag offset; the flags of the last test. Each
// access publishes its original instruction.
// HALO2_REFERENCE_LIGHT_GATE=1 restores the translated body.
// HALO2_VERIFY_LIGHT_GATE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "pstencil/native_tu15_light_gate_systems.hpp"

namespace halo2::native_light_gate_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_light_gate;
using Access=logic::light_gate::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_LIGHT_GATE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_LIGHT_GATE");return v;}
enum Branch : unsigned {none_light, passes, blocked, asked, branch_count};
static_assert(unsigned(logic::light_gate::Outcome::blocked)==Branch::blocked);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Light gate"};return v;}
template<class State,class Route> struct Adapter : Port<State> {
    Route &route;
    bool saved_edi{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case lights:return 0x31508u;
        case data:return 0x3150eu;
        case tags:return 0x31511u;
        // AND EAX,FFFF; IMUL EAX,EAX,110; ADD EAX,EDX.
        case tag:gp[eax]=fl.plus(fl.product(fl.logic(gp[eax]&0xffffu,4),logic::light_size),gp[edx],4);return 0x31524u;
        // AND EDX,FFFF; SHL EDX,4; PUSH EDI.
        case definition:gp[edx]=fl.shift(fl.logic(gp[edx]&0xffffu,4),4u,4);this->push(0x31530u,gp[edi]);saved_edi=true;
            return 0x31531u;
        case portal:set_low_byte(gp[ecx],fl.logic(0u,1));return 0x31537u;                                 // XOR CL,CL
        case source:this->push(0x3153eu,gp[esi]);return 0x3153fu;                                          // PUSH ESI
        case flags:return 0x3154au;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case lights:case tags:gp[ecx]=v;break;
        case data:case tag:gp[edx]=v;break;
        case definition:gp[edi]=v;break;
        case portal:this->flags.minus(v,logic::no_index,2);break;                                        // CMP word [EAX+54],-1
        case source:gp[esi]=v;break;
        // MOV EAX,[EDI]; TEST AL,20; POP EDI.
        case flags:gp[eax]=v;this->test8(v&logic::flag::needs_inside);gp[edi]=this->pop(0x3154eu);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // CALL 3E9B0; MOV CL,AL; POP ESI.
    std::uint32_t inside(std::uint32_t &definition_address,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        this->call_routed(route,0x31542u,0x31547u,0x3e9b0u);
        definition_address=gp[edi];
        set_low_byte(gp[ecx],gp[eax]);gp[esi]=this->pop(0x31549u);
        this->reached(Branch::asked);
        return gp[eax];
    }
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::light_gate::Outcome;
        this->push(0x31500u,gp[ebx]);set_low_byte(gp[ebx],this->flags.logic(0u,1));this->compare32(gp[eax],logic::none);   // PUSH EBX; XOR BL,BL; CMP EAX,-1
        Memory<Adapter> memory{*this};
        const auto outcome=logic::light_gate::passes(memory,*this,gp[eax]);
        this->reached(unsigned(outcome));
        if(outcome!=Outcome::none_light) {
            // TEST CL,CL / TEST AL,40 in the original's order.
            const auto flags_value=gp[eax];
            const bool inside=gp[ecx]&0xffu;
            if(flags_value&logic::flag::needs_inside)this->test8(gp[ecx]);
            if(!(flags_value&logic::flag::needs_inside) || inside) {
                this->test8(flags_value&logic::flag::needs_outside);
                if(flags_value&logic::flag::needs_outside)this->test8(gp[ecx]);
            }
        }
        std::uint32_t exit;
        if(outcome==Outcome::passes){set_low_byte(gp[eax],1u);exit=0x3155du;}
        else{set_low_byte(gp[eax],gp[ebx]);exit=0x31561u;}
        gp[ebx]=this->pop(exit+2u);
        this->ret(exit+3u,0u);this->publish(exit+3u);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    return run_owner(s,0x31500u,0x31564u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Route>(v,c,t,route);});
}
} // namespace halo2::native_light_gate_detail
