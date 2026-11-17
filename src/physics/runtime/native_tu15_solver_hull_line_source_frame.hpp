#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_line_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_line_source_detail {
using P=NativeTu15SolverHullLinePhase;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};

// Retain the generated owner's State::push, w/vw and software x87
// publication. This is intentionally separate from the physical view.
template<bool ClearLoadC1,class State,class Call,class Before,class Checkpoint>class Frame {
    State&s_;Call&call_;Before&before_;Checkpoint&checkpoint_;
    void at(std::uint32_t pc){s_.instruction_literal(pc);before_(s_,pc);}
    void retired(std::uint32_t next){checkpoint_(s_,next);}
    static std::uint32_t low(__m128 x){std::uint32_t value{};std::memcpy(&value,&x,4);return value;}
    std::uint32_t word(std::uint32_t address){return physics_step_detail::r(s_.memory,address);}
    void load_gp(std::uint32_t pc,std::uint32_t next,unsigned reg,std::uint32_t address){
        at(pc);s_.gp[reg]=word(address);retired(next);
    }
    void push(std::uint32_t pc,std::uint32_t next,std::uint32_t value){at(pc);s_.push(pc,value);retired(next);}
    void pop(std::uint32_t pc,std::uint32_t next,unsigned reg){at(pc);s_.gp[reg]=s_.pop();retired(next);}
    void store(std::uint32_t pc,std::uint32_t next,std::uint32_t address,std::uint32_t value){
        at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,value);retired(next);
    }
    void child(std::uint32_t pc,std::uint32_t target,std::uint32_t returned){
        at(pc);s_.push(pc,returned);call_(s_,target,pc);retired(returned);
    }
public:
    using Data=NativeTu15SolverHullLine<__m128>;
    static constexpr bool first_sum_before_sign=false;
    Frame(State&s,Call&call,Before&before,Checkpoint&checkpoint):s_(s),call_(call),before_(before),checkpoint_(checkpoint){}
    void first_support(Data&){
        auto&g=s_.gp;push(0x3136a0u,0x3136a1u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[4]=s_.flags.minus(g[4],0x78u,4);
        push(0x3136a9u,0x3136aau,g[6]);push(0x3136aau,0x3136abu,g[7]);g[6]=g[1];
        load_gp(0x3136adu,0x3136b0u,7u,g[6]+0x50u);
        g[0]=g[4]+0x20u;push(0x3136b4u,0x3136b5u,g[0]);push(0x3136b5u,0x3136b6u,g[7]);child(0x3136b6u,0x313390u,0x3136bbu);
    }
    __m128 vector(P phase,Data&d){
        std::uint32_t pc{},next{},address{};unsigned reg{};
        switch(phase){
        case P::first_vertex:
            load_gp(0x3136bbu,0x3136beu,1u,s_.gp[6]+0x50u);pc=0x3136beu;next=0x3136c1u;address=s_.gp[1];reg=1;break;
        case P::first_transform:pc=0x3136c1u;next=0x3136c6u;address=s_.gp[4]+0x40u;break;
        case P::first_point:pc=0x3136c6u;next=0x3136cbu;address=s_.gp[4]+0x20u;reg=2;break;
        case P::sign:
            s_.xmm[0]=d.squared;s_.xmm[1]=d.xy;s_.xmm[2]=d.z;s_.gp[0]=s_.gp[4]+0x50u;
            pc=0x3136edu;next=0x3136f4u;address=0x461960u;break;
        case P::direction:s_.xmm[2]=d.score;pc=0x3136f8u;next=0x3136fbu;address=s_.gp[7];reg=1;break;
        case P::second_transform:pc=0x31371cu;next=0x313721u;address=s_.gp[4]+0x70u;break;
        case P::second_point:pc=0x313724u;next=0x313729u;address=s_.gp[4]+0x50u;reg=2;break;
        case P::second_vertex:pc=0x313729u;next=0x31372cu;address=s_.gp[2];reg=1;break;
        }
        at(pc);const auto result=s_.xmm[reg]=physics_step_detail::v(s_.memory,address);
        if(phase==P::first_point){d.vertex=s_.xmm[1];d.transformed=s_.xmm[0];}
        if(phase==P::sign){d.xy=s_.xmm[1];d.z=s_.xmm[2];}
        if(phase==P::second_vertex){d.transformed=s_.xmm[0];d.point=s_.xmm[2];}
        retired(next);return result;
    }
    __m128 subtract(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    __m128 multiply(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    __m128 add_scalar(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    template<unsigned Mask>__m128 shuffle(__m128 a,__m128 b){return _mm_shuffle_ps(a,b,Mask);}
    __m128 xor_bits(__m128 a,__m128 b){return _mm_xor_ps(a,b);}
    void prepare_second(Data&d){
        auto&g=s_.gp;push(0x3136fbu,0x3136fcu,g[0]);g[1]=g[4]+0x14u;g[2]=g[4]+0xcu;push(0x313704u,0x313705u,g[1]);
        d.direction=s_.xmm[1];d.sign=s_.xmm[0];
    }
    void second_support(const Data&d){
        auto&g=s_.gp;s_.xmm[1]=d.negative;g[1]=g[6];store(0x31370au,0x31370eu,g[2],low(s_.xmm[2]));
        at(0x31370eu);physics_step_detail::vw(s_.memory,s_.observe,0x31370eu,g[4]+0x18u,s_.xmm[1]);retired(0x313713u);
        child(0x313713u,0x313390u,0x313718u);
    }
    void load_first_score(const Data&){
        at(0x313718u);s_.fp.push(physics_step_detail::ext(physics_step_detail::f(word(s_.gp[4]+8u))));
        if constexpr(ClearLoadC1)s_.fp.status&=~0x200u;
        retired(0x31371cu);
    }
    void second_vertex_owner(const Data&){load_gp(0x313721u,0x313724u,2u,s_.gp[6]+0x50u);}
    bool compare_second(const Data&d){
        s_.xmm[0]=d.squared;s_.xmm[1]=d.xy;s_.xmm[2]=d.score;s_.gp[0]=s_.gp[4]+0xcu;
        store(0x313752u,0x313756u,s_.gp[0],low(s_.xmm[2]));at(0x313756u);
        s_.fp.comparison(physics_step_detail::ext(physics_step_detail::f(word(s_.gp[4]+0xcu))),1u);retired(0x31375au);
        s_.gp[0]=(s_.gp[0]&0xffff0000u)|s_.fp.word();s_.flags.logic((s_.gp[0]>>8u)&1u,1u);return !s_.flags.zf;
    }
    void append(const Data&d){
        auto&g=s_.gp;
        if(d.second){g[2]=g[4]+0x50u;push(0x313777u,0x313778u,g[2]);g[1]=g[6];child(0x31377au,0x313290u,0x31377fu);}
        else{g[1]=g[4]+0x20u;push(0x313765u,0x313766u,g[1]);g[1]=g[6];child(0x313768u,0x313290u,0x31376du);}
    }
    void finish(const Data&d){
        pop(d.second?0x31377fu:0x31376du,d.second?0x313780u:0x31376eu,7u);
        pop(d.second?0x313780u:0x31376eu,d.second?0x313781u:0x31376fu,6u);
        s_.gp[4]=s_.gp[5];pop(d.second?0x313783u:0x313771u,d.second?0x313784u:0x313772u,5u);
        at(d.second?0x313784u:0x313772u);s_.pop();
    }
};
template<bool ClearLoadC1,class State,class Call,class Before=Noop,class Checkpoint=Noop>
void run_private_source(State&s,Call call,Before before={},Checkpoint checkpoint={}){
    Frame<ClearLoadC1,State,Call,Before,Checkpoint>view{s,call,before,checkpoint};typename decltype(view)::Data data;
    native_tu15_solver_hull_line_kernel(view,data);
}
} // namespace halo2::native_solver_hull_line_source_detail
#endif
