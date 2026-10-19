#pragma once
#include "halo2/model_interpolation.hpp"
#include "halo2/cache.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_object_view.hpp"
#include "halo2/source_scene_effect_owner.hpp"
#include <functional>
#include <memory>

namespace halo2 {
class CollisionWorld;
class SceneObjects;
struct ObjectLightingBinding;
struct SourceSceneryQueryState {
    Vec3 sphere_center;
    float sphere_radius{};
    Vec3 velocity;
    std::uint32_t runtime_flags{};
    bool frozen=true; // Actual 10A1B0/B9B90 constructor state.
};
// Prepared-resource census; synchronize separately owns live draw admission.
struct SourceScenerySceneStats {
    unsigned placements{},render_models{},baked_models{},dynamic_models{},direct_probes{};
    unsigned union_baked_materials{},union_dynamic_materials{};
    unsigned unresolved_selections{}; // Explicitly omitted owners only.
    unsigned deferred_resources{}; // Catalogued failures, fatal on a visible source lifetime.
};
struct SourceSceneryResourceBoundary {
    std::size_t source_object{};
    std::string error;
};
// Resolved original SCEN region bytes, exact resident collectors and baked
// section offsets. No RNG choice or current cached sphere is synthesized.
class SourceSceneryScene {
public:
    using InitialSelection=std::function<SourceInitialModelSelection(std::size_t,
        const ScenarioObject &,const SourceInitialModelDefinition &)>;
    using LifetimeSelection=std::function<SourceInitialModelSelection(std::size_t,
        std::uint32_t full_handle,std::uint64_t created_tick)>;
    // Called only for still-unresolved initial bytes. Returning true omits
    // this owner explicitly; it never selects a fallback region/permutation.
    using OmitUnresolvedSelection=std::function<bool(std::size_t,const ScenarioObject &)>;
    using LightingQueryState=std::function<SourceSceneryQueryState(std::size_t,
        std::uint32_t full_handle,const Transform &actual_world_node0,float scale)>;
    // Existing model pose consumes seconds. Multi-frame initialized clips
    // require the caller's actual retained controller-time producer; no birth
    // or render timestamp is silently substituted by this scene module.
    using AnimationTime=std::function<float(std::size_t,std::uint32_t full_handle,
        std::uint64_t created_tick,std::uint64_t simulation_tick)>;
    // Preferred retained controller frame. The actual initialized clip is
    // supplied so the caller can validate its graph/binding/metadata identity.
    using AnimationFrame=std::function<float(std::size_t,std::uint32_t full_handle,
        std::uint64_t created_tick,std::uint64_t simulation_tick,const AnimationClip &)>;
    using AnimationFrames=std::function<AnimationFrameWindow(std::size_t,std::uint32_t,
        std::uint64_t,std::uint64_t,const AnimationClip &)>;
    // Constructor cached bounds are captured from the initial WORLDnode0
    // before any later controller-time sample. This does not query lighting.
    using BirthQueryCapture=std::function<void(std::size_t,std::uint32_t full_handle,
        std::uint64_t created_tick,std::uint32_t definition,const Transform &initial_world_node0,float scale)>;
    SourceSceneryScene(const Cache &,std::span<const ScenarioObject>,std::size_t bsp,
                      Mesh &,const CollisionWorld &,InitialSelection = {},OmitUnresolvedSelection = {},
                      bool source_region_union = false);
    ~SourceSceneryScene();
    SourceSceneryScene(SourceSceneryScene &&) noexcept;
    SourceSceneryScene &operator=(SourceSceneryScene &&) noexcept;
    SourceSceneryScene(const SourceSceneryScene &)=delete;
    SourceSceneryScene &operator=(const SourceSceneryScene &)=delete;
    // Exact completed presentation state. Authored resident models/material
    // slots and callback bindings stay in this scene; no lifetime is rebuilt.
    // The caller separately owns any state reached through those callbacks.
    class Checkpoint {
    public:
        bool valid() const noexcept;
    private:
        friend class SourceSceneryScene;
        struct Data;
        std::shared_ptr<const Data> data_;
    };
    Checkpoint checkpoint() const;
    // Later scene frames preserve historical ownership. Rebinding one of the
    // external services changes the binding identity and retires older cuts.
    bool owns_checkpoint(const Checkpoint &) const noexcept;
    class PreparedRestore {
    public:
        ~PreparedRestore();
        PreparedRestore(PreparedRestore &&) noexcept;
        PreparedRestore &operator=(PreparedRestore &&) noexcept;
    private:
        friend class SourceSceneryScene;
        PreparedRestore();
        struct Data;
        std::unique_ptr<Data> data_;
    };
    PreparedRestore prepare_restore(const Checkpoint &) const;
    bool restore_ready(const PreparedRestore &) const noexcept;
    // Swaps completed values only. The caller retains the consumed token and
    // its retired containers until its enclosing publication has ended.
    bool commit_restore(PreparedRestore &) noexcept;
    // Prepared resources remain inactive until actual source lifetimes bind.
    void synchronize(const SceneObjects &,LifetimeSelection = {});
    // Completed-frame world adoption: invalidate cached lifetimes/clock/LOD,
    // retaining authored resident resources. New synchronization supplies the
    // actual current source regions, WORLD nodes and creation ticks.
    void reset_lifetimes() noexcept;
    void interpolate(const SceneObjects &,const class SceneInterpolation &,float fraction);
    void lighting_query_state(LightingQueryState);
    void animation_time(AnimationTime);
    void animation_frame(AnimationFrame);
    void animation_frames(AnimationFrames);
    void birth_query_capture(BirthQueryCapture);
    void update_pose(std::uint64_t simulation_tick,std::int32_t render_frame,Mesh &);
    std::vector<SourceSceneEffectOwner> effect_owners() const;
    void append_view(std::vector<Vertex> &,std::vector<ObjectLightingBinding> &,
                     const SourceObjectViewLod &,std::vector<SourceObjectViewSelection> * = nullptr);
    const SourceScenerySceneStats &stats() const;
    std::span<const std::size_t> unresolved_selection_owners() const;
    std::span<const SourceSceneryResourceBoundary> deferred_resource_owners() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
