#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_initialize_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_initialize_source_detail {
using P=NativeTu15SolverHullInitializePhase;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
struct LegacySingleSpill{template<class State>std::uint32_t operator()(State&s)const{return physics_step_detail::u(physics_step_detail::spill(s.fp.st(0)));}};

// This private view retains the authored software-stack and provider protocol.
// The independent physical view owns actual x87 retirement and atomic PUSHes.
// In particular a retained observer throw at FSTP precedes the software pop.
template<class State,class Call,class Before,class Checkpoint,class Spill>class Frame {
    State&s_;Call&call_;Before&before_;Checkpoint&checkpoint_;Spill&spill_;
    void at(std::uint32_t pc){s_.instruction_literal(pc);before_(s_,pc);}
    void retired(std::uint32_t pc){
        switch(pc){
        case 0x313560u:checkpoint_(s_,0x313561u);break;
        case 0x313569u:checkpoint_(s_,0x313570u);break;
        case 0x313570u:checkpoint_(s_,0x313571u);break;
        case 0x313573u:checkpoint_(s_,0x313577u);break;
        case 0x31357au:checkpoint_(s_,0x31357fu);break;
        case 0x31359cu:checkpoint_(s_,0x3135a0u);break;
        case 0x3135a0u:checkpoint_(s_,0x3135a4u);break;
        case 0x3135a4u:checkpoint_(s_,0x3135aau);break;
        case 0x3135b1u:checkpoint_(s_,0x3135b9u);break;
        case 0x3135b9u:checkpoint_(s_,0x3135c1u);break;
        case 0x3135c1u:checkpoint_(s_,0x3135c9u);break;
        case 0x3135c9u:checkpoint_(s_,0x3135d1u);break;
        case 0x3135d5u:checkpoint_(s_,0x3135d6u);break;
        case 0x3135dau:checkpoint_(s_,0x3135dbu);break;
        case 0x3135ddu:checkpoint_(s_,0x3135e2u);break;
        case 0x3135e6u:checkpoint_(s_,0x3135e7u);break;
        case 0x3135e9u:checkpoint_(s_,0x3135eeu);break;
        case 0x3135eeu:checkpoint_(s_,0x3135f1u);break;
        case 0x3135f1u:checkpoint_(s_,0x3135f4u);break;
        case 0x313611u:checkpoint_(s_,0x313615u);break;
        case 0x313615u:checkpoint_(s_,0x313619u);break;
        case 0x313619u:checkpoint_(s_,0x31361fu);break;
        case 0x313626u:checkpoint_(s_,0x313629u);break;
        case 0x313629u:checkpoint_(s_,0x31362cu);break;
        case 0x313648u:checkpoint_(s_,0x31364du);break;
        case 0x313651u:checkpoint_(s_,0x313657u);break;
        case 0x313657u:checkpoint_(s_,0x31365cu);break;
        case 0x313660u:checkpoint_(s_,0x313664u);break;
        case 0x313664u:checkpoint_(s_,0x313668u);break;
        case 0x313668u:checkpoint_(s_,0x31366du);break;
        case 0x31366du:checkpoint_(s_,0x313670u);break;
        case 0x313670u:checkpoint_(s_,0x313673u);break;
        case 0x313673u:checkpoint_(s_,0x313678u);break;
        case 0x313678u:checkpoint_(s_,0x31367cu);break;
        case 0x31367cu:checkpoint_(s_,0x313683u);break;
        case 0x313683u:checkpoint_(s_,0x313686u);break;
        case 0x313686u:checkpoint_(s_,0x313689u);break;
        case 0x313689u:checkpoint_(s_,0x31368au);break;
        case 0x31368cu:checkpoint_(s_,0x31368du);break;
        case 0x313690u:checkpoint_(s_,0x313693u);break;
        case 0x313693u:checkpoint_(s_,0x313696u);break;
        case 0x313696u:checkpoint_(s_,0x313697u);break;
        case 0x313699u:checkpoint_(s_,0x31369au);break;
        default:throw std::logic_error("Unsupported hull-initialize retained frontier");
        }
    }
    static std::uint32_t low(__m128 x){std::uint32_t value{};std::memcpy(&value,&x,4);return value;}
    std::uint32_t word(std::uint32_t address){return physics_step_detail::r(s_.memory,address);}
    void push(std::uint32_t pc,std::uint32_t value){at(pc);s_.push(pc,value);retired(pc);}
    std::uint32_t pop(std::uint32_t pc){at(pc);return s_.pop();}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4u){
        at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,address,width,value);retired(pc);
    }
    void vector_store(std::uint32_t pc,std::uint32_t address,__m128 value){
        at(pc);physics_step_detail::vw(s_.memory,s_.observe,pc,address,value);retired(pc);
    }
    void load_float(std::uint32_t pc,std::uint32_t address){
        at(pc);s_.fp.push(physics_step_detail::ext(physics_step_detail::f(word(address))));retired(pc);
    }
    bool compare_threshold(std::uint32_t pc,std::uint32_t address){
        at(pc);s_.fp.comparison(physics_step_detail::ext(physics_step_detail::f(word(address))),1u);retired(pc);
        s_.gp[0]=(s_.gp[0]&0xffff0000u)|s_.fp.word();s_.flags.logic((s_.gp[0]>>8u)&0x41u,1u);return s_.flags.pf;
    }
    void call(std::uint32_t pc,std::uint32_t target,std::uint32_t returned){
        at(pc);s_.push(pc,returned);call_(s_,target,pc);retired(pc);
    }
public:
    using Data=NativeTu15SolverHullInitialize<__m128>;
    Frame(State&s,Call&call,Before&before,Checkpoint&checkpoint,Spill&spill):s_(s),call_(call),before_(before),checkpoint_(checkpoint),spill_(spill){}
    void begin(Data&d){auto&g=s_.gp;push(0x313560u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[4]=s_.flags.minus(g[4],0x5cu,4);d.context=g[1];}
    __m128 vector(P phase,Data&d){
        std::uint32_t pc{},address{};unsigned reg{};
        switch(phase){
        case P::sign:pc=0x313569u;address=0x461960u;reg=1;break;
        case P::translation:push(0x313570u,s_.gp[6]);s_.gp[6]=s_.gp[1];pc=0x313573u;address=s_.gp[6]+0x30u;break;
        case P::difference:pc=0x3135f1u;address=s_.gp[6];break;
        case P::fresh_difference:pc=0x313626u;address=s_.gp[6];break;
        case P::rounded_length:pc=0x313657u;address=s_.gp[4]+0x20u;break;
        case P::direction:pc=0x313668u;address=s_.gp[4]+0x10u;break;
        case P::point:pc=0x313673u;address=s_.gp[4]+0x30u;break;
        }
        at(pc);const auto value=s_.xmm[reg]=physics_step_detail::v(s_.memory,address);
        if(phase==P::translation)d.sign=s_.xmm[1];retired(pc);return value;
    }
    __m128 xor_bits(__m128 a,__m128 b){return _mm_xor_ps(a,b);}
    __m128 multiply(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    __m128 add_scalar(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    template<unsigned Mask>__m128 shuffle(__m128 a,__m128 b){return _mm_shuffle_ps(a,b,Mask);}
    __m128 square_root(__m128 a){asm volatile("sqrtss %0,%0":"+x"(a)::"memory");return a;}
    void save_direction(Data&d){s_.xmm[0]=d.direction;vector_store(0x31357au,s_.gp[4]+0x10u,s_.xmm[0]);d.direction=s_.xmm[0];}
    bool initial_length_above_or_unordered(const Data&d){
        s_.xmm[0]=d.product;s_.xmm[1]=d.xy;s_.xmm[2]=d.length;s_.gp[0]=s_.gp[4]+0xcu;
        store(0x31359cu,s_.gp[0],low(s_.xmm[2]));load_float(0x3135a0u,s_.gp[4]+0xcu);return compare_threshold(0x3135a4u,0x46a180u);
    }
    void default_direction(const Data&){store(0x3135b1u,s_.gp[4]+0x10u,0u);store(0x3135b9u,s_.gp[4]+0x14u,0x3f800000u);store(0x3135c1u,s_.gp[4]+0x18u,0u);store(0x3135c9u,s_.gp[4]+0x1cu,0u);}
    void support(const Data&){auto&g=s_.gp;g[1]=g[4]+0x30u;push(0x3135d5u,g[1]);g[2]=g[4]+0x14u;push(0x3135dau,g[2]);g[1]=g[6];call(0x3135ddu,0x313390u,0x3135e2u);}
    void append(const Data&){auto&g=s_.gp;g[0]=g[4]+0x30u;push(0x3135e6u,g[0]);g[1]=g[6];call(0x3135e9u,0x313290u,0x3135eeu);}
    std::uint32_t differences_owner(const Data&){at(0x3135eeu);const auto value=s_.gp[6]=word(s_.gp[6]+0x50u);retired(0x3135eeu);return value;}
    bool difference_above_or_unordered(const Data&d){
        s_.xmm[0]=d.product;s_.xmm[1]=d.xy;s_.xmm[2]=d.length;s_.gp[1]=s_.gp[4]+0xcu;
        store(0x313611u,s_.gp[1],low(s_.xmm[2]));load_float(0x313615u,s_.gp[4]+0xcu);return compare_threshold(0x313619u,0x46a454u);
    }
    std::uint32_t output_owner(Data&d){at(0x313629u);const auto value=s_.gp[0]=word(s_.gp[5]+0xcu);d.fresh=s_.xmm[0];retired(0x313629u);return value;}
    void save_square(Data&d){s_.xmm[0]=d.square;s_.xmm[1]=d.squared_xy;s_.xmm[2]=d.squared_z;vector_store(0x313648u,s_.gp[4]+0x20u,s_.xmm[0]);d.square=s_.xmm[0];}
    void save_root(const Data&d){s_.xmm[0]=d.root;store(0x313651u,s_.gp[4]+0x20u,low(s_.xmm[0]));}
    void save_and_load_length(const Data&){s_.gp[2]=s_.gp[4]+0xcu;store(0x313660u,s_.gp[2],low(s_.xmm[0]));load_float(0x313664u,s_.gp[4]+0xcu);}
    void output_length(const Data&){
        constexpr auto pc=0x31366du;at(pc);const auto bits=spill_(s_);
        physics_step_detail::w(s_.memory,s_.observe,pc,s_.gp[0]+0x20u,4u,bits);s_.fp.pop();retired(pc);
    }
    void output_direction(const Data&){vector_store(0x313670u,s_.gp[0],s_.xmm[0]);}
    void output_point(const Data&){vector_store(0x313678u,s_.gp[0]+0x10u,s_.xmm[0]);}
    void output_weight(const Data&){store(0x31367cu,s_.gp[0]+0x24u,0x3f000000u);}
    void output_flag(const Data&d){
        const auto pc=d.produced_plane?0x313683u:0x313690u;at(pc);s_.gp[0]=word(s_.gp[5]+8u);retired(pc);
        store(d.produced_plane?0x313686u:0x313693u,s_.gp[0],d.produced_plane?1u:0u,1u);
    }
    void finish(const Data&d){auto&g=s_.gp;
        const auto si=d.produced_plane?0x313689u:0x313696u;g[6]=pop(si);retired(si);g[4]=g[5];
        const auto bp=d.produced_plane?0x31368cu:0x313699u;g[5]=pop(bp);retired(bp);
        pop(d.produced_plane?0x31368du:0x31369au);g[4]+=8u;
    }
};
template<class State,class Call,class Before=Noop,class Checkpoint=Noop,class Spill=LegacySingleSpill>void run_private_source(State&s,Call call,Before before={},Checkpoint checkpoint={},Spill spill={}){
    Frame<State,Call,Before,Checkpoint,Spill>view{s,call,before,checkpoint,spill};typename decltype(view)::Data data;
    native_tu15_solver_hull_initialize_kernel(view,data);
}
} // namespace halo2::native_solver_hull_initialize_source_detail
#endif
