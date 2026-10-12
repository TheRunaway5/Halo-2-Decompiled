#include "halo2/frontend_model.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
std::array<std::int32_t,4> frontend_vista_model_viewport(
    std::array<std::int16_t,4> authored,std::array<std::int16_t,4> drawable) {
  const int width=int(drawable[3])-drawable[1],height=int(drawable[2])-drawable[0];
  if(width<=0 || height<=0)throw FormatError("empty Vista model drawable");
  // MULSS uses the actual reciprocal word, not float(height/1200).
  const float scale=float(height)*std::bit_cast<float>(0x3a5a740eu);
  const float half_width=float(width>>1),half_height=float(height>>1);
  const float top_offset=float(authored[0])*scale,left_offset=float(authored[1])*scale,
      bottom_offset=float(authored[2])*scale,right_offset=float(authored[3])*scale;
  return {std::int32_t(half_height-top_offset),std::int32_t(left_offset+half_width),
          std::int32_t(half_height-bottom_offset),std::int32_t(right_offset+half_width)};
}
Camera frontend_model_camera(const UiScene& scene,Vec3 offset) {
  for(const auto v:{scene.camera.x,scene.camera.y,scene.camera.z,
      scene.scale.x,scene.scale.y,scene.scale.z,offset.x,offset.y,offset.z,
      scene.field_of_view})
    if(!std::isfinite(v))throw FormatError("invalid original model-scene camera input");
  Camera camera;
  camera.position={scene.camera.x-scene.scale.x*offset.x,
      scene.camera.y-scene.scale.y*offset.z,
      scene.camera.z-scene.scale.z*offset.y};
  camera.oriented=true;camera.forward={0,1,0};camera.up={0,0,1};
  camera.field_of_view=scene.field_of_view;
  camera.projection_vertical_angle=std::clamp(scene.field_of_view,1.f,359.f)*
      std::bit_cast<float>(0x3c0efa35u);
  return camera;
}
namespace {
float clock_end(unsigned frames) {
  if(!frames || frames>10000)throw FormatError("frontend model clock clip outside source range");
  return float(frames)-std::bit_cast<float>(0x38d1b717u);
}
float next_clock_frame(const FrontendModelClock&clock,float seconds) {
  const float elapsed=seconds*clock.rate;
  const float frames=elapsed*30.f;
  return frames+clock.frame;
}
void clock_set(FrontendModelClock&clock,float frame,float end,float seconds) {
  clock.frame=std::clamp(frame,0.f,end);
  if(!(clock.flags&9)) {
    if(next_clock_frame(clock,seconds)>=end)clock.flags|=2;
    else clock.flags&=0xfd;
  }
}
}
void frontend_model_clock(FrontendModelClock&clock,unsigned frames,unsigned loop,float seconds) {
  const float end=clock_end(frames);
  if(loop>=frames || !std::isfinite(seconds) || !std::isfinite(clock.frame) || !std::isfinite(clock.rate))
    throw FormatError("invalid source frontend model clock input");
  if(clock.delay){--clock.delay;return;}
  if(clock.flags&9)return;
  float frame=std::max(next_clock_frame(clock,seconds),0.f);
  clock.flags&=0xf1;
  if(frame>=end) {
    if(!loop)clock.rate=1; // Options0x20 and0x08, unchanged single variant.
    const float wrapped=frame-end;
    frame=wrapped+float(loop);
    clock.flags|=4;
  }
  clock_set(clock,frame,end,seconds);
}
void frontend_model_preserve_phase(FrontendModelClock&clock,unsigned old_frames,unsigned frames,float seconds) {
  const float old_end=clock_end(old_frames),end=clock_end(frames);
  const float fraction=std::clamp(std::clamp(clock.frame,0.f,old_end)/old_end,0.f,1.f);
  clock={};
  const float frame=fraction>0?end*fraction:0.f;
  clock_set(clock,frame,end,seconds);
}

}

extern "C" void halo2_frontend_model_camera(const float* in,float* out) {
  halo2::UiScene scene;scene.camera={in[0],in[1],in[2]};
  scene.scale={in[3],in[4],in[5]};scene.field_of_view=in[9];
  const auto camera=halo2::frontend_model_camera(scene,{in[6],in[7],in[8]});
  const std::array<float,10> words{camera.position.x,camera.position.y,camera.position.z,
      camera.forward.x,camera.forward.y,camera.forward.z,camera.up.x,camera.up.y,
      camera.up.z,*camera.projection_vertical_angle};
  std::copy(words.begin(),words.end(),out);
}
extern "C" void halo2_frontend_model_clock(const float*in,std::uint32_t*out) {
  halo2::FrontendModelClock clock{in[4],in[3],std::uint8_t(in[5]),std::uint8_t(in[6])};
  halo2::frontend_model_clock(clock,unsigned(in[0]),unsigned(in[1]),in[2]);
  out[0]=std::bit_cast<std::uint32_t>(clock.frame);out[1]=std::bit_cast<std::uint32_t>(clock.rate);
  out[2]=clock.flags;out[3]=clock.delay;
}
extern "C" void halo2_frontend_model_phase(const float*in,std::uint32_t*out) {
  halo2::FrontendModelClock clock;clock.frame=in[3];
  halo2::frontend_model_preserve_phase(clock,unsigned(in[0]),unsigned(in[1]),in[2]);
  out[0]=std::bit_cast<std::uint32_t>(clock.frame);out[1]=std::bit_cast<std::uint32_t>(clock.rate);
  out[2]=clock.flags;out[3]=clock.delay;
}
