#pragma once
// Boundary adapter for the word change, entered at 25BC70
// (native_tu15_small_queries_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: ECX the first word, AL the answer (SETNZ), RET 8. Each access publishes
// its original instruction.
// HALO2_REFERENCE_WORD_CHANGE=1 restores the translated body.
// HALO2_VERIFY_WORD_CHANGE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_small_queries_detail {
namespace word_change {
using Access=logic::word_change::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_WORD_CHANGE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_WORD_CHANGE");return v;}
enum Branch : unsigned {same, changed, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Word change"};return v;}
template<class State> struct Adapter : Port<State> {
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+4u*k;}
    std::uint32_t instruction(Access a,std::uint32_t){return a==Access::first?0x25bc70u:0x25bc77u;}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        if(a==Access::first){gp[ecx]=(gp[ecx]&0xffff0000u)|(v&0xffffu);gp[eax]=this->flags.logic(0u,4);}   // MOV CX; XOR EAX,EAX
        else this->flags.minus(gp[ecx]&0xffffu,v,2);                                                     // CMP CX,[ESP+8]
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        Memory<Adapter> memory{*this};
        const bool answer=logic::word_change::changed(memory,*this);
        set_low_byte(this->gp[eax],answer?1u:0u);                                                     // SETNZ AL
        this->reached(answer?Branch::changed:Branch::same);
        this->ret(0x25bc7fu,8u);this->publish(0x25bc7fu);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x25bc70u,0x25bc7fu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace word_change
} // namespace halo2::native_small_queries_detail
