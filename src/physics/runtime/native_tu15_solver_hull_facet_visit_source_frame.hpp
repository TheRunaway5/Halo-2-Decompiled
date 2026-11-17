#pragma once
#include "physics/runtime/native_tu15_solver_hull_facet_visit_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_solver_hull_facet_visit_detail {
template<class State,class Call,class Checkpoint> class SourceIo {
    State& s_;
    Call& call_;
    Checkpoint& checkpoint_;
    void at(std::uint32_t pc){s_.instruction_literal(pc);}
    void retired(std::uint32_t pc){checkpoint_(s_,pc);}
public:
    SourceIo(State& s,Call& call,Checkpoint& checkpoint):s_(s),call_(call),checkpoint_(checkpoint){}
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        at(pc);s_.gp[reg]=physics_step_detail::r(s_.memory,address,4u);retired(pc);
    }
    void load_low(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        at(pc);s_.set(reg,physics_step_detail::r(s_.memory,address,1u),1u,0u);retired(pc);
    }
    void push(std::uint32_t pc,std::uint32_t bits){at(pc);s_.push(pc,bits);retired(pc);}
    void pop(std::uint32_t pc,unsigned reg){at(pc);s_.gp[reg]=s_.pop();retired(pc);}
    void ret(std::uint32_t pc){at(pc);s_.pop();s_.gp[4]+=16u;}
    void store(std::uint32_t pc,std::uint32_t address,unsigned width,std::uint32_t bits) {
        at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,address,width,bits);retired(pc);
    }
    void call(std::uint32_t site,std::uint32_t returned,std::uint32_t target) {
        at(site);s_.push(site,returned);call_(s_,target,site);retired(site);
    }
    void compare_status() {
        constexpr auto pc=0x31494cu;at(pc);
        s_.flags.minus(physics_step_detail::r(s_.memory,s_.gp[6]+68u,1u),2u,1u);retired(pc);
    }
    void increment(std::uint32_t pc,std::uint32_t address) {
        at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,
            s_.flags.increment(physics_step_detail::r(s_.memory,address,4u),4u,false));
        retired(pc);
    }
};
template<class State,class Call,class Checkpoint> void run_private_source(State& s,Call call,Checkpoint checkpoint) {
    SourceIo<State,Call,Checkpoint> io{s,call,checkpoint};Schedule<State,decltype(io)> view{s,io};
    native_tu15_solver_hull_facet_visit_kernel(view);
}
} // namespace halo2::native_solver_hull_facet_visit_detail
#endif
