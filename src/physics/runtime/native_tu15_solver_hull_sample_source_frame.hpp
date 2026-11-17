#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_sample_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_sample_source_detail {
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
struct LegacySpill{
    template<class State>std::uint32_t operator()(State&s)const{
        return physics_step_detail::u(physics_step_detail::spill(s.fp.st(0)));
    }
};

// The source view keeps modeled EFLAGS, authored software x87 operations
// and w-before-pop publication. The independent hardware frame uses real
// IMUL flags and commits FSTP before its completed observer.
template<bool ClearAuthoredC1,class State,class Before,class Checkpoint,class Spill>class Frame {
    State&s_;Before&before_;Checkpoint&checkpoint_;Spill&spill_;
    void at(std::uint32_t pc){s_.instruction_literal(pc);before_(s_,pc);}
    void retired(std::uint32_t next){checkpoint_(s_,next);}
    std::uint32_t word(std::uint32_t address){return physics_step_detail::r(s_.memory,address);}
    long double single(std::uint32_t address){return physics_step_detail::ext(physics_step_detail::f(word(address)));}
    void store(std::uint32_t pc,std::uint32_t next,std::uint32_t address,std::uint32_t value){
        at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,value);retired(next);
    }
    void product(unsigned reg){s_.gp[reg]=s_.flags.product(s_.gp[reg],native_solver_hull_sample_multiplier);}
    void next(unsigned out,unsigned in){
        s_.gp[out]=s_.flags.minus(s_.gp[out],s_.gp[in],4u);
        s_.gp[out]=s_.flags.logic(s_.gp[out]&native_solver_hull_sample_mask,4u);
    }
    void clear_c1(){
        if constexpr(ClearAuthoredC1)s_.fp.status&=~0x200u;
    }
public:
    Frame(State&s,Before&before,Checkpoint&checkpoint,Spill&spill):s_(s),before_(before),checkpoint_(checkpoint),spill_(spill){}
    void begin_range(){
        auto&g=s_.gp;at(0x313300u);g[1]=word(g[4]+4u);retired(0x313304u);
        at(0x313304u);s_.fp.push(single(g[4]+0xcu));clear_c1();retired(0x313308u);
        at(0x313308u);s_.fp.st(0)=physics_step_detail::sub(s_.fp.st(0),single(g[4]+8u));retired(0x31330cu);
    }
    void begin_seed(){
        auto&g=s_.gp;product(1u);at(0x313312u);g[2]=word(g[4]+0x10u);retired(0x313316u);
        g[0]=native_solver_hull_sample_offset;next(0u,1u);store(0x313322u,0x313326u,g[4]+4u,g[0]);product(0u);
    }
    void scale_sample(unsigned axis){
        const auto load_pc=std::array{0x31332cu,0x313355u,0x313378u}[axis];
        const auto scale_pc=std::array{0x313330u,0x313359u,0x31337cu}[axis];
        at(load_pc);s_.fp.push(static_cast<long double>(physics_step_detail::si(word(s_.gp[4]+4u))));clear_c1();retired(scale_pc);
        at(scale_pc);s_.fp.st(0)=physics_step_detail::mul(s_.fp.st(0),single(0x41d780u));
        retired(std::array{0x313336u,0x31335fu,0x313382u}[axis]);
        if(axis<2u)s_.gp[1u-axis]=native_solver_hull_sample_offset;
    }
    void add_lower(unsigned axis){
        at(std::array{0x31333bu,0x313364u,0x313382u}[axis]);
        s_.fp.st(0)=physics_step_detail::add(s_.fp.st(0),single(s_.gp[4]+8u));
        retired(std::array{0x31333fu,0x313368u,0x313386u}[axis]);
        if(axis<2u)next(1u-axis,axis);
    }
    void scale_range(){s_.fp.st(0)=physics_step_detail::mul(s_.fp.st(0),s_.fp.st(1));}
    void advance_seed(unsigned axis){
        store(axis?0x313371u:0x313349u,axis?0x313375u:0x31334du,s_.gp[4]+4u,s_.gp[1u-axis]);
        if(!axis)product(1u);
    }
    void publish_sample(unsigned axis){
        const auto pc=std::array{0x313353u,0x313375u,0x313388u}[axis];
        at(pc);const auto bits=spill_(s_);
        physics_step_detail::w(s_.memory,s_.observe,pc,s_.gp[2]+axis*4u,4u,bits);
        s_.fp.pop();retired(std::array{0x313355u,0x313378u,0x31338bu}[axis]);
    }
    void finish(){clear_c1();s_.fp.pop();at(0x31338du);s_.pop();s_.gp[4]+=16u;}
};
template<bool ClearAuthoredC1,class State,class Before=Noop,class Checkpoint=Noop,class Spill=LegacySpill>
void run_private_source(State&s,Before before={},Checkpoint checkpoint={},Spill spill={}){
    Frame<ClearAuthoredC1,State,Before,Checkpoint,Spill>view{s,before,checkpoint,spill};
    native_tu15_solver_hull_sample_kernel(view);
}
} // namespace halo2::native_solver_hull_sample_source_detail
#endif
