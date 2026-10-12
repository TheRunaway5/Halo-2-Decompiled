#pragma once
#include "halo2/frontend.hpp"
#include "halo2/shader_animation.hpp"

namespace halo2 {
// Original2B0BAA: authored animation movement changes the camera, with the
// widget Y/Z axes exchanged. The original draw sends a vertical half-angle
// in camera+28 directly to2C560; it does not run the observer FOV conversion.
Camera frontend_model_camera(const UiScene&,Vec3 animation_offset = {});
// PC65E191 ->664979 and64090D..6409A2. Signed rectangles are
// (top,left,bottom,right); output is the original physical pixel rectangle.
std::array<std::int32_t,4> frontend_vista_model_viewport(
    std::array<std::int16_t,4> authored,std::array<std::int16_t,4> drawable);
// Bounded original primary channel with options0x3F and no event/variant
// producer: the supplied menu clips have no timing/sound/effect events and
// no parent/next variants. Frame/rate/delay/flags retain source rounding.
struct FrontendModelClock {
  float frame{},rate=1;
  std::uint8_t flags{},delay{};
};
void frontend_model_clock(FrontendModelClock&,unsigned frames,unsigned loop_frame,float seconds);
// 1CD070 with state flags0x22/channel options0x3F. Caller only invokes this
// after a successful UI request; a failed request leaves the channel intact.
void frontend_model_preserve_phase(FrontendModelClock&,unsigned old_frames,
    unsigned new_frames,float seconds);
// Original B7B40+49F..4ED: object+F0 is one only for a positive maximum.
float frontend_model_initial_shield_fraction(float maximum_shield);
// BCC20's bounded menu object inputs. Appearance is optional before a model
// widget writes its emblem bytes; shield fraction is constructor state.
std::optional<float> frontend_model_material_input(const FrontendAppearance *,
    float shield_fraction,std::uint32_t name);
// Animate each selected body pass, including independently submitted layers7/8.
// Copies shared child banks before writing per-object animated state.
void animate_frontend_model_material(Material &,double seconds,const ShaderInput &);
}
