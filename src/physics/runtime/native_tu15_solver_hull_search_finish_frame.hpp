#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_search_finish_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_search_refresh_frame.hpp"

namespace halo2::native_solver_hull_search_finish_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
    void read(std::uint32_t pc,unsigned reg,std::uint32_t address){s_.gp[reg]=io_.word(pc,address);}
    void multiply(unsigned to,unsigned from) {
        auto& x=s_.xmm;asm volatile("mulps %1,%0":"+x"(x[to]):"x"(x[from]):"memory");
    }
    void add(unsigned to,unsigned from) {
        auto& x=s_.xmm;asm volatile("addps %1,%0":"+x"(x[to]):"x"(x[from]):"memory");
    }
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    void interpolate_output() {
        auto& g=s_.gp;auto& x=s_.xmm;
        read(0x3145eeu,6u,g[6]+0x48u);read(0x3145f1u,2u,g[4]+0x38u);io_.vector(0x3145f5u,2u,g[6]+0x10u);
        read(0x3145f9u,0u,g[4]+0x3cu);io_.vector(0x3145fdu,3u,g[6]+0x20u);
        g[1]=s_.flags.plus(g[1],0x60u,4u);read(0x314604u,1u,g[4]+0x34u);
        io_.store(0x314608u,g[4]+0x10u,g[1]);io_.scalar(0x31460cu,0u,g[4]+0x10u);
        io_.store(0x314612u,g[4]+0x10u,g[2]);io_.scalar(0x314616u,1u,g[4]+0x10u);
        x[1]=_mm_shuffle_ps(x[1],x[1],0);multiply(1u,2u);
        io_.store(0x314623u,g[4]+0x10u,g[0]);io_.scalar(0x314627u,2u,g[4]+0x10u);
        read(0x31462du,0u,g[5]+8u);x[0]=_mm_shuffle_ps(x[0],x[0],0);x[2]=_mm_shuffle_ps(x[2],x[2],0);
        multiply(2u,3u);io_.vector(0x31463bu,3u,g[6]);multiply(0u,3u);add(0u,1u);add(0u,2u);
        io_.store_vector(0x314647u,g[0]+0x10u,x[0]);
    }
    void publish_plane() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.load_float(0x31464bu,g[7]+0x10u);io_.negate_float();io_.store_byte(0x314650u,g[4]+0x301cu,1u);
        io_.spill_float(0x314658u,g[0]+0x20u);
        io_.vector(0x31465bu,0u,0x461960u);io_.vector(0x314662u,1u,g[7]);
        x[1]=_mm_xor_ps(x[1],x[0]);io_.store_vector(0x314668u,g[0],x[1]);
    }
    void release_payload_if_owned() {
        auto& g=s_.gp;
        read(0x31466bu,0u,g[4]+0x128u);s_.flags.logic(g[0],4u);if(s_.flags.sf)return;
        read(0x314676u,1u,0x48c938u);read(0x31467cu,2u,g[1]);g[0]=s_.flags.logic(g[0]&0x7fffffffu,4u);
        io_.push(0x314683u,0x12u);io_.shift_index(0u);io_.push(0x314688u,g[0]);
        read(0x314689u,0u,g[4]+0x128u);io_.push(0x314690u,g[0]);io_.call_allocator(0x314691u,0x314694u,g[2]+0x14u);
    }
    void release_vertices_if_owned() {
        auto& g=s_.gp;
        read(0x314694u,0u,g[4]+0x4a8u);s_.flags.logic(g[0],4u);io_.store_byte(0x31469du,g[4]+0x301cu,0u);
        if(s_.flags.sf)return;
        read(0x3146a7u,1u,0x48c938u);read(0x3146adu,2u,g[1]);g[0]=s_.flags.logic(g[0]&0x7fffffffu,4u);
        io_.push(0x3146b4u,0x12u);io_.shift_index(0u);io_.push(0x3146b9u,g[0]);
        read(0x3146bau,0u,g[4]+0x4a8u);io_.push(0x3146c1u,g[0]);io_.call_allocator(0x3146c2u,0x3146c5u,g[2]+0x14u);
    }
    void destroy_tetrahedron() {
        auto& g=s_.gp;g[1]=g[4]+0x820u;
        io_.store(0x3146ccu,g[4]+0x301cu,0xffffffffu);io_.call(0x3146d7u,0x3146dcu,0x3139f0u);
    }
    void restore_caller() {
        auto& g=s_.gp;
        read(0x3146dcu,1u,g[4]+0x3014u);read(0x3146e3u,0u,g[4]+0x18u);
        g[7]=io_.pop(0x3146e7u);g[6]=io_.pop(0x3146e8u);io_.store(0x3146e9u,s_.fs_base,g[1]);
        g[3]=io_.pop(0x3146f0u);g[4]=g[5];g[5]=io_.pop(0x3146f3u);io_.pop(0x3146f4u);g[4]+=4u;
    }
};

template<class State,class Before,class Call> class PhysicalIo:
    public native_solver_hull_search_refresh_detail::PhysicalIo<State,Before,Call> {
    using Base=native_solver_hull_search_refresh_detail::PhysicalIo<State,Before,Call>;
    using Foundation=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    State& s_;
    native_raw_x87_detail::Scope<State>& fp_;
public:
    using Foundation::pop;
    PhysicalIo(State& s,Before& before,native_raw_x87_detail::Scope<State>& fp,Call& call)
        :Base{s,before,fp,call},s_(s),fp_(fp){}
    void scalar(std::uint32_t pc,unsigned reg,std::uint32_t address){
        s_.xmm[reg]=_mm_set_ss(std::bit_cast<float>(Base::word(pc,address)));
    }
    void negate_float(){asm volatile("fchs":::"memory","st");fp_.publish();}
    void spill_float(std::uint32_t pc,std::uint32_t address){
        Foundation::at(pc);const auto pending=fp_.stage_single(true);
        Foundation::store(pc,address,pending.bits,[&]{fp_.commit(pending);},true);
    }
};
template<class State,class Call,class Before> void run_private_search_finish(State& s,Call call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>7u)
        throw std::runtime_error("Private hull search finish requires a supported actual TU1.5 frame");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};native_tu15_solver_hull_search_finish_kernel(view);
}
} // namespace halo2::native_solver_hull_search_finish_detail
#endif
