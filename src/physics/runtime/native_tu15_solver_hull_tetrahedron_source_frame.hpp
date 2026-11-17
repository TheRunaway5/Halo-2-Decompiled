#pragma once
#include "physics/runtime/native_tu15_solver_hull_seed_facets_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_tetrahedron_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_solver_hull_tetrahedron_source_detail {
template<unsigned RetBytes,class State,class Call,class Checkpoint> class Io {
    State& s_;
    Call& call_;
    Checkpoint& checkpoint_;
    void at(std::uint32_t pc){s_.instruction_literal(pc);}
    void retired(std::uint32_t pc){checkpoint_(s_,pc);}
public:
    Io(State& s,Call& call,Checkpoint& checkpoint):s_(s),call_(call),checkpoint_(checkpoint){}
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        at(pc);s_.gp[reg]=physics_step_detail::r(s_.memory,address,4u);retired(pc);
    }
    void add_word(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        at(pc);const auto bits=physics_step_detail::r(s_.memory,address,4u);
        s_.gp[reg]=s_.flags.plus(s_.gp[reg],bits,4u);retired(pc);
    }
    void push(std::uint32_t pc,std::uint32_t value){at(pc);s_.push(pc,value);retired(pc);}
    void pop(std::uint32_t pc,unsigned reg){at(pc);s_.gp[reg]=s_.pop();retired(pc);}
    void ret(std::uint32_t pc){at(pc);s_.pop();s_.gp[4]+=RetBytes;}
    void store(std::uint32_t pc,std::uint32_t address,unsigned width,std::uint32_t value) {
        at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,address,width,value);retired(pc);
    }
    void load_vector(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        at(pc);s_.xmm[reg]=physics_step_detail::v(s_.memory,address);retired(pc);
    }
    void store_vector(std::uint32_t pc,std::uint32_t address,__m128 value) {
        at(pc);physics_step_detail::vw(s_.memory,s_.observe,pc,address,value);retired(pc);
    }
    void call(std::uint32_t site,std::uint32_t returned,std::uint32_t target) {
        at(site);s_.push(site,returned);call_(s_,target,site);retired(site);
    }
    void load_float(std::uint32_t pc,std::uint32_t address) {
        using namespace physics_step_detail;
        at(pc);s_.fp.push(ext(f(r(s_.memory,address,4u))));retired(pc);
    }
    void compare_float(std::uint32_t pc,std::uint32_t address) {
        using namespace physics_step_detail;
        at(pc);s_.fp.comparison(ext(f(r(s_.memory,address,4u))),1u);retired(pc);
    }
    std::uint16_t status() const{return s_.fp.word();}
};
template<class State,class Call,class Checkpoint> void run_seed(State& s,Call call,Checkpoint checkpoint) {
    Io<0u,State,Call,Checkpoint> io{s,call,checkpoint};
    native_solver_hull_seed_facets_detail::Schedule<State,decltype(io)> view{s,io};
    native_tu15_solver_hull_seed_facets_kernel(view);
}
template<class State,class Call,class Checkpoint> void run_tetrahedron(State& s,Call call,Checkpoint checkpoint) {
    Io<16u,State,Call,Checkpoint> io{s,call,checkpoint};
    native_solver_hull_tetrahedron_detail::Schedule<State,decltype(io)> view{s,io};
    native_tu15_solver_hull_tetrahedron_kernel(view);
}
} // namespace halo2::native_solver_hull_tetrahedron_source_detail
#endif
