#include "halo2/menu_projection.hpp"
#include "halo2/frontend_canvas.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_frustum_camera.hpp"
#include <cmath>
#include <numbers>

namespace halo2 {
MenuProjectionLens menu_projection_lens(float degrees, MenuVideoMode mode) {
  const float angle = source_camera_angles(degrees * .017453292f).projection_angle;
  const RenderViewport outer{0,0,640,480};
  const float x = mode.high_definition ? 32.f : 48.f;
  const float y = mode.high_definition ? 24.f : 36.f;
  const auto clip = source_camera_frustum(outer,{x,y,640-2*x,480-2*y});
  const float half_x = (clip[1]-clip[0])*.5f;
  const float half_y = (clip[3]-clip[2])*.5f;
  float tangent = float(std::tan(static_cast<long double>(angle)*.5L));
  float aspect = 640.f/480.f;
  if (mode.widescreen && !mode.main_menu) {
    aspect *= 4.f/3.f;
    if (mode.cinematic) tangent = tangent*(4.f/3.f)*.5625f;
  }
  return {angle,{1.f/(half_x*aspect*tangent),1.f/(half_y*tangent)}};
}
Camera menu_render_camera(Camera camera) {
  if (!camera.horizontal_field_of_view)
    throw FormatError("menu projection requires the original horizontal observer FOV");
  camera.projection_vertical_angle =
      menu_projection_lens(camera.field_of_view).render_angle;
  // 12560 selects x/y insets 48/36 for standard video; 16A30 and 2F6B0
  // produce outer (0,0,480,640), inner (36,48,444,592). 2F970 expands
  // the clip bounds by outer/inner, so 2FB70 scales each projection axis
  // by inner/outer. Keep this separate from the raw angle used by haze.
  camera.projection_clip_scale = {544.f / 640.f, 408.f / 480.f};
  return camera;
}
MenuRenderView menu_render_view(const Camera &camera, bool auxiliary_present,
                                bool cinematic_active) {
  if (!camera.horizontal_field_of_view || !camera.oriented ||
      !camera.projection_vertical_angle)
    throw FormatError("menu render bank requires an oriented original menu render camera");
  const float angle = *camera.projection_vertical_angle;
  const auto finite = [](Vec3 v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
  };
  if (!std::isfinite(angle) || !(angle > 0 && angle < std::numbers::pi_v<float>) ||
      !finite(camera.position) || !finite(camera.forward) || !finite(camera.up))
    throw FormatError("menu render bank has invalid camera inputs");
  if (camera.projection_clip_scale.x != 544.f / 640.f ||
      camera.projection_clip_scale.y != 408.f / 480.f ||
      camera.projection_clip_offset.x != 0 || camera.projection_clip_offset.y != 0)
    throw FormatError("menu render bank requires the original standard-video clip fields");
  SourceFrustumProjectionInput input;
  input.projection_angle = angle;
  input.outer = {0, 0, 480, 640};
  input.safe = {36, 48, 444, 592};
  input.layout_state_present = true;
  input.layout_state_flag = 1;
  input.layout_state_index = 0;
  input.layout_state_mode = 3;
  input.auxiliary_state_present = auxiliary_present;
  input.auxiliary_state_flag = cinematic_active;
  input.scenario_present = true;
  input.scenario_type = 2;
  const auto projection = source_frustum_projection(input);
  return {source_view_projection({camera.position, camera.forward, camera.up,
      .015625f, 1024.f, {}, projection.projection_program}),projection.view_modifier};
}
SourceViewProjectionBank menu_render_view_bank(const Camera &camera) {
  return menu_render_view(camera,true,true).bank;
}
MenuFrameProjection menu_full_frame_projection(const Camera &camera,
    std::uint32_t width, std::uint32_t height) {
  const auto canvas = frontend_canvas(width, height);
  MenuFrameProjection result{camera, menu_render_view_bank(camera),
      {canvas.width / canvas.surface_width, canvas.height / canvas.surface_height}};
  // Scale only clip X/Y; W-depth, camera orientation and original depth
  // arithmetic stay intact. Skip identity multiplication for the retail bank.
  for (unsigned axis = 0; axis < 2; ++axis) {
    const float scale = axis ? result.scale.y : result.scale.x;
    if (scale != 1.f)
      for (unsigned column = 0; column < 4; ++column)
        result.bank.projection[column * 4 + axis] *= scale;
  }
  if (result.scale.y != 1.f)
    result.camera.projection_vertical_angle = float(2.0 * std::atan2(
        std::tan(double(*camera.projection_vertical_angle) * .5) / result.scale.y, 1.0));
  return result;
}
} // namespace halo2
