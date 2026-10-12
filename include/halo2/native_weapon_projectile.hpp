#pragma once
#include "halo2/math.hpp"
#include <cstdint>

namespace halo2 {
// Native simulation/transport owner, distinct from the original raw proj body.
// The shot producer retains its WORLD basis at birth; renderers must not
// derive orientation from the later velocity (gravity/contact can change it).
struct NativeWeaponProjectile {
    std::uint32_t datum=0xffffffffu,definition=0xffffffffu;
    Vec3 position,velocity,forward,up;
    float traveled{};
};
struct SourceNeedlerProjectileDefinition {
    float initial_speed{}, guidance_lower{}, guidance_upper{};
    float timer_lower{}, timer_upper{}, arming_seconds{}, air_angular_velocity{};
    float one{}, rng_scale{};
};
struct SourceNeedlerProjectileBirth {
    float timer_step{}, arming_step{}, speed_scale{}, angular_velocity{};
    float guidance_rate{}, speed{};
    Vec3 velocity;
};
// Closed FC2E0 Needler branch: no owner platform, medium, speed override or
// parent. Uses the caller's retained source stream, with the original PC53
// random-range arithmetic. Guidance acquisition/contact remain separate.
SourceNeedlerProjectileBirth source_needler_projectile_birth(
    const SourceNeedlerProjectileDefinition&,Vec3 forward,Vec3 inherited,
    std::int16_t tick_rate,std::uint32_t &rng);
// TU1.1 1090CD..1091F4, on the caller's actual direction at that phase.
// This consumer does not establish the original muzzle/distribution producer.
Vec3 source_weapon_projectile_up(Vec3 forward);
// Retail FBE70 acceleration_range selector and 131C20 flags0 RGB branch.
float source_projectile_acceleration_range(float traveled,float initial_velocity,
    float final_velocity,float start_distance,float inverse_distance,std::uint32_t projectile_flags);
Vec3 source_projectile_rgb_color(Vec3 lower,Vec3 upper,float amount);
}
