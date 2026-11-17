#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_hull_reduction_route_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_seed_route_frame.hpp"

namespace halo2::native_simplex_hull_reduction_route_detail {
using Kind=NativeTu15SimplexReductionKind;
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    bool publish_packed_counts() {
        auto& g=s_.gp;
        g[0]=io_.word(0x30e235u,g[6]);g[3]=io_.word(0x30e237u,g[7]);
        g[0]=s_.flags.logic((g[0]<<3u)|g[3],4u);g[0]=s_.flags.minus(g[0],9u,4u);
        s_.flags.minus(g[0],24u,4u);io_.store(0x30e244u,g[6]+0x10u,0u);
        return s_.flags.cf||s_.flags.zf;
    }
    bool repack_reduced_counts() {
        auto& g=s_.gp;
        g[0]=io_.word(0x30e5ecu,g[6]);g[3]=io_.word(0x30e5eeu,g[7]);
        g[0]=s_.flags.logic((g[0]<<3u)|g[3],4u);g[0]=s_.flags.minus(g[0],9u,4u);
        s_.flags.minus(g[0],24u,4u);return s_.flags.cf||s_.flags.zf;
    }
    Kind resolve_reduction() {
        auto& g=s_.gp;
        g[0]=io_.byte(0x30e251u,0x30e908u+g[0]);
        switch(io_.word(0x30e258u,0x30e8dcu+g[0]*4u)){
        case 0x30e89bu:return Kind::points;
        case 0x30e6bfu:return Kind::second_edge;
        case 0x30e3a7u:return Kind::second_triangle;
        case 0x30e362u:return Kind::second_face;
        case 0x30e7c7u:return Kind::first_edge;
        case 0x30e2afu:return Kind::segments;
        case 0x30e25fu:return Kind::paired_two_three;
        case 0x30e504u:return Kind::first_triangle;
        case 0x30e27du:return Kind::paired_three_two;
        case 0x30e4d3u:return Kind::first_face;
        case 0x30e8b5u:return Kind::degenerate;
        default:throw std::runtime_error("Required original simplex hull reduction unavailable");
        }
    }
};
template<class State,class Before> class PhysicalIo:public native_solver_hull_seed_route_detail::PhysicalIo<State,Before> {
    using Base=native_solver_hull_seed_route_detail::PhysicalIo<State,Before>;
    using Foundation=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
public:
    using Base::Base;
    std::uint32_t byte(std::uint32_t pc,std::uint32_t address){Foundation::at(pc);return Foundation::word(address,1u);}
};
template<class State,class Before> Kind run_private_hull_reduction_route(State& s,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s))
        throw std::runtime_error("Private simplex hull reduction requires the supported actual TU1.5 frame");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before> io{s,before,fp};
    Schedule<State,decltype(io)> view{s,io};return native_tu15_simplex_hull_reduction_route_kernel(view);
}
template<class State,class Before> Kind run_private_hull_redispatch(State& s,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s))
        throw std::runtime_error("Private simplex hull redispatch requires the supported actual TU1.5 frame");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before> io{s,before,fp};
    Schedule<State,decltype(io)> view{s,io};
    return view.repack_reduced_counts()?view.resolve_reduction():Kind::degenerate;
}
} // namespace halo2::native_simplex_hull_reduction_route_detail
#endif
