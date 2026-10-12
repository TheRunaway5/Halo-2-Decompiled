#pragma once
#include "halo2/object_lighting.hpp"
namespace halo2 {
// D1A30 reads the selected render sky, not the object's containing cluster.
// Preserve light ordinals: lights without radiosity still affect the blend
// denominator used by subsequent authored lights.
struct ObjectSkyLightingDefinition {
    struct Light {
        float yaw{},pitch{};
        struct Radiosity {Vec3 color;float power{};};
        std::optional<Radiosity> radiosity;
    };
    std::string name;
    std::uint32_t datum=0xffffffffu;
    Vec3 outdoor_ambient;
    std::vector<Light> lights;
};
struct ObjectSkyLightFrame {
    Vec3 incident,radiosity,outdoor_ambient;
    bool test_collision{};
};
// D1A30/30BF0/131C20. No renderer transform or sky yaw is applied here.
ObjectSkyLightFrame object_sky_light_frame(const ObjectSkyLightingDefinition&);
// A contact material of NONE retains radiosity. A miss or authored material
// uses outdoor ambient. Source diffuse is the literal 0xFF555555.
ObjectLightmapSample object_sky_light_sample(const ObjectSkyLightFrame&,
    std::optional<std::int16_t> collision_material);
} // namespace halo2
