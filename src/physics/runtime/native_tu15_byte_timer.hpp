#pragma once
// Boundary adapter for the byte timer step, entered at 1E15D0
// (native_tu15_byte_timer_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: ECX the timer; EBX saved (BL the limit, or the
// count down meanwhile); the result in AL (XOR AL,AL; MOV AL,1) above the
// rest of EAX; DL the flags byte (with done set by OR DL,2) above the rest
// of EDX; the flags of the last instruction that sets them; RET. Each access
// publishes its original instruction.
// HALO2_REFERENCE_BYTE_TIMER=1 restores the translated body.
// HALO2_VERIFY_BYTE_TIMER=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_byte_timer_systems.hpp"

namespace halo2::native_byte_timer_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_byte_timer;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_BYTE_TIMER");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_BYTE_TIMER");return p && *p;}();
    return value;
}
enum Branch : unsigned {idle, finished, up, up_done, down, down_done, branch_count};
static_assert(unsigned(logic::byte_timer::Outcome::down_done)==Branch::down_done);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Byte timer verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct ByteTimer : Port<State> {
    using Access=logic::byte_timer::Access;
    ByteTimer(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    // OR DL,2 before the done stores.
    void mark_done(){set_low_byte(this->gp[edx],this->flags.logic((this->gp[edx]|logic::timer_flags::done)&0xffu,1));}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case limit:this->push(0x1e15d0u,this->gp[ebx]);return 0x1e15d1u;                           // PUSH EBX
        case flags:return 0x1e15dau;
        case up_count:return 0x1e15e7u;
        case up_store:return 0x1e15edu;
        case up_done_count:mark_done();return 0x1e15f4u;
        case up_done_flags:return 0x1e15f6u;
        case down_count:return 0x1e15fdu;
        case down_store:return 0x1e1605u;
        case down_done_count:mark_done();return 0x1e160cu;
        case down_done_flags:return 0x1e160fu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        // MOV BL,[ECX+1]; XOR AL,AL; TEST BL,BL.
        case limit:set_low_byte(gp[ebx],v);set_low_byte(gp[eax],fl.logic(0u,1));this->test8(v);break;
        // MOV DL,[ECX+3]; TEST DL,2; then TEST DL,1 while not done.
        case flags:set_low_byte(gp[edx],v);this->test8(v&logic::timer_flags::done);
            if(!(v&logic::timer_flags::done))this->test8(v&logic::timer_flags::counts_up);break;
        // MOV AL,[ECX]; INC AL; CMP AL,BL.
        case up_count:set_low_byte(gp[eax],fl.increment(v&0xffu,1,false));fl.minus(gp[eax]&0xffu,gp[ebx]&0xffu,1);break;
        // MOV BL,[ECX]; DEC BL; MOV AL,BL; TEST AL,AL.
        case down_count:set_low_byte(gp[ebx],fl.increment(v&0xffu,1,true));set_low_byte(gp[eax],gp[ebx]);this->test8(gp[eax]);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::byte_timer::Outcome;
        Memory<ByteTimer> memory{*this};
        const auto outcome=logic::byte_timer::step(memory,gp[ecx]);
        this->reached(unsigned(outcome));
        if(outcome==Outcome::up_done) {
            set_low_byte(gp[eax],1u);gp[ebx]=this->pop(0x1e15fbu);this->ret(0x1e15fcu,0);this->publish(0x1e15fcu);return;
        }
        if(outcome!=Outcome::idle && outcome!=Outcome::finished)set_low_byte(gp[eax],1u);           // MOV AL,1 at 1E1612
        gp[ebx]=this->pop(0x1e1614u);this->ret(0x1e1615u,0);this->publish(0x1e1615u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1e15d0u,0x1e1615u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return ByteTimer<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_byte_timer_detail
