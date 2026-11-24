#pragma once
// Boundary adapter for the unit actions, C9010
// (native_tu15_unit_actions_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: the unit on the stack (RET 4),
// the answer in AL; EBX and ESI saved; C6E90, C7DD0 and C85A0 get the unit
// pushed (RET 4), the others EAX the unit; every register each returns
// coming back. Each access publishes its original instruction.
// HALO2_REFERENCE_UNIT_ACTIONS=1 restores the translated body.
// HALO2_VERIFY_UNIT_ACTIONS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_unit_actions_systems.hpp"

namespace halo2::native_unit_actions_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_unit_actions;
using Access=logic::unit_actions::Access;
using Kept=logic::unit_actions::Kept;
using Update=logic::unit_actions::Update;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_UNIT_ACTIONS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_UNIT_ACTIONS");return v;}
enum Branch : unsigned {unchanged, changed, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Unit actions"};return v;}
// Each update's target, its CALL, and whether the unit is pushed (else in EAX).
struct Call {std::uint32_t target,site;bool pushed;};
inline constexpr Call calls[]{{0xc6e90u,0xc9017u,true},{0xc7dd0u,0xc901fu,true},{0xc85a0u,0xc9027u,true},{0xc8390u,0xc9030u,false},
    {0xc8c20u,0xc9039u,false},{0x117340u,0xc9042u,false},{0xc8e70u,0xc9049u,false},{0xc8cf0u,0xc9050u,false},{0xc7580u,0xc9057u,false}};
template<class State,class Steps> struct Adapter : Port<State> {
    Steps &steps;
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Steps &st):Port<State>(s,c,t,{}),steps(st){}
    std::uint32_t argument()const{return this->gp[esp]+0xcu;}
    std::uint32_t instruction(Access,std::uint32_t){return 0xc9012u;}
    void loaded(Access,std::uint32_t,std::uint32_t v){this->gp[esi]=v;}
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH ESI (the first three) or MOV EAX,ESI; CALL.
    std::uint32_t update(Update which,Kept &kept) {
        auto &gp=this->gp;
        const auto &c=calls[unsigned(which)];
        if(c.pushed)this->push(c.site-(which==Update::controls?1u:3u),kept.unit);
        else gp[eax]=kept.unit;
        auto child=[&](State &v){steps(v,c.target);};
        this->call(c.site,c.site+5u,c.target,child);
        kept={gp[esi],gp[ebx]&0xffu};
        return gp[eax]&0xffu;
    }
    // MOV BL,AL (the first), then OR BL,AL.
    void answered(unsigned k,const Kept &kept) {
        if(k)this->flags.logic(kept.changed,1);
        set_low_byte(this->gp[ebx],kept.changed);
    }
    void run() {
        auto &gp=this->gp;
        this->push(0xc9010u,gp[ebx]);this->push(0xc9011u,gp[esi]);
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::unit_actions::run(memory,*this)));
        gp[esi]=this->pop(0xc905cu);set_low_byte(gp[eax],gp[ebx]);gp[ebx]=this->pop(0xc905fu);       // POP ESI; MOV AL,BL; POP EBX
        this->ret(0xc9060u,4u);this->publish(0xc9060u);
    }
};
template<class State,class Reference,class Steps> bool bridge(State &s,Reference reference,Steps steps) {
    return run_owner(s,0xc9010u,0xc9060u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Steps>(v,c,t,steps);});
}
} // namespace halo2::native_unit_actions_detail
