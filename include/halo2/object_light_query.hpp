#pragma once
#include "halo2/bsp_surface_lighting.hpp"
#include "halo2/collision.hpp"
#include "halo2/object_sky_lighting.hpp"
#include <functional>
namespace halo2 {
// 0..3 are source D2A50 statuses. Status4 retains an explicitly unported
// contact-owner choice, rather than claiming an authored surface miss.
enum class ObjectLightQueryStatus {resolved=0, unavailable=1, miss=2, temporary=3, attribution_unresolved=4};
struct ObjectLightQueryResult {
    ObjectLightQueryStatus status=ObjectLightQueryStatus::miss;
    std::optional<ObjectLightmapSample> sample;
    std::optional<RenderSurfaceHit> surface;
    unsigned rays{},attribution_failures{};
};
struct ObjectLightMapQueryContext {
    unsigned mode{};
    // D1850's first explicit argument: 3DFE0 sets it until both the source
    // sample and converted lighting state have initialized. It is separate
    // from the cardinal-ray mode derived from definition/runtime flags.
    bool initial_query=true;
    float detail=1; //Legacy caller field; original MAP query always samples1.
    // Original3DFE0 passes the retained cache+30 target to D1850/D2A50.
    // D1E10 part types4/5 preserve it on retained queries. Absence is an
    // explicit missing-input boundary when that no-write branch is reached.
    std::optional<ObjectLightmapSample> caller_sample;
};
std::uint32_t source_object_light_collision_flags(bool initial_query);
// D2A50 ordered source displacement vectors; origin is root object +0x30.
// Mode bit0 chooses five rays. A nonresolved result never overwrites the
// caller's retained sample. The outer control uses source D2A50. MAP contacts
// use the original BSP/MOPP walk; live dynamic registry and asynchronous
// bitmap/residency/status producers remain separate inputs.
ObjectLightQueryResult query_object_lightmap(const BspSurfaceLighting&,const CollisionWorld&,
    Vec3 origin,unsigned mode=0,float detail=1);
ObjectLightQueryResult query_object_lightmap(const BspSurfaceLighting&,const CollisionWorld&,
    Vec3 origin,const ObjectLightMapQueryContext&);
// D1A30's source0x10000001 query checks only the first static collision bank;
// it does not test instances, moving objects or render-surface attribution.
ObjectLightQueryResult query_object_sky_lightmap(const BspSurfaceLighting&,
    const ObjectSkyLightingDefinition* selected_render_sky,Vec3 origin);
struct ObjectLightQueryContext {
    bool bank_valid=true; // D1850's active bank/bitmap/checksum admission.
    std::int16_t object_type=-1;
    std::uint16_t definition_flags{};
    std::uint32_t runtime_flags{};
    int biped_state=-1; // Source E5670, not an inferred physics grounded flag.
    std::uint8_t vehicle_type{}; // Type1 definition+0x1F0, source reads low byte.
    std::uint32_t creature_flags{}; // Type12 definition+0xD4.
};
// Synchronous cache loading represents missing first groups by a false
// checksum match. Bucket residency is a subsequent D2A50 status, not this guard.
bool source_object_light_query_bank_valid(const BspLighting&);
// E5980/E5930's initialization/reset choice after DE4C0. Later source
// character-physics/death/lunge transitions can override the stored mode;
// this helper is not a classification of whether the player is grounded.
int source_biped_light_query_initial_state(std::uint32_t definition_physics_flags,
    std::uint16_t runtime_unit_flags_10a);
struct ObjectLightQueryAdmission {
    bool accepted{},retried{},used_sky{};
    ObjectLightQueryResult query;
};
// D1850 control flow with explicit source services/state. A successful retry
// alone does not admit an update. Rejected samples must retain the old cache.
// The native unresolved status4 is never treated as an original success.
ObjectLightQueryAdmission source_object_light_query(const ObjectLightQueryContext&,
    const std::function<ObjectLightQueryResult(unsigned mode)>& map_query,
    const std::function<ObjectLightQueryResult()>& sky_query);
} // namespace halo2
