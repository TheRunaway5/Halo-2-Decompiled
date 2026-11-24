#pragma once
// Boundary adapter for the cluster match, entered at 226B0
// (native_tu15_small_queries_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: ECX and EDX the last values read, AL the answer. Each access publishes
// its original instruction.
// HALO2_REFERENCE_CLUSTER_MATCH=1 restores the translated body.
// HALO2_VERIFY_CLUSTER_MATCH=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_small_queries_detail {
namespace cluster_match {
using Access=logic::cluster_match::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_CLUSTER_MATCH");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_CLUSTER_MATCH");return v;}
enum Branch : unsigned {unplaced, matched, unmatched, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Cluster match"};return v;}
template<class State> struct Adapter : Port<State> {
    bool compared{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        static constexpr std::uint32_t sites[]{0x226b0u,0x226bcu,0x226c6u,0x226d0u,0x226d6u,0x226dcu,0x226dfu};
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case location:gp[ecx]=v;set_low_byte(gp[eax],0u);this->test32(v);break;                // MOV ECX; XOR AL,AL; TEST ECX,ECX
        case clusters:case structure:gp[edx]=v;this->test32(v);break;
        case cluster:case name:gp[ecx]=v;break;
        case present:this->compare32(v,logic::none);break;
        case structure_name:this->compare32(gp[ecx],v);compared=true;break;                     // CMP ECX,[EDX+8]
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        Memory<Adapter> memory{*this};
        const bool answer=logic::cluster_match::matches(memory,*this);
        if(answer)set_low_byte(this->gp[eax],1u);
        this->reached(answer?Branch::matched:compared?Branch::unmatched:Branch::unplaced);
        this->ret(0x226e6u,0u);this->publish(0x226e6u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x226b0u,0x226e6u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace cluster_match
} // namespace halo2::native_small_queries_detail
