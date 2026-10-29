#pragma once
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/muzzle_effects.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace halo2 {
// Native graphics consumer for retained effect systems simulated by the
// completed original source particle phase.
class SourceWorldParticleDrawsTu15 {
public:
    using MaterialValidator=std::function<void(std::shared_ptr<const ParticleMaterial>)>;
    // Validators run on the resource-loading thread. They must not call a
    // renderer that may concurrently own the previous frame's GPU work.
    SourceWorldParticleDrawsTu15(const Cache &,std::span<const ScenarioObject>,MaterialValidator = {});
    ~SourceWorldParticleDrawsTu15();
    bool active() const;
    struct ResourceState {
        std::size_t materials{},world_systems{},first_person_systems{};
    };
    // Observe authored resident resources without touching a native world.
    ResourceState resource_state()const noexcept;
    const Cache &resident_cache()const noexcept;
    // Retain resources for actual source-created FP systems. Birth, movement,
    // event clocks and RNG remain in the original world particle phase.
    bool preload_first_person(std::uint32_t effect);
    bool first_person_effect(std::uint32_t effect) const;
    bool preload_world(std::uint32_t effect);
    bool world_effect(std::uint32_t effect) const;
    // Strict preload methods above publish a whole pass atomically or throw.
    // These content-safe variants quarantine only ParticleResourceError;
    // allocation, device and native-world invariant failures still propagate.
    bool try_preload_first_person(std::uint32_t effect);
    bool try_preload_world(std::uint32_t effect);
    enum class ResourcePhase {catalog,world,first_person};
    struct ResourceFailure {
        std::uint32_t definition{};
        ResourcePhase phase{};
        std::string reason;
    };
    struct CatalogAdmission {std::size_t requested{},admitted{},rejected{};};
    // Prepares visual resources only: no native effect/system admission,
    // commands, particles, clocks or RNG are produced by this census.
    CatalogAdmission preload_catalog();
    std::vector<std::shared_ptr<const ParticleMaterial>> resident_materials()const;
    // Diagnostics are bounded to64 records of at most512 bytes. Full-handle
    // failure caching prevents the same rejected content retrying each frame.
    std::span<const ResourceFailure> resource_failures()const noexcept;
    std::size_t resource_failure_count()const noexcept;
    // Admit resources named by occupied original effect/system records. This
    // reads actual full-generation pools; it creates no effect or particle,
    // consumes no source RNG and does not advance any source frame.
    std::vector<std::uint32_t> preload_live_world(const SourceRuntimeMemory &);
    // Same full-salt native effect/system census, but no loading or resource
    // registration. Rejects an active authored system absent from the actual
    // resident pass/material bank unless its content is already quarantined.
    // Native pool identities and event/system indices remain strict even for
    // quarantined visuals. Does not run native code or consume RNG.
    std::vector<std::uint32_t> require_live_world_resident(const SourceRuntimeMemory &)const;
    // Publish the real native observer packet and run complete174BF0. Its
    // unscaled FOV ratio at observer+50 drives251C80's WORLD emission LOD;
    // the split render camera's lighting scale is a different value.
    void publish_observer(PhysicsLiveWorldTu15 &,unsigned local_view,
        Vec3 position,Vec3 forward,Vec3 up,float horizontal_angle);
    // Queue construction is separate from the shared once-per-frame update.
    void prepare_storage(PhysicsLiveWorldTu15 &);
    void advance(PhysicsLiveWorldTu15 &,float frame_seconds);
    // Actual camera/frustum/collector inputs before ordinary main collection.
    // Does not run40E80, request effects or produce particle commands.
    void prepare_view(PhysicsLiveWorldTu15 &,const SourcePortalVisibilitySet &,
        const MuzzleEffectView &,unsigned local_view,float projection_angle,float lighting_scale,
        std::optional<std::int32_t> actual_local_user = {});
    std::vector<ParticleDraw> draw(PhysicsLiveWorldTu15 &,
        const SourcePortalVisibilitySet &,const MuzzleEffectView &,
        unsigned local_view,float projection_angle,float lighting_scale,
        bool lens_gates_prepared=false,bool main_commands_prepared=false);
    std::vector<ParticleDraw> draw_first_person(PhysicsLiveWorldTu15 &,
        const SourcePortalVisibilitySet &,const MuzzleEffectView &,
        unsigned local_view,float projection_angle,float lighting_scale);
private:
    std::vector<ParticleDraw> draw_pass(PhysicsLiveWorldTu15 &,
        const SourcePortalVisibilitySet &,const MuzzleEffectView &,
        unsigned,float,float,bool first_person,bool lens_gates_prepared,bool main_commands_prepared);
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
