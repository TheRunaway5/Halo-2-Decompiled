#pragma once

#include "rendering/effects/native_tu15_contrail_vertex_emission.hpp"
#include "rendering/models/native_tu15_model_light_source.hpp"

namespace halo2::native_contrail_object_color_detail {

inline bool bridge(physics_world_runtime_detail::WorldState &state) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_OBJECT_COLOR");
        return value && *value;
    }();
    if (reference || state.image != SourcePhysicsContactImage::TitleUpdate15 ||
        state.services.instruction || state.observe ||
        !native_contrail_vertex_emission_detail::ScopedDrawServices::identifies(state))
        return false;
    model_light_source_detail::color_channel_with_live_frame(state);
    return true;
}

} // namespace halo2::native_contrail_object_color_detail
