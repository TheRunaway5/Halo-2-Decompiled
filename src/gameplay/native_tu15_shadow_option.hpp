#pragma once
// Boundary adapter for the shadow option, entered at 335B0
// (native_tu15_small_queries_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: EAX the state, ECX the gate byte, AL the answer. Each access publishes
// its original instruction.
// HALO2_REFERENCE_SHADOW_OPTION=1 restores the translated body.
// HALO2_VERIFY_SHADOW_OPTION=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_small_queries_detail {
namespace shadow_option {
using Access=logic::shadow_option::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_SHADOW_OPTION");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_SHADOW_OPTION");return v;}
enum Branch : unsigned {stateless, ungated, gated, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Shadow option"};return v;}
template<class State> struct Adapter : Port<State> {
    bool gate_set{},stateful{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a){case state:return 0x335b0u;case gate:return 0x335b9u;case option:return 0x335beu;}
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case state:gp[eax]=v;this->test32(v);stateful=v;break;                                   // TEST EAX,EAX
        case gate:set_low_byte(gp[ecx],v);this->test8(v);gate_set=v&0xffu;break;                 // MOV CL; TEST CL,CL
        case option:set_low_byte(gp[eax],v);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        Memory<Adapter> memory{*this};
        const auto answer=logic::shadow_option::enabled(memory,*this);
        set_low_byte(this->gp[eax],answer);
        this->reached(!stateful?Branch::stateless:gate_set?Branch::gated:Branch::ungated);
        this->ret(0x335c7u,0u);this->publish(0x335c7u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x335b0u,0x335c7u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace shadow_option
} // namespace halo2::native_small_queries_detail
