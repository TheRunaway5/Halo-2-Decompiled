#pragma once
#include "halo2/cache.hpp"
#include <cstdint>
namespace halo2 {
// TU CC320 ordinary unit branch, with authored heights/current crouch/scale.
// Attached units and other object-mode branches have separate producers.
Vec3 ordinary_unit_eye(Vec3 position, float standing_height,
                       float crouching_height, float crouch, float scale = 1.f);
// Original30BD0. Below its authored epsilon, returns zero without changing
// the supplied vector; otherwise returns rounded length and normalizes it.
float original_vector_normalize(Vec3 &);
struct MovementDefinition {
  float forward{}, backward{}, sideways{}, acceleration{}, crouch_forward{},
      crouch_backward{}, crouch_sideways{}, crouch_acceleration{},
      air_acceleration{}, limited_speed{};
};
struct MovementIntent {
  Vec3 velocity;
  float acceleration{}, air_acceleration{};
};
// Retail 1E6120's player-control branch. The caller supplies the control and
// crouch state; animation/mode selection and the other input setters are
// separate.
MovementIntent movement_intent(const MovementDefinition &, Vec3 control,
                               float crouch, bool limited = false,
                               bool ignore_crouch = false);
struct SlopeDefinition {
  float downhill_scale = 1, uphill_scale = 1;
  float downhill_full = -1, downhill_start{}, uphill_start{}, uphill_full = 1;
};
struct WalkingInput {
  Vec3 input, current, facing{1, 0, 0}, normal{0, 0, 1}, basis{1, 0, 0};
  SlopeDefinition slope;
  float scale = 1, acceleration{}, air_acceleration{}, gravity{}, dt{},
        damping{};
  // Retail state bits: 4 airborne, 8 gravity, 16 horizontal damping.
  std::uint32_t flags{};
  bool suppress_ground_push{};
};
struct WalkingResult {
  Vec3 velocity;
  bool gravity{};
};
// Retail 1F0870 walking/airborne branches with the player-definition bit set.
// Havok contacts, carriers, animation-driven/flying modes and dispatch are not
// part of this kernel. Double intermediates reproduce the retail main thread's
// x87 53-bit precision; floating-point rounding must be nearest-even.
WalkingResult walking_velocity(const WalkingInput &);
// Retail FA170. Positive zero is preserved when either operand is zero.
float projectile_acceleration(float gravity_scale, float gravity);
struct JumpState {
  std::int8_t since_jump = 127, held_ticks{};
};
// Retail E4680, with E4050 support and E4770 kick acceptance supplied by
// caller.
bool jump_gate(JumpState &, bool held, bool player, bool unsupported,
               bool hard_landing, bool kick_accepted, float dt, int tick_rate);
// E4770's scalar upward impulse, relative to the supplied support velocity.
Vec3 jump_impulse(Vec3 velocity, Vec3 up, Vec3 support_velocity, float speed);
struct ProjectileFlight {
  Vec3 velocity, position, displacement;
  float traveled{};
  bool range_exhausted{};
};
// F8EB0's unaccelerated ballistic segment. Collision, water, guidance,
// detonation and material response belong to the projectile state machine.
ProjectileFlight projectile_flight(Vec3 position, Vec3 velocity, float gravity,
                                   float dt, float traveled,
                                   float maximum_range);
// Retail 0x000DD7D0: only the scalar crouch transition, with its external
// actor-speed query supplied by the caller. Differential-tested against XBE.
float crouch_delta(float crouch, float dt, float velocity, bool held,
                   bool forced);
// Retail 0x000DF5F0 capsule dimensions (world-origin query belongs to caller).
float capsule_segment_height(float standing, float crouching, float radius,
                             float crouch, bool sphere);
// Retail 0x17CA10, curve 5. Requires round-to-nearest floating-point mode.
float fade_curve_fraction(float fraction, std::span<const std::uint8_t, 1024> curve);
// Retail scalar cast helpers 20A000..20A110, including preserved high bytes.
std::uint32_t scalar_cast_bits(std::uint32_t bits, unsigned source,
                               unsigned target);
struct RechargeState {
  float body = 1, shield = 1;
  int body_stun{}, shield_stun{};
  bool dead{}, paused{};
  bool body_recharging{}, shield_recharging{}, shield_damaged{},
      shield_depleted{};
};
// Scalar ordinary-recharge branch of retail 0x000D5DE0. Overshields,
// difficulty/actor scaling and damage effects are outside this kernel.
bool recharge(RechargeState &, float dt, float body_rate, float shield_rate,
              float maximum_body_fraction = 1, float team_scale = 1,
              bool body_enabled = true, bool shield_enabled = true);
struct ArmorModifier {
  std::uint32_t name{};
  float multiplier = 1;
};
struct DamageGroup {
  std::uint32_t name{};
  std::vector<ArmorModifier> modifiers;
};
// D5BC0, including the retail binary search's duplicate-key selection and
// signed, wrapping StringID comparison. Preserve authored table ordering.
float damage_multiplier(std::span<const DamageGroup>, std::uint32_t group_a,
                        std::uint32_t group_b, std::uint32_t armor_a,
                        std::uint32_t armor_b);
struct ShieldDamageState {
  float shield = 1;
  int stun{};
  std::uint16_t flags{};
};
struct ShieldDamageInput {
  float damage{}, maximum_shield{}, multiplier = 1, minimum_stun_damage{},
        stun_seconds{}, damaged_threshold{};
  int tick_rate = 30, side_effect{};
  bool radius{};
};
struct ShieldDamageResult {
  float body_damage{}, shield_damage{};
  std::uint32_t flags{};
  bool depleted_effect{}, damaged_effect{};
};
// D9110 scalar state and overflow. Maximum vitality, material multiplier and
// actor stun override are supplied; effect/parent notifications are reported
// to the caller, rather than executing retail engine callbacks.
ShieldDamageResult shield_damage(ShieldDamageState &, const ShieldDamageInput &);
struct BodyDamageState {
  float body = 1;
  int stun{};
  std::uint16_t flags{};
};
struct BodyDamageInput {
  float damage{}, maximum_body{}, death_maximum_body{}, multiplier = 1,
        minimum_stun_damage{}, stun_seconds{}, region_vitality{};
  int tick_rate = 30;
  std::uint32_t info_flags{}, definition_flags{}, region_flags{},
                accumulator_flags{}, event_flags{};
  std::uint8_t reporting_type{};
  bool region_valid{}, hit_region{}, headshot_allowed{}, riders{}, impact_effect{};
};
struct BodyDamageResult {
  float remaining{}, normalized_damage{}, region_damage{};
  std::uint32_t flags{};
  std::uint8_t reporting_type{};
  bool region_update{}, depleted{}, impact_effect{};
};
// D8CB0 with D9020 armor scaling and DB090 region routing. Maximum vitality,
// material selection, player headshot exemption and riders are supplied.
// Region permutation/death/effect execution remains an external service.
BodyDamageResult body_damage(BodyDamageState &, const BodyDamageInput &);
} // namespace halo2
