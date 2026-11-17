#pragma once
#include "halo2/cache.hpp"
#include "halo2/render_program.hpp"
namespace halo2 {
// Standalone, spherical effect-light inputs; attached functions, HSV color
// interpolation and authored animation blocks retain separate owners.
struct ShaderFunctionTables;
// Authored +C0 animation of a sphere light: the first mapping function of
// its brightness (+C4) and colour (+CC) blocks. No shipped sphere names a
// gel block (+D4); a definition that does carries no animation here.
struct EffectLightAnimation {
  bool synchronized{}; // +C0 bit0: every instance shares the game clock.
  std::vector<std::byte> brightness,color;
  std::shared_ptr<const ShaderFunctionTables> tables;
};
struct EffectLightDefinition {
  std::array<std::byte,228> bytes{};
  std::uint32_t flags{},lens=0xffffffffu;
  float radius{},specular_radius{},duration{};
  std::shared_ptr<const std::array<std::uint8_t,1024>> timed_curve;
  // Only the completed-animation loader fills this; load_effect_light still
  // declines every animated definition.
  std::shared_ptr<const EffectLightAnimation> animation;
};
struct EffectLightState {
  float size{},control{};
  Vec3 diffuse,specular;
  float brightness{};
};
EffectLightDefinition load_effect_light(const Cache &,std::uint32_t datum);
EffectLightDefinition source_effect_light_definition(std::span<const std::byte>);
// A sphere definition with authored brightness/colour/gel animation blocks
// (+C4/+CC/+D4). Original C2CB0/C2B30 evaluates those blocks into the light
// record each tick, so this definition is only valid for a light whose
// completed size/colour/brightness comes from that native record. The
// standalone lifetime owner (source_effect_light_state) has no animation
// evaluator and must never tick it. Frustum and HSV definitions still throw.
EffectLightDefinition source_completed_effect_light_definition(std::span<const std::byte>);
EffectLightDefinition load_completed_effect_light(const Cache &,std::uint32_t datum);
// True when the definition names any of the three animation blocks.
bool effect_light_is_animated(const EffectLightDefinition &);
struct EffectLightState;
// Whole original C2B30 for a sphere without a gel block, called by C2CB0
// after it has produced the tick's control, colours and brightness. The
// brightness function scales +EC; the colour function scales the diffuse
// colour at +D4 only. An unsynchronized light offsets the game tick by
// (identity*4999 & 0x7fffffff) % (tick_rate*60). Specular, size and control
// are not touched.
void source_effect_light_animate(const EffectLightAnimation &,std::uint32_t identity,
    std::int32_t tick,std::int16_t tick_rate,EffectLightState &);
// Original retailC0C80 / TU1.1 C1B30 standalone scalar/color update.
// Elapsed is the signed source tick difference; no render fraction is added.
EffectLightState source_effect_light_state(const EffectLightDefinition &,float control,
    std::int32_t elapsed,std::int16_t tick_rate,std::span<const std::uint8_t,1024> fade_curve);
struct EffectLightUniformInput {
  Vec3 position,forward,up,left;
  float radius{};
  Vec3 diffuse,specular;
  float brightness{},gel_u{},gel_v{};
  // 497A0 preserves C32.w, C37/38.w and C39 from the caller's actual bank.
  std::array<ShaderVector,9> previous{};
};
struct EffectLightUniforms {
  std::array<ShaderVector,9> constants; // Original C31..39.
  ShaderVector gel_basis_u,gel_basis_v;
  unsigned color_shift{};
};
// Retail497A0 spherical, ordinary falloff geometry, no animated gel offset.
// Distance/selection fades must already have been applied by the caller.
EffectLightUniforms source_effect_light_uniforms(const EffectLightUniformInput &);
struct EffectLightViewFades {
  float diffuse{},specular{},secondary{},overall{},volume{}; // record F8,FC,104,100,108
};
// RetailC12D0/C0A50, including the definition's three authored distance
// classes. Radius is the original live record+34; tick/color fades are separate.
EffectLightViewFades source_effect_light_view_fades(const EffectLightDefinition &,
    Vec3 position,float radius,Vec3 camera_position);
// C1930/C1A80 sphere shape, distinct from C28B0's persistent spatial bound.
struct EffectLightSphere {
  std::array<std::uint32_t,31> shape{};
  float diffuse_radius{},specular_radius{},radius{},spatial_radius{};
  Vec3 left;
};
EffectLightSphere source_effect_light_sphere(const EffectLightDefinition &,
    Vec3 position,Vec3 forward,Vec3 up,float size);
// 4A1E0 applies record+100 overall fade and the selected light's cached fade
// to diffuse, and min(record+104,record+100) to specular before 497A0. The
// ordinary gameplay sphere later suppresses the separate specular draw, but
// its color still participates in 497A0's shared color-shift selection.
EffectLightUniforms source_effect_light_view_uniforms(const EffectLightDefinition &,
    const EffectLightState &,Vec3 position,Vec3 forward,Vec3 up,
    const EffectLightViewFades &,float selected_fade,
    const std::array<ShaderVector,9> &previous);
}
