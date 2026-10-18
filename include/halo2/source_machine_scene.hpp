#pragma once
#include "halo2/cache.hpp"
#include "halo2/source_object_view.hpp"
#include "halo2/source_portal_mask.hpp"
#include "halo2/source_scene_effect_owner.hpp"
#include <memory>

namespace halo2 {
class CollisionWorld;
class SceneObjects;
class SourceDeviceWorld;
struct ObjectLightingBinding;
struct ReferenceDeviceState;
// Prepared-resource census; synchronize separately owns live draw admission.
struct SourceMachineSceneStats {
    unsigned placements{},render_models{},position_overlays{};
    unsigned dynamic_materials{},prt_materials{},initial_state_only{};
    unsigned union_dynamic_materials{},union_prt_materials{},union_section_banks{};
    unsigned deferred_resources{}; // Retained failures, rejected on a visible lifetime.
};
struct SourceMachineResourceBoundary {
    std::size_t source_object{};
    std::string error;
};
struct SourceMachinePortalState {
    std::size_t source_object{};
    std::uint64_t created_tick{};
    // first/count are the constructor NONE/0. The mask caller preserves
    // assigned ranges across snapshots and calls step(def,owner,door).
    // owner.position_group staysNONE: this typed bridge fabricates no datum.
    SourcePortalOwnerState owner;
    SourcePortalDoorInput door;
    bool initialized_group_state_only{};
};
// Authored type7 resources, with the recovered live device controller/PRT path.
// The cache and collision service must outlive this scene. Construction
// allocates all material slots before Renderer creates its resident resources.
class SourceMachineScene {
public:
    SourceMachineScene(const Cache &,std::span<const ScenarioObject>,std::size_t bsp,
                       Mesh &,const CollisionWorld &,bool source_resource_defer=false);
    ~SourceMachineScene();
    SourceMachineScene(SourceMachineScene &&) noexcept;
    SourceMachineScene &operator=(SourceMachineScene &&) noexcept;
    SourceMachineScene(const SourceMachineScene &)=delete;
    SourceMachineScene &operator=(const SourceMachineScene &)=delete;
    // Consume the caller's actual full handles and creation ticks. Placement
    // indices are global LevelContent::objects indices. Destroyed instances
    // stop submitting; a new generation resets private device state/cache.
    // Pass the simulation owner for current groups, transforms and lifetimes.
    // A replicated owner is authoritative even when the client scene's local
    // allocator has different handles. No replica clock runs in this renderer.
    // Prepared resources remain inactive until actual source lifetimes bind.
    void synchronize(const SceneObjects &,const SourceDeviceWorld * = nullptr);
    // Finite diagnostic image inputs only. Bind once before advancing the
    // scene. Full definition/placement/render-model identity must match an
    // admitted source machine; unmatched/duplicate entries are rejected.
    // Only these explicit devices freeze phase/root; normal gameplay keeps
    // the source discrete gear controller when no reference bank is bound.
    void set_reference_devices(std::span<const ReferenceDeviceState>,Mesh &);
    // A bound simulation owner supplies position/power/target/device flags.
    // Unbound diagnostic scenes retain the discrete private gear controller.
    void update(std::uint64_t tick,std::int32_t render_frame,Vec3 first_submitting_eye,
                float source_lighting_distance_scale,Mesh &);
    void update_pose(std::uint64_t tick,std::int32_t render_frame,Mesh &);
    std::vector<SourceSceneEffectOwner> effect_owners() const;
    // Render-only channels; portal/collision snapshots retain the current state.
    void interpolate(const struct SourceDeviceWorldSnapshot &previous,float fraction);
    // Retained source WORLD endpoints; no second machine controller is run.
    void interpolate(const SceneObjects &,const class SceneInterpolation &,float fraction);
    void append(std::vector<Vertex> &,std::vector<ObjectLightingBinding> &) const;
    void append_view(std::vector<Vertex> &,std::vector<ObjectLightingBinding> &,
                     const SourceObjectViewLod &,std::vector<SourceObjectViewSelection> * = nullptr);
    // Actual synchronized lifetimes, including hidden owners. A bound
    // simulation supplies current group/channel/device state. Unbound finite
    // diagnostics retain explicit initialization/private gear state. This
    // snapshot does not bind/step a mask or run HS scripts.
    std::vector<SourceMachinePortalState> portal_owners() const;
    const SourceMachineSceneStats &stats() const;
    std::span<const SourceMachineResourceBoundary> deferred_resource_owners() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace halo2
