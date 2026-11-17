#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_solver_hull_basis_schedule.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_basis_source_detail {
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
struct LegacySpill{
    template<class State>std::uint32_t operator()(State&s)const{
        return physics_step_detail::u(physics_step_detail::spill(s.fp.st(0)));
    }
};

// These are retained instructions, not hardware transactions. In particular
// a throwing PUSH leaves the decremented stack pointer and a throwing FSTP
// observer interrupts before the modeled pop. Every complete statement may
// hand off the live state at its genuine next PC.
template<bool ClearC1,class State,class Before,class Checkpoint,class Spill,class Estimate,class Call>class SourceIo {
    State&s_;
    Before&before_;
    Checkpoint&checkpoint_;
    Spill&spill_;
    Estimate&estimate_;
    Call&call_;
    void at(std::uint32_t pc){s_.instruction_literal(pc);before_(s_,pc);}
    std::uint32_t word(std::uint32_t address){return physics_step_detail::r(s_.memory,address);}
    void clear_c1(){if constexpr(ClearC1)s_.fp.status&=~0x200u;}
    void retired(std::uint32_t pc){checkpoint_(s_,pc);}
    static __m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));}
    static std::uint32_t low(__m128 value){return physics_step_detail::u(_mm_cvtss_f32(value));}
public:
    SourceIo(State&s,Before&before,Checkpoint&checkpoint,Spill&spill,Estimate&estimate,Call&call):
        s_(s),before_(before),checkpoint_(checkpoint),spill_(spill),estimate_(estimate),call_(call){}
    static __m128 sub(__m128 a,__m128 b){return _mm_sub_ps(a,b);}
    // Retain the original destination operand. The host compiler may
    // otherwise swap commutative intrinsics and change a paired NaN payload.
    static __m128 mul(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static __m128 add_scalar(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static __m128 sub_scalar(__m128 a,__m128 b){return _mm_sub_ss(a,b);}
    static __m128 mul_scalar(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    void push(std::uint32_t pc,std::uint32_t value){at(pc);s_.push(pc,value);retired(pc);}
    void pop(std::uint32_t pc,unsigned reg){at(pc);s_.gp[reg]=s_.pop();retired(pc);}
    void ret(std::uint32_t pc){at(pc);s_.pop();}
    void load_gp(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);s_.gp[reg]=word(address);retired(pc);}
    void load_vector(std::uint32_t pc,unsigned reg,std::uint32_t address){
        at(pc);s_.xmm[reg]=physics_step_detail::v(s_.memory,address);retired(pc);
    }
    void load_scalar(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);s_.xmm[reg]=scalar(word(address));retired(pc);}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){
        at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,value);retired(pc);
    }
    void vector_store(std::uint32_t pc,std::uint32_t address,__m128 value){
        at(pc);physics_step_detail::vw(s_.memory,s_.observe,pc,address,value);retired(pc);
    }
    void load_float(std::uint32_t pc,std::uint32_t address){
        at(pc);s_.fp.push(physics_step_detail::ext(physics_step_detail::f(word(address))));clear_c1();retired(pc);
    }
    void save_float(std::uint32_t pc,std::uint32_t address,bool pop_value){
        at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,spill_(s_));
        if(pop_value)s_.fp.pop();retired(pc);
    }
    void absolute(){s_.fp.st(0)=std::fabs(s_.fp.st(0));clear_c1();}
    void negate(){s_.fp.st(0)=-s_.fp.st(0);clear_c1();}
    void drop(){clear_c1();s_.fp.pop();}
    void compare_stack(){s_.fp.comparison(s_.fp.st(1),1u);}
    void compare_float(std::uint32_t pc,std::uint32_t address){
        at(pc);s_.fp.comparison(physics_step_detail::ext(physics_step_detail::f(word(address))),1u);retired(pc);
    }
    void status_to_ax(){s_.set(0,s_.fp.word(),2,0);}
    void estimate(std::uint32_t pc,unsigned destination){
        at(pc);
        // This callback is the actual source_estimate owner, which journals
        // inputs and may throw. The source instruction retains upper lanes.
        s_.xmm[destination]=_mm_move_ss(s_.xmm[destination],scalar(estimate_(s_,low(s_.xmm[0]))));
        retired(pc);
    }
    void child(std::uint32_t pc,std::uint32_t target,std::uint32_t returned){
        at(pc);s_.push(pc,returned);call_(s_,target,pc);retired(pc);
    }
};

template<bool ClearC1,class State,class Before,class Checkpoint,class Spill,class Estimate,class Call>
void run_private_source(State&s,Before before,Checkpoint checkpoint,Spill spill,Estimate estimate,Call call){
    SourceIo<ClearC1,State,Before,Checkpoint,Spill,Estimate,Call> io{s,before,checkpoint,spill,estimate,call};
    native_solver_hull_basis_detail::Schedule<State,decltype(io)> view{s,io};
    native_tu15_solver_hull_basis_kernel(view);
}
} // namespace halo2::native_solver_hull_basis_source_detail
#endif
