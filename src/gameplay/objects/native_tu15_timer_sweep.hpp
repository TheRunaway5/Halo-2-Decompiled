#pragma once
// Boundary adapter for the timer sweep, entered at 115FF0
// (native_tu15_timer_sweep_systems.hpp). Its callers and 1E15D0 keep the
// original ABI, which the adapter owns: EAX the object handle; EBX and ESI
// saved (BL gathers the answers, MOV BL,AL then OR BL,AL); 1E15D0 gets ECX
// the timer, and every register it returns comes back; the result in AL
// (OR AL,BL) above the rest of EAX as the last call leaves it; ECX and EDX
// as the last call leaves them; the flags of the last instruction that sets
// them; RET. Each access publishes its original instruction.
// HALO2_REFERENCE_TIMER_SWEEP=1 restores the translated body.
// HALO2_VERIFY_TIMER_SWEEP=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_timer_sweep_systems.hpp"

namespace halo2::native_timer_sweep_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_timer_sweep;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_TIMER_SWEEP");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_TIMER_SWEEP");return p && *p;}();
    return value;
}
enum Branch : unsigned {quiet, answered, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Timer sweep verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Step> struct TimerSweep : Port<State> {
    using Access=logic::timer_sweep::Access;
    using Kept=logic::timer_sweep::Kept;
    Step &step_child;
    std::uint32_t calls{};
    TimerSweep(State &s,std::uint32_t *c,Trace<State> *t,Step &k):Port<State>(s,c,t,{}),step_child(k){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0x115ff0u;
        case header_datums:return 0x115ff6u;
        case object_address:return 0x116001u;
        case block:this->push(0x116005u,this->gp[ebx]);this->push(0x116006u,this->gp[esi]);return 0x116007u;   // PUSH EBX; PUSH ESI
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[ecx]=v;break;
        case header_datums:gp[edx]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;break;                 // AND EAX,FFFF; LEA
        case object_address:gp[eax]=v;break;
        case block:gp[esi]=fl.plus(logic::widen16(v),gp[eax],4);break;                             // MOVSX ESI; ADD ESI,EAX
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // LEA ECX,[ESI+80+4k]; CALL 1E15D0.
    std::uint32_t step(Kept &kept,std::uint32_t timer) {
        auto &gp=this->gp;
        static constexpr std::uint32_t sites[]{0x116016u,0x116023u,0x116030u,0x11603du,0x11604au};
        gp[ecx]=timer;this->call(sites[calls],sites[calls]+5u,0x1e15d0u,step_child);++calls;
        kept={gp[esi],gp[ebx]&0xffu};
        return gp[eax];
    }
    // MOV BL,AL after the first, OR BL,AL after the others.
    void gathered(const Kept &kept,std::uint32_t k) {
        auto &gp=this->gp;
        set_low_byte(gp[ebx],k==0?kept.answers:this->flags.logic(kept.answers,1));
    }
    void run() {
        auto &gp=this->gp;
        Memory<TimerSweep> memory{*this};
        const auto result=logic::timer_sweep::step(memory,*this,gp[eax]);
        this->reached(result?Branch::answered:Branch::quiet);
        gp[esi]=this->pop(0x11604fu);set_low_byte(gp[eax],this->flags.logic(result,1));gp[ebx]=this->pop(0x116052u);   // POP ESI; OR AL,BL; POP EBX
        this->ret(0x116053u,0);this->publish(0x116053u);
    }
};
template<class State,class Reference,class Step> bool bridge(State &s,Reference reference,Step step) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x115ff0u,0x116053u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return TimerSweep<State,Step>(v,c,t,step);});
    return true;
}
} // namespace halo2::native_timer_sweep_detail
