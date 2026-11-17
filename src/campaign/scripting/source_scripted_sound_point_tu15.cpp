#include "halo2/source_scripted_sound_point_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/native_campaign_script_world_tu15.hpp"
#include "halo2/cache.hpp"
namespace halo2 {
Vec3 source_scripted_sound_point_tu15(PhysicsLiveWorldTu15 &world,std::uint32_t object) {
    // Native BAD60 (marker 0x04000095, one record, owner resolution enabled)
    // into private scratch. 18C090 retains the record; the native mixer
    // consumes its completed WORLD point at +0x60.
    NativeCampaignScriptScratch scratch(world.memory(),0x70u);
    NativeCampaignScriptBank bank(scratch,{},native_campaign_script_floating_point(world));
    native_campaign_script_tu15::object_get_markers(bank,object,0x04000095u,1,false,
        NativeCampaignScriptScratch::base);
    const Bytes marker(scratch.bytes());
    return {marker.f32(0x60),marker.f32(0x64),marker.f32(0x68)};
}
}
