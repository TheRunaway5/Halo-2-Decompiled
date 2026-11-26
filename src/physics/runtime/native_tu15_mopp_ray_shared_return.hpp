#pragma once

#include "physics/runtime/native_tu15_mopp_ray_dispatch.hpp"

namespace halo2::native_mopp_ray_shared_return_detail {
enum class Rejection { original, frame, complete };

inline bool reference_shared_return() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_SHARED_RETURN");
        return value&&*value;
    }();
    return reference;
}

template<unsigned Count,class State>
void pop_x87(State &s) {
    static_assert(Count==1u||Count==2u);
    s.fp.status&=~0x200u;
    for(unsigned i=0;i<Count;++i)s.fp.pop();
}

template<std::uint32_t ReturnSite,class State>
bool frame(State &s) noexcept {
    return !reference_shared_return() &&
        native_mopp_ray_dispatch_detail::empty_case<State,ReturnSite>(s);
}

template<class State>
Rejection rejection(State &s) {
    namespace dispatch=native_mopp_ray_dispatch_detail;
    if(reference_shared_return() || s.active_mmx || s.fp.count<2u || !dispatch::admitted(s))
        return Rejection::original;
    pop_x87<2u>(s);
    dispatch::retire(s,0x30b6adu);
    if(physics_contact_runtime_detail::reference_scalar_stack_pops() ||
       !dispatch::ordinary_return_frame<0x30b6b5u>(s))return Rejection::frame;
    return Rejection::complete;
}
}
