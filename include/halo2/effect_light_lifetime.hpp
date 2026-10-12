#pragma once
#include "halo2/effect_light_registration.hpp"
#include <functional>

namespace halo2 {
// Original "lights" data array:350 records,272 bytes each. Occupancy and
// generation belong to simulation; render views only inspect this state.
struct SourceEffectLightDatumPool {
    static constexpr std::uint32_t capacity=350;
    using Record=std::array<std::uint32_t,68>;
    std::array<Record,capacity> records{};
    std::array<std::uint32_t,11> occupied{};
    std::uint32_t cursor{},highwater{},active_count{};
    std::uint16_t next_generation=0xe96c; //16B7A0:literal "li"|8000
    bool debug_fill{};
    void activate(bool debug=false);
    std::uint32_t allocate(); //16B840
    void release(std::uint32_t); //16BA40; caller owns unregister chronology
    bool contains(std::uint32_t) const;
    Record& record(std::uint32_t);
    const Record& record(std::uint32_t) const;
    std::vector<std::uint32_t> identities() const;
};
struct SourceEffectLightLifetimeWorld {
    const BspAtmosphere& locations;
    const ObjectLightSpatialBsp& memberships;
    const RawCollisionBsp& terrain;
    std::span<const std::uint8_t,1024> fade_curve;
    std::uint32_t tick{},epoch{};
    std::int16_t tick_rate=30,selected_bsp{};
    bool registration_enabled=true;
};
struct SourceStandaloneEffectLight {
    std::uint32_t definition=0xffffffffu,birth{},visited_epoch{};
    SourceEffectLightRegistration registration;
    Vec3 forward,up;
    EffectLightState state;
};
SourceStandaloneEffectLight source_standalone_effect_light(
    const SourceEffectLightDatumPool&,std::uint32_t);
// Whole C0350/C0C80/C3260 ordinary standalone path:attached owners, nodes,
// HSV and animation retain their own producers. No effect A/B value is invented.
std::uint32_t source_standalone_effect_light_create(SourceEffectLightDatumPool&,
    SourceEffectLightSpatialPool&,const EffectLightDefinition&,std::uint32_t definition,
    float control,Vec3 position,Vec3 forward,Vec3 up,const SourceEffectLightLifetimeWorld&);
void source_standalone_effect_light_update(SourceEffectLightDatumPool&,
    SourceEffectLightSpatialPool&,const EffectLightDefinition&,std::uint32_t,
    const SourceEffectLightLifetimeWorld&);
void source_standalone_effect_light_delete(SourceEffectLightDatumPool&,
    SourceEffectLightSpatialPool&,std::uint32_t);
// Whole C1400 standalone iteration:ascending occupied slots, update before
// strict duration retirement, signed wrapping source tick difference.
void source_standalone_effect_lights_advance(SourceEffectLightDatumPool&,
    SourceEffectLightSpatialPool&,
    const std::function<const EffectLightDefinition&(std::uint32_t)>&,
    const SourceEffectLightLifetimeWorld&);
}
