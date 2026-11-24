#pragma once
// Boundary adapter for the countdown step, 1CEB70
// (native_tu15_countdown_step_systems.hpp). Its callers and the child keep
// the original ABI, which the adapter owns: EAX the record, three arguments
// on the stack (RET C); 1CE950 gets the record, the value (stored into the
// slot pushed for it) and the three arguments pushed (RET 14), every
// register it returns coming back. Each access publishes its original
// instruction.
// HALO2_REFERENCE_COUNTDOWN_STEP=1 restores the translated body.
// HALO2_VERIFY_COUNTDOWN_STEP=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_countdown_step_systems.hpp"

namespace halo2::native_countdown_step_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_countdown_step;
using Access=logic::countdown_step::Access;
using Outcome=logic::countdown_step::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_COUNTDOWN_STEP");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_COUNTDOWN_STEP");return v;}
enum Branch : unsigned {counted, disabled, blocked, stepped, branch_count};
static_assert(unsigned(Outcome::stepped)==Branch::stepped);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Countdown step"};return v;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0x1ceb70u,0x1ceb77u,0x1ceb7bu,0x1ceb83u,0x1ceb89u,0x1ceb8fu,0x1ceb95u,0x1ceb99u,0x1ceb9eu,
    0x1ceba3u,0x1ceba9u,0x1cebaeu,0x1cebb3u,0x1cebbbu,0x1cebc2u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::value_store)+1u);
template<class State,class Step> struct Adapter : Port<State> {
    Step &step_child;
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Step &st):Port<State>(s,c,t,{}),step_child(st){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    std::uint32_t value_slot()const{return this->gp[esp];}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        if(a==Access::countdown_store)set_low_byte(gp[ecx],this->flags.increment(gp[ecx]&0xffu,1,true));   // DEC CL
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case countdown:set_low_byte(gp[ecx],v);gp[edx]=fl.logic(0u,4);fl.minus(v&0xffu,0u,1);break;     // XOR EDX,EDX; CMP CL,DL
        case enabled:fl.logic(v&logic::enabled_bit,1);break;
        case blocking:fl.logic(v&logic::blocking_bits,1);break;
        case c_argument:case a_argument:gp[ecx]=v;break;
        case b_argument:case game_time:gp[edx]=v;break;
        case tick:this->s.xmm[0]=lane0_only(v);break;
        default:break;
        }
    }
    // MULSS, MULSS, ADDSS: XMM0's low lane the value.
    void stored(Access a,std::uint32_t,std::uint32_t v){if(a==Access::value_store)this->s.xmm[0]=_mm_move_ss(this->s.xmm[0],lane0_only(v));}
    // PUSH ECX (c); PUSH EDX (b); PUSH ECX (a); PUSH ECX (the value's slot).
    void pushing(std::uint32_t,unsigned which) {
        static constexpr std::uint32_t at[]{0x1ceb9du,0x1ceba2u,0x1cebc0u,0x1cebc1u};
        this->push(at[which],which==1u?this->gp[edx]:this->gp[ecx]);depth+=4u;
    }
    // PUSH EAX; CALL 1CE950.
    void step(std::uint32_t) {
        this->push(0x1cebc7u,this->gp[eax]);depth+=4u;
        this->call(0x1cebc8u,0x1cebcdu,0x1ce950u,step_child);depth-=20u;
    }
    void run() {
        Memory<Adapter> memory{*this};
        const auto outcome=logic::countdown_step::run(memory,*this,this->gp[eax]);
        this->reached(unsigned(outcome));
        const std::uint32_t exit=outcome==Outcome::counted?0x1ceb86u:0x1cebcdu;
        this->ret(exit,0xcu);this->publish(exit);
    }
};
template<class State,class Reference,class Step> bool bridge(State &s,Reference reference,Step step) {
    return run_owner(s,0x1ceb70u,0x1cebcdu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Step>(v,c,t,step);});
}
} // namespace halo2::native_countdown_step_detail
