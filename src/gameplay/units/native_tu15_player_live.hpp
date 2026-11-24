#pragma once
// Boundary adapter for the Player live, entered at 15b470
// (native_tu15_player_queries_systems.hpp). Its callers and
// 160570 keep the original ABI, which the adapter owns: ESI the player
// handle; EBX saved (BL zero); 160570 gets ECX the handle, and every
// register it returns comes back; the result in AL (MOV AL,1 or MOV AL,BL)
// above the rest of EAX (the player's address); ECX (NONE past the flag
// tests) and EDX (the flags word) as the original leaves them; the flags of
// the last instruction that sets them; RET. Each access publishes its
// original instruction.
// HALO2_REFERENCE_PLAYER_LIVE=1 restores the translated body.
// HALO2_VERIFY_PLAYER_LIVE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_player_queries_systems.hpp"

namespace halo2::native_player_live_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_player_queries;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_PLAYER_LIVE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_PLAYER_LIVE");return p && *p;}();
    return value;
}
enum Branch : unsigned {inactive, leaving, unmarked, has_unit, barred, unanswered, answered, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Player live verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Child> struct Query : Port<State> {
    Child &child;
    Query(State &s,std::uint32_t *c,Trace<State> *t,Child &k):Port<State>(s,c,t,{}),child(k){}
    using Access=logic::player_live::Access;
    using Kept=logic::player_live::Kept;
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a){case players:return 0x15b470u;case player_datums:return 0x15b476u;case flags:return 0x15b488u;
            case marker:this->gp[ecx]=this->flags.logic(this->gp[ecx]|logic::none,4);return 0x15b49cu;   // OR ECX,-1
            case unit:return 0x15b4a4u;}
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        namespace bits=logic::player_live::player_flags;
        switch(a) {
        case players:gp[ecx]=v;break;
        // MOV EDX,[ECX+44]; MOV EAX,ESI; AND EAX,FFFF; IMUL EAX,EAX,21C; ADD EAX,EDX.
        case player_datums:gp[edx]=v;gp[eax]=fl.plus(fl.product(fl.logic(gp[esi]&0xffffu,4),logic::player_live::player::size),v,4);break;
        // MOV DX,[EAX+2]; PUSH EBX; XOR BL,BL; TEST DL,1; then TEST DL,2.
        case flags:gp[edx]=(gp[edx]&0xffff0000u)|(v&0xffffu);this->push(0x15b48cu,gp[ebx]);set_low_byte(gp[ebx],fl.logic(0u,1));
            this->test8(v&bits::active);if(v&bits::active)this->test8(v&bits::leaving);break;
        case marker:fl.minus(v&0xffu,gp[ecx]&0xffu,1);break;                                         // CMP [EAX+C0],CL
        // CMP [EAX+2C],ECX; then TEST DH,40.
        case unit:fl.minus(v,gp[ecx],4);if(v==logic::none)this->test8((gp[edx]>>8)&(bits::barred>>8));break;
        }
    }
    // MOV ECX,ESI; CALL 160570; TEST AL,AL.
    std::uint32_t ask(Kept &kept,std::uint32_t handle) {
        auto &gp=this->gp;
        gp[ecx]=handle;this->call(0x15b4b4u,0x15b4b9u,0x160570u,child);this->test8(gp[eax]);
        kept.result=gp[ebx]&0xffu;
        return gp[eax]&0xffu;
    }
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::player_live::Outcome;
        Memory<Query> memory{*this};
        const auto result=logic::player_live::test(memory,*this,gp[esi]);
        this->reached(unsigned(result.outcome));
        set_low_byte(gp[eax],result.value);                                                          // MOV AL,1 or MOV AL,BL
        if(result.outcome==Outcome::has_unit){gp[ebx]=this->pop(0x15b4abu);this->ret(0x15b4acu,0);this->publish(0x15b4acu);return;}
        gp[ebx]=this->pop(0x15b4c1u);this->ret(0x15b4c2u,0);this->publish(0x15b4c2u);
    }
    template<class A> void stored(A,std::uint32_t,std::uint32_t){}
};
template<class State,class Reference,class Child> bool bridge(State &s,Reference reference,Child child) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x15b470u,0x15b4c2u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Query<State,Child>(v,c,t,child);});
    return true;
}
} // namespace halo2::native_player_live_detail
