#pragma once
#include "halo2/effect_light_draw.hpp"
#include "halo2/effect_light.hpp"
#include <map>
#include <tuple>

namespace halo2::pstencil {
// The 2003 dependent-light stage includes specular, and its environment
// stage retains separate facing/grazing reflection inputs. Current assets
// supply their authored shader-template bindings; no shininess is invented.
std::optional<Material> bind_light_pass(const Cache&,const Material&,unsigned,
    const EffectLightUniforms&);
class MaterialBank {
public:
    MaterialBank(const Cache& cache,Mesh& mesh):cache_(cache),mesh_(mesh){}
    void preload(const Material&,std::uint32_t light,const EffectLightDefinition&);
    std::shared_ptr<const std::vector<EffectLightDraw>> complete(
        std::uint32_t shader,unsigned lod,std::uint32_t light,const EffectLightUniforms&) const;
private:
    struct Pass {unsigned slot,index;Material authored;};
    const Cache& cache_;Mesh& mesh_;
    // Specular texture/pass residency is independent of a sphere light's
    // identity. Only its completed uniforms vary per draw.
    std::map<std::pair<std::uint32_t,unsigned>,std::vector<Pass>> passes_;
};
}
