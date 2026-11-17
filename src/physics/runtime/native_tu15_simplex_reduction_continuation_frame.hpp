#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_simplex_hull_plane_frame.hpp"
#include "physics/runtime/native_tu15_simplex_iteration_abi.hpp"

namespace halo2::native_simplex_reduction_continuation_detail {
enum class Entry : std::uint32_t {
    segment_normal=0x30efa5u,second_vertex=0x30f032u,first_vertex=0x30f155u,direction_count=0x30f268u
};
template<class State,class Io> std::uint32_t apply_reduction_continuation(State& s,Entry entry,Io& io){
    native_simplex_hull_plane_detail::Schedule<State,Io,true> geometry{s,io};
    auto& g=s.gp;
    if(entry==Entry::first_vertex||entry==Entry::second_vertex){
        const bool first=entry==Entry::first_vertex;geometry.demote_vertex(first);return first?0x30f175u:0x30f066u;
    }
    const bool reverse=entry==Entry::segment_normal&&!geometry.segments_plane();
    if(reverse){
        geometry.reverse_segments_plane();io.store_vector(0x30f01fu,g[6]+0x120u,s.xmm[1]);
    }
    g[2]=io.word(reverse?0x30f026u:0x30f268u,g[7]);io.store(reverse?0x30f028u:0x30f26au,g[6]+8u,g[2]);
    g[0]=s.flags.logic(0u,4u);return 0x30edd5u;
}
template<class State,class Before> std::uint32_t run_private_reduction_continuation(State& s,Entry entry,Before before){
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s))
        throw std::runtime_error("Private simplex reduction continuation requires supported actual TU1.5 state");
    native_raw_x87_detail::Scope fp{s};
    auto unexpected=[](State&,std::uint32_t,std::uint32_t){throw std::logic_error("Unexpected simplex continuation child");};
    native_solver_hull_seed_transition_detail::PhysicalIo<State,Before,decltype(unexpected)> io{s,before,fp,unexpected};
    return apply_reduction_continuation(s,entry,io);
}
template<class State,class Before> NativeTu15SimplexReductionKind run_private_reduction_redispatch(State& s,Before before){
    native_raw_x87_detail::Scope fp{s};
    native_simplex_iteration_detail::Access<State,Before> io{s,before,fp};
    native_simplex_iteration_detail::Data data{};
    auto& g=s.gp;
    data.work=g[6];data.count_address=g[7];
    // The redispatch starts after a reduction. It does not touch the initial
    // iteration's epsilon, counter, or progress cells.
    native_solver_hull_seed_route_detail::PhysicalIo<State,Before> words{s,before,fp};
    g[0]=words.word(0x30edbbu,g[6]);g[3]=words.word(0x30edbdu,g[7]);
    g[0]=s.flags.logic((g[0]<<3u)|g[3],4u);g[0]=s.flags.minus(g[0],9u,4u);
    data.key=g[0];s.flags.minus(g[0],24u,4u);
    if(!s.flags.cf&&!s.flags.zf)return NativeTu15SimplexReductionKind::degenerate;
    return io.classify(data);
}
template<class State> void mark_private_simplex_degenerate(State& s){s.gp[0]=1u;}
} // namespace halo2::native_simplex_reduction_continuation_detail
#endif
