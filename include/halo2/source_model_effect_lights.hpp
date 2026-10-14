#pragma once
#include "halo2/effect_light_draw.hpp"
#include "halo2/effect_light.hpp"

namespace halo2 {
struct SourceModelEffectLightDrawInput {
    std::uint32_t object=0xffffffffu,light=0xffffffffu,shader=0xffffffffu;
    unsigned part{},shader_lod{},pass{},vertex_class{};
    MenuWorldSourceTopology topology{};
    std::optional<MenuWorldSourceIndexRange> range;
    std::span<const std::array<float,12>> palette;
    EffectLightUniforms uniforms;
    MenuWorldSourceConstants constants{};
    MenuWorldRasterPacket raster;
    std::array<float,4> clip_range{};
    std::array<float,2> fog_parameters{};
};
// Resident layer12/implementation1 binding for an actual selected model
// section. Preload before Renderer creation. Collector, receiver admission,
// shared sorting, selected ranges and current palettes remain caller owners.
// Local classes consume unchanged bone-index/weight streams and actual c50
// palettes. This consumer does not create live skinned receiver ownership.
class SourceModelEffectLights {
public:
    SourceModelEffectLights(const Cache&,Mesh&,std::uint32_t render_model,unsigned section,
        std::span<const std::uint32_t> lights);
    ~SourceModelEffectLights();
    SourceModelEffectLights(const SourceModelEffectLights&)=delete;
    SourceModelEffectLights& operator=(const SourceModelEffectLights&)=delete;
    EffectLightRawDraw draw(const SourceModelEffectLightDrawInput&)const;
    const std::vector<std::string>& unavailable()const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
