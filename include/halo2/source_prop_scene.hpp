#pragma once
#include "halo2/cache.hpp"
#include "halo2/source_object_view.hpp"
#include "halo2/source_scene_effect_owner.hpp"
#include <memory>
#include <functional>

namespace halo2 {
class CollisionWorld;
class SceneObjects;
struct ObjectLightingBinding;
struct ModelObjectDraw;
struct ObjectLightingState;
struct SourceLiveModelBaseDrawTu15;
struct SourcePropDefinition {
    std::string name;
    std::uint16_t object_flags{};
    std::uint32_t model=0xffffffffu,render_model=0xffffffffu,animation_graph=0xffffffffu;
    std::uint32_t physics_model=0xffffffffu,physics_bodies{},initial_prop_flags{};
    bool constructor_admitted{}; // 11BBF0 requires a nonempty phmo body bank.
};
struct SourcePropSceneStats {
    //Dynamic/PRT counts cover prepared authored model shader entries;
    //resident counts cover only actually selected geometry's material slots.
    unsigned placements{},render_models{},dynamic_materials{},prt_materials{},section_banks{};
    unsigned resident_dynamic_materials{},resident_prt_materials{}; //LegacyLOD5.
    unsigned union_dynamic_materials{},union_prt_materials{},union_section_banks{};
};
struct SourcePropLightingSphere {Vec3 center;float radius{};};
struct SourcePropBodyBounds {Vec3 minimum,maximum;float margin{};};
struct SourcePropQueryState {
    SourcePropLightingSphere sphere;
    Vec3 velocity;
    std::uint32_t runtime_flags{};
    bool frozen{}; // Actual object+C1bit0, supplied by its cache/physics producer.
};
//1D44F0 consumes the actual backend broadphase AABBs, expanded by the
//maximum authored body margin for each original physics group. Shape bounds
//are not a substitute for the backend's quantized bounds.
std::optional<SourcePropLightingSphere> source_prop_physics_sphere(
    std::span<const SourcePropBodyBounds>);
// Original type11 BLOC placements. Resident material/section slots are built
// before Renderer allocation; current root/lifetime state comes from the caller.
// Physics, contact ownership and later animation controllers are separate services.
class SourcePropScene {
    struct PreparedData;
public:
    using LightingQueryState=std::function<SourcePropQueryState(std::size_t source_object,
        std::uint32_t full_handle,const Transform &actual_world_node0,float object_scale)>;
    SourcePropScene(const Cache &,std::span<const ScenarioObject>,std::size_t bsp,
                    Mesh &,const CollisionWorld &,bool source_region_union = false);
    ~SourcePropScene();
    SourcePropScene(SourcePropScene &&) noexcept;
    SourcePropScene &operator=(SourcePropScene &&) noexcept;
    SourcePropScene(const SourcePropScene &)=delete;
    SourcePropScene &operator=(const SourcePropScene &)=delete;
    // Exact completed local presentation state, including salted lifetimes,
    // posed/selected geometry, light/query caches and retained source views.
    // Resident Cache/Mesh/collision and callback owners remain here. The
    // geometry value currently also copies its immutable CPU catalog; no
    // material slot, native owner, resource or pose is reconstructed.
    class Checkpoint {
    public:
        Checkpoint()=default;
        bool valid()const noexcept;
    private:
        friend class SourcePropScene;
        struct Data;
        std::shared_ptr<const Data> data_;
    };
    class PreparedRestore {
    public:
        ~PreparedRestore();
        PreparedRestore(PreparedRestore &&)noexcept;
        PreparedRestore &operator=(PreparedRestore &&)noexcept;
    private:
        friend class SourcePropScene;
        PreparedRestore();
        std::unique_ptr<PreparedData> data_;
    };
    Checkpoint checkpoint()const;
    // Later frames do not expire a checkpoint. Replacing any callback binding
    // does: the captured values must not be paired with a different producer.
    // PRT links pin their actual resident pass. Pass replacement/COW makes that
    // older cut ineligible; a fresh capture follows the new pass. Restoring a
    // complete material graph remains the enclosing material owner's task.
    bool owns_checkpoint(const Checkpoint &)const noexcept;
    PreparedRestore prepare_restore(const Checkpoint &)const;
    bool restore_ready(const PreparedRestore &)const noexcept;
    // Same executor, quiescent scene/callbacks and resident Mesh. Only swaps;
    // the caller retains the consumed token after the enclosing publication
    // lock. Previously exported ModelObjectDraw spans borrow scene storage:
    // retire them before publication, or keep the token until they are gone.
    bool commit_restore(PreparedRestore &)noexcept;
    void synchronize(const SceneObjects &);
    // Completed-frame world adoption: discard handle/pose/light/view history,
    // retaining the prepared geometry/material union. Re-synchronize and pose
    // from the actual imported world before drawing, even for identical IDs.
    void reset_lifetimes() noexcept;
    // Actual selected per-view geometry after source pose/LOD selection.
    void observe_draws(std::function<void(unsigned,std::uint32_t,std::uint32_t,std::span<const Vertex>)>);
    // Observe completed authority/client region choices without re-running RNG.
    void region_state(std::function<const SourceInitialModelSelection *(std::uint32_t)>);
    void interpolate(const SceneObjects &,const class SceneInterpolation &,float fraction);
    //BDEF0 may consume a cached regular sphere or a successful1D44F0
    //physics override. The source physics/cache producer owns that decision,
    //velocity and genericobject flags. No callback means unresolved inputs.
    void lighting_query_state(LightingQueryState);
    void update(std::int32_t render_frame,Vec3 first_submitting_eye,
                float source_lighting_distance_scale,Mesh &);
    //Production pose/banks only; append_view evaluates the original cache
    //cadence for each admitted view. update retains diagnostic behavior.
    void update_pose(std::int32_t render_frame,Mesh &);
    std::vector<SourceSceneEffectOwner> effect_owners() const;
    void append(std::vector<Vertex> &,std::vector<ObjectLightingBinding> &) const;
    //Uses each source player's retained LOD byte and explicit camera/flag
    //inputs. Empty exact collectors emit no geometry or lighting bindings.
    void append_view(std::vector<Vertex> &,std::vector<ObjectLightingBinding> &,
                     const SourceObjectViewLod &,std::vector<SourceObjectViewSelection> * = nullptr);
    // Actual model roots prepared before renderer allocation. The native
    // source view must retain a complete eligible owner before light admission.
    void observe_source_draws(std::function<void(unsigned,std::uint32_t,std::uint32_t,
        const ObjectLightingState&,bool,unsigned,std::span<const Vertex>)>);
    std::vector<std::uint32_t> source_render_models() const;
    void begin_source_frame();
    bool retain_source_view(unsigned,std::span<const SourceLiveModelBaseDrawTu15>,
                            std::span<const Transform>,const SourceModelShaderView&);
    // Submit the retained base geometry once. append_view then skips only
    // these completed owners; forgotten or duplicate submission is rejected.
    void append_source_models(std::vector<ModelObjectDraw>&,const SourceObjectViewLod&);
    const SourcePropSceneStats &stats() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
