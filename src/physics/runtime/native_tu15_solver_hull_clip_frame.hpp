#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_clip_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_clip_detail {
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    native_raw_x87_detail::Scope<State>&fp_;
    std::uint32_t read(std::uint32_t pc,std::uint32_t address){at(pc);return word(address);}
    __m128 vector(std::uint32_t pc,std::uint32_t address){
        at(pc);
        if(address&15u)throw std::runtime_error("Unaligned original hull clip MOVAPS source");
        alignas(16)std::array<std::byte,16>bytes{};
        fp_.io([&]{s_.memory.read_vector(address,bytes);});
        __m128 value;std::memcpy(&value,bytes.data(),16u);return value;
    }
    static __m128 minus(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static __m128 product(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static __m128 sum(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static std::uint32_t low(__m128 a){std::uint32_t bits;std::memcpy(&bits,&a,4u);return bits;}
    void load_float(std::uint32_t pc,std::uint32_t address){
        const auto bits=read(pc,address);fp_.require_capacity(1u);
        asm volatile("flds %0"::"m"(bits):"memory","st");fp_.publish();
    }
    void compare(std::uint32_t pc,std::uint32_t address,bool consume){
        const auto bits=read(pc,address);
        if(consume)asm volatile("fcomps %0"::"m"(bits):"memory","st");
        else asm volatile("fcoms %0"::"m"(bits):"memory","st");
        fp_.publish();
    }
    void test_status(std::uint32_t mask){
        std::uint16_t status;asm volatile("fnstsw %0":"=a"(status));
        s_.gp[0]=(s_.gp[0]&0xffff0000u)|status;
        s_.flags.logic((s_.gp[0]>>8u)&mask,1u);
    }
public:
    Frame(State&s,Before&before,native_raw_x87_detail::Scope<State>&fp)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp){}
    void project_and_compare_lower(){
        auto&g=s_.gp;auto&x=s_.xmm;
        push(0x3147d0u,g[5]);g[5]=g[4];
        g[4]=s_.flags.logic(g[4]&0xfffffff0u,4u);g[4]=s_.flags.minus(g[4],12u,4u);
        g[0]=read(0x3147d9u,g[1]+0x14u);x[0]=vector(0x3147dcu,g[0]);
        g[2]=read(0x3147dfu,g[5]+8u);x[1]=vector(0x3147e2u,g[2]);x[1]=minus(x[1],x[0]);
        x[0]=vector(0x3147e8u,g[1]);g[1]=read(0x3147ebu,g[5]+12u);
        x[0]=product(x[0],x[1]);x[1]=_mm_shuffle_ps(x[0],x[0],0x55);x[2]=x[0];g[0]=g[4]+8u;
        x[1]=sum(x[1],x[0]);x[2]=_mm_shuffle_ps(x[2],x[0],0xaa);x[2]=sum(x[2],x[1]);
        Base::store(0x31480bu,g[0],low(x[2]),[]{});
        load_float(0x31480fu,g[4]+8u);compare(0x314813u,g[1],true);
        push(0x314815u,g[6]);g[6]=read(0x314816u,g[5]+16u);load_float(0x314819u,g[4]+12u);
        g[2]=1u;test_status(0x41u);
    }
    bool use_upper(){return s_.flags.zf;}
    bool update_upper(){compare(0x314829u,s_.gp[6],true);test_status(5u);return !s_.flags.pf;}
    void publish_upper(){
        auto&g=s_.gp;g[0]=read(0x314832u,g[4]+12u);
        Base::store(0x314836u,g[6],g[0],[]{});
    }
    bool update_lower(){
        const auto bits=read(0x31483au,0x41d7a0u);
        asm volatile("fadds %0"::"m"(bits):"memory","st");fp_.publish();
        compare(0x314840u,s_.gp[1],false);test_status(0x41u);return s_.flags.zf;
    }
    void publish_lower(){
        at(0x314849u);const auto pending=fp_.stage_single(true);
        Base::store(0x314849u,s_.gp[1],pending.bits,[&]{fp_.commit(pending);},true);
    }
    void discard_lower(){asm volatile("fstp %%st(0)":::"memory","st");fp_.publish();}
    void mark_lower(){s_.gp[2]=2u;}
    std::uint32_t finish(){
        auto&g=s_.gp;
        load_float(0x314854u,g[1]);compare(0x314856u,g[6],true);test_status(1u);
        g[0]=s_.flags.zf?4u:g[2];
        g[6]=pop(0x314866u);g[4]=g[5];g[5]=pop(0x314869u);pop(0x31486au);g[4]+=12u;
        return g[0];
    }
};
template<class State,class Before=Noop>void run_private_owner(State&s,Before before={}){
    if(!native_raw_x87_detail::supported(s)||s.fp.count>7u)
        throw std::runtime_error("Private hull clip requires dense masked FP with one free slot");
    native_raw_x87_detail::Scope fp{s};Frame<State,Before>view{s,before,fp};
    native_tu15_solver_hull_clip_kernel(view);
}
} // namespace halo2::native_solver_hull_clip_detail
#endif
