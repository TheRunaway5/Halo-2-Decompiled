#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_triangle_result_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_search_finish_frame.hpp"

namespace halo2::native_solver_hull_triangle_result_detail {
enum class Result { near_origin, degenerate };

template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){s_.gp[reg]=io_.word(pc,address);}
    static std::uint32_t low(__m128 a){return static_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(a)));}
    static void add(__m128& a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");}
    static void add_scalar(__m128& a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");}
    static void multiply(__m128& a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");}
    static void multiply_scalar(__m128& a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");}
    static void subtract_scalar(__m128& a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");}
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    void normalize_triangle_direction() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.vector(0x314353u,2u,g[4]+0x60u);load(0x314358u,7u,g[5]+8u);
        x[0]=x[2];multiply(x[0],x[2]);x[1]=_mm_shuffle_ps(x[0],x[0],0x55);add_scalar(x[1],x[0]);
        x[3]=_mm_shuffle_ps(x[0],x[0],0xaa);x[0]=x[3];add_scalar(x[0],x[1]);
        io_.store_vector(0x31437au,g[4]+0x40u,x[0]);
        asm volatile("rsqrtss %1,%0":"+x"(x[1]):"x"(x[0]):"memory");
        io_.store(0x314383u,g[4]+0x40u,low(x[1]));io_.vector(0x314389u,1u,g[4]+0x40u);
        multiply_scalar(x[0],x[1]);multiply_scalar(x[0],x[1]);
        io_.store(0x314396u,g[4]+0xcu,0x40400000u);io_.scalar(0x31439eu,3u,g[4]+0xcu);
        subtract_scalar(x[3],x[0]);io_.store(0x3143a8u,g[4]+0xcu,0x3f000000u);io_.scalar(0x3143b0u,0u,g[4]+0xcu);
        multiply_scalar(x[0],x[1]);multiply_scalar(x[0],x[3]);x[1]=_mm_shuffle_ps(x[0],x[0],0);x[0]=x[1];multiply(x[0],x[2]);
        io_.store(0x3143cbu,g[7]+0x20u,0u);io_.store_vector(0x3143d2u,g[7],x[0]);
    }
    void project_origin() {
        auto& g=s_.gp;
        load(0x3143d5u,0u,g[6]+0x50u);g[2]=g[4]+0x20u;io_.push(0x3143dcu,g[2]);g[1]=g[0]+0x20u;
        io_.push(0x3143e0u,g[1]);g[2]=g[0]+0x10u;io_.push(0x3143e4u,g[2]);io_.push(0x3143e5u,g[0]);io_.push(0x3143e6u,0x48ca00u);
        io_.store_vector(0x3143ebu,g[4]+0x74u,s_.xmm[0]);io_.call(0x3143f0u,0x3143f5u,0x3122f0u);
    }
    void publish_transformed_point() {
        auto& g=s_.gp;auto& x=s_.xmm;
        load(0x3143f5u,0u,g[4]+0x34u);load(0x3143f9u,1u,g[4]+0x38u);load(0x3143fdu,6u,g[6]+0x48u);
        load(0x314400u,2u,g[4]+0x3cu);io_.vector(0x314404u,3u,g[6]+0x20u);
        io_.store(0x314408u,g[4]+0x20u,g[0]);io_.scalar(0x31440cu,0u,g[4]+0x20u);
        io_.store(0x314412u,g[4]+0x20u,g[1]);io_.scalar(0x314416u,1u,g[4]+0x20u);
        io_.store(0x31441cu,g[4]+0x20u,g[2]);io_.scalar(0x314420u,2u,g[4]+0x20u);
        x[4]=_mm_shuffle_ps(x[2],x[2],0);io_.vector(0x31442du,2u,g[6]+0x10u);multiply(x[4],x[3]);
        x[3]=_mm_shuffle_ps(x[1],x[1],0);io_.vector(0x31443bu,1u,g[6]);multiply(x[3],x[2]);
        x[2]=_mm_shuffle_ps(x[0],x[0],0);multiply(x[2],x[1]);add(x[2],x[3]);add(x[2],x[4]);
        g[4]=s_.flags.plus(g[4],0x14u,4u);io_.store_vector(0x314454u,g[7]+0x10u,x[2]);io_.store(0x314458u,g[7]+0x24u,0x3f000000u);
    }
    void restore_short_caller(Result result) {
        auto& g=s_.gp;
        const bool near=result==Result::near_origin;
        g[0]=near?s_.flags.logic(0u,4u):2u;
        load(near?0x314461u:0x31447du,1u,g[4]+0x3014u);io_.store(near?0x314468u:0x314484u,s_.fs_base,g[1]);
        g[7]=io_.pop(near?0x31446fu:0x31448bu);g[6]=io_.pop(near?0x314470u:0x31448cu);g[3]=io_.pop(near?0x314471u:0x31448du);
        g[4]=g[5];g[5]=io_.pop(near?0x314474u:0x314490u);io_.pop(near?0x314475u:0x314491u);g[4]+=4u;
    }
};

template<class State,class Call,class Before> void run_private_triangle_result(State& s,Call call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>4u)
        throw std::runtime_error("Private hull triangle result requires a supported actual TU1.5 frame");
    native_raw_x87_detail::Scope fp{s};native_solver_hull_search_finish_detail::PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};native_tu15_solver_hull_triangle_result_kernel(view);
}
template<class State,class Call,class Before> void run_private_short_return(State& s,Result result,Call call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s))
        throw std::runtime_error("Private hull short return requires a supported actual TU1.5 frame");
    native_raw_x87_detail::Scope fp{s};native_solver_hull_search_finish_detail::PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};view.restore_short_caller(result);
}
} // namespace halo2::native_solver_hull_triangle_result_detail
#endif
