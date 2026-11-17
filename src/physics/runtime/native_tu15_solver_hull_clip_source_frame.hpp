#pragma once
#include "halo2/native_tu15_solver_hull_clip_kernel.hpp"
#include <xmmintrin.h>

namespace halo2::native_solver_hull_clip_source_detail {
// Original 3147EE MULPS xmm0,xmm1. With NaNs in both operands x86 returns the
// destination's quieted NaN, so the destination must stay the destination.
inline __m128 ordered_multiply(__m128 destination,__m128 source){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("mulps %1,%0":"+x"(destination):"x"(source):"memory");return destination;
#else
    return _mm_mul_ps(destination,source);
#endif
}
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_clip_source_detail {
using namespace physics_step_detail;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
struct LegacySpill{template<class State>std::uint32_t operator()(State&s)const{return u(spill(s.fp.st(0)));}};

// The retained body owns software x87 state and State::push/w publication.
// The provider/observer must see that state before the source FSTP's pop.
template<bool ClearC1,class State,class Checkpoint,class Spill>class Frame {
    State&s_;Checkpoint&checkpoint_;Spill&spill_;
    void at(std::uint32_t pc){s_.instruction_literal(pc);}
    void retired(std::uint32_t pc){checkpoint_(s_,pc);}
    std::uint32_t word(std::uint32_t address){return r(s_.memory,address,4u);}
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);s_.gp[reg]=word(address);retired(pc);}
    void load_vector(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);s_.xmm[reg]=v(s_.memory,address);retired(pc);}
    void push(std::uint32_t pc,std::uint32_t value){at(pc);s_.push(pc,value);retired(pc);}
    void pop(std::uint32_t pc,unsigned reg){at(pc);s_.gp[reg]=s_.pop();retired(pc);}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){
        at(pc);w(s_.memory,s_.observe,pc,address,4u,value);retired(pc);
    }
    void load_float(std::uint32_t pc,std::uint32_t address){
        at(pc);s_.fp.push(ext(f(word(address))));
        if constexpr(ClearC1)s_.fp.status&=~0x200u;
        retired(pc);
    }
    void compare(std::uint32_t pc,std::uint32_t address,unsigned pop_count){
        at(pc);s_.fp.comparison(ext(f(word(address))),pop_count);retired(pc);
    }
    void test_status(std::uint32_t mask){
        s_.gp[0]=(s_.gp[0]&0xffff0000u)|s_.fp.word();
        s_.flags.logic((s_.gp[0]>>8u)&mask,1u);
    }
public:
    Frame(State&s,Checkpoint&checkpoint,Spill&spill):s_(s),checkpoint_(checkpoint),spill_(spill){}
    void project_and_compare_lower(){
        auto&g=s_.gp;auto&x=s_.xmm;
        push(0x3147d0u,g[5]);g[5]=g[4];
        g[4]=s_.flags.logic(g[4]&0xfffffff0u,4u);g[4]=s_.flags.minus(g[4],12u,4u);
        load(0x3147d9u,0u,g[1]+0x14u);load_vector(0x3147dcu,0u,g[0]);
        load(0x3147dfu,2u,g[5]+8u);load_vector(0x3147e2u,1u,g[2]);x[1]=_mm_sub_ps(x[1],x[0]);
        load_vector(0x3147e8u,0u,g[1]);load(0x3147ebu,1u,g[5]+12u);
        x[0]=ordered_multiply(x[0],x[1]);x[1]=x[0];x[1]=_mm_shuffle_ps(x[1],x[0],0x55);x[2]=x[0];g[0]=g[4]+8u;
        x[1]=_mm_add_ss(x[1],x[0]);x[2]=_mm_shuffle_ps(x[2],x[0],0xaa);x[2]=_mm_add_ss(x[2],x[1]);
        store(0x31480bu,g[0],u(_mm_cvtss_f32(x[2])));
        load_float(0x31480fu,g[4]+8u);compare(0x314813u,g[1],1u);
        push(0x314815u,g[6]);load(0x314816u,6u,g[5]+16u);load_float(0x314819u,g[4]+12u);
        g[2]=1u;test_status(0x41u);
    }
    bool use_upper(){return s_.flags.zf;}
    bool update_upper(){compare(0x314829u,s_.gp[6],1u);test_status(5u);return !s_.flags.pf;}
    void publish_upper(){
        auto&g=s_.gp;load(0x314832u,0u,g[4]+12u);
        store(0x314836u,g[6],g[0]);
    }
    bool update_lower(){
        at(0x31483au);s_.fp.st(0)=add(s_.fp.st(0),ext(f(word(0x41d7a0u))));retired(0x31483au);
        compare(0x314840u,s_.gp[1],0u);test_status(0x41u);return s_.flags.zf;
    }
    void publish_lower(){
        at(0x314849u);
        w(s_.memory,s_.observe,0x314849u,s_.gp[1],4u,spill_(s_));s_.fp.pop();retired(0x314849u);
    }
    void discard_lower(){
        s_.fp.st(0)=s_.fp.st(0);
        if constexpr(ClearC1)s_.fp.status&=~0x200u;
        s_.fp.pop();
    }
    void mark_lower(){s_.gp[2]=2u;}
    std::uint32_t finish(){
        auto&g=s_.gp;
        load_float(0x314854u,g[1]);compare(0x314856u,g[6],1u);test_status(1u);g[0]=s_.flags.zf?4u:g[2];
        pop(0x314866u,6u);g[4]=g[5];pop(0x314869u,5u);at(0x31486au);s_.pop();g[4]+=12u;
        return g[0];
    }
};
template<bool ClearC1,class State,class Checkpoint=Noop,class Spill=LegacySpill>
void run_private_source(State&s,Checkpoint checkpoint={},Spill spill={}){
    Frame<ClearC1,State,Checkpoint,Spill>view{s,checkpoint,spill};
    native_tu15_solver_hull_clip_kernel(view);
}
} // namespace halo2::native_solver_hull_clip_source_detail
#endif
