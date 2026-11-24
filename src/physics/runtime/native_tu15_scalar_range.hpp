#pragma once
// Boundary adapter for the scalar range checks, entered at 175280, 175240
// and 1752C0 (native_tu15_finite_checks_systems.hpp, scalar_range). The
// three bodies are laid out alike, so the original instructions are each
// check's entry plus the same offsets. Their callers keep the original ABI,
// which the adapter owns: XMM0 the value; the result in AL (MOV AL,1 or
// XOR AL,AL; EAX above it the value's exponent bits, AND 7F800000); the
// slot the entry PUSH ECX makes holds the value, so ECX returns its bits
// (POP ECX); XMM1 the upper bound once loaded; the flags of the last
// instruction that sets them; RET. Each access publishes its original
// instruction.
// HALO2_REFERENCE_SCALAR_RANGE=1 restores the translated bodies.
// HALO2_VERIFY_SCALAR_RANGE=1 records the native runs and replays the
// unmodified translated bodies against them, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_finite_checks_systems.hpp"

namespace halo2::native_scalar_range_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_finite_checks;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_SCALAR_RANGE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_SCALAR_RANGE");return p && *p;}();
    return value;
}
enum Branch : unsigned {not_finite, below, above, within, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Scalar range verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
// Offsets of the original instructions from each check's entry.
namespace offset {
inline constexpr std::uint32_t spill=1u,reload=6u,low=0x15u,high=0x1eu,pass_pop=0x2du,pass_ret=0x2eu,fail_pop=0x31u,
    fail_ret=0x32u;
}
template<class State> struct ScalarRange : Port<State> {
    using Access=logic::scalar_range::Access;
    std::uint32_t entry;
    ScalarRange(State &s,std::uint32_t *c,Trace<State> *t,std::uint32_t e):Port<State>(s,c,t,{}),entry(e){}
    std::uint32_t slot()const{return this->gp[esp];}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case spill:return entry+offset::spill;
        case reload:return entry+offset::reload;
        case low:return entry+offset::low;
        case high:return entry+offset::high;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        // MOV EAX,[ESP]; AND EAX,7F800000; CMP EAX,7F800000.
        if(a==Access::reload){gp[eax]=this->flags.logic(v&logic::exponent_bits,4);this->flags.minus(gp[eax],logic::exponent_bits,4);}
        else if(a==Access::high)this->s.xmm[1]=logic::sse::scalar(v);                          // MOVSS XMM1
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void compared(std::uint32_t a,std::uint32_t b){this->flags.scalar_compare(logic::sse::scalar(a),logic::sse::scalar(b),false);}
    void run(const logic::scalar_range::Range &range) {
        auto &gp=this->gp;
        this->push(entry,gp[ecx]);                                                                // PUSH ECX: the slot
        Memory<ScalarRange> memory{*this};
        const auto outcome=logic::scalar_range::within(memory,*this,logic::sse::bits(this->s.xmm[0]),range);
        using logic::scalar_range::Outcome;
        this->reached(outcome==Outcome::not_finite?Branch::not_finite:outcome==Outcome::below?Branch::below:
                      outcome==Outcome::above?Branch::above:Branch::within);
        if(outcome==Outcome::within) {
            set_low_byte(gp[eax],1u);gp[ecx]=this->pop(entry+offset::pass_pop);                  // MOV AL,1; POP ECX
            this->ret(entry+offset::pass_ret,0);this->publish(entry+offset::pass_ret);
        } else {
            set_low_byte(gp[eax],this->flags.logic(0u,1));gp[ecx]=this->pop(entry+offset::fail_pop);   // XOR AL,AL; POP ECX
            this->ret(entry+offset::fail_ret,0);this->publish(entry+offset::fail_ret);
        }
    }
};
// check: 0 for 175280, 1 for 175240, 2 for 1752C0.
template<unsigned check,class State,class Reference> bool bridge(State &s,Reference reference) {
    static constexpr std::uint32_t entries[]{0x175280u,0x175240u,0x1752c0u};
    constexpr auto entry=entries[check];
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,entry,entry+offset::fail_ret,cursor,reference_mode()))return false;
    struct Run : ScalarRange<State> {
        using ScalarRange<State>::ScalarRange;
        void run(){ScalarRange<State>::run(logic::scalar_range::ranges[check]);}
    };
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Run(v,c,t,entry);});
    return true;
}
} // namespace halo2::native_scalar_range_detail
