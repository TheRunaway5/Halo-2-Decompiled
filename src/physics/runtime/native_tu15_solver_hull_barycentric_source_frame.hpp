#pragma once
#include "physics/runtime/native_tu15_solver_hull_barycentric_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_solver_hull_barycentric_detail {
template<bool Fresh,class State,class Checkpoint,class Spill> class SourceIo {
    State& s_;
    Checkpoint& checkpoint_;
    Spill& spill_;
    void at(std::uint32_t pc){s_.instruction_literal(pc);}
    void retired(std::uint32_t pc){checkpoint_(s_,pc);}
public:
    SourceIo(State& s,Checkpoint& checkpoint,Spill& spill):s_(s),checkpoint_(checkpoint),spill_(spill){}
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        at(pc);s_.gp[reg]=physics_step_detail::r(s_.memory,address,4u);retired(pc);
    }
    void load_vector(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        at(pc);s_.xmm[reg]=physics_step_detail::v(s_.memory,address);retired(pc);
    }
    void push(std::uint32_t pc,std::uint32_t bits){at(pc);s_.push(pc,bits);retired(pc);}
    void pop(std::uint32_t pc,unsigned reg){at(pc);s_.gp[reg]=s_.pop();retired(pc);}
    void ret(std::uint32_t pc){at(pc);s_.pop();}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t bits) {
        at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,bits);retired(pc);
    }
    void scalar(std::uint32_t pc,std::uint32_t address,Scalar op) {
        using namespace physics_step_detail;
        at(pc);const auto value=ext(f(r(s_.memory,address,4u)));auto& fp=s_.fp;
        switch(op) {
        case Scalar::load:fp.push(value);if constexpr(Fresh)fp.status&=~0x200u;break;
        case Scalar::multiply:fp.st(0)=mul(fp.st(0),value);break;
        case Scalar::divide:fp.st(0)=physics_solver_detail::divide(fp.st(0),value);break;
        case Scalar::subtract:fp.st(0)=sub(fp.st(0),value);break;
        case Scalar::compare_pop:fp.comparison(value,1u);break;
        }
        retired(pc);
    }
    void stack(Stack op) {
        using namespace physics_step_detail;
        auto& fp=s_.fp;
        switch(op) {
        case Stack::duplicate:fp.push(fp.st(0));if constexpr(Fresh)fp.status&=~0x200u;break;
        case Stack::discard:if constexpr(Fresh)fp.status&=~0x200u;fp.pop();break;
        case Stack::subtract_pop:fp.st(1)=sub(fp.st(1),fp.st(0));fp.pop();break;
        case Stack::subtract_st1:fp.st(0)=sub(fp.st(0),fp.st(1));break;
        case Stack::multiply_st1:fp.st(0)=mul(fp.st(0),fp.st(1));break;
        case Stack::multiply_st2:fp.st(0)=mul(fp.st(0),fp.st(2));break;
        }
    }
    std::uint16_t status()const{return s_.fp.word();}
    void spill(std::uint32_t pc,std::uint32_t address,bool pop) {
        at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,spill_(s_));
        if(pop)s_.fp.pop();
        retired(pc);
    }
};
template<bool Fresh,class State,class Checkpoint,class Spill> void run_private_source(State& s,Checkpoint checkpoint,Spill spill) {
    SourceIo<Fresh,State,Checkpoint,Spill> io{s,checkpoint,spill};Schedule<State,decltype(io)> view{s,io};
    native_tu15_solver_hull_barycentric_kernel(view);
}
} // namespace halo2::native_solver_hull_barycentric_detail
#endif
