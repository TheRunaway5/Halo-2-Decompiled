#include "halo2/source_live_shadow_main_inputs_tu15.hpp"
#include "halo2/source_live_model_draws_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <cmath>

namespace halo2 {
void source_live_shadow_main_user_tu15(PhysicsLiveWorldTu15& world,std::int32_t local_user) {
    if(local_user< -1 || local_user>=4)
        throw FormatError("TU1.5 main caller local-user identity is outside its original bank");
    world.write_source_call_input(0x4c66f8u,std::as_bytes(std::span(&local_user,1)));
}
void source_live_shadow_main_location_tu15(PhysicsLiveWorldTu15& world,std::int32_t cluster) {
    const auto bsp=Bytes(world.memory().view(0x4ecc18u,4)).u32(0);
    if(!bsp)throw FormatError("TU1.5 main camera location lacks its actual BSP owner");
    const auto clusters=Bytes(world.memory().view(bsp+0x9cu,4)).u32(0);
    if(!clusters || clusters>32768u || cluster< -1 ||
       (cluster>=0 && std::uint32_t(cluster)>=clusters))
        throw FormatError("TU1.5 main camera location is outside its original BSP cluster bank");
    //2BA31 receives the same actual camera cluster supplied to portal/fog
    //admission. Publish the caller input before any source query uses it.
    world.write_source_call_input(0x4c6704u,std::as_bytes(std::span(&cluster,1)));
}
void source_live_shadow_main_collection_fog_tu15(PhysicsLiveWorldTu15& world,const BspFogSample& fog) {
    if(fog.classification>5u || !std::isfinite(fog.eye_fraction))
        throw FormatError("TU1.5 main collection fog lacks a valid completed camera sample");
    const auto put=[&](std::uint32_t address,const auto& value) {
        world.write_source_call_input(address,std::as_bytes(std::span(&value,1)));
    };
    // Publish only the four proven consumed2BA96 caller fields. The remaining
    // original fog block is not reconstructed and no visibility flags are set.
    put(0x4c6710u,std::uint32_t(fog.classification));
    put(0x4c67acu,fog.planar_datum);put(0x4c681cu,fog.eye_fraction);
    put(0x4c6824u,std::uint8_t(fog.planar_only));
}
void source_live_shadow_main_camera_tu15(PhysicsLiveWorldTu15& world,
    const SourceViewProjectionInput& camera,std::array<std::int16_t,4> viewport,
    std::int16_t mode,bool scaled) {
    if(mode || scaled || viewport[2]<=viewport[0] || viewport[3]<=viewport[1])
        throw FormatError("TU1.5 submitting shadow camera requires its actual ordinary unscaled caller");
    for(const float value:{camera.position.x,camera.position.y,camera.position.z,
        camera.up.x,camera.up.y,camera.up.z,
        camera.near_distance,camera.far_distance})
        if(!std::isfinite(value))throw FormatError("TU1.5 submitting shadow camera is not finite");
    if(camera.far_distance<=0 || !std::isfinite(1.f/camera.far_distance))
        throw FormatError("TU1.5 submitting shadow camera has no valid original W distance");
    const auto put=[&](std::uint32_t address,const auto& value) {
        world.write_source_call_input(address,std::as_bytes(std::span(&value,1)));
    };
    // The ordinary2BA09 aliases both camera sources. These are their actual
    // consumed fields, not values reconstructed from the host target extent.
    put(0x4c6600u,camera.near_distance);put(0x4c6604u,camera.far_distance);
    //2BAAB copies camera+18..20 into4C65D8..E0. The main query's441A0
    //reads this raw current up, independently of the completed projection.
    put(0x4c65d8u,camera.up);
    put(0x491e38u,camera.position);
    put(0x491e78u,camera.near_distance);put(0x491e7cu,camera.far_distance);
    put(0x491e68u,viewport);put(0x491e22u,mode);put(0x491e27u,std::uint8_t(scaled));
}
void source_live_shadow_main_inputs_tu15(PhysicsLiveWorldTu15& world,
    const SourceLiveModelDrawContextTu15& context) {
    if(context.entry.mode || context.draw.draw.mode || context.view>=4 ||
       !context.local_players || context.local_players>4 ||
       context.draw.viewport[2]<=context.draw.viewport[0] || context.draw.viewport[3]<=context.draw.viewport[1])
        throw FormatError("TU1.5 shadow caller requires its actual ordinary viewport and player count");
    for(float value:{context.entry.eye.x,context.entry.eye.y,context.entry.eye.z,
        context.forward.x,context.forward.y,context.forward.z,context.entry.distance_scale})
        if(!std::isfinite(value))throw FormatError("TU1.5 shadow caller camera input is not finite");
    const auto put=[&](std::uint32_t address,const auto& value) {
        world.write_source_call_input(address,std::as_bytes(std::span(&value,1)));
    };
    //2BAA1 copies the actual camera, then2FD70 completes this projection
    //bank. These are the consumed completed caller fields, not a replacement
    //0x74-byte camera or a projection inferred from the shadow target.
    static_assert(sizeof(SourceViewProjectionBank)==0xc0);
    put(0x4c6634u,context.draw.rendering);
    put(0x4c65c0u,context.entry.eye);put(0x4c65ecu,context.entry.distance_scale);
    //1365C0 uses the current main view direction when sorting retained model
    //presentation. Publish it before collection, including secondary views.
    put(0x4c65ccu,context.forward);
    put(0x4c65f0u,context.draw.viewport);
    put(0x4c66f4u,std::int32_t(context.view));
    put(0x4c6839u,std::uint8_t(context.view_modifier));
    //Whole2C470 owns cap, geometry/model scales, player shader multiplier
    //and every feature byte. Publishing only the feature bytes loses real
    //secondary coverage.1BE90 consumes the outer signed-short viewport and
    //the actual retained51DF50 auxiliary owner; never substitute128 here.
    auto a=world.prepare_source_call();a.gp[0]=context.local_players;
    world.run_actual(0x2c470u,a);
    a=world.prepare_source_call();world.run_actual(0x1be90u,a);
}
}
