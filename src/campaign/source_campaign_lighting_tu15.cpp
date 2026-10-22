#include "halo2/source_campaign_lighting_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/frontend_model_lighting.hpp"
#include <cstring>
namespace halo2 {
namespace {
std::uint32_t bank(PhysicsLiveWorldTu15 &world) {
    const auto result=Bytes(world.memory().view(0x4eabc4u,4)).u32(0);
    if(!result)throw FormatError("Cinematic lighting has no retained original bank");
    return result;
}
ObjectLightingState current(PhysicsLiveWorldTu15 &world,std::uint32_t address) {
    static_assert(sizeof(ObjectLightingState)==80);
    ObjectLightingState state;
    std::memcpy(&state,world.memory().view(address+0x1cu,sizeof(state)).data(),sizeof(state));
    return state;
}
void publish(PhysicsLiveWorldTu15 &world,std::uint32_t address,const ObjectLightingState &state) {
    world.write_source_call_input(address+0x1cu,std::as_bytes(std::span(&state,1)));
}
}
void source_campaign_primary_light_tu15(PhysicsLiveWorldTu15 &world,float elevation,float azimuth,Vec3 color) {
    const auto address=bank(world);auto state=current(world,address);
    frontend_cinematic_primary(state,elevation,azimuth,color);publish(world,address,state);
    // BDFF0's final word store retains the upper half of packed metadata.
    const std::uint16_t none=0xffff;
    world.write_source_call_input(address+0x6cu,std::as_bytes(std::span(&none,1)));
}
void source_campaign_secondary_light_tu15(PhysicsLiveWorldTu15 &world,float elevation,float azimuth,Vec3 color) {
    const auto address=bank(world);auto state=current(world,address);
    frontend_cinematic_secondary(state,elevation,azimuth,color);publish(world,address,state);
}
void source_campaign_ambient_light_tu15(PhysicsLiveWorldTu15 &world,Vec3 color) {
    world.write_source_call_input(bank(world)+0x1cu,std::as_bytes(std::span(&color,1)));
}
void source_campaign_object_lighting_tu15(PhysicsLiveWorldTu15 &world,std::uint32_t object,bool active) {
    if(object==0xffffffffu)return;
    const auto address=world.source_object_body(object)+4u;
    const auto old=Bytes(world.memory().view(address,4)).u32(0);
    const auto value=active?old|0x01000000u:old&0xfeffffffu;
    world.write_source_call_input(address,std::as_bytes(std::span(&value,1)));
}
}
