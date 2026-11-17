#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_facet_slots_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_facet_slots_detail {
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State,class Io>class Schedule {
    State&s_;Io&io_;
public:
    Schedule(State&s,Io&io):s_(s),io_(io){}
    bool begin(){
        auto&g=s_.gp;
        io_.load(0x3149b0u,2u,g[1]+12u);io_.load(0x3149b3u,0u,g[4]+4u);
        io_.push(0x3149b7u,g[3]);io_.push(0x3149b8u,g[6]);
        io_.load(0x3149b9u,6u,g[4]+16u);g[2]=s_.flags.increment(g[2],4u,true);s_.flags.logic(g[6],4u);
        io_.push(0x3149c0u,g[7]);return !s_.flags.zf;
    }
    void begin_reuse(){io_.push(0x3149c3u,s_.gp[5]);}
    bool have_reusable_slot(){s_.flags.logic(s_.gp[2],4u);return s_.flags.sf==s_.flags.of;}
    void reuse_slot(){
        auto&g=s_.gp;
        io_.load(0x3149c8u,7u,g[1]+8u);io_.load(0x3149cbu,7u,g[7]+g[2]*4u);
        io_.load(0x3149ceu,3u,g[0]+4u);io_.load(0x3149d1u,5u,g[0]);
        io_.store(0x3149d3u,g[5]+g[3]*4u,g[7]);io_.increment_output_count();
        io_.load(0x3149dau,7u,g[1]+12u);g[7]=s_.flags.increment(g[7],4u,true);
        g[6]=s_.flags.increment(g[6],4u,true);g[2]=s_.flags.increment(g[2],4u,true);
        s_.flags.logic(g[6],4u);io_.store(0x3149e2u,g[1]+12u,g[7]);
    }
    bool reuse_request_done(){return s_.flags.zf;}
    bool finish_reuse(){
        s_.flags.logic(s_.gp[6],4u);io_.pop(0x3149e9u,5u);return !s_.flags.zf&&s_.flags.sf==s_.flags.of;
    }
    void append_slot(){
        auto&g=s_.gp;
        io_.load(0x3149f0u,2u,g[1]+0x1b4u);io_.load(0x3149f6u,3u,g[1]+0x1b0u);
        g[7]=g[2]+1u;io_.store(0x3149ffu,g[1]+0x1b4u,g[7]);
        io_.load(0x314a05u,7u,g[0]+4u);
        g[2]=s_.flags.shift(g[2]+g[2]*4u,4u,4u);g[2]=s_.flags.plus(g[2],g[3],4u);
        io_.load(0x314a10u,3u,g[0]);io_.store(0x314a12u,g[3]+g[7]*4u,g[2]);
        io_.load(0x314a15u,7u,g[0]+4u);g[7]=s_.flags.increment(g[7],4u,false);g[6]=s_.flags.increment(g[6],4u,true);
        io_.store(0x314a1au,g[0]+4u,g[7]);
    }
    bool append_request_done(){return s_.flags.zf;}
    void finish(){io_.pop(0x314a1fu,7u);io_.pop(0x314a20u,6u);io_.pop(0x314a21u,3u);io_.ret(0x314a22u);}
};

template<class State,class Before>class PhysicalIo:
    public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;
public:
    using Base::push;
    PhysicalIo(State&s,Before&before,native_raw_x87_detail::Scope<State>&fp)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}}{}
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);s_.gp[reg]=word(address);}
    void pop(std::uint32_t pc,unsigned reg){s_.gp[reg]=Base::pop(pc);}
    void ret(std::uint32_t pc){Base::pop(pc);s_.gp[4]+=8u;}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){Base::store(pc,address,value,[]{});}
    void increment_output_count(){
        constexpr auto pc=0x3149d7u;at(pc);const auto address=s_.gp[0]+4u;
        auto flags=s_.flags;const auto incremented=flags.increment(word(address),4u,false);
        Base::store(pc,address,incremented,[&]{s_.flags=flags;},true);
    }
};
template<class State,class Checkpoint>class SourceIo {
    State&s_;Checkpoint&checkpoint_;
    void at(std::uint32_t pc){s_.instruction_literal(pc);}
    void retired(std::uint32_t pc){checkpoint_(s_,pc);}
public:
    SourceIo(State&s,Checkpoint&checkpoint):s_(s),checkpoint_(checkpoint){}
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){
        at(pc);s_.gp[reg]=physics_step_detail::r(s_.memory,address,4u);retired(pc);
    }
    void push(std::uint32_t pc,std::uint32_t value){at(pc);s_.push(pc,value);retired(pc);}
    void pop(std::uint32_t pc,unsigned reg){at(pc);s_.gp[reg]=s_.pop();retired(pc);}
    void ret(std::uint32_t pc){at(pc);s_.pop();s_.gp[4]+=8u;}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){
        at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,value);retired(pc);
    }
    void increment_output_count(){
        constexpr auto pc=0x3149d7u;at(pc);
        physics_step_detail::w(s_.memory,s_.observe,pc,s_.gp[0]+4u,4u,
            s_.flags.increment(physics_step_detail::r(s_.memory,s_.gp[0]+4u,4u),4u,false));
        retired(pc);
    }
};
template<class State,class Before=Noop>void run_private_owner(State&s,Before before={}){
    if(!native_raw_x87_detail::supported(s))throw std::runtime_error("Private hull facet slots require dense masked FP transport");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before>io{s,before,fp};Schedule<State,decltype(io)>view{s,io};
    native_tu15_solver_hull_facet_slots_kernel(view);
}
template<class State,class Checkpoint=Noop>void run_private_source(State&s,Checkpoint checkpoint={}){
    SourceIo<State,Checkpoint>io{s,checkpoint};Schedule<State,decltype(io)>view{s,io};
    native_tu15_solver_hull_facet_slots_kernel(view);
}
} // namespace halo2::native_solver_hull_facet_slots_detail
#endif
