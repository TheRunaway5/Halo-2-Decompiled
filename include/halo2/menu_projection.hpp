#pragma once
#include "halo2/scene.hpp"
#include "halo2/source_view_projection.hpp"

namespace halo2 {
struct MenuVideoMode {
  bool widescreen{}, high_definition{};
  bool cinematic = true;
  // 2FB70 suppresses widescreen when options.flag is set, index!=-1,
  // and state==3. The live mainmenu scenario takes that branch.
  bool main_menu = true;
};
struct MenuProjectionLens {
  float render_angle{}; // Raw 2F800 angle, also consumed by haze.
  Vec2 scale{}; // Original 2FB70 normalized projection, before viewport mapping.
};
// Original display branches on the 640x480 raster, including the widescreen
// cinematic tangent adjustment and the SD/HD safe-area rectangles. This does
// not decide how a host window presents the UI or extends retail aspect ratios.
MenuProjectionLens menu_projection_lens(float horizontal_degrees, MenuVideoMode = {});
// Original single-view frontend path: 171D90 -> 2F800 -> 2F970 -> 2FB70.
// This is the standard-video projection captured at 640x480. The authored
// observer FOV remains available; haze uses the render angle before clipping.
Camera menu_render_camera(Camera);
struct MenuRenderView {
  SourceViewProjectionBank bank;
  bool view_modifier{}; // Original2F970 side effect4BA019.
};
// Same canonical title factory with explicit actual510C50 pointer/byte5 state.
// The source view modifier is retained alongside the render bank.
MenuRenderView menu_render_view(const Camera &, bool auxiliary_present,
                                bool cinematic_active);
// Original 2FD90 title render bank from menu_render_camera's output. The
// canonical standard-video outer/safe rectangles and suppressed widescreen
// path remain independent of the physical host viewport. Unsupported camera
// orientation, missing render angle, or changed clip fields are rejected.
SourceViewProjectionBank menu_render_view_bank(const Camera &);
struct MenuFrameProjection {
  Camera camera;
  SourceViewProjectionBank bank;
  Vec2 scale;
};
// Native aspect extension: keep the centered 640x480 camera image at its
// fitted scale and reveal scenery outside it. This is separate from the
// original display branches. At 4:3 the original float bank is unchanged.
MenuFrameProjection menu_full_frame_projection(const Camera &, std::uint32_t width,
                                               std::uint32_t height);
} // namespace halo2
