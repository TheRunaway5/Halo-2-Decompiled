#pragma once
// Boundary adapter for the channel seek, 1CEBD0
// (native_tu15_channel_seek_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: ESI the channel, four arguments
// pushed (RET 0x10); 1E3150 (EDX the definition; the sub pushed, RET 4) and
// 1CE950 (five pushed, RET 0x14) through the unit's dispatch, every register
// each returns coming back; XMM0 to XMM2 as the clamp leaves them. Each
// access publishes its original instruction.
// HALO2_REFERENCE_CHANNEL_SEEK=1 restores the translated body.
// HALO2_VERIFY_CHANNEL_SEEK=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/animation/native_tu15_channel_seek_systems.hpp"

namespace halo2::native_channel_seek_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_channel_seek;
using Access=logic::channel_seek::Access;
using Kept=logic::channel_seek::Kept;
using Outcome=logic::channel_seek::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_CHANNEL_SEEK");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_CHANNEL_SEEK");return v;}
enum Branch : unsigned {seeked, unused, at_zero, at_end, between, branch_count};
static_assert(unsigned(Outcome::unused)==Branch::unused);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Channel seek"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0x1cebd2u,0x1cebd9u,0x1cebdbu,0x1cebe9u,0x1cebedu,0x1cebf9u,0x1cebfdu,0x1cec06u,0x1cec11u,0x1cec1cu,
    0x1cec27u,0x1cec2fu,0x1cec35u,0x1cec39u,0x1cec3du,0x1cec42u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::frame_load)+1u);
template<class State,class Dispatch> struct Adapter : Port<State> {
    Dispatch &dispatch;
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,{}),dispatch(d){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept){kept={this->gp[esi]};}
    std::uint32_t instruction(Access a,std::uint32_t){
        if(a==Access::frame_zero)this->reached(Branch::at_zero);
        if(a==Access::frame_end)this->reached(Branch::at_end);
        if(a==Access::frame_store)this->reached(Branch::between);
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case tag:case sub:case b_value:gp[eax]=v;break;
        case instances:case a_value:gp[ecx]=v;break;
        case definition:case c_value:case frame_load:gp[edx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void cleared(){this->gp[eax]=this->flags.logic(0u,4);}                                             // XOR EAX,EAX
    void high_tested(std::uint32_t high){this->flags.minus(high&0xffffu,0xffffu,2);}                  // CMP word [ESI+6],-1
    std::uint32_t tag_index(std::uint32_t tag){auto &fl=this->flags;return this->gp[eax]=fl.shift(fl.logic(tag&0xffffu,4),4u,4);}   // AND; SHL
    std::uint32_t record(std::uint32_t definition,std::uint32_t sub,Kept &kept) {                      // PUSH EAX; CALL 1E3150
        auto &gp=this->gp;
        gp[edx]=definition;put(0x1cebf0u,sub);
        auto child=[&](State &v){dispatch(v,0x1e3150u,0x1cebf1u);};
        this->call(0x1cebf1u,0x1cebf6u,0x1e3150u,child);depth-=4u;
        refresh(kept);
        return gp[eax];
    }
    void zeroed(){this->s.xmm[2]=_mm_setzero_ps();}                                                    // XORPS XMM2
    std::uint32_t last_frame(std::uint32_t frames){return this->gp[ecx]=this->flags.increment(frames,4,true);}   // MOVSX ECX; DEC ECX
    void converted(std::uint32_t v){this->s.xmm[0]=_mm_move_ss(this->s.xmm[0],lane0(v));}            // CVTSI2SS XMM0,ECX
    void computed(std::uint32_t end,std::uint32_t product) {                                           // ADDSS; MOVAPS XMM1,XMM0; MULSS XMM1
        auto &x=this->s.xmm;
        x[0]=_mm_move_ss(x[0],lane0(end));x[1]=_mm_move_ss(x[0],lane0(product));
    }
    void compared(std::uint32_t a,std::uint32_t b){this->flags.scalar_compare(lane0(a),lane0(b),false);}   // COMISS
    void pushing(std::uint32_t c){put(0x1cec41u,c);}                                                   // PUSH EDX
    void advance(std::uint32_t b,std::uint32_t a,std::uint32_t frame_value,Kept &kept) {               // PUSH EAX, ECX, EDX, ESI; CALL 1CE950
        put(0x1cec46u,b);put(0x1cec47u,a);put(0x1cec48u,frame_value);put(0x1cec49u,kept.channel);
        auto child=[&](State &v){dispatch(v,0x1ce950u,0x1cec4au);};
        this->call(0x1cec4au,0x1cec4fu,0x1ce950u,child);depth-=20u;
        refresh(kept);
    }
    void run() {
        Memory<Adapter> memory{*this};
        const auto outcome=logic::channel_seek::seek(memory,*this,Kept{this->gp[esi]});
        this->reached(unsigned(outcome));
        this->ret(0x1cec4fu,0x10u);this->publish(0x1cec4fu);
    }
};
template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    return run_owner(s,0x1cebd0u,0x1cec4fu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Dispatch>(v,c,t,dispatch);});
}
} // namespace halo2::native_channel_seek_detail
