#pragma once
#include "halo2/native_tu15_solver_hull_cleanup_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_cleanup_source_detail {
using Storage=NativeTu15SolverHullStorage;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};

// Private retained-publication adapter. Authored FS accesses are ordinary
// provider operations addressed with the live source FS base. This is not a
// native exception handler: never intercept an allocator exception or assume
// its virtual target returned with an earlier GP tuple.
template<class State,class Before,class Checkpoint,class Call>class Frame {
    State&s_;
    Before&before_;
    Checkpoint&checkpoint_;
    Call&call_;
    void at(std::uint32_t pc){s_.instruction_literal(pc);before_(s_,pc);}
    void retired(std::uint32_t pc){checkpoint_(s_,pc);}
    std::uint32_t word(std::uint32_t address){return physics_step_detail::r(s_.memory,address);}
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);s_.gp[reg]=word(address);retired(pc);}
    void push(std::uint32_t pc,std::uint32_t value){at(pc);s_.push(pc,value);retired(pc);}
    void pop(std::uint32_t pc,unsigned reg){at(pc);s_.gp[reg]=s_.pop();retired(pc);}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4){
        at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,address,width,value);retired(pc);
    }
    void release(std::uint32_t pc,unsigned table_register){
        at(pc);const auto target=word(s_.gp[table_register]+0x14u);
        s_.push(pc,pc+3u);call_(s_,target,pc,false);retired(pc);
    }
public:
    Frame(State&s,Before&before,Checkpoint&checkpoint,Call&call):s_(s),before_(before),checkpoint_(checkpoint),call_(call){}
    void open_frame(){
        auto&g=s_.gp;
        push(0x3139f0u,0xffffffffu);push(0x3139f2u,0x37f1b9u);
        load(0x3139f7u,0u,s_.fs_base);push(0x3139fdu,g[0]);store(0x3139feu,s_.fs_base,g[4]);
        push(0x313a05u,g[1]);push(0x313a06u,g[6]);g[6]=g[1];push(0x313a09u,g[7]);
        store(0x313a0au,g[4]+8u,g[6]);
    }
    bool owns_storage(Storage storage){
        auto&g=s_.gp;
        switch(storage){
        case Storage::stride_32:
            load(0x313a0eu,0u,g[6]+0x2108u);s_.flags.logic(g[0],4);store(0x313a16u,g[4]+0x14u,1u);break;
        case Storage::stride_80:
            load(0x313a3du,0u,g[6]+0x1b8u);s_.flags.logic(g[0],4);store(0x313a45u,g[4]+0x14u,0u,1u);break;
        case Storage::stride_4:
            load(0x313a6cu,0u,g[6]+0x10u);s_.flags.logic(g[0],4);store(0x313a71u,g[4]+0x14u,0xffffffffu);break;
        }
        return !s_.flags.sf;
    }
    void release_stride_32(){
        auto&g=s_.gp;
        load(0x313a20u,2u,g[6]+0x2100u);load(0x313a26u,1u,0x48c938u);load(0x313a2cu,7u,g[1]);
        g[0]=s_.flags.logic(g[0]&0x7fffffffu,4);push(0x313a33u,0x12u);
        g[0]=s_.flags.shift(g[0],5u,4);push(0x313a38u,g[0]);push(0x313a39u,g[2]);release(0x313a3au,7u);
    }
    void release_stride_80(){
        auto&g=s_.gp;
        load(0x313a4cu,2u,g[6]+0x1b0u);load(0x313a52u,1u,0x48c938u);load(0x313a58u,7u,g[1]);
        g[0]=s_.flags.logic(g[0]&0x7fffffffu,4);g[0]+=g[0]*4u;push(0x313a62u,0x12u);
        g[0]=s_.flags.shift(g[0],4u,4);push(0x313a67u,g[0]);push(0x313a68u,g[2]);release(0x313a69u,7u);
    }
    void release_stride_4(){
        auto&g=s_.gp;
        load(0x313a7bu,1u,0x48c938u);load(0x313a81u,2u,g[1]);
        g[0]=s_.flags.logic(g[0]&0x7fffffffu,4);push(0x313a88u,0x12u);
        g[0]=s_.flags.shift(g[0],2u,4);push(0x313a8du,g[0]);
        load(0x313a8eu,0u,g[6]+8u);push(0x313a91u,g[0]);release(0x313a92u,2u);
    }
    void close_frame(){
        auto&g=s_.gp;
        load(0x313a95u,1u,g[4]+0xcu);pop(0x313a99u,7u);pop(0x313a9au,6u);
        store(0x313a9bu,s_.fs_base,g[1]);g[4]=s_.flags.plus(g[4],0x10u,4);
        at(0x313aa5u);s_.pop();
    }
};
template<class State,class Call,class Before=Noop,class Checkpoint=Noop>
void run_private_source(State&s,Call call,Before before={},Checkpoint checkpoint={}){
    Frame<State,Before,Checkpoint,Call> view{s,before,checkpoint,call};
    native_tu15_solver_hull_cleanup_kernel(view);
}
} // namespace halo2::native_solver_hull_cleanup_source_detail
