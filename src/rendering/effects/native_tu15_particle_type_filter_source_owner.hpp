#pragma once
#include "rendering/effects/native_tu15_particle_type_filter_source_frame.hpp"
#include <cstdlib>

namespace halo2::native_particle_type_filter_owner_detail {
inline bool reference() {
    static const bool enabled=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_PARTICLE_TYPE_FILTER");
        return value&&*value;
    }();
    return enabled;
}

template<bool Literal,class State,class Lookup> bool bridge(State& state,Lookup lookup) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(state.image!=SourcePhysicsContactImage::TitleUpdate15||reference())return false;
    native_particle_type_filter_detail::run_source_frame<Literal,true>(state,lookup);
    return true;
#else
    (void)state;
    (void)lookup;
    return false;
#endif
}
} // namespace halo2::native_particle_type_filter_owner_detail
