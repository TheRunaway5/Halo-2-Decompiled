#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_seed_result_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_search_finish_frame.hpp"

namespace halo2::native_solver_hull_seed_result_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
    void read(std::uint32_t pc,unsigned reg,std::uint32_t address){s_.gp[reg]=io_.word(pc,address);}
    static std::uint32_t low(__m128 a){return static_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(a)));}
    static void add(__m128& a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");}
    static void add_scalar(__m128& a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");}
    static void multiply(__m128& a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");}
    static void multiply_scalar(__m128& a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");}
    static void subtract(__m128& a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");}
    static void subtract_scalar(__m128& a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");}
    void record_status(){auto& eax=s_.gp[0];eax=(eax&0xffff0000u)|io_.status();}
    void test_status(){s_.flags.logic((s_.gp[0]>>8u)&5u,1u);}
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    void select_direction_components() {
        auto& g=s_.gp;auto& x=s_.xmm;
        read(0x3141c2u,3u,g[5]+8u);g[1]=s_.flags.logic(0u,4u);io_.store(0x3141c7u,g[3]+0x20u,g[1]);
        read(0x3141cau,0u,g[6]+0x50u);io_.vector(0x3141cdu,0u,g[0]+0x10u);io_.vector(0x3141d1u,1u,g[0]);
        subtract(x[1],x[0]);io_.store_vector(0x3141d7u,g[4]+0x20u,x[1]);
        io_.load_float(0x3141dcu,g[4]+0x20u);io_.absolute();g[2]=1u;
        io_.load_float(0x3141e7u,g[4]+0x24u);io_.absolute();g[7]=2u;io_.spill(0x3141f2u,g[4]+0xcu,false);
        io_.load_float(0x3141f6u,g[4]+0x28u);io_.absolute();io_.spill(0x3141fcu,g[4]+0x10u,true);
        io_.compare_register_pop();record_status();test_status();
        if(!s_.flags.pf){io_.drop();g[2]=s_.flags.logic(0u,4u);io_.load_float(0x31420du,g[4]+0xcu);g[1]=1u;}
        io_.load_float(0x314216u,g[4]+0x10u);io_.compare_register_pop();record_status();io_.drop();test_status();
        if(!s_.flags.pf){g[7]=g[1];g[1]=2u;}
        io_.load_float(0x31422cu,g[4]+0x20u+g[2]*4u);io_.store(0x314230u,g[3]+g[1]*4u,0u);
        read(0x314237u,1u,g[4]+0x20u+g[7]*4u);io_.negate_float();io_.store(0x31423du,g[3]+g[2]*4u,g[1]);
        io_.spill(0x314240u,g[3]+g[7]*4u,true);
    }
    void publish_unit_direction() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.vector(0x314243u,1u,g[3]);x[0]=x[1];multiply(x[0],x[1]);
        x[2]=_mm_shuffle_ps(x[0],x[0],0x55);add_scalar(x[2],x[0]);x[3]=_mm_shuffle_ps(x[0],x[0],0xaa);
        x[0]=x[3];add_scalar(x[0],x[2]);io_.store_vector(0x314265u,g[4]+0x40u,x[0]);
        asm volatile("rsqrtss %1,%0":"+x"(x[2]):"x"(x[0]):"memory");
        io_.store(0x31426eu,g[4]+0x40u,low(x[2]));io_.vector(0x314274u,2u,g[4]+0x40u);
        multiply_scalar(x[0],x[2]);multiply_scalar(x[0],x[2]);
        io_.store(0x314281u,g[4]+0xcu,0x40400000u);io_.scalar(0x314289u,3u,g[4]+0xcu);
        subtract_scalar(x[3],x[0]);io_.store(0x314293u,g[4]+0xcu,0x3f000000u);io_.scalar(0x31429bu,0u,g[4]+0xcu);
        multiply_scalar(x[0],x[2]);multiply_scalar(x[0],x[3]);x[2]=_mm_shuffle_ps(x[0],x[0],0);multiply(x[2],x[1]);
        io_.store_vector(0x3142b3u,g[3],x[2]);
    }
    void interpolate_seed() {
        auto& g=s_.gp;auto& x=s_.xmm;
        read(0x3142b6u,2u,g[6]+0x50u);io_.vector(0x3142b9u,1u,0x461950u);io_.vector(0x3142c0u,0u,g[2]);
        x[0]=_mm_and_ps(x[0],x[1]);x[1]=_mm_shuffle_ps(x[0],x[0],0x55);add_scalar(x[1],x[0]);
        x[2]=_mm_shuffle_ps(x[0],x[0],0xaa);x[0]=x[2];add_scalar(x[0],x[1]);
        g[1]=g[4]+0xcu;io_.store(0x3142e3u,g[1],low(x[0]));io_.load_float(0x3142e7u,g[4]+0xcu);
        g[0]=g[4]+0x14u;io_.store(0x3142efu,g[0],low(x[0]));io_.add_float(0x3142f3u,g[4]+0x14u);
        read(0x3142f7u,6u,g[6]+0x48u);io_.vector(0x3142fau,1u,g[6]+0x10u);io_.add_float(0x3142feu,0x41d774u);
        io_.vector(0x314304u,3u,0x41c500u);g[0]=s_.flags.logic(0u,4u);io_.reverse_divide_float(0x31430du,g[4]+0x14u);
        io_.spill(0x314311u,g[4]+0xcu,true);io_.scalar(0x314315u,0u,g[4]+0xcu);
        x[0]=_mm_shuffle_ps(x[0],x[0],0);x[2]=x[0];multiply(x[2],x[1]);io_.vector(0x314325u,1u,g[6]);
        subtract(x[3],x[0]);multiply(x[3],x[1]);add(x[3],x[2]);io_.store_vector(0x314331u,g[3]+0x10u,x[3]);
        io_.store(0x314335u,g[3]+0x24u,0x3f000000u);
    }
    void restore_caller() {
        auto& g=s_.gp;read(0x31433cu,1u,g[4]+0x3014u);io_.store(0x314343u,s_.fs_base,g[1]);
        g[7]=io_.pop(0x31434au);g[6]=io_.pop(0x31434bu);g[3]=io_.pop(0x31434cu);g[4]=g[5];
        g[5]=io_.pop(0x31434fu);io_.pop(0x314350u);g[4]+=4u;
    }
};

template<class State,class Before,class Call> class PhysicalIo:
    public native_solver_hull_search_finish_detail::PhysicalIo<State,Before,Call> {
    using Base=native_solver_hull_search_finish_detail::PhysicalIo<State,Before,Call>;
    using Foundation=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    native_raw_x87_detail::Scope<State>& fp_;
public:
    PhysicalIo(State& s,Before& before,native_raw_x87_detail::Scope<State>& fp,Call& call)
        :Base{s,before,fp,call},fp_(fp){}
    void compare_register_pop(){asm volatile("fcomp %%st(1)":::"memory","st","st(1)");fp_.publish();}
    void drop(){asm volatile("fstp %%st(0)":::"memory","st");fp_.publish();}
    void add_float(std::uint32_t pc,std::uint32_t address){
        const auto bits=Base::word(pc,address);asm volatile("fadds %0"::"m"(bits):"memory","st");fp_.publish();
    }
    void reverse_divide_float(std::uint32_t pc,std::uint32_t address){
        const auto bits=Base::word(pc,address);asm volatile("fdivrs %0"::"m"(bits):"memory","st");fp_.publish();
    }
    void spill(std::uint32_t pc,std::uint32_t address,bool pop){
        Foundation::at(pc);const auto pending=fp_.stage_single(pop);
        Foundation::store(pc,address,pending.bits,[&]{fp_.commit(pending);},true);
    }
};
template<class State,class Call,class Before> void run_private_seed_result(State& s,Call call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>5u)
        throw std::runtime_error("Private hull seed result requires a supported actual TU1.5 frame");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};native_tu15_solver_hull_seed_result_kernel(view);
}
} // namespace halo2::native_solver_hull_seed_result_detail
#endif
