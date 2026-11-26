#pragma once
// E05D0's engine-call boundary. The forward output uses ECX, the direction
// uses EDX, and the fallback and perpendicular output use ESI and EDI.
// The caller observes the original register and floating-point exit state,
// including ECX reloaded from the stack slot used to spill the length.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_camera_basis_systems.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/physics_solver_internal.hpp"
#include <cmath>

namespace halo2::native_camera_basis_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;
using native_small_queries_detail::VerifySummary;
using native_camera_basis::Access;
namespace fp_math=halo2::physics_step_detail;

inline bool reference_mode(){static const bool value=flag("HALO2_REFERENCE_CAMERA_BASIS");return value;}
inline bool verify_mode(){static const bool value=flag("HALO2_VERIFY_CAMERA_BASIS");return value;}
enum Branch : unsigned {direction, fallback, defaulted, flipped, unflipped, zero_length, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
inline VerifySummary &summary(){static VerifySummary value{"Camera horizontal basis"};return value;}

inline constexpr std::uint32_t sites[]{
    0xe05d1u,0xe05d5u,0xe05d3u,0xe05d8u,
    0xe05dbu,0xe05deu,0xe05e0u,0xe05fbu,0xe0600u,
    0xe060cu,0xe0611u,0xe0623u,0xe0627u,0xe0634u,
    0xe063fu,0xe0643u,0xe0641u,0xe0646u,0xe0649u,
    0xe0656u,0xe065au,0xe0661u,0xe0666u,
    0xe066bu,0xe066eu,0xe0680u,0xe0685u,
    0xe0691u,0xe0696u,0xe06a5u,0xe06a9u,0xe06b6u,
    0xe06bdu,0xe06c2u,0xe06c4u,0xe06c6u,0xe06c9u,
    0xe06ccu,0xe06d1u,0xe06d5u,0xe06d7u,
};
static_assert(std::size(sites)==unsigned(Access::count));

inline __m128 scalar(std::uint32_t word){return _mm_set_ss(fp_math::f(word));}
inline std::uint32_t word(__m128 value){return fp_math::u(_mm_cvtss_f32(value));}

template<class State>
struct Adapter : Port<State> {
    Adapter(State &state,std::uint32_t *cursor,Trace<State> *trace)
        :Port<State>(state,cursor,trace,{}){}
    std::uint32_t instruction(Access access,std::uint32_t)const{return sites[unsigned(access)];}
    void loaded(Access access,std::uint32_t,std::uint32_t value) {
        using enum Access;
        switch(access) {
        case direction_x:case direction_y:case fallback_x:case fallback_y:
        case default_pointer:case default_y:this->gp[eax]=value;break;
        case default_x:this->gp[edx]=value;break;
        case perpendicular_read_x:this->gp[ecx]=value;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    std::uint32_t length_slot()const{return this->gp[esp];}
    void load_extended(std::uint32_t value){this->s.fp.push(fp_math::ext(fp_math::f(value)));}
    void set_unit_scale(std::uint32_t value) {
        this->s.xmm[1]=scalar(value);
        this->s.xmm[2]=_mm_setzero_ps();
    }
    // The length uses the source x87 precision and preserves its caller's
    // logical stack, including the live length during the spill and epsilon read.
    std::uint32_t horizontal_length() {
        auto &fp=this->s.fp;
        fp.push(fp.st(0));
        fp.st(0)=fp_math::mul(fp.st(0),fp.st(1));
        fp.push(fp.st(2));
        fp.st(0)=fp_math::mul(fp.st(0),fp.st(3));
        const auto squared=fp_math::add(fp.st(1),fp.st(0));
        fp.pop();
        const auto length=physics_solver_detail::root(squared);
        fp.st(2)=length;
        fp.pop();
        fp.pop();
        return fp_math::u(fp_math::spill(fp.st(0)));
    }
    void absolute_length(){auto &length=this->s.fp.st(0);length=std::fabs(length);}
    bool length_below_threshold(std::uint32_t threshold) {
        auto &state=this->s;
        load_extended(threshold);
        state.compare_and_pop(state.fp.st(1));
        this->flags=state.flags;
        state.fp.pop();
        return !this->flags.cf&&!this->flags.zf;
    }
    template<bool Fallback>
    std::uint32_t scale_first(std::uint32_t length,std::uint32_t first) {
        auto &lanes=this->s.xmm;
        constexpr unsigned size=Fallback?0u:3u,component=Fallback?3u:4u,scale=Fallback?1u:0u;
        lanes[size]=scalar(length);
        lanes[component]=scalar(first);
        lanes[scale]=_mm_div_ss(lanes[1],lanes[size]);
        // When both are NaNs, MULSS retains the destination's payload.
        lanes[component]=native_tu15_leaves::ordered_mul_ss(lanes[component],lanes[scale]);
        this->flags.scalar_compare(lanes[size],lanes[2],true);
        return word(lanes[component]);
    }
    template<bool Fallback>
    std::uint32_t scale_second(std::uint32_t second) {
        auto &lanes=this->s.xmm;
        constexpr unsigned component=Fallback?3u:4u,scale=Fallback?1u:0u;
        lanes[component]=scalar(second);
        const auto ah=(this->flags.bits()|2u)&0xffu;
        auto &accumulator=this->gp[eax];
        accumulator=(accumulator&0xffff00ffu)|(ah<<8u);
        this->flags.logic(ah&0x44u,1);
        lanes[component]=native_tu15_leaves::ordered_mul_ss(lanes[component],lanes[scale]);
        return word(lanes[component]);
    }
    bool nonzero_length() {
        if(!this->flags.pf)this->reached(Branch::zero_length);
        return this->flags.pf;
    }
    bool flip_fallback(std::uint32_t z) {
        auto &lanes=this->s.xmm;
        lanes[0]=scalar(z);
        this->flags.scalar_compare(lanes[0],lanes[2],false);
        const bool flip=!this->flags.cf&&!this->flags.zf;
        this->reached(flip?Branch::flipped:Branch::unflipped);
        return flip;
    }
    std::uint32_t negate_for_flip(std::uint32_t value) {
        auto &lanes=this->s.xmm;
        return word(lanes[0]=_mm_sub_ss(lanes[2],scalar(value)));
    }
    std::uint32_t perpendicular_x(std::uint32_t value) {
        auto &lane=this->s.xmm[2];
        return word(lane=_mm_sub_ss(lane,scalar(value)));
    }
    void direction_used(){this->reached(Branch::direction);}
    void fallback_used(){this->reached(Branch::fallback);}
    void default_used(){this->reached(Branch::defaulted);}
    void run() {
        auto &gp=this->gp;
        this->push(0xe05d0u,gp[ecx]);
        Memory<Adapter> memory{*this};
        native_camera_basis::build(memory,*this,{gp[edx],gp[esi],gp[ecx],gp[edi]});
        gp[ecx]=this->pop(0xe06dau);
        this->ret(0xe06dbu,0u);
        this->publish(0xe06dbu);
    }
};

template<class State,class Reference>
bool bridge(State &state,Reference reference) {
    return run_owner(state,0xe05d0u,0xe06dbu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [](State &value,std::uint32_t *cursor,Trace<State> *trace){return Adapter<State>{value,cursor,trace};});
}
} // namespace halo2::native_camera_basis_detail
