#pragma once
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/muzzle_effects.hpp"
#include <memory>

namespace halo2 {
// Native graphics consumer for retained effect systems simulated by the
// completed original source particle phase.
class SourceWorldParticleDrawsTu15 {
public:
    SourceWorldParticleDrawsTu15(const Cache &,std::span<const ScenarioObject>);
    ~SourceWorldParticleDrawsTu15();
    bool active() const;
    // Retain resources for actual source-created FP systems. Birth, movement,
    // event clocks and RNG remain in the original world particle phase.
    bool preload_first_person(std::uint32_t effect);
    bool first_person_effect(std::uint32_t effect) const;
    bool preload_world(std::uint32_t effect);
    bool world_effect(std::uint32_t effect) const;
    // Admit resources named by occupied original effect/system records. This
    // reads actual full-generation pools; it creates no effect or particle,
    // consumes no source RNG and does not advance any source frame.
    std::vector<std::uint32_t> preload_live_world(const SourceRuntimeMemory &);
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
        const MuzzleEffectView &,unsigned local_view,float projection_angle,float lighting_scale);
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
