#pragma once
#include "halo2/effect_light_draw.hpp"
#include "halo2/effect_light.hpp"
#include <map>
#include <tuple>

namespace halo2::pstencil {
// The 2003 dependent-light stage includes specular, and its environment
// stage retains separate facing/grazing reflection inputs. Current assets
// supply their authored shader-template bindings. Later-asset art adaptations
// are explicitly scoped in adaptation.hpp.
std::optional<Material> bind_light_pass(const Cache&,const Material&,unsigned,
    const EffectLightUniforms&);
std::optional<Material> complete_light_pass(const Cache&,const Material&,unsigned,
    const EffectLightUniforms&);
std::optional<Material> bind_texture_mapping_pass(const Cache&,const Material&);
bool supports_projected_light_response_2003(const EffectLightDefinition&);
class MaterialBank {
public:
    MaterialBank(const Cache&,Mesh&);
    ~MaterialBank();
    MaterialBank(const MaterialBank&)=delete;
    MaterialBank& operator=(const MaterialBank&)=delete;
    void preload(const Material&,std::uint32_t light,const EffectLightDefinition&);
    std::shared_ptr<const std::vector<EffectLightDraw>> complete(
        std::uint32_t shader,unsigned lod,std::uint32_t light,const EffectLightUniforms&) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
