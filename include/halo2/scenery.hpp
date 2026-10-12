#pragma once
#include "halo2/cache.hpp"
#include "halo2/object_lighting.hpp"

namespace halo2 {
struct SceneryDefinition {
    std::uint32_t datum=0xffffffffu,model=0xffffffffu;
    std::uint16_t object_flags{};
    std::int16_t lightmapping_policy{}; // Tag +C0: vertex0, pixel1, dynamic2.
};
struct SceneryLightprobe {
    std::uint32_t unique_id{};
    std::int16_t origin_bsp=-1;
    std::int8_t type{},source{};
    ObjectLightingState lighting;
    std::int16_t sh_index=-1,unknown{}; // Preserved final four authored bytes.
};
// BFD20 binds only definitions with flag8, comparing origin BSP only for
// structure-origin identities. 3DB00 consumes the matching state directly.
const SceneryLightprobe* scenery_lightprobe(std::span<const SceneryLightprobe>,
    const ScenarioObject&,const SceneryDefinition&);

// Original D5060 creation gate. Engine values are retail g_4E6948+180:
// CTF1, Slayer2, Oddball3, King4, Juggernaut7, Territories8, Assault9.
// This is independent of automatic creation and BSP membership.
bool scenery_multiplayer_game_allowed(const ScenarioObject&,int original_engine);
// Map initialization D59F0/D5640 additionally reject placement flag bit0.
bool scenery_automatic_creation_allowed(const ScenarioObject&,int original_engine);

enum class SceneryLightingImplementation {per_vertex=1,dynamic=2,unsupported=3};
// Source 10A820/10A8B0 identity lookup and 2D000's model path.
// An invalid original bank retains implementation3 as unresolved here;
// the separate source object-state override is not inferred.
SceneryLightingImplementation scenery_lighting_implementation(const BspLighting&,
    const RenderModel&,const ScenarioObject&,const SceneryDefinition&);
} // namespace halo2
