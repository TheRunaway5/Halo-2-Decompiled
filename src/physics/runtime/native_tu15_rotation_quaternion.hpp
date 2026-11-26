#pragma once
// EAX's low word carries the source x87 comparison status. The rotation
// pointer is the stack argument, and ECX names the quaternion (RET 4).
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_live_support_systems.hpp"
#include "physics/runtime/native_tu15_rotation_quaternion_systems.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
#include "physics/runtime/physics_solver_internal.hpp"
#include <utility>

namespace halo2::native_rotation_quaternion_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;
using native_small_queries_detail::VerifySummary;
namespace logic=halo2::native_rotation_quaternion;
namespace fp_math=halo2::physics_step_detail;
using logic::Access;

inline bool reference_mode(){static const bool value=flag("HALO2_REFERENCE_ROTATION_QUATERNION");return value;}
inline bool verify_mode(){static const bool value=flag("HALO2_VERIFY_ROTATION_QUATERNION");return value;}
enum Branch : unsigned {positive,nonpositive,major_x,major_y,major_z,branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
inline VerifySummary &summary(){static VerifySummary value{"Havok rotation quaternion"};return value;}

inline constexpr std::uint32_t sites[]{
    0x2e4353u,0x2e4357u,0x2e435au,0x2e435du,0x2e435fu,0x2e4362u,
    0x2e436fu,0x2e4377u,0x2e437fu,0x2e4382u,0x2e4387u,
    0x2e4389u,0x2e438cu,0x2e4391u,0x2e4394u,0x2e4397u,
    0x2e439cu,0x2e43a1u,0x2e43a7u,
    0x2e43b3u,0x2e43b7u,0x2e43bdu,0x2e43c7u,0x2e43cfu,0x2e43ddu,0x2e43e3u,0x2e43f2u,0x2e43f6u,
    0x2e43fdu,0x2e4403u,0x2e440fu,0x2e4412u,0x2e4416u,0x2e441eu,
    0x2e4428u,0x2e442eu,0x2e4431u,0x2e4437u,0x2e443fu,
    0x2e4442u,0x2e4445u,0x2e444au,0x2e4450u,0x2e4456u,0x2e445bu,
};
static_assert(std::size(sites)==unsigned(Access::count));

// An x87 memory operand also raises x87's denormal-operand flag. A C++
// float-to-extended cast can lower through SSE and omit that flag.
inline long double load_operand(std::uint32_t bits) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return native_live_support::x87::load_single(bits);
#else
    return fp_math::ext(fp_math::f(bits));
#endif
}

template<class State>
struct Adapter : Port<State> {
    std::uint32_t saved_output{};
    Adapter(State &state,std::uint32_t *cursor,Trace<State> *trace)
        :Port<State>(state,cursor,trace,{}){}
    std::uint32_t instruction(Access role,std::uint32_t)const{return sites[unsigned(role)];}
    void loaded(Access role,std::uint32_t,std::uint32_t bits) {
        using enum Access;
        switch(role) {
        case matrix_arg:this->gp[edx]=bits;break;
        case axis_next:this->gp[eax]=bits;break;
        case axis_last:this->gp[edi]=bits;break;
        case restore_output:this->gp[ecx]=bits;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    std::uint32_t matrix_argument()const{return saved_output+0x14u;}
    std::uint32_t output_slot()const{return saved_output;}
    std::uint32_t cycle_table()const{return this->gp[esp]+0x14u;}
    void load_extended(std::uint32_t bits){this->s.fp.push(load_operand(bits));}
    void pop(){this->s.fp.pop();}
    void add(std::uint32_t bits) {
        auto &top=this->s.fp.st(0);
        top=fp_math::add(top,load_operand(bits));
    }
    void subtract(std::uint32_t bits) {
        auto &top=this->s.fp.st(0);
        top=fp_math::sub(top,load_operand(bits));
    }
    void reverse_subtract(std::uint32_t bits) {
        auto &top=this->s.fp.st(0);
        top=fp_math::sub(load_operand(bits),top);
    }
    void multiply(std::uint32_t bits) {
        auto &top=this->s.fp.st(0);
        top=fp_math::mul(top,load_operand(bits));
    }
    void square_root() {
        auto &top=this->s.fp.st(0);
        top=physics_solver_detail::root(top);
    }
    void reciprocal(std::uint32_t bits) {
        auto &fp=this->s.fp;
        load_extended(bits);
        fp.st(0)=physics_solver_detail::divide(fp.st(0),fp.st(1));
    }
    void apply_reciprocal() {
        auto &fp=this->s.fp;
        fp.st(0)=fp_math::mul(fp.st(0),fp.st(1));
    }
    void swap_top(){std::swap(this->s.fp.st(0),this->s.fp.st(1));}
    std::uint32_t word(){return fp_math::u(fp_math::spill(this->s.fp.st(0)));}
    void compare(std::uint32_t bits,unsigned pops){this->s.fp.comparison(load_operand(bits),pops);}
    void save_comparison(){auto &accumulator=this->gp[eax];accumulator=(accumulator&0xffff0000u)|this->s.fp.word();}
    bool comparison_is_greater() {
        this->test8((this->gp[eax]>>8u)&0x41u);
        return this->flags.zf;
    }
    bool positive_trace(std::uint32_t bits) {
        compare(bits,0u);
        save_comparison();
        const bool above=comparison_is_greater();
        this->reached(above?Branch::positive:Branch::nonpositive);
        return above;
    }
    void begin_diagonal(){pop();this->push(0x2e43b2u,this->gp[ebx]);}
    void save_diagonal_frame(){this->push(0x2e43b6u,this->gp[ebp]);}
    void save_diagonal_axes() {
        auto &gp=this->gp;
        this->push(0x2e43b9u,gp[esi]);
        gp[esi]=this->flags.logic(0u,4);
        this->push(0x2e43bcu,gp[edi]);
    }
    unsigned leader_xy(){return this->gp[esi]=comparison_is_greater()?1u:0u;}
    void leader_address(unsigned axis){this->gp[eax]=axis+logic::stride*axis;}
    unsigned leader_xyz(unsigned leader) {
        save_comparison();
        const unsigned major=comparison_is_greater()?2u:leader;
        this->reached(unsigned(Branch::major_x)+major);
        return this->gp[esi]=major;
    }
    void last_term_address(unsigned major,unsigned last){this->gp[eax]=major+logic::stride*last;}
    void finish(std::uint32_t exit) {
        auto &stack=this->gp[esp];
        stack=this->flags.plus(stack,0x10u,4);
        this->ret(exit,4u);
        this->publish(exit);
    }
    void finish_positive(){finish(0x2e43adu);}
    void finish_diagonal() {
        auto &gp=this->gp;
        gp[edi]=Port<State>::pop(0x2e445eu);
        gp[esi]=Port<State>::pop(0x2e445fu);
        gp[ebp]=Port<State>::pop(0x2e4460u);
        pop();
        gp[ebx]=Port<State>::pop(0x2e4463u);
        finish(0x2e4467u);
    }
    void run() {
        saved_output=this->gp[esp]=this->flags.minus(this->gp[esp],0x10u,4);
        Memory<Adapter> memory{*this};
        logic::convert(memory,*this,this->gp[ecx]);
    }
};

template<class State,class Reference>
bool bridge(State &state,Reference reference) {
    using halo2::physics_contact_runtime_detail::source_load_x87_exception_flags;
    using halo2::physics_contact_runtime_detail::source_x87_exception_flags;
    const auto verify=verify_mode();
    const auto entry_exceptions=verify?source_x87_exception_flags():std::uint16_t{0};
    // FCOM publishes exception bits raised before it. Give replay the same
    // entry hardware flags so a later native FSQRT cannot affect that FCOM.
    const auto replay=[&](State &original) {
        const auto live_exceptions=source_x87_exception_flags();
        source_load_x87_exception_flags(entry_exceptions);
        try {
            reference(original);
        } catch(...) {
            source_load_x87_exception_flags(live_exceptions);
            throw;
        }
        const auto expected_exceptions=source_x87_exception_flags();
        source_load_x87_exception_flags(live_exceptions);
        if(live_exceptions!=expected_exceptions)
            throw std::runtime_error("Native rotation quaternion x87 exceptions differ");
    };
    return run_owner(state,0x2e4350u,0x2e4467u,reference_mode(),verify,coverage(),summary(),replay,
        [](State &value,std::uint32_t *cursor,Trace<State> *trace){return Adapter<State>{value,cursor,trace};});
}
} // namespace halo2::native_rotation_quaternion_detail
