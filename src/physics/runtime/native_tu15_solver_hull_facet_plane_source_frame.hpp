#pragma once
#include "physics/runtime/native_tu15_solver_hull_facet_plane_frame.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_solver_hull_facet_plane_detail {
template<class State,class Checkpoint,class Estimate>class SourceIo {
    State&s_;Checkpoint&checkpoint_;Estimate&estimate_;
    void at(std::uint32_t pc){s_.instruction_literal(pc);}
    void retired(std::uint32_t pc){checkpoint_(s_,pc);}
    std::uint32_t word(std::uint32_t address){return physics_step_detail::r(s_.memory,address,4u);}
public:
    SourceIo(State&s,Checkpoint&checkpoint,Estimate&estimate):s_(s),checkpoint_(checkpoint),estimate_(estimate){}
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);s_.gp[reg]=word(address);retired(pc);}
    void push(std::uint32_t pc,std::uint32_t value){at(pc);s_.push(pc,value);retired(pc);}
    void pop(std::uint32_t pc,unsigned reg){at(pc);s_.gp[reg]=s_.pop();retired(pc);}
    void ret(std::uint32_t pc){at(pc);s_.pop();}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t bits){
        at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,bits);retired(pc);
    }
    void load_vector(std::uint32_t pc,unsigned reg,std::uint32_t address){
        at(pc);s_.xmm[reg]=physics_step_detail::v(s_.memory,address);retired(pc);
    }
    void store_vector(std::uint32_t pc,std::uint32_t address,__m128 value){
        at(pc);physics_step_detail::vw(s_.memory,s_.observe,pc,address,value);retired(pc);
    }
    void load_scalar(std::uint32_t pc,unsigned reg,std::uint32_t address){
        at(pc);s_.xmm[reg]=_mm_set_ss(physics_step_detail::f(word(address)));retired(pc);
    }
    void estimate(std::uint32_t pc){
        at(pc);
        s_.xmm[2]=_mm_move_ss(s_.xmm[2],_mm_castsi128_ps(_mm_cvtsi32_si128(
            std::bit_cast<std::int32_t>(estimate_(s_,physics_step_detail::u(_mm_cvtss_f32(s_.xmm[0])))))));
        retired(pc);
    }
};
template<class State,class Estimate,class Checkpoint=Noop>
void run_private_source(State&s,Estimate estimate,Checkpoint checkpoint={}){
    SourceIo<State,Checkpoint,Estimate>io{s,checkpoint,estimate};Schedule<State,decltype(io)>view{s,io};
    native_tu15_solver_hull_facet_plane_kernel(view);
}
} // namespace halo2::native_solver_hull_facet_plane_detail
#endif
