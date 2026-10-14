#pragma once
#include "halo2/effect_light_draw.hpp"
#include "halo2/world_effects.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/source_live_model_draws_tu15.hpp"

namespace halo2 {
struct SourceEffectLightModelViewTu15 {
    PhysicsLiveWorldTu15& world;
    SourceObjectAdmission main;
    SourceLiveModelDrawContextTu15 context;
    std::span<const std::uint8_t> containing_fog;
    // Consume the completed main prefix before a light query can mutate its
    // shared contexts/arena. The caller copies all retained draw inputs here.
    std::function<void(const SourceDrawUnitState&)> main_draws;
    // Native renderer availability, after genuine source collection. Rejected
    // model units keep their original ordinals but submit no light pass.
    std::function<bool(const SourceCullDrawUnit&)> model_available;
};
// Shared BSP/model part of31C80. Immutable cache resources are loaded before
// renderer creation; tick state, selection and receiver queries come from the
// live NativeWorldEffects owner. Optional model views supply the actual live
// object world and main-view queue; both families use one command/range sort.
class SourceBspEffectLights {
public:
    SourceBspEffectLights(const Cache&,Mesh&,std::size_t bsp,
        std::span<const std::uint32_t> light_definitions,
        std::span<const std::uint32_t> render_models={});
    ~SourceBspEffectLights();
    SourceBspEffectLights(const SourceBspEffectLights&)=delete;
    SourceBspEffectLights& operator=(const SourceBspEffectLights&)=delete;
    std::vector<EffectLightRawDraw> draw(const SourceEffectLightView&,
        std::span<const WorldEffectLightReceiver>,const SourceBspAdmission& main,
        const SourceViewProjectionBank&,const BspFogSample&,
        Vec3 eye,std::array<std::int16_t,4> viewport,
        std::array<std::uint32_t,2> surface,unsigned local_players,
        SourceEffectLightModelViewTu15* models=nullptr);
    const std::vector<std::string>& unavailable() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
