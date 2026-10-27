#pragma once

namespace halo2 {
// Actual authored point-sprite resources: Lockout snow (32/16/8/4) and campaign
// flood spores (64/32/16/8/4). TU1.5's point caller binds the current bitmap at
// stage 3; its native VS/PS have no texture-dimension specialization. The
// resource owner separately validates BC3, one 2D layer and complete mip bytes.
constexpr bool source_weather_sprite_profile(unsigned width,unsigned height,unsigned mips) noexcept {
    return (width==32&&height==32&&mips==4)||(width==64&&height==64&&mips==5);
}
constexpr bool source_weather_sprite_resource_profile(unsigned width,unsigned height,
    unsigned mips,unsigned format,unsigned layers,unsigned depth) noexcept {
    return format==16&&layers==1&&depth==0&&source_weather_sprite_profile(width,height,mips);
}
}
