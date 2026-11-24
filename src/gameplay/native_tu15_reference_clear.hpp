#pragma once
// Boundary adapter for the reference clear, entered at 1E5A50
// (native_tu15_small_queries_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: ECX NONE (OR ECX,-1). Each access publishes
// its original instruction.
// HALO2_REFERENCE_REFERENCE_CLEAR=1 restores the translated body.
// HALO2_VERIFY_REFERENCE_CLEAR=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_small_queries_detail {
namespace reference_clear {
using Access=logic::reference_clear::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_REFERENCE_CLEAR");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_REFERENCE_CLEAR");return v;}
enum Branch : unsigned {cleared, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Reference clear"};return v;}
template<class State> struct Adapter : Port<State> {
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        static constexpr std::uint32_t sites[]{0x1e5a53u,0x1e5a55u,0x1e5a58u,0x1e5a5cu,0x1e5a62u,0x1e5a65u};
        return sites[unsigned(a)];
    }
    void loaded(Access,std::uint32_t,std::uint32_t){}
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        this->gp[ecx]=this->flags.logic(0xffffffffu,4);                                         // OR ECX,-1
        Memory<Adapter> memory{*this};
        logic::reference_clear::clear(memory,*this,this->gp[eax]);
        this->reached(Branch::cleared);
        this->ret(0x1e5a68u,0u);this->publish(0x1e5a68u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x1e5a50u,0x1e5a68u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace reference_clear
} // namespace halo2::native_small_queries_detail
