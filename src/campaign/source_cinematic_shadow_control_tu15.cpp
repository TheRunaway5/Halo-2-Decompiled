#include "halo2/source_cinematic_shadow_control_tu15.hpp"
#include "halo2/native_campaign_script_world_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <bit>
namespace halo2 {
void source_cinematic_shadow_control_tu15(PhysicsLiveWorldTu15 &world,bool enabled,
    std::uint32_t object,std::uint32_t marker,float radius) {
    auto bank=native_campaign_script_bank(world);
    native_campaign_script_tu15::cinematic_shadow_control(bank,enabled?1u:0u,object,marker,
        std::bit_cast<std::uint32_t>(radius)); // native 30E10
}
}
