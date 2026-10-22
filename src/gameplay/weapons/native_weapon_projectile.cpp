#include "halo2/native_weapon_projectile.hpp"
#include "halo2/cache.hpp"
#include <cmath>
#include <algorithm>

namespace halo2 {
SourceNeedlerProjectileBirth source_needler_projectile_birth(
    const SourceNeedlerProjectileDefinition &d,Vec3 forward,Vec3 inherited,
    std::int16_t tick_rate,std::uint32_t &rng) {
    if(tick_rate<=0)throw FormatError("Needler constructor requires a positive original tick rate");
    const auto random_range=[&](float lower,float upper) {
        // FC3C8..FC431 and FC45E..FC4EC. Equal ranges take no random draw.
        if(lower==upper)return double(lower);
        rng=rng*0x19660du+0x3c6ef35fu;
        volatile double scaled=double(rng>>16)*double(d.rng_scale);
        volatile double range=double(upper)-double(lower);
        volatile double product=range*scaled;
        volatile double value=double(lower)+product;
        return double(value);
    };
    SourceNeedlerProjectileBirth out;
    out.guidance_rate=float(random_range(d.guidance_lower,d.guidance_upper));
    // The unequal timer branch multiplies by the signed source tick rate in
    // x87 before the sole FST binary32 rounding. Do not round seconds first.
    const auto timer_ticks=float(random_range(d.timer_lower,d.timer_upper)*double(tick_rate));
    if(timer_ticks>=d.one)out.timer_step=d.one/timer_ticks;
    const float arming_ticks=float(tick_rate)*d.arming_seconds;
    if(arming_ticks>=d.one)out.arming_step=d.one/arming_ticks;
    out.speed_scale=d.one;out.angular_velocity=d.air_angular_velocity;
    out.speed=out.speed_scale*d.initial_speed;
    out.velocity={forward.x*out.speed+inherited.x,forward.y*out.speed+inherited.y,
        forward.z*out.speed+inherited.z};
    return out;
}
Vec3 source_weapon_projectile_up(Vec3 f) {
    const auto x=std::abs(f.x),y=std::abs(f.y),z=std::abs(f.z);
    // The source compares abs(Y) >= abs(X), then Z < X for its first
    // branch. Preserve its tie decisions, SSE subtractions and x87 X,Z,Y
    // sum. FPCW027F gives a 53-bit significand; FST rounds only the divisor.
    Vec3 up;
    if(y>=x && z>=x)up={0.f,f.z,0.f-f.y};
    else if(z>=y)up={0.f-f.z,0.f,f.x};
    else up={f.y,0.f-f.x,0.f};
    const double squared=(double(up.x)*up.x+double(up.z)*up.z)+double(up.y)*up.y;
    const auto magnitude=std::sqrt(squared);
    if(std::abs(magnitude)>=double(0x1.a36e2ep-14f)) {
        const float reciprocal=1.f/float(magnitude);
        up={up.x*reciprocal,up.y*reciprocal,up.z*reciprocal};
    }
    return up;
}
float source_projectile_acceleration_range(float distance,float initial,float final,
    float start,float inverse,std::uint32_t flags) {
    if((flags&0x100u) || initial==0 || final==0)return 1;
    return std::clamp((distance-start)*inverse,0.f,1.f);
}
Vec3 source_projectile_rgb_color(Vec3 lower,Vec3 upper,float amount) {
    const float inverse=1.f-amount;
    return {std::clamp(lower.x*inverse+upper.x*amount,0.f,1.f),
        std::clamp(lower.y*inverse+upper.y*amount,0.f,1.f),
        std::clamp(lower.z*inverse+upper.z*amount,0.f,1.f)};
}
}

namespace halo2 {
float source_projectile_speed_scale(std::uint32_t flags,std::uint32_t player,
    bool campaign,std::int16_t difficulty,const std::optional<std::array<float,4>> &table) {
  if(!(flags&0x400u))return 1.f;
  if(player!=0xffffffffu)return 1.5f; // Original446548/449FD4, bits3FC00000.
  if(!table)return 1.f;
  return (*table)[campaign?std::clamp<int>(difficulty,0,3):1];
}
}
