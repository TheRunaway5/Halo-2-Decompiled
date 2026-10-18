#pragma once
#include "halo2/scene.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_frustum_camera.hpp"

namespace halo2 {
// Ordinary single-view cinematic lens. Canonical safe edges come from the
// caller's existing source viewport owner; physical scaling uses the same
// signed-short edge publication as the campaign's gameplay path. The retained
// TU1.5 checkpoints establish the 640x480 standard view only. Other physical
// extents are a native extension, not evidence for original wide/split modes.
struct SourceCampaignCameraLensTu15 {
    SourceFrustumProjectionInput source;
    SourceCameraProjection raster;
    void apply(Camera &camera) const {
        camera.projection_vertical_angle=source.projection_angle;
        camera.projection_clip_scale=raster.clip_scale;
        camera.projection_clip_offset=raster.clip_offset;
    }
};
inline SourceCampaignCameraLensTu15 source_campaign_camera_lens_tu15(
    float projection_angle,std::uint32_t width,std::uint32_t height,
    RenderViewport canonical_safe) {
    const auto outer=full_surface_viewport({0,0,640,480},width,height);
    const auto safe=full_surface_viewport(canonical_safe,width,height);
    const auto edges=[](RenderViewport r) {
        return std::array<std::int16_t,4>{std::int16_t(r.top),std::int16_t(r.left),
            std::int16_t(r.top+r.height),std::int16_t(r.left+r.width)};
    };
    SourceCampaignCameraLensTu15 result;
    result.source.projection_angle=projection_angle;
    result.source.outer=edges(outer);result.source.safe=edges(safe);
    result.raster=source_camera_projection(projection_angle,outer,safe,false);
    return result;
}
} // namespace halo2
