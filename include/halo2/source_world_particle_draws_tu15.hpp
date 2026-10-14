#pragma once
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/muzzle_effects.hpp"
#include <memory>

namespace halo2 {
// Native graphics consumer for placed effect systems whose swarm controller
// requires the completed original source particle simulation.
class SourceWorldParticleDrawsTu15 {
public:
    SourceWorldParticleDrawsTu15(const Cache &,std::span<const ScenarioObject>);
    ~SourceWorldParticleDrawsTu15();
    bool active() const;
    void advance(PhysicsLiveWorldTu15 &,float frame_seconds);
    std::vector<ParticleDraw> draw(PhysicsLiveWorldTu15 &,
        const SourcePortalVisibilitySet &,const MuzzleEffectView &,
        unsigned local_view,float projection_angle,float lighting_scale);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
