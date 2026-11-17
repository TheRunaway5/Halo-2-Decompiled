#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_simplex_hull_retained_frame.hpp"

namespace halo2::native_simplex_hull_entry_frontier_detail {
enum class Retirement { declined, retained, revoked };
inline constexpr std::uint32_t next_site=0x30de61u;

// Only the first retired PUSH is qualified here. Both successful outcomes
// leave the caller at next_site; they do not admit the rest of this owner.
template<class State,class Before,class Policy>
Retirement first_push(State& s,Before before,Policy policy){
    using native_simplex_hull_retained_detail::Io;
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||
       !native_raw_x87_detail::supported(s)||s.fp.count>4u||!policy(s))return Retirement::declined;
    std::nullptr_t unused=nullptr;
    {
        native_raw_x87_detail::Scope fp{s};
        Io<State,decltype(unused),Before,true> io{s,unused,before,fp};
        io.push(0x30de60u,s.gp[5]);
    }
    return policy(s)?Retirement::retained:Retirement::revoked;
}
} // namespace halo2::native_simplex_hull_entry_frontier_detail
#endif
