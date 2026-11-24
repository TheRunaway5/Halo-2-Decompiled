#pragma once
// Boundary adapter for the variant option, entered at 161530
// (native_tu15_tiny_leaves_systems.hpp). Its callers keep the
// original ABI, which the adapter owns: the result in AL (XOR AL,AL; or the
// option byte AND 1 above the game globals' address); ECX and EDX as the
// original leaves them; the flags of the last instruction that sets them;
// RET.
// HALO2_REFERENCE_TINY_LEAVES=1 restores the translated body.
// HALO2_VERIFY_TINY_LEAVES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_tiny_leaves_systems.hpp"

namespace halo2::native_variant_option_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_tiny_leaves;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_TINY_LEAVES");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_TINY_LEAVES");return p && *p;}();
    return value;
}
enum Branch : unsigned {unvaried, varied, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Tiny leaves (variant option) verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct Leaf : Port<State> {
    Leaf(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    using Access=logic::variant_option::Access;
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a){case session:return 0x161530u;case variant_index:return 0x161536u;case variant:return 0x16153cu;
            case game_state:return 0x161549u;case option:return 0x16154eu;}
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case session:gp[ecx]=v;break;
        case variant_index:gp[edx]=v;break;
        case variant:gp[ecx]=v;set_low_byte(gp[eax],fl.logic(0u,1));this->test32(v);break;          // XOR AL,AL; TEST ECX,ECX
        case game_state:gp[eax]=v;break;
        case option:fl.logic(v&logic::variant_option::option_bit,1);break;                         // MOV AL,[EAX+184]; AND AL,1
        }
    }
    void run() {
        auto &gp=this->gp;
        Memory<Leaf> memory{*this};
        const auto result=logic::variant_option::flag(memory);
        set_low_byte(gp[eax],result.value);
        this->reached(result.varied?Branch::varied:Branch::unvaried);
        this->ret(0x161556u,0);this->publish(0x161556u);
    }
    template<class A> void stored(A,std::uint32_t,std::uint32_t){}
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x161530u,0x161556u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Leaf<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_variant_option_detail
