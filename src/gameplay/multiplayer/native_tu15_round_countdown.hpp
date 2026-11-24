#pragma once
// Boundary adapter for the round countdown, 15E870
// (native_tu15_round_countdown_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: no arguments (RET); 0x28
// bytes of frame with EBP, ESI, EBX and EDI saved below them as the paths
// save them, read from the ESP each child left; 164EF0 (BL the announce flag)
// and 15DD50 direct, B1A10 (0x10 pushed, RET 4) and 1A87A0 (EAX the event)
// through the unit's dispatch, 15DDC0 (0 pushed, RET 4) through the World's
// required owner, every register each returns coming back. XMM0 holds what
// the conversions leave in it. Each access publishes its original
// instruction.
// HALO2_REFERENCE_ROUND_COUNTDOWN=1 restores the translated body.
// HALO2_VERIFY_ROUND_COUNTDOWN=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/multiplayer/native_tu15_round_countdown_systems.hpp"

namespace halo2::native_round_countdown_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_round_countdown;
using Access=logic::round_countdown::Access;
using Kept=logic::round_countdown::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ROUND_COUNTDOWN");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ROUND_COUNTDOWN");return v;}
enum Branch : unsigned {timed, untimed, sounded, announced, counted_down, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Round countdown"};return v;}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(v)));}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t low(__m128 x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));}
inline constexpr std::uint32_t sites[]{0x15e875u,0x15e87bu,0x15e881u,0x15e898u,0x15e8a2u,0x15e8a8u,0x15e8b5u,0x15e8cau,0x15e8dcu,0x15e8e8u,
    0x15e8edu,0x15e8f3u,0x15e8ffu,0x15e904u,0x15e918u,0u,0u,0u,0u,0u,0x15e9ecu,0x15e9f2u,0x15e9f5u,0x15ea07u,0x15ea0cu,0x15ea12u,
    0x15ea28u,0x15ea35u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::untimed_announced_store)+1u);
// The two events' stores: the type first (the announcement) or after the zero (the countdown).
inline constexpr std::uint32_t announcement_sites[]{0x15e97bu,0x15e973u,0x15e97fu,0x15e983u,0x15e987u,0x15e98bu,0x15e98fu,0x15e993u,0x15e997u};
inline constexpr std::uint32_t countdown_sites[]{0x15e9a9u,0x15e9adu,0x15e9b5u,0x15e9b9u,0x15e9bdu,0x15e9c1u,0x15e9c5u,0x15e9c9u,0x15e9cdu};
template<class State,class Timer,class End,class Required,class Dispatch> struct Adapter : Port<State> {
    Timer &timer;End &end_child;Required &required;Dispatch &dispatch;
    std::uint32_t depth{};      // bytes pushed below EBX (negative before it)
    bool countdown_event{};unsigned handles{};
    std::uint32_t exit{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Timer &a,End &e,Required &r,Dispatch &d)
        :Port<State>(s,c,t,{}),timer(a),end_child(e),required(r),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        const auto *event_sites=countdown_event?countdown_sites:announcement_sites;
        switch(a) {
        case first_store:return event_sites[0];
        case type_store:return event_sites[1];
        case handle_store:return event_sites[2u+handles++];
        case last_store:return event_sites[7];
        case word_store:return event_sites[8];
        default:return sites[unsigned(a)];
        }
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case session_load:gp[esi]=v;break;
        case player_load:case session_again:case tick_load:gp[eax]=v;break;
        case display_load:case variant_load:case time_load:case time_untimed:gp[ecx]=v;break;
        case mode_load:gp[eax]=sx8(v);break;
        case seconds_load:case untimed_load:this->s.xmm[0]=lane0(v);break;                             // MOVSS XMM0
        case announced_load:gp[edx]=sx16(v);break;
        case announced_untimed:gp[ecx]=sx16(v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing() {                                                                                     // SUB ESP,28; PUSH EBP; PUSH ESI
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],logic::local::size,4);
        this->push(0x15e873u,gp[ebp]);this->push(0x15e874u,gp[esi]);depth=0u-4u;
    }
    std::uint32_t zeroed(){return this->gp[ebp]=this->flags.logic(0u,4);}                              // XOR EBP,EBP
    void compared(std::uint32_t a,std::uint32_t b){this->compare32(a,b);}
    void word_compared(std::uint32_t a,std::uint32_t b){this->flags.minus(a&0xffffu,b&0xffffu,2);}     // CMP word,BX
    void saving(unsigned n) {
        auto &gp=this->gp;
        if(n==1u){this->push(0x15e892u,gp[ebx]);depth=0u;gp[ebx]=logic::running;}                       // PUSH EBX; MOV EBX,1
        else{put(0x15e8d6u,gp[edi]);}                                                                     // PUSH EDI
    }
    std::uint32_t mode(std::uint32_t byte){const auto v=sx8(byte);this->compare32(v,logic::variant::untimed_mode);return v;}   // CMP EAX,4
    void timed(Kept &kept) {                                                                             // CALL 164EF0
        this->reached(Branch::timed);
        this->call(0x15e8d7u,0x15e8dcu,0x164ef0u,timer);
        refresh(kept);
    }
    std::uint32_t shown_kept(){return this->gp[ebx]=this->gp[eax];}                                     // MOV EBX,EAX
    std::uint32_t converted(std::uint32_t v) {                                                           // CVTSI2SS XMM0,reg
        auto &x=this->s.xmm[0];
        x=_mm_cvtsi32_ss(x,int(v));
        return low(x);
    }
    std::uint32_t scaled(std::uint32_t v){auto &x=this->s.xmm[0];x=_mm_move_ss(x,lane0(v));return v;}  // MULSS XMM0,[ECX+4]
    std::uint32_t truncated(std::uint32_t v) {                                                           // CVTTSS2SI EAX,XMM0; MOV ESI,EAX
        auto &gp=this->gp;
        gp[eax]=std::uint32_t(_mm_cvttss_si32(lane0(v)));
        return gp[esi]=gp[eax];
    }
    std::uint32_t widened(std::uint32_t word){return sx16(word);}
    std::uint32_t handles_none(){return this->gp[edi]=this->flags.logic(this->gp[edi]|0xffffffffu,4);}  // OR EDI,-1
    void sound_pushed(){put(0x15e916u,logic::sound);}                                                   // PUSH 10
    void sounded(Kept &kept) {                                                                           // CALL B1A10
        auto child=[&](State &v){dispatch(v,0xb1a10u,0x15e91fu);};
        this->call(0x15e91fu,0x15e924u,0xb1a10u,child);depth-=4u;
        refresh(kept);
        this->reached(Branch::sounded);
    }
    std::uint32_t event_address(bool type_first) {                                                       // (MOV EAX,type); LEA EAX,[ESP+14]
        countdown_event=!type_first;handles=0u;
        return this->gp[eax]=local(logic::local::event);
    }
    void raised(std::uint32_t,Kept &kept) {                                                              // CALL 1A87A0
        const std::uint32_t pc=countdown_event?0x15e9d2u:0x15e99cu;
        auto child=[&](State &v){dispatch(v,0x1a87a0u,pc);};
        this->call(pc,pc+5u,0x1a87a0u,child);
        refresh(kept);
        this->reached(countdown_event?Branch::counted_down:Branch::announced);
    }
    void ended(Kept &kept) {                                                                             // PUSH EBP; CALL 15DD50; MOV EBX,EAX; CALL 15DDC0
        auto &gp=this->gp;
        put(0x15e9d7u,kept.zero);
        this->call(0x15e9d8u,0x15e9ddu,0x15dd50u,end_child);
        refresh(kept);
        kept.shown=gp[ebx]=gp[eax];
        this->call(0x15e9dfu,0x15e9e4u,0x15ddc0u,required);depth-=4u;
        refresh(kept);
    }
    std::uint32_t elapsed(std::uint32_t tick,std::uint32_t start) {                                     // SUB EAX,[ESI+70]; MOV EDX,0; SETS DL; DEC EDX; AND EAX,EDX
        auto &gp=this->gp;auto &fl=this->flags;
        this->reached(Branch::untimed);
        gp[eax]=fl.minus(tick,start,4);gp[edx]=fl.sf?1u:0u;gp[edx]=fl.increment(gp[edx],4,true);
        return gp[eax]=fl.logic(gp[eax]&gp[edx],4);
    }
    std::uint32_t truncated_untimed(std::uint32_t v){return this->gp[eax]=std::uint32_t(_mm_cvttss_si32(lane0(v)));}   // CVTTSS2SI EAX,XMM0
    std::uint32_t limited(std::uint32_t seconds,std::uint32_t longest) {                                // CMP EAX,7FFF; JLE; MOV EAX,7FFF
        this->compare32(seconds,longest);
        return this->gp[eax]=std::int32_t(seconds)<=std::int32_t(longest)?seconds:longest;
    }
    std::uint32_t widened_untimed(std::uint32_t word){return sx16(word);}
    void sound_pushed_untimed(){put(0x15ea33u,logic::sound);}                                           // PUSH 10
    void sounded_untimed(Kept &kept) {                                                                   // CALL B1A10
        auto child=[&](State &v){dispatch(v,0xb1a10u,0x15ea3cu);};
        this->call(0x15ea3cu,0x15ea41u,0xb1a10u,child);depth-=4u;
        refresh(kept);
        this->reached(Branch::sounded);
    }
    void restoring(unsigned n) {
        auto &gp=this->gp;
        if(n==3u) {                                                                                      // POP EBX; POP ESI; POP EBP (the untimed exit)
            gp[ebx]=this->pop(0x15ea41u);gp[esi]=this->pop(0x15ea42u);gp[ebp]=this->pop(0x15ea43u);exit=0x15ea47u;
        } else {
            if(n==2u)gp[edi]=this->pop(0x15e9e4u);                                                       // POP EDI
            if(n>=1u)gp[ebx]=this->pop(0x15e9e5u);                                                       // POP EBX
            gp[esi]=this->pop(0x15e9e6u);gp[ebp]=this->pop(0x15e9e7u);exit=0x15e9ebu;                     // POP ESI; POP EBP
        }
        gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);                                         // ADD ESP,28
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::round_countdown::announce(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]});
        this->ret(exit,0u);this->publish(exit);
    }
};
template<class State,class Reference,class Timer,class End,class Required,class Dispatch>
bool bridge(State &s,Reference reference,Timer timer,End end,Required required,Dispatch dispatch) {
    return run_owner(s,0x15e870u,0x15ea47u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Timer,End,Required,Dispatch>(v,c,t,timer,end,required,dispatch);});
}
} // namespace halo2::native_round_countdown_detail
