#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_minimum_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_minimum_source_detail {
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
struct LegacySpill{
    template<class State>std::uint32_t operator()(State&s)const{
        return physics_step_detail::u(physics_step_detail::spill(s.fp.st(0)));
    }
};

// Source provider transport is modeled separately from hardware publication.
// Capture only values already acquired by the authored statement; later
// frame addresses and the rounded winner are read from live state.
template<bool ClearAuthoredC1,class State,class Before,class Checkpoint,class Spill>class Frame {
    State&s_;Before&before_;Checkpoint&checkpoint_;Spill&spill_;
    void at(std::uint32_t pc){s_.instruction_literal(pc);before_(s_,pc);}
    void retired(std::uint32_t next){checkpoint_(s_,next);}
    std::uint32_t word(std::uint32_t address){return physics_step_detail::r(s_.memory,address);}
    void load(std::uint32_t pc,std::uint32_t next,unsigned reg,std::uint32_t address){
        at(pc);s_.gp[reg]=word(address);retired(next);
    }
    void push(std::uint32_t pc,std::uint32_t next,std::uint32_t value){at(pc);s_.push(pc,value);retired(next);}
    void pop(std::uint32_t pc,std::uint32_t next,unsigned reg){at(pc);s_.gp[reg]=s_.pop();retired(next);}
    void clear_c1(){if constexpr(ClearAuthoredC1)s_.fp.status&=~0x200u;}
    void load_float(std::uint32_t pc,std::uint32_t next,std::uint32_t address){
        at(pc);s_.fp.push(physics_step_detail::ext(physics_step_detail::f(word(address))));
        clear_c1();retired(next);
    }
    void drop(){clear_c1();s_.fp.pop();}
    void save_candidate(std::uint32_t pc,std::uint32_t next,std::uint32_t address){
        at(pc);const auto value=spill_(s_);
        physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,value);retired(next);
    }
    bool promote(){
        s_.fp.comparison(s_.fp.st(1),1u);s_.gp[0]=(s_.gp[0]&0xffff0000u)|s_.fp.word();
        s_.flags.logic((s_.gp[0]>>8u)&5u,1u);return !s_.flags.pf;
    }
public:
    Frame(State&s,Before&before,Checkpoint&checkpoint,Spill&spill)
        :s_(s),before_(before),checkpoint_(checkpoint),spill_(spill){}
    void begin(){
        auto&g=s_.gp;push(0x3131b0u,0x3131b1u,g[1]);
        load(0x3131b1u,0x3131b7u,2u,g[1]+0x1b0u);load(0x3131b7u,0x3131bdu,1u,g[1]+0x1b4u);
        load_float(0x3131bdu,0x3131c0u,g[2]+0x10u);
        g[1]=s_.flags.plus(g[1],0xfffffffeu,4u);g[0]=g[1]+1u;s_.flags.minus(g[0],4u,4u);
        push(0x3131c9u,0x3131cau,g[6]);g[6]=g[2];
    }
    bool begin_batches(){
        if(s_.flags.sf!=s_.flags.of)return false;
        auto&g=s_.gp;g[0]=s_.flags.right(g[0],2u,4u,false);
        push(0x3131d5u,0x3131d6u,g[7]);g[7]=g[0];g[0]=s_.flags.minus(0u,g[0],4u);g[1]+=g[0]*4u;
        return true;
    }
    void batch_candidate(unsigned slot){
        constexpr std::array loads{0x3131e0u,0x3131f9u,0x313218u,0x313237u};
        constexpr std::array saves{0x3131e3u,0x3131ffu,0x31321eu,0x31323du};
        constexpr std::array compares{0x3131e7u,0x313203u,0x313222u,0x313241u};
        constexpr std::array reloads{0x3131f5u,0x313214u,0x313233u,0x313252u};
        constexpr std::array next{0x3131f9u,0x313218u,0x313237u,0x313256u};
        auto&g=s_.gp;const auto offset=(slot+1u)*0x50u;
        load_float(loads[slot],saves[slot],g[2]+offset+0x10u);
        save_candidate(saves[slot],compares[slot],g[4]+8u);
        if(promote()){drop();g[6]=g[2]+offset;load_float(reloads[slot],next[slot],g[4]+8u);}
    }
    bool next_batch(){
        auto&g=s_.gp;g[2]=s_.flags.plus(g[2],0x140u,4u);g[7]=s_.flags.increment(g[7],4u,true);
        return !s_.flags.zf;
    }
    void end_batches(){pop(0x31325fu,0x313260u,7u);}
    bool begin_tail(){
        auto&g=s_.gp;s_.flags.logic(g[1],4u);
        if(s_.flags.sf!=s_.flags.of)return false;
        g[1]=s_.flags.increment(g[1],4u,false);return true;
    }
    void tail_candidate(){
        auto&g=s_.gp;load_float(0x313265u,0x313268u,g[2]+0x60u);
        g[2]=s_.flags.plus(g[2],0x50u,4u);save_candidate(0x31326bu,0x31326fu,g[4]+4u);
        if(promote()){drop();g[6]=g[2];load_float(0x31327cu,0x313280u,g[4]+4u);}
    }
    bool next_tail(){s_.gp[1]=s_.flags.increment(s_.gp[1],4u,true);return !s_.flags.zf;}
    void finish(){
        s_.gp[0]=s_.gp[6];drop();pop(0x313287u,0x313288u,6u);pop(0x313288u,0x313289u,1u);
        at(0x313289u);s_.pop();
    }
};
template<bool ClearAuthoredC1,class State,class Before=Noop,class Checkpoint=Noop,class Spill=LegacySpill>
void run_private_source(State&s,Before before={},Checkpoint checkpoint={},Spill spill={}){
    Frame<ClearAuthoredC1,State,Before,Checkpoint,Spill>view{s,before,checkpoint,spill};
    native_tu15_solver_hull_minimum_kernel(view);
}
} // namespace halo2::native_solver_hull_minimum_source_detail
#endif
