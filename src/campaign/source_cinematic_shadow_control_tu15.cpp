#include "halo2/source_cinematic_shadow_control_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <bit>
namespace halo2 {
void source_cinematic_shadow_control_tu15(PhysicsLiveWorldTu15 &world,bool enabled,
    std::uint32_t object,std::uint32_t marker,float radius) {
    auto abi=world.prepare_source_call(std::array{std::bit_cast<std::uint32_t>(radius)});
    abi.gp[3]=enabled?1u:0u;abi.gp[6]=object;abi.gp[7]=marker;
    world.run_actual(0x30e10u,abi);
}
}
