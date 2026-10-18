#pragma once
#include "halo2/world_effects.hpp"
namespace halo2 {
class PhysicsLiveWorldTu15;
namespace pstencil {
// Presentation over the completed native light/spatial banks. Includes effect
// lights and direct attachments without creating a second simulation owner.
class CampaignLights {
public:
    CampaignLights(const Cache&,unsigned bsp);
    ~CampaignLights();
    std::vector<std::uint32_t> definitions()const;
    const std::vector<std::string>& unavailable()const;
    SourceEffectLightView view(PhysicsLiveWorldTu15&,Vec3 eye,
        SourceEffectLightCollectionContext,SourceEffectLightCandidateContext,
        std::int32_t frame,bool cinematic);
    SourceEffectLightView view(PhysicsLiveWorldTu15&,Vec3 eye,
        SourceEffectLightCollectionContext,SourceEffectLightCandidateContext,
        std::int32_t frame,bool cinematic,bool defer_native_selection);
    // After the actual main collector's C35D0 update. Preserves the collected
    // PVS order and completes original native shape/owner/feature admission,
    // then the separate source candidate coverage/history owners.
    void select_native(PhysicsLiveWorldTu15&,SourceEffectLightView&,
        const SourceEffectLightCandidateContext&,std::int32_t local_player,
        std::int32_t frame,bool cinematic);
    std::vector<WorldEffectLightReceiver> receivers(const SourceEffectLightView&,
        const SourceBspAdmissionBsp&,std::span<const std::uint8_t> containing_fog,
        const SourceBspAdmission* main);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
}
