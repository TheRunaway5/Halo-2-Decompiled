#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_hull_triangle_edges_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_seed_transition_frame.hpp"

namespace halo2::native_simplex_hull_triangle_edges_detail {
template<class State,class Io,std::uint32_t PcBias,std::uint32_t ScratchOffset> class Schedule {
    State& s_;
    Io& io_;
    static constexpr std::uint32_t pc(std::uint32_t original){return original+PcBias;}
    static constexpr std::uint32_t first_edges=0x41d760u,second_edges=0x41d762u;
    static std::uint32_t signed_byte(std::uint32_t value){return (value^0x80u)-0x80u;}
    static void subtract(__m128& a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");}
    static void multiply(__m128& a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");}
    static void add_low(__m128& a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");}
    void add(unsigned dest,std::uint32_t value){s_.gp[dest]=s_.flags.plus(s_.gp[dest],value,4u);}
    void decrement(unsigned dest){s_.gp[dest]=s_.flags.increment(s_.gp[dest],4u,true);}
    void shift(unsigned dest){io_.shift_index(dest);}
    bool negative(unsigned mask,__m128 value) {
        s_.gp[mask]=static_cast<std::uint32_t>(_mm_movemask_ps(value));
        s_.flags.logic(s_.gp[mask]&1u,1u);return !s_.flags.zf;
    }
    void differences() {
        auto& x=s_.xmm;subtract(x[1],x[0]);subtract(x[2],x[0]);
        x[0]=x[1];multiply(x[0],x[3]);
        x[3]=_mm_shuffle_ps(x[0],x[0],85);x[4]=x[0];
        add_low(x[3],x[0]);x[4]=_mm_shuffle_ps(x[4],x[0],170);
    }
    bool finish_far_projection(unsigned mask) {
        auto& x=s_.xmm;
        x[0]=_mm_shuffle_ps(x[1],x[1],85);x[2]=x[1];
        add_low(x[0],x[1]);x[2]=_mm_shuffle_ps(x[2],x[1],170);add_low(x[2],x[0]);
        return negative(mask,x[2]);
    }
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    bool second_edge_projection() {
        auto& g=s_.gp;auto& x=s_.xmm;
        g[2]=signed_byte(io_.byte(pc(0x30e3d3u),first_edges+g[0]));
        g[1]=signed_byte(io_.byte(pc(0x30e3dau),second_edges+g[0]));
        io_.vector(pc(0x30e3e1u),1u,g[3]);
        add(0u,10u);shift(0u);io_.vector(pc(0x30e3eau),0u,g[6]+g[0]);
        g[0]=g[1]+10u;shift(0u);io_.vector(pc(0x30e3f4u),2u,g[6]+g[0]);
        add(0u,g[6]);io_.store(pc(0x30e3fau),g[4]+ScratchOffset,g[0]);
        g[0]=g[2]+10u;shift(0u);io_.vector(pc(0x30e404u),3u,g[6]+g[0]);
        subtract(x[3],x[0]);add(0u,g[6]);differences();
        io_.vector(pc(0x30e42bu),0u,g[6]+0xc0u);add_low(x[4],x[3]);
        const auto other=negative(3u,x[4]);io_.store(pc(0x30e43cu),g[7],2u);
        return other;
    }
    void place_near_second() {
        auto& g=s_.gp;auto& x=s_.xmm;
        g[2]=io_.word(pc(0x30e4b6u),g[4]+ScratchOffset);add(1u,14u);io_.store_vector(pc(0x30e4bdu),g[2],x[0]);
        io_.vector(pc(0x30e4c0u),0u,g[6]+0x100u);shift(1u);io_.store_vector(pc(0x30e4cau),g[6]+g[1],x[0]);
    }
    bool place_far_second() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.store_vector(pc(0x30e444u),g[0],x[0]);io_.vector(pc(0x30e447u),0u,g[6]+0x100u);
        multiply(x[1],x[2]);g[0]=g[2]+14u;shift(0u);io_.store_vector(pc(0x30e457u),g[6]+g[0],x[0]);
        return finish_far_projection(0u);
    }
    void extend_second() {
        auto& g=s_.gp;auto& x=s_.xmm;
        g[0]=io_.word(pc(0x30e47cu),g[7]);s_.flags.minus(g[1],g[0],4u);if(s_.flags.zf)g[1]=g[2];
        decrement(0u);io_.store(pc(0x30e485u),g[7],g[0]);
        add(0u,10u);shift(0u);io_.vector(pc(0x30e48du),0u,g[6]+g[0]);
        g[2]=g[1]+10u;shift(2u);io_.store_vector(pc(0x30e497u),g[6]+g[2],x[0]);
        g[0]=io_.word(pc(0x30e49bu),g[7]);add(0u,14u);shift(0u);io_.vector(pc(0x30e4a3u),0u,g[6]+g[0]);
        add(1u,14u);shift(1u);io_.store_vector(pc(0x30e4adu),g[6]+g[1],x[0]);
    }
    bool first_edge_projection() {
        auto& g=s_.gp;auto& x=s_.xmm;
        g[1]=signed_byte(io_.byte(pc(0x30e530u),first_edges+g[0]));
        io_.vector(pc(0x30e537u),1u,g[3]);add(0u,2u);io_.store(pc(0x30e53du),g[4]+ScratchOffset,g[1]);
        g[1]=signed_byte(io_.byte(pc(0x30e541u),first_edges+g[0]));shift(0u);io_.vector(pc(0x30e54bu),0u,g[6]+g[0]);
        g[0]=io_.word(pc(0x30e54fu),g[4]+ScratchOffset);add(0u,2u);
        g[2]=g[1]+2u;shift(2u);io_.vector(pc(0x30e55cu),2u,g[6]+g[2]);
        shift(0u);io_.vector(pc(0x30e563u),3u,g[6]+g[0]);subtract(x[3],x[0]);
        add(2u,g[6]);add(0u,g[6]);differences();
        io_.vector(pc(0x30e58cu),0u,g[6]+0x40u);add_low(x[4],x[3]);
        const auto other=negative(3u,x[4]);io_.store(pc(0x30e59au),g[6],2u);
        return other;
    }
    void place_near_first(){io_.store_vector(pc(0x30e5e9u),s_.gp[2],s_.xmm[0]);}
    bool place_far_first() {
        auto& g=s_.gp;auto& x=s_.xmm;
        multiply(x[1],x[2]);io_.store_vector(pc(0x30e5a5u),g[0],x[0]);
        return finish_far_projection(2u);
    }
    void extend_first() {
        auto& g=s_.gp;
        g[0]=io_.word(pc(0x30e5c6u),g[6]);s_.flags.minus(g[1],g[0],4u);
        if(s_.flags.zf)g[1]=io_.word(pc(0x30e5ccu),g[4]+ScratchOffset);
        decrement(0u);io_.store(pc(0x30e5d1u),g[6],g[0]);
        add(0u,2u);shift(0u);io_.vector(pc(0x30e5d9u),0u,g[6]+g[0]);
        add(1u,2u);shift(1u);io_.store_vector(pc(0x30e5e3u),g[6]+g[1],s_.xmm[0]);
    }
};
template<std::uint32_t PcBias=0u,std::uint32_t ScratchOffset=0x18u,class State,class Before> void run_private_hull_triangle_edges(State& s,bool first,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s))
        throw std::runtime_error("Private simplex hull triangle edges require supported actual TU1.5 state");
    native_raw_x87_detail::Scope fp{s};
    auto unexpected=[](State&,std::uint32_t,std::uint32_t){throw std::logic_error("Unexpected triangle edge child");};
    native_solver_hull_seed_transition_detail::PhysicalIo<State,Before,decltype(unexpected)> io{s,before,fp,unexpected};
    Schedule<State,decltype(io),PcBias,ScratchOffset> view{s,io};native_tu15_simplex_hull_triangle_edges_kernel(view,first);
}
} // namespace halo2::native_simplex_hull_triangle_edges_detail
#endif
