#pragma once
// Boundary adapter for the code class, entered at DF820
// (native_tu15_code_class_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: ECX the code (the subtractions leave their
// remainder in it); the class in AL (MOV AL,2 first; XOR AL,AL or MOV AL,1)
// above the rest of EAX; the flags of the last instruction that sets them;
// RET. The routine reads no memory.
// HALO2_REFERENCE_CODE_CLASS=1 restores the translated body.
// HALO2_VERIFY_CODE_CLASS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_code_class_systems.hpp"

namespace halo2::native_code_class_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_code_class;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_CODE_CLASS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_CODE_CLASS");return p && *p;}();
    return value;
}
enum Branch : unsigned {class_zero, class_one, class_two, above, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Code class verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct CodeClass : Port<State> {
    CodeClass(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    void compared(std::uint32_t code,std::uint32_t constant){this->flags.minus(code,constant,4);}  // CMP ECX,constant
    std::uint32_t subtracted(std::uint32_t rest,std::uint32_t constant) {                          // SUB ECX,constant
        this->gp[ecx]=this->flags.minus(rest,constant,4);return this->gp[ecx];
    }
    void run() {
        auto &gp=this->gp;
        using logic::code_class::Step;
        set_low_byte(gp[eax],2u);                                                                   // MOV AL,2 (after the first CMP)
        const auto result=logic::code_class::of(*this,gp[ecx]);
        std::uint32_t exit=0xdf872u;
        switch(result.value) {
        case 0u:set_low_byte(gp[eax],this->flags.logic(0u,1));exit=0xdf85au;this->reached(Branch::class_zero);break;  // XOR AL,AL
        case 1u:set_low_byte(gp[eax],1u);this->reached(Branch::class_one);break;                    // MOV AL,1
        default:
            this->reached(Branch::class_two);
            if(result.step==Step::low_other)exit=0xdf846u;
            else if(result.step==Step::middle_high)exit=0xdf857u;
        }
        if(result.step==Step::above)this->reached(Branch::above);
        this->ret(exit,0);this->publish(exit);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xdf820u,0xdf872u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return CodeClass<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_code_class_detail
