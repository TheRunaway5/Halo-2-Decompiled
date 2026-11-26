#pragma once
// EAX names the output, EDX names the input and XMM0 supplies the motion
// scale. This adapter owns the RET 16 frame, FP lanes and original accesses.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_unit_motion_parameters_systems.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_unit_motion_parameters_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;
using native_small_queries_detail::VerifySummary;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;
namespace logic=halo2::native_unit_motion_parameters;
namespace fp_math=halo2::physics_step_detail;
using logic::Access;
using logic::Local;

inline bool reference_mode(){static const bool value=flag("HALO2_REFERENCE_UNIT_MOTION_PARAMETERS");return value;}
inline bool verify_mode(){static const bool value=flag("HALO2_VERIFY_UNIT_MOTION_PARAMETERS");return value;}
enum Branch : unsigned {backward_copy,forward_copy,fixed_blend,variable_blend,disabled,skipped,
    defaulted,retained,normalized,unit_length,forward_direction,reverse_direction,
    speed_override,override_forward,override_reverse,branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
inline VerifySummary &summary(){static VerifySummary value{"Unit motion parameters"};return value;}

inline constexpr std::uint32_t sites[]{
    0x1ee3d0u,0x1ee3d3u,0x1ee3edu,0x1ee3edu,
    0x1ee3efu,0x1ee3f7u,0x1ee3f9u,0x1ee3fcu,0x1ee3ffu,0x1ee402u,
    0x1ee405u,0x1ee409u,0x1ee415u,0x1ee41fu,0x1ee421u,0x1ee424u,0x1ee427u,0x1ee42au,
    0x1ee42du,0x1ee435u,0x1ee43du,0x1ee446u,0x1ee44cu,0x1ee44fu,0x1ee45du,
    0x1ee469u,0x1ee46fu,
    0x1ee47bu,0x1ee47du,0x1ee481u,0x1ee484u,0x1ee487u,0x1ee48du,0x1ee491u,
    0x1ee497u,0x1ee49bu,0x1ee4bbu,0x1ee4c6u,0x1ee4ceu,0x1ee4d4u,0x1ee4dau,
    0x1ee4e0u,0x1ee4e4u,0x1ee4eau,0x1ee4eeu,0x1ee4f2u,
    0x1ee501u,0x1ee506u,0x1ee50du,0x1ee512u,0x1ee523u,0x1ee52bu,0x1ee530u,0x1ee541u,
    0x1ee549u,0x1ee551u,0x1ee55au,0x1ee567u,0x1ee56fu,0x1ee572u,0x1ee578u,0x1ee585u,
    0x1ee58fu,0x1ee597u,0x1ee59fu,0x1ee5abu,0x1ee5b3u,0x1ee5bfu,
    0x1ee5ccu,0x1ee5ceu,0x1ee5d1u,0x1ee5d4u,0x1ee5d6u,0x1ee5dau,0x1ee5eeu,0x1ee5f6u,
};
static_assert(std::size(sites)==unsigned(Access::count));
inline __m128 scalar(std::uint32_t bits){return _mm_set_ss(fp_math::f(bits));}
inline std::uint32_t word(__m128 value){return fp_math::u(_mm_cvtss_f32(value));}

template<class State>
struct Adapter : Port<State> {
    Adapter(State &state,std::uint32_t *cursor,Trace<State> *trace):Port<State>(state,cursor,trace,{}){}
    std::uint32_t instruction(Access access,std::uint32_t address) {
        auto &gp=this->gp;
        using enum Access;
        switch(access) {
        case vector_x_store:gp[ecx]=address;break;
        case fallback_x:gp[edx]=address;break;
        case fallback_x_store:gp[esi]=gp[ecx]=address;break;
        case direction_x:gp[edx]=address;break;
        default:break;
        }
        return sites[unsigned(access)];
    }
    void loaded(Access access,std::uint32_t,std::uint32_t bits) {
        auto &gp=this->gp;
        using enum Access;
        switch(access) {
        case selector:case globals:case tuning:case position_x:gp[ecx]=bits;break;
        case vector_x:case vector_y:gp[esi]=bits;break;
        case vector_z:case fallback_z:case options:case direction_z:case padding:case position_y:gp[edx]=bits;break;
        case fallback_x:case fallback_y:case descriptor:case direction_x:case direction_y:gp[edi]=bits;break;
        case descriptor_options:gp[ebx]=bits;break;
        case option_arg:set_low_byte(gp[ecx],bits);break;
        case skip_arg:set_low_byte(gp[ebx],bits);break;
        case override_arg:set_low_byte(gp[edx],bits);break;
        case position_z:gp[eax]=bits;break;
        case default_limit:this->s.xmm[0]=scalar(bits);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    std::uint32_t local(Local offset)const{return this->gp[esp]+std::uint32_t(offset);}
    void begin_matrix(std::uint32_t source,std::uint32_t target,std::uint32_t count) {
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],0xcu,4);
        this->push(0x1ee3d9u,gp[ebx]);
        this->push(0x1ee3dau,gp[esi]);
        gp[esi]=source;
        gp[edx]=this->flags.plus(gp[edx],logic::input_field::vector,4);
        this->s.xmm[4]=_mm_setzero_ps();
        this->push(0x1ee3e4u,gp[edi]);
        gp[edi]=target;
        gp[ecx]=count;
    }
    std::uint32_t matrix_step() {
        const bool reverse=(this->s.other_flags&0x400u)!=0u;
        this->reached(reverse?Branch::backward_copy:Branch::forward_copy);
        return reverse?0u-4u:4u;
    }
    void advance_matrix(std::uint32_t source,std::uint32_t target) {
        this->gp[esi]=source;
        this->gp[edi]=target;
        --this->gp[ecx];
    }
    bool use_fixed_blend(std::uint32_t options) {
        const auto value=this->gp[ecx]=this->flags.logic(options&logic::option::fixed_blend,4);
        this->reached(value?Branch::fixed_blend:Branch::variable_blend);
        return value!=0u;
    }
    void zero_blend(){this->s.xmm[2]=this->s.xmm[4];}
    void set_blend(std::uint32_t bits){this->s.xmm[2]=scalar(bits);}
    bool motion_enabled(std::uint32_t descriptor) {
        const auto bits=this->gp[ebx]=this->flags.right(descriptor,2u,4,false);
        this->test8(bits&1u);
        if(this->flags.zf)this->reached(Branch::disabled);
        return !this->flags.zf;
    }
    bool motion_skipped(std::uint32_t value) {
        this->test8(value);
        if(!this->flags.zf)this->reached(Branch::skipped);
        return !this->flags.zf;
    }
    bool needs_default_limit(std::uint32_t options) {
        this->test8(options&logic::option::preserve_limit);
        if(this->flags.zf)this->test32(this->gp[ecx]);
        const bool use_default=this->flags.zf;
        this->reached(use_default?Branch::defaulted:Branch::retained);
        return use_default;
    }
    void load_axis_x(std::uint32_t bits){this->s.xmm[3]=scalar(bits);}
    void load_axis_y(std::uint32_t bits){this->s.xmm[5]=scalar(bits);}
    std::uint32_t squared_length(std::uint32_t z) {
        auto &x=this->s.xmm;
        x[0]=scalar(z);
        const auto zz=ordered_mul_ss(x[0],x[0]);
        const auto yy=ordered_mul_ss(x[5],x[5]);
        x[1]=ordered_mul_ss(x[3],x[3]);
        return word(x[0]=ordered_add_ss(ordered_add_ss(zz,yy),x[1]));
    }
    bool needs_normalization(std::uint32_t unit) {
        auto &x=this->s.xmm;
        x[1]=scalar(unit);
        this->flags.scalar_compare(x[0],x[1],false);
        const bool blend=!this->flags.cf&&!this->flags.zf;
        this->reached(blend?Branch::normalized:Branch::unit_length);
        return blend;
    }
    void load_length(std::uint32_t squared) {
        auto &fp=this->s.fp;
        fp.push(fp_math::ext(fp_math::f(squared)));
        fp.st(0)=physics_solver_detail::root(fp.st(0));
    }
    void inverse_length(std::uint32_t unit) {
        auto &fp=this->s.fp;
        fp.st(0)=physics_solver_detail::divide(fp_math::ext(fp_math::f(unit)),fp.st(0));
    }
    std::uint32_t scaled_x(std::uint32_t bits) {
        auto &fp=this->s.fp;
        fp.push(fp_math::ext(fp_math::f(bits)));
        fp.st(0)=fp_math::mul(fp.st(0),fp.st(1));
        return fp_math::u(fp_math::spill(fp.st(0)));
    }
    std::uint32_t scaled_y(std::uint32_t bits) {
        auto &fp=this->s.fp;
        fp.st(0)=fp_math::mul(fp.st(0),fp_math::ext(fp_math::f(bits)));
        return fp_math::u(fp_math::spill(fp.st(0)));
    }
    void pop_extended(){this->s.fp.pop();}
    bool forward() {
        auto &x=this->s.xmm;
        this->flags.scalar_compare(x[3],x[4],false);
        return !this->flags.cf&&!this->flags.zf;
    }
    bool begin_blend() {
        const bool positive=forward();
        auto &x=this->s.xmm;
        x[1]=_mm_sub_ss(x[1],x[2]);
        this->reached(positive?Branch::forward_direction:Branch::reverse_direction);
        return positive;
    }
    std::uint32_t blend(std::uint32_t first,std::uint32_t second) {
        auto &x=this->s.xmm;
        const auto first_weighted=ordered_mul_ss(scalar(first),x[1]);
        x[6]=ordered_mul_ss(scalar(second),x[2]);
        return word(x[0]=ordered_add_ss(first_weighted,x[6]));
    }
    void prepare_limit(std::uint32_t value) {
        auto &x=this->s.xmm;
        x[0]=ordered_mul_ss(scalar(value),x[1]);
    }
    std::uint32_t finish_limit(std::uint32_t value) {
        auto &x=this->s.xmm;
        x[1]=ordered_mul_ss(scalar(value),x[2]);
        return word(x[0]=ordered_add_ss(x[0],x[1]));
    }
    bool overrides_speed(std::uint32_t value) {
        this->test8(value);
        if(value)this->reached(Branch::speed_override);
        return value!=0u;
    }
    bool forward_override() {
        const bool positive=forward();
        this->reached(positive?Branch::override_forward:Branch::override_reverse);
        return positive;
    }
    std::uint32_t set_override(std::uint32_t value){return word(this->s.xmm[0]=scalar(value));}
    std::uint32_t scale_speed(std::uint32_t value) {
        auto &x=this->s.xmm;
        return word(x[0]=ordered_mul_ss(scalar(value),x[3]));
    }
    std::uint32_t scale_lateral(std::uint32_t value) {
        auto &x=this->s.xmm;
        return word(x[0]=ordered_mul_ss(scalar(value),x[5]));
    }
    std::uint32_t position_address(std::uint32_t output){return this->gp[eax]=this->flags.plus(output,logic::output_field::base_position,4);}
    void restore_before_position_z(){this->gp[edi]=this->pop(0x1ee5d9u);}
    void finish(std::uint32_t restore_esi,std::uint32_t restore_ebx,std::uint32_t exit) {
        auto &gp=this->gp;
        gp[esi]=this->pop(restore_esi);
        gp[ebx]=this->pop(restore_ebx);
        gp[esp]=this->flags.plus(gp[esp],0xcu,4);
        this->ret(exit,16u);
        this->publish(exit);
    }
    void finish_motion(){finish(0x1ee5ddu,0x1ee5deu,0x1ee5e2u);}
    void finish_without_motion() {
        this->gp[edi]=this->pop(0x1ee5feu);
        finish(0x1ee5ffu,0x1ee600u,0x1ee604u);
    }
    void run() {
        Memory<Adapter> memory{*this};
        logic::prepare(memory,*this,this->gp[eax],this->gp[edx],word(this->s.xmm[0]));
    }
};

template<class State,class Reference>
bool bridge(State &state,Reference reference) {
    return run_owner(state,0x1ee3d0u,0x1ee604u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [](State &value,std::uint32_t *cursor,Trace<State> *trace){return Adapter<State>{value,cursor,trace};});
}
} // namespace halo2::native_unit_motion_parameters_detail
