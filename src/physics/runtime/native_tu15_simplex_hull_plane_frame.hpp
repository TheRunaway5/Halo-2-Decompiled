#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_hull_plane_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_seed_transition_frame.hpp"

namespace halo2::native_simplex_hull_plane_detail {
template<class State,class Io,bool MainSimplex=false> class Schedule {
    State& s_;
    Io& io_;
    static __m128 yzx(__m128 a){return _mm_shuffle_ps(a,a,201);}
    static __m128 zxy(__m128 a){return _mm_shuffle_ps(a,a,210);}
    static __m128 y(__m128 a){return _mm_shuffle_ps(a,a,85);}
    static __m128 z(__m128 a){return _mm_shuffle_ps(a,a,170);}
    static void sub(__m128& a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");}
    static void mul(__m128& a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");}
    static void add_low(__m128& a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");}
    void add(unsigned dest,std::uint32_t value){s_.gp[dest]=s_.flags.plus(s_.gp[dest],value,4u);}
    void decrement(unsigned dest){s_.gp[dest]=s_.flags.increment(s_.gp[dest],4u,true);}
    std::uint32_t mask(__m128 a){return static_cast<std::uint32_t>(_mm_movemask_ps(a));}
    static constexpr std::uint32_t vertex_pc(bool first,std::uint32_t hull){
        return hull+(MainSimplex?(first?0x9aeu:0x9a7u):0u);
    }
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    bool segments_plane() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.vector(MainSimplex?0x30efa5u:0x30e606u,0u,g[4]+(MainSimplex?0x50u:0xc0u));x[1]=yzx(x[0]);x[2]=x[1];
        io_.vector(MainSimplex?0x30efb4u:0x30e618u,1u,g[4]+(MainSimplex?0x60u:0xb0u));
        x[3]=zxy(x[1]);mul(x[3],x[2]);x[2]=zxy(x[0]);x[0]=yzx(x[1]);x[1]=x[0];
        io_.vector(MainSimplex?0x30efd4u:0x30e63bu,0u,g[6]+0xa0u);mul(x[1],x[2]);
        io_.vector(MainSimplex?0x30efdeu:0x30e645u,2u,g[3]);
        sub(x[1],x[3]);sub(x[2],x[0]);x[0]=x[1];mul(x[0],x[2]);
        x[2]=y(x[0]);x[3]=x[0];add_low(x[2],x[0]);x[3]=z(x[3]);add_low(x[3],x[2]);
        g[0]=mask(x[3]);s_.flags.logic(g[0]&1u,1u);io_.store_vector(MainSimplex?0x30f008u:0x30e66fu,g[6]+0x120u,x[1]);
        return !s_.flags.zf;
    }
    void reverse_segments_plane(){
        io_.vector(MainSimplex?0x30f015u:0x30e67cu,0u,0x461960u);s_.xmm[1]=_mm_xor_ps(s_.xmm[1],s_.xmm[0]);
    }
    void demote_vertex(bool first) {
        auto& g=s_.gp;auto& x=s_.xmm;
        add(0u,8u);s_.flags.minus(g[0],3u,4u);
        if(!s_.flags.zf&&s_.flags.sf==s_.flags.of)g[0]=1u;
        if(first){
            g[2]=io_.word(vertex_pc(true,0x30e7b4u),g[6]);io_.vector(vertex_pc(true,0x30e7b6u),0u,g[6]+0x40u);
            decrement(2u);add(0u,2u);io_.shift_index(0u);
            io_.store(vertex_pc(true,0x30e7c1u),g[6],g[2]);io_.store_vector(vertex_pc(true,0x30e7c3u),g[6]+g[0],x[0]);
        }else{
            g[3]=io_.word(vertex_pc(false,0x30e698u),g[7]);io_.vector(vertex_pc(false,0x30e69au),0u,g[6]+0xc0u);g[1]=g[0]+10u;
            decrement(3u);io_.shift_index(1u);io_.store(vertex_pc(false,0x30e6a8u),g[7],g[3]);add(0u,14u);
            io_.store_vector(vertex_pc(false,0x30e6adu),g[6]+g[1],x[0]);io_.vector(vertex_pc(false,0x30e6b1u),0u,g[6]+0x100u);
            io_.shift_index(0u);io_.store_vector(vertex_pc(false,0x30e6bbu),g[6]+g[0],x[0]);
        }
    }
    bool crosses_edge(bool first) {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.vector(first?0x30e7c7u:0x30e6bfu,5u,g[6]+(first?0x30u:0xb0u));
        io_.vector(first?0x30e7cbu:0x30e6c6u,0u,g[6]+(first?0x20u:0xa0u));
        io_.vector(first?0x30e7cfu:0x30e6cdu,1u,g[6]+(first?0xa0u:0x20u));
        x[2]=x[5];sub(x[2],x[0]);sub(x[0],x[1]);x[4]=x[0];
        x[3]=x[5];sub(x[3],x[1]);x[0]=x[3];mul(x[0],x[2]);
        x[6]=y(x[0]);add_low(x[6],x[0]);x[7]=z(x[0]);add_low(x[7],x[6]);
        x[1]=x[4];mul(x[1],x[2]);x[0]=y(x[1]);x[6]=x[1];
        add_low(x[0],x[1]);x[6]=z(x[6]);add_low(x[6],x[0]);
        const auto other=first?1u:2u;
        g[0]=mask(x[7]);g[other]=mask(x[6]);g[0]=s_.flags.logic(g[0]&1u,4u);
        g[other]=s_.flags.logic(g[other]&1u,4u);s_.flags.minus(g[0],g[other],4u);
        return !s_.flags.zf;
    }
    void edge_plane(bool first) {
        auto& x=s_.xmm;
        x[1]=zxy(x[3]);x[0]=yzx(x[4]);mul(x[1],x[0]);x[0]=zxy(x[4]);
        x[4]=yzx(x[3]);x[3]=x[4];mul(x[3],x[0]);x[0]=x[3];sub(x[0],x[1]);
        if(first){
            x[1]=yzx(x[0]);x[3]=zxy(x[2]);mul(x[3],x[1]);x[1]=zxy(x[0]);
            x[0]=yzx(x[2]);mul(x[0],x[1]);sub(x[0],x[3]);
            io_.store_vector(0x30e880u,s_.gp[6]+0x120u,x[0]);
        }else{
            x[1]=yzx(x[2]);x[3]=zxy(x[0]);mul(x[3],x[1]);x[1]=zxy(x[2]);
            x[2]=yzx(x[0]);mul(x[2],x[1]);sub(x[2],x[3]);
            io_.store_vector(0x30e77bu,s_.gp[6]+0x120u,x[2]);
        }
    }
    void collapse_edge(bool first) {
        auto& g=s_.gp;auto& x=s_.xmm;s_.flags.logic(g[0],4u);
        if(!s_.flags.zf){
            io_.vector(first?0x30e88du:0x30e78bu,0u,g[6]+(first?0x70u:0xf0u));
            io_.store_vector(first?0x30e891u:0x30e792u,g[6]+(first?0x20u:0xa0u),x[5]);
            io_.store_vector(first?0x30e895u:0x30e799u,g[6]+(first?0x60u:0xe0u),x[0]);
        }
        io_.decrement(first?0x30e899u:0x30e7a0u,first?g[6]:g[7]);
    }
};
template<class State,class Before> NativeTu15SimplexHullPlaneResume run_private_hull_plane(
    State& s,NativeTu15SimplexHullPlaneEntry entry,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s))
        throw std::runtime_error("Private simplex hull plane requires supported actual TU1.5 state");
    native_raw_x87_detail::Scope fp{s};
    auto unexpected=[](State&,std::uint32_t,std::uint32_t){throw std::logic_error("Unexpected hull plane child");};
    native_solver_hull_seed_transition_detail::PhysicalIo<State,Before,decltype(unexpected)> io{s,before,fp,unexpected};
    Schedule<State,decltype(io)> view{s,io};return native_tu15_simplex_hull_plane_kernel(view,entry);
}
} // namespace halo2::native_simplex_hull_plane_detail
#endif
