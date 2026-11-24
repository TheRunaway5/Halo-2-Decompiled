#pragma once
// Boundary adapter for the player timers, 154D00
// (native_tu15_player_timers_systems.hpp). Its callers and the child keep
// the original ABI, which the adapter owns: EAX the player; EBX, EBP, ESI
// and EDI saved; D3340 gets EAX the unit and XMM0 the factor (XMM1 holds
// it), every register it returns coming back. Each access publishes its
// original instruction.
// HALO2_REFERENCE_PLAYER_TIMERS=1 restores the translated body.
// HALO2_VERIFY_PLAYER_TIMERS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/players/native_tu15_player_timers_systems.hpp"

namespace halo2::native_player_timers_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_player_timers;
using Access=logic::player_timers::Access;
using Kept=logic::player_timers::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_PLAYER_TIMERS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_PLAYER_TIMERS");return v;}
// The outcome, then each timer's: counted down, expired (D3340 told).
enum Branch : unsigned {ticked, counted, expired, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Player timers"};return v;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x154d00u,0x154d13u,0x154d1eu,0x154d32u,0x154d3eu,0x154d47u,0x154d4du,0x154d50u};
template<class State,class Expired> struct Adapter : Port<State> {
    Expired &expired_child;
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Expired &e):Port<State>(s,c,t,{}),expired_child(e){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        if(a==Access::timer)this->gp[eax]=this->flags.logic(0u,4);                                     // XOR EAX,EAX
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case one:                                                                                     // PUSH EBX; PUSH EBP; AND EAX,FFFF; PUSH ESI; PUSH EDI; MOV EDI,EAX
            this->s.xmm[1]=lane0_only(v);this->push(0x154d08u,gp[ebx]);this->push(0x154d09u,gp[ebp]);
            gp[eax]=fl.logic(gp[eax]&0xffffu,4);this->push(0x154d0fu,gp[esi]);this->push(0x154d10u,gp[edi]);gp[edi]=gp[eax];break;
        case players:gp[eax]=v;gp[edi]=fl.product(gp[edi],logic::player_size);break;                 // IMUL EDI,EDI,21C
        case player_data:                                                                             // ADD EAX,EDI; XOR EBX,EBX; LEA ESI,[EAX+198]; MOV EBP,2
            gp[eax]=fl.plus(v,gp[edi],4);gp[ebx]=fl.logic(0u,4);gp[esi]=gp[eax]+logic::player::timers;gp[ebp]=logic::timer_count;break;
        case timer:                                                                                   // TEST AX,AX (then DEC EAX; TEST AX,AX)
            set_low_word(gp[eax],v);fl.logic(v&0xffffu,2);
            if(std::int16_t(v)>0){gp[eax]=fl.increment(gp[eax],4,true);fl.logic(gp[eax]&0xffffu,2);this->reached(Branch::counted);}
            break;
        case timer_store:break;
        case players_again:gp[ecx]=v;break;
        case player_data_again:gp[edx]=v;break;
        case unit:gp[eax]=v;break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v) {
        if(a==Access::timer_store && !(v&0xffffu))this->test32(this->gp[ebx]);                       // TEST EBX,EBX
    }
    // MOVAPS XMM0,XMM1; CALL D3340.
    void expired(std::uint32_t,std::uint32_t,Kept &kept) {
        auto &gp=this->gp;
        this->s.xmm[0]=this->s.xmm[1];
        this->call_routed(expired_child,0x154d57u,0x154d5cu,0xd3340u);this->reached(Branch::expired);
        kept={gp[ebx],gp[esi],gp[ebp],gp[edi]};
    }
    // INC EBX; ADD ESI,2; DEC EBP.
    void next(const Kept &) {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ebx]=fl.increment(gp[ebx],4,false);gp[esi]=fl.plus(gp[esi],logic::timer_size,4);gp[ebp]=fl.increment(gp[ebp],4,true);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::player_timers::tick(memory,*this,gp[eax])));
        gp[edi]=this->pop(0x154d63u);gp[esi]=this->pop(0x154d64u);gp[ebp]=this->pop(0x154d65u);gp[ebx]=this->pop(0x154d66u);
        this->ret(0x154d67u,0u);this->publish(0x154d67u);
    }
};
template<class State,class Reference,class Expired> bool bridge(State &s,Reference reference,Expired expired) {
    return run_owner(s,0x154d00u,0x154d67u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Expired>(v,c,t,expired);});
}
} // namespace halo2::native_player_timers_detail
