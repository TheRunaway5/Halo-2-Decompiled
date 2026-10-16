#pragma once
#include "halo2/physics_live_world_tu15.hpp"
#include <array>

namespace halo2::pstencil {
// Photo input through the unchanged native constructor. TU1.5 B9960 owns the
// C4-byte request; BA1D7..BA1DC passes request+C to BC570's model-variant
// resolver. The retained world supplies the actual caller stack/TLS/FP bank.
inline std::uint32_t create_variant_photo_biped(PhysicsLiveWorldTu15& world,
    std::uint32_t definition,Vec3 position,Vec3 forward,std::uint32_t variant) {
    constexpr std::uint32_t request=0x60010000u;
    const std::array initial{definition,0xffffffffu,0u};
    auto frame=world.prepare_source_call(initial);frame.gp[6]=request;
    world.run_actual(0xb9960u,frame);
    world.write_source_call_input(request+0x1cu,std::as_bytes(std::span(&position,1)));
    world.write_source_call_input(request+0x28u,std::as_bytes(std::span(&forward,1)));
    world.write_source_call_input(request+0xcu,std::as_bytes(std::span(&variant,1)));
    const std::array birth{request};return world.call_actual(0xb9b70u,birth);
}
}
