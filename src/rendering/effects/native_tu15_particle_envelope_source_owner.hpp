#pragma once
#include "rendering/effects/native_tu15_particle_envelope_source_frame.hpp"
#include <cstdlib>

namespace halo2::native_particle_envelope_owner_detail {
inline bool reference() {
    static const bool enabled=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_PARTICLE_ENVELOPE");
        return value&&*value;
    }();
    return enabled;
}

template<bool Literal,class State> bool bridge(State& state) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(state.image!=SourcePhysicsContactImage::TitleUpdate15||reference())return false;
    native_particle_envelope_detail::run_source_frame<Literal>(state);
    return true;
#else
    (void)state;
    return false;
#endif
}
} // namespace halo2::native_particle_envelope_owner_detail
