#pragma once
// Boundary adapter for the animation fallbacks, entered at 1E5F20
// (native_tu15_animation_fallbacks_systems.hpp). Its callers and 1E5E40
// keep the original ABI, which the adapter owns: AL the forced byte, ESI and
// EDI passed on, the three arguments on the stack (RET C); EBX (BL the
// forced byte meanwhile) and EBP (the second argument) saved; 1E5E40 gets
// its five arguments pushed (and pops them), and every register it returns
// comes back; EAX, ECX and EDX (the last argument read) as the original
// leaves them; the flags of the last instruction that sets them. Each
// access publishes its original instruction.
// HALO2_REFERENCE_ANIMATION_FALLBACKS=1 restores the translated body.
// HALO2_VERIFY_ANIMATION_FALLBACKS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_animation_fallbacks_systems.hpp"

namespace halo2::native_animation_fallbacks_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_animation_fallbacks;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ANIMATION_FALLBACKS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ANIMATION_FALLBACKS");return p && *p;}();
    return value;
}
enum Branch : unsigned {skipped, forced, by_argument, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Animation fallbacks verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
// Each lookup's instructions: the two argument reads, the five pushes, the call.
struct Sites {std::uint32_t third,first,push[5],call;};
inline constexpr Sites sites[logic::lookups]{
    {0x1e5f34u,0x1e5f38u,{0x1e5f3cu,0x1e5f3du,0x1e5f3eu,0x1e5f3fu,0x1e5f40u},0x1e5f41u},
    {0x1e5f46u,0x1e5f4au,{0x1e5f4eu,0x1e5f4fu,0x1e5f54u,0x1e5f55u,0x1e5f56u},0x1e5f57u},
    {0x1e5f5cu,0u,{0x1e5f60u,0x1e5f61u,0x1e5f62u,0x1e5f67u,0x1e5f68u},0x1e5f69u},
    {0x1e5f6eu,0u,{0x1e5f72u,0x1e5f73u,0x1e5f78u,0x1e5f7du,0x1e5f7eu},0x1e5f7fu},
    {0x1e5f84u,0u,{0x1e5f88u,0x1e5f89u,0x1e5f8eu,0x1e5f93u,0x1e5f98u},0x1e5f99u}};
// The register each lookup reads the third and first arguments into.
inline constexpr std::uint32_t third_registers[logic::lookups]{eax,edx,ecx,edx,eax},first_registers[logic::lookups]{ecx,eax,0,0,0};
template<class State,class Lookup> struct AnimationFallbacks : Port<State> {
    using Access=logic::animation_fallbacks::Access;
    using Kept=logic::animation_fallbacks::Kept;
    Lookup &lookup_child;
    std::uint32_t k{};
    AnimationFallbacks(State &s,std::uint32_t *c,Trace<State> *t,Lookup &l):Port<State>(s,c,t,{}),lookup_child(l){}
    // The k-th argument past the return address, EBX and EBP.
    std::uint32_t argument_slot(std::uint32_t index)const{return this->gp[esp]+0xcu+4u*index;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case second:return 0x1e5f26u;
        case forced_byte:return 0x1e5f2cu;
        case third:return sites[k].third;
        case first:return sites[k].first;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case second:gp[ebp]=v;break;
        case forced_byte:set_low_byte(gp[eax],v);this->test8(v);break;                             // MOV AL,[ESP+14]; TEST AL,AL
        case third:gp[third_registers[k]]=v;break;
        case first:gp[first_registers[k]]=v;break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH c; PUSH b; PUSH a; PUSH EDI (or any); PUSH ESI; CALL 1E5E40.
    void lookup(Kept &kept,std::uint32_t index,bool own_edi,std::uint32_t a,std::uint32_t b,std::uint32_t c) {
        auto &gp=this->gp;
        const auto &site=sites[index];
        const std::uint32_t values[5]{c,b,a,own_edi?gp[edi]:logic::any,gp[esi]};
        for(std::uint32_t p=0;p<5;++p)this->push(site.push[p],values[p]);
        this->call(site.call,site.call+5u,0x1e5e40u,lookup_child);
        kept.second=gp[ebp];k=index+1u;
    }
    void run() {
        auto &gp=this->gp;
        // PUSH EBX; MOV BL,AL; TEST BL,BL; PUSH EBP.
        this->push(0x1e5f20u,gp[ebx]);set_low_byte(gp[ebx],gp[eax]);this->test8(gp[eax]);this->push(0x1e5f25u,gp[ebp]);
        const auto forced_value=gp[eax]&0xffu;
        Memory<AnimationFallbacks> memory{*this};
        const bool applied=logic::animation_fallbacks::apply(memory,*this,forced_value);
        this->reached(!applied?Branch::skipped:forced_value?Branch::forced:Branch::by_argument);
        gp[ebp]=this->pop(0x1e5f9eu);gp[ebx]=this->pop(0x1e5f9fu);this->ret(0x1e5fa0u,0xcu);this->publish(0x1e5fa0u);
    }
};
template<class State,class Reference,class Lookup> bool bridge(State &s,Reference reference,Lookup lookup) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1e5f20u,0x1e5fa0u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return AnimationFallbacks<State,Lookup>(v,c,t,lookup);});
    return true;
}
} // namespace halo2::native_animation_fallbacks_detail
