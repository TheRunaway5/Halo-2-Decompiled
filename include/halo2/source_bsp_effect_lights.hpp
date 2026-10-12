#pragma once
#include "halo2/effect_light_draw.hpp"
#include "halo2/world_effects.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/bsp_atmosphere.hpp"

namespace halo2 {
// BSP-owned part of31C80. Immutable cache resources are loaded before renderer
// creation; tick state, selection and receiver queries come from the live
// NativeWorldEffects owner. Dynamic objects and their collector are separate.
class SourceBspEffectLights {
public:
    SourceBspEffectLights(const Cache&,Mesh&,std::size_t bsp,
        std::span<const std::uint32_t> light_definitions);
    ~SourceBspEffectLights();
    SourceBspEffectLights(const SourceBspEffectLights&)=delete;
    SourceBspEffectLights& operator=(const SourceBspEffectLights&)=delete;
    std::vector<EffectLightRawDraw> draw(const SourceEffectLightView&,
        std::span<const WorldEffectLightReceiver>,const SourceBspAdmission& main,
        const SourceViewProjectionBank&,const BspFogSample&,
        Vec3 eye,std::array<std::int16_t,4> viewport,
        std::array<std::uint32_t,2> surface,unsigned local_players);
    const std::vector<std::string>& unavailable() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
