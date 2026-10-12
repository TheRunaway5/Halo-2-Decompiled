#pragma once
#include "halo2/cache.hpp"
#include <array>
#include <optional>
#include <span>
#include <vector>

namespace halo2 {
// Literal inputs to retail1A1D55/1A20E6. These are original object fields,
// not flags inferred from native animation names or a crouch button.
struct MotionSensorObject {
  Vec3 velocity;
  std::uint8_t type{}, control_c1{}, control_14a{};
  std::uint32_t player_13c=0xffffffffu, flags_134{};
  std::int32_t estimate_mode{}; // E70E0's signed unit+signed346+0C grenade phase.
};
bool source_motion_sensor_moving(const MotionSensorObject &,float minimum_speed,
                                 float sneak_speed);
bool source_motion_sensor_visible(const MotionSensorObject &,float minimum_speed,
    float sneak_speed,bool multiplayer,bool force_visible);
struct MotionSensorBlip {
  std::uint8_t x{},y{},type{},size{};
  friend bool operator==(const MotionSensorBlip &,const MotionSensorBlip &)=default;
};
struct MotionSensorSample {
  std::array<MotionSensorBlip,16> blips{};
  unsigned count{};
};
// The full source object/visibility/type/size producers are explicit inputs.
// Preserve the source nearby-then-other enumeration order; do not distance-sort.
struct MotionSensorCandidate {
  Vec3 position;
  bool present{},visible{};
  std::uint8_t type{},size{};
};
// Complete1A264E arithmetic and capacity, including strict range exclusion.
// origin is CAFC0's actual unit center and yaw is local control+24.
MotionSensorSample source_motion_sensor_sample(bool unit_present,Vec3 origin,
    float yaw,float range,std::span<const MotionSensorCandidate>);
struct MotionSensorHistory {
  std::array<std::array<MotionSensorSample,10>,4> players;
  unsigned sample_index{};
};
// The sample-storage phase of1A22B4. The world enumeration/refresh owner
// supplies each active local sample once per simulation tick.
void source_motion_sensor_store_samples(MotionSensorHistory &,
    const std::array<std::optional<MotionSensorSample>,4> &);
// Clear only this view's history counts; the world owner resets its separate
// nearby/other lists and refresh counter at the same original reset phase.
void source_motion_sensor_reset(MotionSensorHistory &,unsigned local_player);
float source_motion_sensor_pulse(std::int32_t game_tick,float tick_rate);
struct MotionSensorDrawBlip {
  Vec2 point;
  float scale{},intensity{};
  Vec3 color;
  bool custom{};
};
struct MotionSensorRenderPlan {
  bool visible{};
  Vec2 center;
  float pulse{};
  std::vector<MotionSensorDrawBlip> blips;
};
// Original1A2823->1A2197 submission order. This describes the original
// offscreen draw inputs; it does not replace254200/2548F0 graphics passes.
MotionSensorRenderPlan source_motion_sensor_render_plan(const MotionSensorHistory &,
    unsigned local_player,std::int16_t camera_mode,std::array<std::int16_t,2> origin,
    std::array<std::int16_t,2> viewport_top_left,std::int32_t game_tick,float tick_rate);
struct MotionSensorArtwork {
  Texture sweep,mask,blip,custom;
};
// Software rasterization of254200/254490/2548F0's64x64 intermediate passes.
// Authored bitmap samples, additive blips, ONE/SRC_ALPHA sweeper and
// ZERO/SRC_ALPHA mask remain separate, with original RGB-only channel writes
// preserving cleared alpha; the caller uploads the resulting page.
Texture source_motion_sensor_composite(const MotionSensorRenderPlan &,
    const MotionSensorArtwork &,Vec3 hud_color);
// Opt-in presentation reconstruction. Positions and coverage snap remain in
// the source's 64-unit coordinate system; only raster sampling density grows.
Texture source_motion_sensor_composite(const MotionSensorRenderPlan &,
    const MotionSensorArtwork &,Vec3 hud_color,unsigned raster_size);
} // namespace halo2
