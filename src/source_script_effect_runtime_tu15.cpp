#include "halo2/source_script_effect_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"

namespace halo2 {
void source_script_marker_effect_tu15(PhysicsLiveWorldTu15 &world,
    std::uint32_t definition,std::uint32_t object,std::uint32_t marker) {
    auto abi=world.prepare_source_call(std::array{definition});
    abi.gp[0]=marker;abi.gp[7]=object;
    world.run_actual(0x2aa360u,abi);
}
}
