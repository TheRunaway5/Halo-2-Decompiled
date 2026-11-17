#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_clear_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_clear_detail {
using Table=NativeTu15SolverHullClearTable;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State,class Io>class Schedule {
    State&s_;
    Io&io_;
public:
    Schedule(State&s,Io&io):s_(s),io_(io){}
    void begin(){
        auto&g=s_.gp;io_.load(0x314870u,2u,g[1]+0x1b4u);io_.push(0x314876u,g[3]);
        g[3]=s_.flags.logic(0,4);g[0]=s_.flags.logic(0,4);s_.flags.minus(g[2],g[3],4);
        io_.push(0x31487du,g[6]);
    }
    bool start(Table table){
        auto&g=s_.gp;
        if(table==Table::edges){
            io_.load(0x3148a8u,2u,g[1]+0x2104u);g[0]=s_.flags.logic(0,4);s_.flags.minus(g[2],g[3],4);
        }
        if(s_.flags.zf||s_.flags.sf!=s_.flags.of)return false;
        g[2]=s_.flags.logic(0,4);return true;
    }
    void clear(Table table){
        auto&g=s_.gp;const auto layout=native_tu15_solver_hull_clear_layout(table);
        io_.load(table==Table::facets?0x314890u:0x3148c0u,6u,g[1]+layout.base);
        io_.store(layout.store_pc,g[6]+g[2]+layout.field,g[3],layout.width);
    }
    bool next(Table table){
        auto&g=s_.gp;const auto layout=native_tu15_solver_hull_clear_layout(table);
        io_.load(table==Table::facets?0x31489au:0x3148cau,6u,g[1]+layout.count);
        g[0]=s_.flags.increment(g[0],4,false);g[2]=s_.flags.plus(g[2],layout.stride,4);
        s_.flags.minus(g[0],g[6],4);return s_.flags.sf!=s_.flags.of;
    }
    void finish(){io_.pop(0x3148d8u,6u);io_.pop(0x3148d9u,3u);io_.ret(0x3148dau);}
};

template<class State,class Before>class PhysicalIo:
    public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;
    native_raw_x87_detail::Scope<State>&fp_;
public:
    using Base::push;
    PhysicalIo(State&s,Before&before,native_raw_x87_detail::Scope<State>&fp):
        Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp){}
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);s_.gp[reg]=word(address);}
    void pop(std::uint32_t pc,unsigned reg){s_.gp[reg]=Base::pop(pc);}
    void ret(std::uint32_t pc){Base::pop(pc);}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width){
        at(pc);std::array<std::byte,4>bytes{};std::memcpy(bytes.data(),&value,4);
        const auto payload=std::span(bytes).first(width);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);fp_.io([&]{s_.memory.read(address,event.before);});}
        fp_.io([&]{s_.memory.write(address,payload);});
        if(s_.observe){event.written.assign(payload.begin(),payload.end());fp_.io([&]{s_.observe(event);});}
    }
};

template<class State,class Checkpoint>class SourceIo {
    State&s_;Checkpoint&checkpoint_;
    void at(std::uint32_t pc){s_.instruction_literal(pc);}
    void retired(std::uint32_t pc){checkpoint_(s_,pc);}
public:
    SourceIo(State&s,Checkpoint&checkpoint):s_(s),checkpoint_(checkpoint){}
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);s_.gp[reg]=physics_step_detail::r(s_.memory,address,4u);retired(pc);}
    void push(std::uint32_t pc,std::uint32_t value){at(pc);s_.push(pc,value);retired(pc);}
    void pop(std::uint32_t pc,unsigned reg){at(pc);s_.gp[reg]=s_.pop();retired(pc);}
    void ret(std::uint32_t pc){at(pc);s_.pop();}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width){
        at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,address,width,value);retired(pc);
    }
};

template<class State,class Before=Noop>void run_private_owner(State&s,Before before={}){
    if(!native_raw_x87_detail::supported(s))throw std::runtime_error("Private hull table clear requires dense masked FP transport");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before> io{s,before,fp};Schedule<State,decltype(io)>view{s,io};
    native_tu15_solver_hull_clear_kernel(view);
}
template<class State,class Checkpoint=Noop>void run_private_source(State&s,Checkpoint checkpoint={}){
    SourceIo<State,Checkpoint> io{s,checkpoint};Schedule<State,decltype(io)>view{s,io};native_tu15_solver_hull_clear_kernel(view);
}
} // namespace halo2::native_solver_hull_clear_detail
#endif
