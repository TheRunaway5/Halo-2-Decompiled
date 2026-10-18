#include "halo2/pstencil/shared_light_materials.hpp"
#include "halo2/pstencil/material.hpp"
#include "halo2/world_material.hpp"
#include <map>
#include <tuple>

namespace halo2::pstencil {
struct SharedLightMaterials::Impl {
    const Cache& cache;Mesh& mesh;MaterialBank specular;
    std::map<std::uint32_t,Material> shaders;
    std::map<std::pair<std::uint32_t,unsigned>,std::shared_ptr<const Material>> lods;
    std::map<std::tuple<std::uint32_t,unsigned,std::uint32_t>,std::vector<Entry>> entries;
    Impl(const Cache& c,Mesh& m):cache(c),mesh(m),specular(c,m){}
    const Material& authored(std::uint32_t shader) {
        const auto found=shaders.find(shader);
        if(found!=shaders.end())return found->second;
        return shaders.emplace(shader,cache.load_material(shader)).first->second;
    }
    std::shared_ptr<const Material> selected(std::uint32_t shader,unsigned lod) {
        const auto found=lods.find({shader,lod});if(found!=lods.end())return found->second;
        auto value=std::make_shared<Material>(authored(shader));
        if(lod>=value->lods.size())throw FormatError("shared light material lost its authored shader LOD");
        value->selected_shader_lod=lod;
        return lods.emplace(std::pair{shader,lod},std::move(value)).first->second;
    }
};
SharedLightMaterials::SharedLightMaterials(const Cache& c,Mesh& m):impl_(std::make_unique<Impl>(c,m)){}
SharedLightMaterials::~SharedLightMaterials()=default;
const Material& SharedLightMaterials::authored(std::uint32_t shader){return impl_->authored(shader);}
std::shared_ptr<const Material> SharedLightMaterials::selected(std::uint32_t shader,unsigned lod){return impl_->selected(shader,lod);}
const std::vector<SharedLightMaterials::Entry>& SharedLightMaterials::preload(
    std::uint32_t shader,unsigned lod,std::uint32_t light,const EffectLightDefinition& definition) {
    auto& i=*impl_;const auto key=std::tuple{shader,lod,light};
    const auto found=i.entries.find(key);if(found!=i.entries.end())return found->second;
    const auto selected=i.selected(shader,lod);
    EffectLightState state;state.size=state.brightness=1;
    const auto uniforms=source_effect_light_view_uniforms(definition,state,{},
        {1,0,0},{0,0,1},{1,1,1,1,1},1,{});
    auto prepared=prepare_effect_light_diffuse_material(i.cache,*selected,definition,uniforms);
    std::vector<Entry> entries;entries.reserve(prepared.size());
    for(unsigned pass=0;pass<prepared.size();++pass)
        entries.push_back({unsigned(i.mesh.materials.size())+pass,selected,
            load_menu_world_source_pass(i.cache,shader,{lod,12,pass,1})});
    for(auto& material:prepared)i.mesh.materials.push_back(std::move(material));
    return i.entries.emplace(key,std::move(entries)).first->second;
}
void SharedLightMaterials::preload_specular(std::uint32_t shader,unsigned lod,
    std::uint32_t light,const EffectLightDefinition& definition) {
    auto& i=*impl_;i.specular.preload(*i.selected(shader,lod),light,definition);
}
std::shared_ptr<const std::vector<EffectLightDraw>> SharedLightMaterials::specular(
    std::uint32_t shader,unsigned lod,std::uint32_t light,const EffectLightUniforms& uniforms)const {
    return impl_->specular.complete(shader,lod,light,uniforms);
}
}
