#pragma once
#include "halo2/source_shadow.hpp"
#include <map>

namespace halo2 {
struct SourceSceneryInitialQueryState {
    ShadowCasterSphere sphere;
    Vec3 velocity;
    // The initial query producer only establishes D1850 bit13. Other object
    // flags, including the graph controller's lighting flag, have own owners.
    std::uint32_t query_runtime_flags{};
    bool frozen=true;
};
struct SourceSceneryAnimationRefreshInput {
    std::int16_t controller_offset=-1;
    std::uint32_t graph_datum=0xffffffffu;
    std::uint32_t animation_graph_datum=0xffffffffu;
    std::uint32_t animation_binding=0xffffffffu;
    std::int16_t animation_frames{};
    // Actual root header flags after the controller's 1CB710 service.
    // Attached ownership/BAF80 is a separate caller producer.
    std::uint8_t root_header_flags{};
};
struct SourceSceneryAnimationRefresh {
    bool multi_frame{}; // The original 10A2F0 return byte.
    std::uint8_t root_header_flags{}; // BBA20/B7360 output.
};
struct SourceSceneryAnimationControllerState {
    // Equivalent to 10A2F0's post-1CB710 controller/graph/binding readiness.
    // Native owners provide their actual initialized controller state without
    // manufacturing an original object-heap offset.
    bool ready{};
    std::int16_t animation_frames{};
    std::uint8_t root_header_flags{};
};
SourceSceneryAnimationRefresh source_scenery_animation_refresh(
    const SourceSceneryAnimationControllerState&);
SourceSceneryAnimationRefresh source_scenery_animation_refresh(
    const SourceSceneryAnimationRefreshInput&);
// The cold type6 factory executes BD020/BDEF0 before B8890 registration;
// 10A1B0/B9B90 freezes it. Named idle initialization can call BD020 again.
// Capture the final constructor node0 and retain this state only while that
// pose, scale and identity remain unchanged and physics has not advanced.
class SourceSceneryInitialState {
public:
    const SourceSceneryInitialQueryState& capture(std::uint32_t full_handle,
        std::uint32_t definition,const ShadowCasterDefinition&,
        const Transform& initial_world_node0,float initial_object_scale);
    bool contains(std::uint32_t full_handle)const;
    const SourceSceneryInitialQueryState& retained(std::uint32_t full_handle,
        std::uint32_t definition,const Transform& current_world_node0,
        float current_object_scale,bool physics_advanced)const;
    // An explicitly admitted pose/bounds update refreshes regular type6
    // BDEF0. The port may supply its actual native pose update here; this
    // service does not prove its scheduling equals original BC820. C1 bit0
    // freezes physics, not the animated bounds producer.
    const SourceSceneryInitialQueryState& refresh_cached_bounds(
        std::uint32_t full_handle,std::uint32_t definition,
        const ShadowCasterDefinition&,const Transform& actual_world_node0,
        float actual_object_scale,bool actual_bounds_update_admitted);
    void destroy(std::uint32_t full_handle);
private:
    struct Entry {
        std::uint32_t definition{};
        Transform initial_world_node0;
        float initial_object_scale{};
        SourceSceneryInitialQueryState state;
    };
    std::map<std::uint32_t,Entry> entries_;
};
}
