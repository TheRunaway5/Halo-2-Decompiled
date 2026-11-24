#pragma once
// Boundary adapter for the finite vector check, entered at B16C0
// (native_tu15_finite_checks_systems.hpp, finite_vector). Its callers keep
// the original ABI, which the adapter owns: EAX the vector; the result in
// EAX (MOV EAX,1 or XOR EAX,EAX); each component passes through the slot
// the entry PUSH ECX makes and its exponent bits land in ECX, EDX, then EAX
// (MOV, AND 7F800000); ECX returns the slot's bits (POP ECX); XMM0 the last
// component read; the flags of the last instruction that sets them; RET.
// Each access publishes its original instruction.
// HALO2_REFERENCE_FINITE_VECTOR=1 restores the translated body.
// HALO2_VERIFY_FINITE_VECTOR=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_finite_checks_systems.hpp"

namespace halo2::native_finite_vector_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_finite_checks;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_FINITE_VECTOR");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_FINITE_VECTOR");return p && *p;}();
    return value;
}
enum Branch : unsigned {x_not_finite, y_not_finite, z_not_finite, finite, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Finite vector verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct FiniteVector : Port<State> {
    using Access=logic::finite_vector::Access;
    std::uint32_t k{};
    FiniteVector(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t slot()const{return this->gp[esp];}
    void component(std::uint32_t index){k=index;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        static constexpr std::uint32_t components[]{0xb16c1u,0xb16dbu,0xb16f6u},spills[]{0xb16c5u,0xb16e0u,0xb16fbu},
            reloads[]{0xb16cau,0xb16e5u,0xb1700u};
        using enum Access;
        switch(a) {
        case component:return components[k];
        case spill:return spills[k];
        case reload:return reloads[k];
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        static constexpr unsigned registers[]{ecx,edx,eax};
        if(a==Access::component)this->s.xmm[0]=logic::sse::scalar(v);                         // MOVSS XMM0
        else if(a==Access::reload) {
            // MOV reg,[ESP]; AND reg,7F800000; CMP reg,7F800000.
            auto &r=this->gp[registers[k]];
            r=this->flags.logic(v&logic::exponent_bits,4);this->flags.minus(r,logic::exponent_bits,4);
            if(!logic::finite(v))this->reached(Branch::x_not_finite+k);
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        this->push(0xb16c0u,gp[ecx]);                                                             // PUSH ECX: the slot
        Memory<FiniteVector> memory{*this};
        if(logic::finite_vector::finite(memory,*this,gp[eax])) {
            this->reached(Branch::finite);
            gp[eax]=1u;gp[ecx]=this->pop(0xb1714u);this->ret(0xb1715u,0);this->publish(0xb1715u);  // MOV EAX,1; POP ECX
        } else {
            gp[eax]=this->flags.logic(0u,4);gp[ecx]=this->pop(0xb1718u);                           // XOR EAX,EAX; POP ECX
            this->ret(0xb1719u,0);this->publish(0xb1719u);
        }
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xb16c0u,0xb1719u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return FiniteVector<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_finite_vector_detail
