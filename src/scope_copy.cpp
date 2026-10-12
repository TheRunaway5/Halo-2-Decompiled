#include "halo2/scope_copy.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
ScopeCopyFrame scope_copy_frame(ShaderVector rectangle) {
  ScopeCopyFrame frame;
  frame.source_rectangle=rectangle;
  frame.program.words[8]=8;
  frame.program.words[53]=0x11001;
  frame.program.words[54]=1;
  frame.texture_state.address_u=frame.texture_state.address_v=
    frame.texture_state.address_w=3;
  frame.texture_state.mag_filter=frame.texture_state.min_filter=2;
  frame.texture_state.mip_filter=0;
  return frame;
}
ShaderVector scope_source_rectangle(RenderViewport viewport,Vec2 size,
                                   Vec2 center,Vec2 extent) {
  if(!(extent.x>0 && extent.y>0))throw FormatError("scope scene extent must be positive");
  const float half_x=viewport.width*size.x*.5f;
  const float half_y=viewport.height*size.y*.5f;
  return {(center.x-half_x)/extent.x,(center.y-half_y)/extent.y,
          (center.x+half_x)/extent.x,(center.y+half_y)/extent.y};
}
Vec2 scope_sample_center(RenderViewport viewport,std::optional<float> aim_y) {
  // The original camera rectangle has signed integer bounds, and division
  // truncates before the floating-point pixel-center conversion.
  const int left=int(viewport.left),right=int(viewport.left+viewport.width);
  const int top=int(viewport.top),bottom=int(viewport.top+viewport.height);
  return {std::floor(float((left+right)/2)+.5f),
          std::floor(aim_y.value_or(float((top+bottom)/2))+.5f)};
}
ShaderVector scope_copy_vertex(Vec2 uv,float w,float near,float far) {
  constexpr float maximum=16777215.f;
  const float q=far/(far-near);
  return {64.f*uv.x,64.f*uv.y,
    std::clamp(((q*w-near*q)/w)*maximum,0.f,maximum),
    std::clamp((w/far)*maximum,0.f,maximum)};
}
} // namespace halo2
