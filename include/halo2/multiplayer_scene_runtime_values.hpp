#pragma once
#include "halo2/model_selection.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_portal_mask.hpp"
#include "halo2/source_prop_initial_state.hpp"
#include "halo2/source_scenery_idle_phase.hpp"
#include "halo2/source_scenery_initial_state.hpp"
#include <map>
#include <optional>
#include <set>
#include <type_traits>
#include <utility>

namespace halo2 {
// The actual owning values reached by the multiplayer scenery/prop callbacks
// and machine portal publisher. Resident definitions and callback bindings stay
// outside this bundle. The enclosing receipt owner supplies executor/revision,
// authentic origin/proof and completed-frame capture/publication admission.
// Copying retains existing constructor/idle/mask history; no callback, source
// controller, physics/cache query or initial-state reconstruction runs here.
struct MultiplayerSceneRuntimeValues {
    struct SceneryQueryLifetime {
        std::size_t placement{};
        std::uint64_t created_tick{};
        bool native_animated_bounds_admitted{};
        std::uint64_t advanced_tick{};
        std::optional<SourceSceneryIdlePhaseDefinition> idle_definition;
        SourceSceneryIdlePhaseState idle_state,previous_idle_state;
        bool idle_paired{};
    };
    SourceSceneryInitialState scenery_initial_state;
    std::map<std::uint32_t,SceneryQueryLifetime> scenery_query_lifetimes;
    float scenery_render_fraction=1;
    SourcePropInitialState prop_initial_state;
    std::set<std::uint32_t> prop_query_handles;
    std::optional<SourceInitialModelSelection> actual_prop_regions;
    SourcePortalMaskState source_portal_mask;
    std::map<std::uint32_t,SourcePortalOwnerState> source_portal_owners;
    std::uint64_t source_portal_tick{};
    std::array<ObjectLightingCache,multiplayer_capacity> object_light_cache;
    std::array<std::uint32_t,multiplayer_capacity> object_light_units=[] {
        std::array<std::uint32_t,multiplayer_capacity> units;units.fill(null_datum);return units;
    }();
    std::map<std::uint32_t,ObjectLightingCache> corpse_light_cache,vehicle_light_cache;
    std::map<std::uint32_t,std::array<std::int8_t,4>> vehicle_previous_lods;
    std::array<ShadowCasterSphere,multiplayer_capacity> object_owner_spheres;
    std::array<bool,multiplayer_capacity> object_light_alive{},object_light_reported{};

    // Swap the field values, preserving references retained by the existing
    // callback closures. The caller keeps the replaced bundle alive until all
    // borrowers of its former element storage have retired.
    void swap(MultiplayerSceneRuntimeValues &other) noexcept {
        static_assert(std::is_nothrow_swappable_v<SourceSceneryInitialState>);
        static_assert(std::is_nothrow_swappable_v<SourcePropInitialState>);
        static_assert(std::is_nothrow_swappable_v<SourceInitialModelSelection>);
        using std::swap;
        swap(scenery_initial_state,other.scenery_initial_state);
        scenery_query_lifetimes.swap(other.scenery_query_lifetimes);
        swap(scenery_render_fraction,other.scenery_render_fraction);
        swap(prop_initial_state,other.prop_initial_state);
        prop_query_handles.swap(other.prop_query_handles);
        swap(actual_prop_regions,other.actual_prop_regions);
        swap(source_portal_mask,other.source_portal_mask);
        source_portal_owners.swap(other.source_portal_owners);
        swap(source_portal_tick,other.source_portal_tick);
        swap(object_light_cache,other.object_light_cache);swap(object_light_units,other.object_light_units);
        corpse_light_cache.swap(other.corpse_light_cache);vehicle_light_cache.swap(other.vehicle_light_cache);
        vehicle_previous_lods.swap(other.vehicle_previous_lods);swap(object_owner_spheres,other.object_owner_spheres);
        swap(object_light_alive,other.object_light_alive);swap(object_light_reported,other.object_light_reported);
    }
    friend void swap(MultiplayerSceneRuntimeValues &a,MultiplayerSceneRuntimeValues &b) noexcept {a.swap(b);}
};
}
