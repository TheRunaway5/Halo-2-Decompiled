#include "halo2/source_campaign_model.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/content.hpp"
#include "halo2/appearance.hpp"
#include "halo2/world_material.hpp"
#include "halo2/object_lighting.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
SourceCampaignModel::SourceCampaignModel(const Cache &cache,const ScenarioObject &object)
    :definition_(object.definition),variant_(object.variant) {
    const auto initial=cache.load_initial_model_definition(definition_);
    render_model_=initial.render_model;
    if(render_model_==null_datum)throw FormatError("Campaign source model has no render-model definition");
    const auto palettes=cache.load_source_model_palette_definition(render_model_);
    for(unsigned lod=0;lod<6;++lod) {
        auto &sections=sections_[lod];
        // Resource union only: include every authored permutation, without
        // treating an unresolved random preview as a live selection.
        for(const auto &region:initial.render_sections)for(const auto &permutation:region) {
            const auto section=permutation[lod];if(section<0)continue;
            if(section>=255)throw FormatError("Campaign source section exceeds its original byte bank");
            if(std::ranges::find(sections,unsigned(section))==sections.end())sections.push_back(unsigned(section));
            if(std::ranges::find(union_sections_,unsigned(section))==union_sections_.end())union_sections_.push_back(unsigned(section));
        }
        // Explicit section decoding depends on LOD only through the compound
        // palette. Retain one immutable bank when both inputs are identical.
        // Distinct palettes and section sets keep distinct resource banks.
        for(unsigned prior=0;prior<lod;++prior) {
            if(sections_[prior]==sections && std::ranges::equal(
                std::as_bytes(std::span(palettes.compounds[prior])),
                std::as_bytes(std::span(palettes.compounds[lod])))) {
                lods_[lod]=lods_[prior];break;
            }
        }
        if(!lods_[lod])lods_[lod]=std::make_shared<RenderModel>(cache.load_model_sections(definition_,false,variant_,lod,sections));
        const auto &model=*lods_[lod];
        if(model.vertex_origins.size()!=model.mesh.vertices.size())throw FormatError("Campaign resource union lost its source vertex identities");
        if(lod && (model.bones.size()!=lods_[0]->bones.size() || model.checksum!=lods_[0]->checksum))
            throw FormatError("Campaign source LOD changed its model hierarchy");
    }
}
void SourceCampaignModel::prepare_materials(const Cache &cache,Mesh &destination) {
    if(shaders_)throw FormatError("Campaign source material bank is already prepared");
    // Load the shared authored material table without decoding other LODs
    // against LOD5's compound palette. Shader preparation uses the streams
    // already decoded under each section's actual LOD inputs above.
    auto resources=cache.load_model_sections(definition_,true,variant_,5,{});
    std::vector<unsigned> prepared;
    for(unsigned lod:{5u,0u,1u,2u,3u,4u})for(auto section:sections_[lod]) {
        if(std::ranges::find(prepared,section)!=prepared.end())continue;
        prepared.push_back(section);const auto &model=*lods_[lod];
        for(unsigned i=0;i<model.mesh.vertices.size();++i)if(model.mesh.vertices[i].source_section==section) {
            resources.mesh.vertices.push_back(model.mesh.vertices[i]);resources.vertex_origins.push_back(model.vertex_origins[i]);
        }
    }
    for(auto &material:resources.mesh.materials) {
        const auto colors=material.object_render_colors;
        if(!material.lods.empty() && !prepare_dynamic_material(cache,material,{},true,0.f))
            throw FormatError("Campaign cinematic model shader is unresolved: "+material.name);
        if(colors)apply_object_render_colors(material,*colors);
    }
    auto shaders=std::make_unique<SourceDynamicModelShaders>(cache,resources,render_model_,2,0.f);
    if(destination.materials.size()+resources.mesh.materials.size()>0xffffffffull)
        throw FormatError("Campaign source material namespace exhausted");
    material_base_=unsigned(destination.materials.size());
    for(auto &material:resources.mesh.materials)destination.materials.push_back(std::move(material));
    shaders_=std::move(shaders);
}
void SourceCampaignModel::rebase_materials(unsigned before,unsigned after) {
    if(!shaders_ || material_base_<before || std::uint64_t(material_base_-before)+after>0xffffffffull)
        throw FormatError("Campaign source material rebase lost its resident namespace");
    material_base_=material_base_-before+after;
}
std::vector<Vertex> SourceCampaignModel::vertices(std::uint32_t owner,
    const SourceObjectDrawCollector &collector,std::span<const Transform> nodes)const {
    const Bytes c(collector.bytes);
    if(c.u32(0)!=owner)throw FormatError("Campaign source collector lost its full owner");
    const auto count=std::bit_cast<std::int16_t>(c.u16(0x166));
    if(count<0 || count>5)throw FormatError("Campaign source collector exceeds its five model rows");
    if(!count)return {};
    const auto lod=c.u16(0x168);
    if(lod>=6)throw FormatError("Campaign source collector LOD exceeds its six resource banks");
    if(nodes.size()!=lods_[lod]->bones.size())throw FormatError("Campaign model requires its complete rendered WORLD pose");
    // Original submission skins only the selected sections. Keep the full
    // palette, but do not pose every unselected permutation in the resource
    // union. Ordered duplicate section requests remain duplicate vertices.
    const auto &resource=*lods_[lod];RenderModel selected;
    selected.bones=resource.bones;selected.compound_nodes=resource.compound_nodes;
    bool found=false;
    for(unsigned row=0;row<unsigned(count);++row) {
        if(c.u32(0x34+row*4)!=owner)
            throw FormatError("Campaign collector requires another live model owner");
        if(c.u32(0x5c+row*4)!=render_model_)
            throw FormatError("Campaign collector requires another resident render-model owner");
        found=true;
        const auto regions=c.u32(0x18+row*4);
        if(regions>16)throw FormatError("Campaign source collector exceeds its sixteen regions");
        for(unsigned region=0;region<regions;++region) {
            const auto section=std::to_integer<unsigned>(c.slice(0x75+row*16+region,1).front());
            if(section==255)continue;
            if(std::ranges::find(sections_[lod],section)==sections_[lod].end())
                throw FormatError("Campaign source selected an unprepared section");
            for(unsigned i=0;i<resource.mesh.vertices.size();++i)if(resource.mesh.vertices[i].source_section==section) {
                selected.mesh.vertices.push_back(resource.mesh.vertices[i]);selected.skin.push_back(resource.skin.at(i));
            }
        }
    }
    if(!found)throw FormatError("Campaign source collector did not retain the requested model owner");
    return skin_model_world_nodes(selected,nodes);
}
std::vector<Vertex> SourceCampaignModel::draw(std::uint32_t owner,
    const SourceObjectDrawCollector &collector,std::span<const Transform> nodes,
    const SourceModelShaderCollector &shader,const SourceModelShaderView &view)const {
    if(!shaders_)throw FormatError("Campaign source model has no resident shader bank");
    auto result=vertices(owner,collector,nodes);shaders_->select(result,shader,view);
    for(auto &vertex:result)vertex.material+=material_base_;
    return result;
}
}
