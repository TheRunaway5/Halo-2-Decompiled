#pragma once
// Boundary adapter for the Uncontrolled unit, entered at c6e50
// (native_tu15_player_queries_systems.hpp). Its callers and
// C6DD0 keep the original ABI, which the adapter owns: ECX the unit handle;
// EBX and ESI saved (BL 1, ESI the unit's player meanwhile); C6DD0 gets ECX
// the handle, EAX the unit, EDX NONE, and every register it returns comes
// back; the result in AL (XOR AL,AL or MOV AL,BL) above the rest of EAX; EDX
// as the original leaves it; the flags of the last instruction that sets
// them; RET. Each access publishes its original instruction.
// HALO2_REFERENCE_UNCONTROLLED_UNIT=1 restores the translated body.
// HALO2_VERIFY_UNCONTROLLED_UNIT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_player_queries_systems.hpp"

namespace halo2::native_uncontrolled_unit_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_player_queries;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_UNCONTROLLED_UNIT");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_UNCONTROLLED_UNIT");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_player, unparented, answered, unanswered, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Uncontrolled unit verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Child> struct Query : Port<State> {
    Child &child;
    Query(State &s,std::uint32_t *c,Trace<State> *t,Child &k):Port<State>(s,c,t,{}),child(k){}
    using Access=logic::uncontrolled_unit::Access;
    using Kept=logic::uncontrolled_unit::Kept;
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a){case headers:return 0xc6e50u;case header_datums:return 0xc6e56u;case object_address:return 0xc6e64u;
            case player:return 0xc6e69u;case parent:return 0xc6e78u;}
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[edx]=v;break;
        // MOV EDX,[EDX+44]; MOV EAX,ECX; AND EAX,FFFF; PUSH EBX; LEA EAX,[EAX+EAX*2].
        case header_datums:gp[edx]=v;gp[eax]=fl.logic(gp[ecx]&0xffffu,4);this->push(0xc6e60u,gp[ebx]);gp[eax]*=3u;break;
        case object_address:gp[eax]=v;this->push(0xc6e68u,gp[esi]);break;                          // PUSH ESI
        // MOV ESI,[EAX+13C]; OR EDX,-1; CMP ESI,EDX; MOV BL,1.
        case player:gp[esi]=v;gp[edx]=fl.logic(gp[edx]|logic::none,4);fl.minus(v,gp[edx],4);set_low_byte(gp[ebx],1u);break;
        case parent:fl.minus(v,gp[edx],4);break;                                                    // CMP [EAX+14],EDX
        }
    }
    // CALL C6DD0; TEST AL,AL.
    std::uint32_t ask(Kept &kept) {
        auto &gp=this->gp;
        this->call(0xc6e7du,0xc6e82u,0xc6dd0u,child);this->test8(gp[eax]);
        kept.result=gp[ebx]&0xffu;
        return gp[eax]&0xffu;
    }
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::uncontrolled_unit::Outcome;
        Memory<Query> memory{*this};
        const auto result=logic::uncontrolled_unit::test(memory,*this,gp[ecx]);
        this->reached(unsigned(result.outcome));
        if(result.outcome==Outcome::unparented || result.outcome==Outcome::answered) {
            gp[esi]=this->pop(0xc6e86u);set_low_byte(gp[eax],this->flags.logic(result.value,1));gp[ebx]=this->pop(0xc6e89u);
            this->ret(0xc6e8au,0);this->publish(0xc6e8au);return;                                   // XOR AL,AL
        }
        gp[esi]=this->pop(0xc6e8bu);set_low_byte(gp[eax],result.value);gp[ebx]=this->pop(0xc6e8eu);  // MOV AL,BL
        this->ret(0xc6e8fu,0);this->publish(0xc6e8fu);
    }
    template<class A> void stored(A,std::uint32_t,std::uint32_t){}
};
template<class State,class Reference,class Child> bool bridge(State &s,Reference reference,Child child) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xc6e50u,0xc6e8fu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Query<State,Child>(v,c,t,child);});
    return true;
}
} // namespace halo2::native_uncontrolled_unit_detail
