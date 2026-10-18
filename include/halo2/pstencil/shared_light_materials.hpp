#pragma once
#include "halo2/effect_light_draw.hpp"
#include "halo2/effect_light.hpp"

namespace halo2::pstencil {
// One immutable shader/light residency bank for a scene's model sections.
// Geometry, palette, source owner, selected ranges and completed uniforms are
// deliberately absent: those remain independent per native draw.
class SharedLightMaterials {
public:
    struct Entry {
        unsigned slot{};
        std::shared_ptr<const Material> authored;
        MenuWorldSourcePass pass;
    };
    SharedLightMaterials(const Cache&,Mesh&);
    ~SharedLightMaterials();
    const Material& authored(std::uint32_t shader);
    std::shared_ptr<const Material> selected(std::uint32_t shader,unsigned lod);
    const std::vector<Entry>& preload(std::uint32_t shader,unsigned lod,
        std::uint32_t light,const EffectLightDefinition&);
    void preload_specular(std::uint32_t shader,unsigned lod,
        std::uint32_t light,const EffectLightDefinition&);
    std::shared_ptr<const std::vector<EffectLightDraw>> specular(
        std::uint32_t shader,unsigned lod,std::uint32_t light,const EffectLightUniforms&)const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
