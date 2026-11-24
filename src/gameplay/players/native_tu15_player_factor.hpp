#pragma once
// Boundary adapter for the player factor, 15B2D0
// (native_tu15_player_factor_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: ECX the player, the kind on the stack (RET 4),
// the factor in XMM0; ESI saved. The jump table's entry goes through the
// unit's canonical branch mapping, so an unbound entry raises the translated
// body's fault. Each access publishes its original instruction.
// HALO2_REFERENCE_PLAYER_FACTOR=1 restores the translated body.
// HALO2_VERIFY_PLAYER_FACTOR=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/players/native_tu15_player_factor_systems.hpp"
#include <stdexcept>

namespace halo2::native_player_factor_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_player_factor;
using Access=logic::player_factor::Access;
using Outcome=logic::player_factor::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_PLAYER_FACTOR");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_PLAYER_FACTOR");return v;}
enum Branch : unsigned {other_kind, first, second, third, unscaled, unclassed, branch_count};
static_assert(unsigned(Outcome::unclassed)==Branch::unclassed);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Player factor"};return v;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0x15b2d0u,0x15b2d6u,0x15b2edu,0x15b2f0u,0x15b2ffu,0x15b305u,0x15b315u,0x15b31bu,0x15b322u,0x15b32bu,
    0x15b335u,0x15b346u,0x15b357u,0x15b364u,0x15b370u,0x15b377u,0x15b383u,0x15b38fu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::third_scale)+1u);
template<class State,class Branch> struct Adapter : Port<State> {
    Branch &branch;
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Branch &b):Port<State>(s,c,t,{}),branch(b){}
    std::uint32_t argument()const{return this->gp[esp]+8u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        if(a==Access::mask_address)gp[esi]=fl.shift(1u,gp[ecx]&0xffu,4);                              // MOV ESI,1; SHL ESI,CL
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case players:gp[edx]=v;break;
        case one:                                                                                     // AND ECX,FFFF; MOV EAX,ECX; IMUL EAX,EAX,21C; PUSH ESI
            this->s.xmm[0]=lane0_only(v);gp[ecx]=fl.logic(gp[ecx]&0xffffu,4);gp[eax]=fl.product(gp[ecx],logic::player_size);
            this->push(0x15b2ecu,gp[esi]);break;
        case player_data:gp[esi]=v;break;
        case kind:gp[edx]=v;gp[eax]=fl.plus(gp[eax],gp[esi],4);gp[edx]=fl.minus(v,logic::applying_kind,4);break;   // ADD EAX,ESI; SUB EDX,2
        case game:gp[edx]=v;break;
        case mode:fl.minus(v,logic::masked_mode,4);break;
        case mask_address:gp[ecx]=v;break;
        case mask:gp[ecx]=v&0xffffu;fl.logic(gp[ecx]&gp[esi],4);break;                                // MOVZX ECX,word; TEST ECX,ESI
        case game_flags:fl.logic(v&logic::special_flag,1);break;
        case special_masked:case special:this->s.xmm[0]=lane0_only(v);break;
        case masked_setting:case setting: {                                                           // SUB ECX,0; DEC ECX; DEC ECX
            auto c=fl.minus(v,0u,4);
            if(c){c=fl.increment(c,4,true);if(c)c=fl.increment(c,4,true);}
            gp[ecx]=c;break;
        }
        case class_case:(void)branch(v,this->s.image);break;                                           // JMP [15B39C+EAX*4]
        case player_class:gp[eax]=std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(v))));fl.minus(gp[eax],logic::classes,4);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void zeroed(){this->s.xmm[0]=_mm_setzero_ps();}
    void scaled(std::uint32_t product){this->s.xmm[0]=_mm_move_ss(this->s.xmm[0],lane0_only(product));}
    [[noreturn]] void unknown_case(){throw std::runtime_error("Required original solver dispatch target unavailable");}
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto result=logic::player_factor::of(memory,*this,gp[ecx]);
        this->reached(unsigned(result.outcome));
        const std::uint32_t exit=result.outcome==Outcome::first?0x15b37fu:result.outcome==Outcome::second?0x15b38bu:0x15b397u;
        gp[esi]=this->pop(exit);
        this->ret(exit+1u,4u);this->publish(exit+1u);
    }
};
template<class State,class Reference,class Branch> bool bridge(State &s,Reference reference,Branch branch) {
    return run_owner(s,0x15b2d0u,0x15b398u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Branch>(v,c,t,branch);});
}
} // namespace halo2::native_player_factor_detail
