#pragma once
// Boundary adapter for the light shape, C3A20
// (native_tu15_light_shape_source_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the light, EDI the record;
// EBX and ESI saved; C3960 gets EAX the definition and EDX the record;
// C39B0 ESI the entry and EDX the record; every register each returns
// coming back. Which children are direct and which go through the unit's
// dispatch differs by unit, so the bridge takes the unit's steps. Each
// access publishes its original instruction.
// HALO2_REFERENCE_LIGHT_SHAPE_SOURCE=1 restores the translated body.
// HALO2_VERIFY_LIGHT_SHAPE_SOURCE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "pstencil/native_tu15_light_shape_source_systems.hpp"

namespace halo2::native_light_shape_source_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_light_shape_source;
using Access=logic::light_shape::Access;
using Kept=logic::light_shape::Kept;
using Outcome=logic::light_shape::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_LIGHT_SHAPE_SOURCE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_LIGHT_SHAPE_SOURCE");return v;}
enum Branch : unsigned {definition_kind1, definition_kind3, definition_other, entry_kind1, entry_kind3, entry_other, branch_count};
static_assert(unsigned(Outcome::entry_other)==Branch::entry_other);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Light shape source"};return v;}
inline constexpr std::uint32_t sites[]{0xc3a20u,0xc3a32u,0xc3a35u,0xc3a39u,0xc3a4au,0xc3a4eu,0xc3a57u,0xc3a60u,0xc3a66u,0xc3a84u,0xc3a93u,0xc3a9cu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::far_store)+1u);
template<class State,class Steps> struct Adapter : Port<State> {
    Steps &steps;
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Steps &st):Port<State>(s,c,t,{}),steps(st){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case definition:gp[edx]=fl.shift(fl.logic(gp[edx]&0xffffu,4),4u,4);break;                    // AND EDX,FFFF; SHL EDX,4
        case near_store:case far_store:this->s.xmm[0]=_mm_setzero_ps();break;                          // XORPS XMM0,XMM0
        default:break;
        }
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case lights:                                                                                  // AND EAX,FFFF; IMUL EAX,EAX,110; PUSH EBX
            gp[ecx]=v;gp[eax]=fl.product(fl.logic(gp[eax]&0xffffu,4),logic::light_size);this->push(0xc3a31u,gp[ebx]);break;
        case light_data:gp[ebx]=v;break;
        case light_tag:gp[edx]=v;break;
        case instances:gp[ecx]=v;gp[eax]=fl.plus(gp[eax],gp[ebx],4);break;                            // ADD EAX,EBX
        case definition:gp[ebx]=v;break;
        case marker:gp[ecx]=v;fl.minus(v,logic::none,4);this->push(0xc3a54u,gp[esi]);break;          // CMP ECX,-1; PUSH ESI
        case structure:gp[edx]=v;gp[ecx]=fl.product(gp[ecx],logic::entry_size);break;                  // IMUL ECX,ECX,6C
        case entries:gp[esi]=v;break;
        case entry_flags:set_low_byte(gp[eax],v);gp[ecx]=fl.plus(gp[ecx],gp[esi],4);fl.logic(v&logic::entry::own_shape,1);break;   // ADD ECX,ESI; TEST AL,1
        case record_kind:                                                                              // DEC EAX; POP ESI; POP EBX; SUB EAX,2
            gp[eax]=fl.increment(v,4,true);gp[esi]=this->pop(0xc3a87u);gp[ebx]=this->pop(0xc3a88u);
            if(gp[eax])gp[eax]=fl.minus(gp[eax],2u,4);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void step(std::uint32_t pc,std::uint32_t target) {
        auto child=[&](State &v){steps(v,target,pc);};
        this->call(pc,pc+5u,target,child);
    }
    // MOV EDX,EDI; MOV EAX,EBX; CALL C3960.
    void from_definition(std::uint32_t definition_address,Kept &kept) {
        auto &gp=this->gp;
        gp[edx]=kept.record;gp[eax]=definition_address;step(0xc3a74u,0xc3960u);kept.record=gp[edi];
    }
    // MOV EDX,EDI; MOV ESI,ECX; CALL C39B0.
    void from_entry(std::uint32_t entry_address,Kept &kept) {
        auto &gp=this->gp;
        gp[edx]=kept.record;gp[esi]=entry_address;step(0xc3a7fu,0xc39b0u);kept.record=gp[edi];
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::light_shape::fill(memory,*this,gp[eax],gp[edi]);
        this->reached(unsigned(outcome));
        const auto slot=unsigned(outcome)%3u;
        const std::uint32_t exit=slot==0u?0xc3aa1u:slot==1u?0xc3a98u:0xc3aa1u;
        this->ret(exit,0u);this->publish(exit);
    }
};
template<class State,class Reference,class Steps> bool bridge(State &s,Reference reference,Steps steps) {
    return run_owner(s,0xc3a20u,0xc3aa1u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Steps>(v,c,t,steps);});
}
} // namespace halo2::native_light_shape_source_detail
