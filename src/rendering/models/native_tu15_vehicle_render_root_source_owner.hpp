#pragma once

#include <cstdint>
#include <cstdlib>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "rendering/models/native_tu15_render_object_ownership_bridge.hpp"
#include "rendering/models/native_tu15_vehicle_render_root_source_frame.hpp"
#endif

namespace halo2::native_vehicle_render_root_detail {
inline bool reference() {
    static const bool selected = [] {
        const auto* value = std::getenv("HALO2_REFERENCE_VEHICLE_RENDER_ROOT");
        return value && *value;
    }();
    return selected;
}
inline std::uint64_t& source_calls() {
    thread_local std::uint64_t count{};
    return count;
}

// Candidate only. Preserve the existing empty-callback route; the source
// adapter adds native control/data ownership for observed calls without
// claiming a general ability to bypass their callback or remap.
template<bool Literal, class State> bool bridge(State& s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if (s.image != SourcePhysicsContactImage::TitleUpdate15 || reference()) return false;
    if (s.services.instruction) {
        ++source_calls();
        run_source_private<Literal>(s);
    }
    else native_tu15_render_object_ownership_bridge(s, 0xbcfb0u);
    return true;
#else
    (void)s;
    return false;
#endif
}
} // namespace halo2::native_vehicle_render_root_detail
