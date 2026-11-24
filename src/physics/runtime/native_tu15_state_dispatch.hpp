#pragma once
// Boundary adapter for the state dispatch, 1EDDA0
// (native_tu15_state_dispatch_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EDX the state, ECX the
// owner, the argument on the stack (RET 4); EBX and ESI saved (and EDI for
// 1F5230); 1F9720 gets the owner pushed (RET 4), 202CE0 the body pushed
// (RET 4), 1F5230 ESI and EDI; every register each returns coming back. The
// jump table's entry goes through the unit's canonical branch mapping, so an
// unbound entry raises the translated body's fault. Each access publishes its
// original instruction.
// HALO2_REFERENCE_STATE_DISPATCH=1 restores the translated body.
// HALO2_VERIFY_STATE_DISPATCH=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_state_dispatch_systems.hpp"
#include <stdexcept>

namespace halo2::native_state_dispatch_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_state_dispatch;
using Access=logic::state_dispatch::Access;
using Outcome=logic::state_dispatch::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_STATE_DISPATCH");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_STATE_DISPATCH");return v;}
enum Branch : unsigned {first, nothing, second, third, branch_count};
static_assert(unsigned(Outcome::third)==Branch::third);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"State dispatch"};return v;}
inline constexpr std::uint32_t sites[]{0x1edda1u,0x1edda6u,0x1eddaau};
template<class State,class Branches,class Steps> struct Adapter : Port<State> {
    Branches &branch;
    Steps &steps;
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Branches &b,Steps &st):Port<State>(s,c,t,{}),branch(b),steps(st){}
    std::uint32_t argument()const{return this->gp[esp]+8u;}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case argument:gp[ebx]=v;this->push(0x1edda5u,gp[esi]);break;                                  // PUSH ESI
        case kind:gp[esi]=this->flags.increment(v&0xffu,4,true);break;                                // MOVZX ESI,byte; DEC ESI
        case kind_case:(void)branch(v,this->s.image);break;                                           // JMP [1EDDE0+ESI*4]
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void step(std::uint32_t pc,std::uint32_t target) {
        auto child=[&](State &v){steps(v,target,pc);};
        this->call(pc,pc+5u,target,child);
    }
    // PUSH ECX; ADD EDX,10; CALL 1F9720.
    void first(std::uint32_t,std::uint32_t body) {
        auto &gp=this->gp;
        this->push(0x1eddb1u,gp[ecx]);gp[edx]=this->flags.plus(body-logic::state_body,logic::state_body,4);
        step(0x1eddb5u,0x1f9720u);
    }
    // ADD EDX,10; PUSH EDX; CALL 202CE0.
    void second(std::uint32_t body) {
        auto &gp=this->gp;
        gp[edx]=this->flags.plus(body-logic::state_body,logic::state_body,4);this->push(0x1eddc2u,gp[edx]);
        step(0x1eddc3u,0x202ce0u);
    }
    // PUSH EDI; MOV ESI,ECX; MOV EDI,EBX; CALL 1F5230.
    void third(std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        this->push(0x1eddcdu,gp[edi]);gp[esi]=gp[ecx];gp[edi]=gp[ebx];
        step(0x1eddd2u,0x1f5230u);
    }
    [[noreturn]] void unknown_case(){throw std::runtime_error("Required original solver dispatch target unavailable");}
    void run() {
        auto &gp=this->gp;
        this->push(0x1edda0u,gp[ebx]);
        Memory<Adapter> memory{*this};
        const auto outcome=logic::state_dispatch::run(memory,*this,gp[edx],gp[ecx]);
        this->reached(unsigned(outcome));
        std::uint32_t exit;
        switch(outcome) {
        case Outcome::second:exit=0x1eddc8u;break;
        case Outcome::third:gp[edi]=this->pop(0x1eddd7u);exit=0x1eddd8u;break;
        default:exit=0x1eddbau;break;
        }
        gp[esi]=this->pop(exit);gp[ebx]=this->pop(exit+1u);
        this->ret(exit+2u,4u);this->publish(exit+2u);
    }
};
template<class State,class Reference,class Branches,class Steps> bool bridge(State &s,Reference reference,Branches branch,Steps steps) {
    return run_owner(s,0x1edda0u,0x1eddd8u+2u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Branches,Steps>(v,c,t,branch,steps);});
}
} // namespace halo2::native_state_dispatch_detail
