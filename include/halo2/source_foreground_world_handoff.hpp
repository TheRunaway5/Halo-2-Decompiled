#pragma once
#include "halo2/effect_presentation_history.hpp"
#include "halo2/source_native_publication.hpp"
#include "halo2/source_world_particle_draws_tu15.hpp"
#include <memory>
#include <thread>

namespace halo2 {
class SourceBspEffectLights;
class SourcePeerHostTakeover;
class MultiplayerPreparedWorld;
class MultiplayerObjectRuntime;
// Local producer routing/collision subtransaction only. This does not restore
// receipt/effect/lens/input/HUD/playing-audio owners, enable native readiness,
// authenticate a candidate or activate a world. The enclosing native owner
// independently validates its learned decision and complete prepared boundary.
class SourceForegroundWorldHandoff {
    struct Data;
public:
    struct Policy {bool particles{},models{};}; // startup renderer feature policy
    SourceForegroundWorldHandoff(std::shared_ptr<const Cache>,const Mesh &,
        const SourceWorldParticleDrawsTu15 &,const SourceBspEffectLights &,
        std::size_t uploaded_materials,Policy,
        MultiplayerObjectRuntime *&particle_world,MultiplayerObjectRuntime *&model_world,
        ParticleCollisionQuery muzzle_collision={},ParticleCollisionQuery world_collision={});
    ~SourceForegroundWorldHandoff();
    SourceForegroundWorldHandoff(const SourceForegroundWorldHandoff &)=delete;
    SourceForegroundWorldHandoff &operator=(const SourceForegroundWorldHandoff &)=delete;
    class Prepared {
    public:
        ~Prepared();
        Prepared(Prepared &&)noexcept;
        Prepared &operator=(Prepared &&)noexcept;
        const ParticleCollisionQuery &muzzle_collision()const;
        const ParticleCollisionQuery &world_collision()const;
    private:
        friend class SourceForegroundWorldHandoff;
        Prepared();std::unique_ptr<Data> data_;
    };
    // Copy these prebuilt queries into the separately prepared CPU effect
    // restore. No callback is invoked during prepare/ready/commit.
    Prepared prepare_host(SourcePeerHostTakeover &,
        const EffectPresentationHistory::RuntimeCheckpoint &)const;
    Prepared prepare_client(MultiplayerPreparedWorld &,
        const EffectPresentationHistory::RuntimeCheckpoint &)const;
    // Bind once to the same actual native target scope that sealed this
    // preparation's exact cut and derived execution boundary. Repeating that
    // scope is idempotent; another scope cannot revive the prepared owner.
    // This records weak eligibility only, never native or complete readiness.
    bool bind_publication(Prepared &,const SourceNativePublication::Boundary &)const noexcept;
    bool ready(const Prepared &)const noexcept;
    // Caller retains/quiesces this owner, both worlds, resident Cache/Mesh and
    // resource banks through commit. Target world must then outlive all copied
    // queries; root runtime authority owns it. No native memory/model view is
    // borrowed across calls. Bound query getters refuse an expired scope;
    // queries already copied out must still be retired by their actual outer
    // owner before target teardown. Do not invoke these queries while native
    // publication is held. Commit swaps pointers/query ownership only; token
    // retains old queries until destruction outside the session lock.
    bool commit(Prepared &)noexcept;
private:
    friend class MultiplayerRuntimeAuthority;
    Prepared prepare(MultiplayerPreparedWorld &,
        const EffectPresentationHistory::RuntimeCheckpoint &,bool native)const;
    void own()const;
    bool resources_unchanged()const noexcept;
    std::shared_ptr<const Cache> cache_;
    const Mesh &mesh_;
    const SourceWorldParticleDrawsTu15 &particles_;
    const SourceBspEffectLights &models_;
    std::size_t materials_,model_sections_;
    SourceWorldParticleDrawsTu15::ResourceState particle_resources_;
    Policy policy_;
    MultiplayerObjectRuntime *&particle_slot_,*&model_slot_;
    ParticleCollisionQuery muzzle_collision_,world_collision_;
    const std::thread::id executor_=std::this_thread::get_id();
    const std::shared_ptr<const int> identity_=std::make_shared<const int>(0);
    std::uint64_t revision_{};
};
}
