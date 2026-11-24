#pragma once
// Boundary adapter for the ray hit accept, 185820
// (native_tu15_ray_hit_accept_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: ECX the query, the body and the
// hit pushed (RET 8); a 12-byte frame (the normal) below the saved EBX (ESI,
// EDI and EBP as the tests pass); 186C10 (ECX the body) direct and 1D7530
// (ECX the object) through the unit's dispatch, every register each returns
// coming back; XMM0 to XMM4 as the original leaves them. Each access
// publishes its original instruction.
// HALO2_REFERENCE_RAY_HIT_ACCEPT=1 restores the translated body.
// HALO2_VERIFY_RAY_HIT_ACCEPT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_ray_hit_accept_systems.hpp"

namespace halo2::native_ray_hit_accept_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_ray_hit_accept;
using Access=logic::ray_hit_accept::Access;
using Kept=logic::ray_hit_accept::Kept;
using Outcome=logic::ray_hit_accept::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_RAY_HIT_ACCEPT");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_RAY_HIT_ACCEPT");return v;}
enum Branch : unsigned {beyond, farther, ignored, facing, accepted, branch_count};
static_assert(unsigned(Outcome::accepted)==Branch::accepted);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Ray hit accept"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0x185820u,0x18582cu,0x185830u,0x18583du,0x185843u,0x185846u,0x18584eu,0x18585bu,0x185863u,0x18586cu,
    0x185870u,0x185878u,0x18588cu,0x185891u,0x185896u,0x18589bu,0x1858a3u,0x1858a9u,0x1858b2u,0x1858b8u,0x1858c1u,0x1858d7u,0x1858dbu,
    0x1858dfu,0x1858e3u,0x1858e8u,0x1858eeu,0x1858f0u,0x1858f4u,0x1858f7u,0x1858fau};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::query_fraction)+1u);
template<class State,class Body,class Dispatch> struct Adapter : Port<State> {
    Body &body_child;
    Dispatch &dispatch;
    std::uint32_t depth{};      // bytes below the return address
    unsigned exit_kind{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Body &b,Dispatch &d):Port<State>(s,c,t,{}),body_child(b),dispatch(d){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case one:case best_fraction:x[0]=lane0(v);break;
        case hit_argument:gp[ebx]=v;break;
        case hit_fraction_again:case fraction_load:x[3]=lane0(v);break;
        case best:gp[edi]=v;break;
        case ignored:gp[ebp]=v;break;
        case body_argument:case body_object:case nx_load:case nz_load:gp[ecx]=v;break;
        case ny_load:gp[edx]=v;break;
        case best_again:gp[eax]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing(std::uint32_t) {                                                                        // SUB ESP,C; PUSH EBX
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],0xcu,4);depth=0xcu;put(0x18582bu,gp[ebx]);
    }
    void compared(std::uint32_t a,std::uint32_t b){this->flags.scalar_compare(lane0(a),lane0(b),false);}   // COMISS
    void compared32(std::uint32_t a,std::uint32_t b){this->compare32(a,b);}
    void saving(unsigned n) {
        auto &gp=this->gp;
        if(n==1u){put(0x185834u,gp[esi]);gp[esi]=gp[ecx];}                                            // PUSH ESI; MOV ESI,ECX
        else if(n==2u)put(0x185842u,gp[edi]);                                                           // PUSH EDI
        else put(0x18585au,gp[ebp]);                                                                     // PUSH EBP
    }
    void weighed(std::uint32_t best_value,std::uint32_t fraction){compared(best_value,fraction);}      // COMISS XMM0,XMM3
    std::uint32_t body(std::uint32_t,Kept &kept) {                                                      // CALL 186C10
        this->call(0x185867u,0x18586cu,0x186c10u,body_child);
        refresh(kept);
        return this->gp[eax];
    }
    void reloaded(std::uint32_t){}
    std::uint32_t xmm3()const{return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(this->s.xmm[3])));}
    void cleared(){this->gp[ecx]=this->flags.logic(0u,4);}                                             // XOR ECX,ECX
    void tested(std::uint32_t object_handle){this->test32(object_handle);}                             // TEST ECX,ECX
    std::uint32_t object_of(std::uint32_t,Kept &kept) {                                                // CALL 1D7530
        auto child=[&](State &v){dispatch(v,0x1d7530u,0x185883u);};
        this->call(0x185883u,0x185888u,0x1d7530u,child);
        refresh(kept);
        return this->gp[eax];
    }
    void dotted(std::uint32_t dot,std::uint32_t y_term,std::uint32_t x_term) {                         // XORPS XMM0; ADDSS; ADDSS; COMISS XMM0,XMM4
        auto &x=this->s.xmm;
        x[0]=_mm_setzero_ps();x[1]=lane0(x_term);x[2]=lane0(y_term);x[4]=lane0(dot);
        this->flags.scalar_compare(x[0],x[4],false);
    }
    std::uint32_t normal_slot(std::uint32_t best_address){return this->gp[eax]=this->flags.plus(best_address,logic::best::normal,4);}   // ADD EAX,1C
    void restoring(unsigned k){exit_kind=k;}
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::ray_hit_accept::accept(memory,*this,Kept{gp[ebx],gp[ebp],gp[ecx],gp[edi]});
        this->reached(unsigned(outcome));
        if(exit_kind>=2u)gp[ebp]=take(0x1858ffu);
        if(exit_kind>=1u)gp[edi]=take(0x185900u);
        gp[esi]=take(0x185901u);gp[ebx]=take(0x185902u);
        gp[esp]=fl.plus(gp[esp],0xcu,4);                                                                 // ADD ESP,C
        this->ret(0x185906u,8u);this->publish(0x185906u);
    }
};
template<class State,class Reference,class Body,class Dispatch> bool bridge(State &s,Reference reference,Body body,Dispatch dispatch) {
    return run_owner(s,0x185820u,0x185906u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Body,Dispatch>(v,c,t,body,dispatch);});
}
} // namespace halo2::native_ray_hit_accept_detail
