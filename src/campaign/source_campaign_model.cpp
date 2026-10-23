#include "halo2/source_campaign_model.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/content.hpp"
#include "halo2/appearance.hpp"
#include "halo2/world_material.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/scenery.hpp"
#include <algorithm>
#include <bit>
#include <map>
#include <set>

namespace halo2 {
bool source_campaign_lit_scenery(const RenderModel &model,const ScenarioObject &object) {
    if(object.type!=6)return false;
    return std::ranges::any_of(model.mesh.materials,[](const auto &material) {
        return std::ranges::any_of(material.lods,[](const auto &lod) {
            return (lod.available_layers&(1u<<3))!=0;
        });
    });
}
SourceCampaignModel::SourceCampaignModel(const Cache &cache,const ScenarioObject &object)
    :SourceCampaignModel(cache,object.definition,object.variant) {}
SourceCampaignModel::SourceCampaignModel(const Cache &cache,std::uint32_t definition,std::uint32_t variant)
    :definition_(definition),variant_(variant) {
    const auto initial=cache.load_initial_model_definition(definition_);
    variant_names_.reserve(initial.variants.size());
    for(const auto &entry:initial.variants)variant_names_.push_back(entry.name);
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
    scale_resets_=cache.load_model_scale_resets(*lods_[5]);
}
void SourceCampaignModel::prepare_materials(const Cache &cache,Mesh &destination,bool per_vertex_scenery) {
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
    // Shader banks share immutable geometry. A missing optional baked shader
    // capability stays explicit if an original class1 unit later requires it.
    std::optional<RenderModel> baked;
    std::unique_ptr<SourceDynamicModelShaders> baked_shaders;
    if(per_vertex_scenery) {
        baked=resources;
        try {
            for(auto &material:baked->mesh.materials) {
                const auto colors=material.object_render_colors;
                if(!material.lods.empty() && !prepare_world_material(cache,material,true))
                    throw FormatError("Campaign per-vertex scenery shader is unresolved: "+material.name);
                if(colors)apply_object_render_colors(material,*colors);
            }
            baked_shaders=std::make_unique<SourceDynamicModelShaders>(cache,*baked,render_model_,1);
        } catch(const FormatError &e) {baked_failure_=e.what();baked.reset();}
    }
    for(auto &material:resources.mesh.materials) {
        const auto colors=material.object_render_colors;
        if(!material.lods.empty() && !prepare_dynamic_material(cache,material,{},true,0.f))
            throw FormatError("Campaign cinematic model shader is unresolved: "+material.name);
        if(colors)apply_object_render_colors(material,*colors);
    }
    auto shaders=std::make_unique<SourceDynamicModelShaders>(cache,resources,render_model_,2,0.f);
    if(destination.materials.size()+resources.mesh.materials.size()+
        (baked?baked->mesh.materials.size():0)>0xffffffffull)
        throw FormatError("Campaign source material namespace exhausted");
    material_base_=unsigned(destination.materials.size());
    for(auto &material:resources.mesh.materials)destination.materials.push_back(std::move(material));
    shaders_=std::move(shaders);
    if(baked) {
        baked_material_base_=unsigned(destination.materials.size());
        for(auto &material:baked->mesh.materials)destination.materials.push_back(std::move(material));
        baked_shaders_=std::move(baked_shaders);
    }
}
void SourceCampaignModel::rebase_materials(unsigned before,unsigned after) {
    if(!shaders_ || material_base_<before || std::uint64_t(material_base_-before)+after>0xffffffffull)
        throw FormatError("Campaign source material rebase lost its resident namespace");
    material_base_=material_base_-before+after;
    if(baked_shaders_) {
        if(baked_material_base_<before || std::uint64_t(baked_material_base_-before)+after>0xffffffffull)
            throw FormatError("Campaign baked material rebase lost its resident namespace");
        baked_material_base_=baked_material_base_-before+after;
    }
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
    std::vector<unsigned> sections;
    for(unsigned row=0;row<unsigned(count);++row) {
        if(c.u32(0x34+row*4)!=owner)
            throw FormatError("Campaign collector requires another live model owner");
        if(c.u32(0x5c+row*4)!=render_model_)
            throw FormatError("Campaign collector requires another resident render-model owner");
        const auto regions=c.u32(0x18+row*4);
        if(regions>16)throw FormatError("Campaign source collector exceeds its sixteen regions");
        for(unsigned region=0;region<regions;++region) {
            const auto section=std::to_integer<unsigned>(c.slice(0x75+row*16+region,1).front());
            if(section==255)continue;
            sections.push_back(section);
        }
    }
    return vertices(lod,sections,nodes);
}
std::vector<Vertex> SourceCampaignModel::vertices(unsigned lod,std::span<const unsigned> sections,
    std::span<const Transform> nodes,const BspLighting *lighting,unsigned instance)const {
    if(lod>=6)throw FormatError("Campaign source selected a LOD outside its six resource banks");
    if(nodes.size()!=lods_[lod]->bones.size())throw FormatError("Campaign model requires its complete rendered WORLD pose");
    // Original submission skins only the selected sections. Keep the full
    // palette, but do not pose every unselected permutation in the resource
    // union. Ordered duplicate section requests remain duplicate vertices.
    const auto &resource=*lods_[lod];RenderModel selected;
    selected.bones=resource.bones;selected.compound_nodes=resource.compound_nodes;
    selected.checksum=resource.checksum;
    for(auto section:sections) {
        if(std::ranges::find(sections_[lod],section)==sections_[lod].end())
            throw FormatError("Campaign source selected an unprepared section");
        for(unsigned i=0;i<resource.mesh.vertices.size();++i)if(resource.mesh.vertices[i].source_section==section) {
            selected.mesh.vertices.push_back(resource.mesh.vertices[i]);selected.skin.push_back(resource.skin.at(i));
            selected.vertex_origins.push_back(resource.vertex_origins.at(i));
        }
    }
    if(lighting)apply_scenery_vertex_lighting(selected,*lighting,instance);
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
std::vector<Vertex> SourceCampaignModel::draw(std::uint32_t owner,
    std::span<const SourceLiveModelBaseDrawTu15> draws,std::span<const Transform> nodes,
    const SourceModelShaderView &view,const BspLighting *current_lighting)const {
    if(!shaders_)throw FormatError("Campaign source model has no resident shader bank");
    std::map<std::pair<unsigned,unsigned>,std::vector<Vertex>> posed;
    std::vector<Vertex> result;
    for(const auto &draw:draws) {
        if(draw.object!=owner || draw.definition!=definition_ || draw.unit.words[1]!=render_model_)
            throw FormatError("Campaign main unit lost its full owner/definition/render-model identity");
        if((draw.lighting_class!=1 && draw.lighting_class!=2) ||
            draw.lighting_class!=(draw.unit.words[0]>>29) || draw.override_slot!=255 || !(draw.unit.words[0]&1u))
            throw FormatError("Campaign ordinary draw received an unsupported class or override unit");
        if(draw.lod>=6 || draw.lod!=draws.front().lod)
            throw FormatError("Campaign main units lost their single selected geometry LOD");
        // Original449D0 returns before consuming a palette or emitting parts
        // when the current source section has no resident resource.
        if(!draw.geometry.geometry.resident)continue;
        const bool baked=draw.lighting_class==1;
        if(baked && (!baked_shaders_ || !current_lighting))
            throw FormatError("Campaign class1 unit has no resident baked lighting owner: "+baked_failure_);
        const auto instance=baked?(draw.unit.words[3]>>18&2047u):2047u;
        const auto section=draw.unit.words[3]>>9&511u;
        const auto key=std::pair{section,instance};
        auto at=posed.find(key);
        if(at==posed.end()) {
            const std::array sections{section};
            at=posed.emplace(key,vertices(draw.lod,sections,nodes,baked?current_lighting:nullptr,instance)).first;
        }
        if(at->second.empty())throw FormatError("Campaign main unit has no selected resident vertices");
        (baked?baked_shaders_:shaders_)->append_selected(result,at->second,{0,draw.coverage},view,
            baked?baked_material_base_:material_base_);
    }
    return result;
}
SourceCampaignDrawGeometry SourceCampaignModel::draw_deferred(std::uint32_t owner,
    std::span<const SourceLiveModelBaseDrawTu15> draws,std::span<const Transform> nodes,
    const SourceModelShaderView &view,const BspLighting *current_lighting,bool allow_gpu)const {
    if(!allow_gpu)return {draw(owner,draws,nodes,view,current_lighting),{}};
    if(!shaders_)throw FormatError("Campaign source model has no resident shader bank");
    using Key=std::pair<unsigned,unsigned>;
    // The original cache key has no lighting-class component. If a class1
    // instance shares a key with class2, preserve its existing CPU reuse too.
    std::set<Key> baked_keys;
    for(const auto &draw:draws)if(draw.lighting_class==1)
        baked_keys.emplace(draw.unit.words[3]>>9&511u,draw.unit.words[3]>>18&2047u);
    struct Pose {
        ModelSkinWorldPose pose;
        std::vector<std::uint32_t> ordinals;
    };
    std::map<Key,Pose> posed;
    SourceCampaignDrawGeometry result;
    for(const auto &draw:draws) {
        if(draw.object!=owner || draw.definition!=definition_ || draw.unit.words[1]!=render_model_)
            throw FormatError("Campaign main unit lost its full owner/definition/render-model identity");
        if((draw.lighting_class!=1 && draw.lighting_class!=2) ||
            draw.lighting_class!=(draw.unit.words[0]>>29) || draw.override_slot!=255 || !(draw.unit.words[0]&1u))
            throw FormatError("Campaign ordinary draw received an unsupported class or override unit");
        if(draw.lod>=6 || draw.lod!=draws.front().lod)
            throw FormatError("Campaign main units lost their single selected geometry LOD");
        if(!draw.geometry.geometry.resident)continue;
        const bool baked=draw.lighting_class==1;
        if(baked && (!baked_shaders_ || !current_lighting))
            throw FormatError("Campaign class1 unit has no resident baked lighting owner: "+baked_failure_);
        const auto instance=baked?(draw.unit.words[3]>>18&2047u):2047u;
        const auto section=draw.unit.words[3]>>9&511u;
        const Key key{section,instance};
        auto at=posed.find(key);
        if(at==posed.end()) {
            Pose prepared;
            const std::array sections{section};
            if(baked || baked_keys.contains(key)) {
                prepared.pose.vertices=vertices(draw.lod,sections,nodes,baked?current_lighting:nullptr,instance);
            } else {
                const auto &resource=*lods_[draw.lod];
                if(nodes.size()!=resource.bones.size())
                    throw FormatError("Campaign model requires its complete rendered WORLD pose");
                if(std::ranges::find(sections_[draw.lod],section)==sections_[draw.lod].end())
                    throw FormatError("Campaign source selected an unprepared section");
                // Match vertices(): construct only the selected section, then
                // run its original palette at the existing final-kernel point.
                RenderModel selected;
                selected.bones=resource.bones;selected.compound_nodes=resource.compound_nodes;
                selected.checksum=resource.checksum;
                for(unsigned i=0;i<resource.mesh.vertices.size();++i)if(resource.mesh.vertices[i].source_section==section) {
                    selected.mesh.vertices.push_back(resource.mesh.vertices[i]);selected.skin.push_back(resource.skin.at(i));
                    selected.vertex_origins.push_back(resource.vertex_origins.at(i));
                    prepared.ordinals.push_back(i);
                }
                prepared.pose=model_skin_world_pose(selected,nodes,true);
            }
            at=posed.emplace(key,std::move(prepared)).first;
        }
        const auto &prepared=at->second;
        if(prepared.pose.vertices.empty())throw FormatError("Campaign main unit has no selected resident vertices");
        if(!prepared.pose.gpu) {
            (baked?baked_shaders_:shaders_)->append_selected(result.vertices,prepared.pose.vertices,{0,draw.coverage},view,
                baked?baked_material_base_:material_base_);
            continue;
        }
        auto selected=prepared.pose.vertices;auto ordinals=prepared.ordinals;
        shaders_->select_indexed(selected,ordinals,{0,draw.coverage},view);
        if(selected.empty())continue;
        for(auto &vertex:selected)vertex.material+=material_base_;
        auto geometry=std::make_shared<ModelSkinGeometry>();
        geometry->owner=owner;geometry->definition=definition_;geometry->render_model=render_model_;geometry->lod=draw.lod;
        geometry->vertices=selected;geometry->palette=prepared.pose.palette;geometry->source_vertices=std::move(ordinals);
        geometry->bindings.reserve(geometry->source_vertices.size());
        for(const auto ordinal:geometry->source_vertices)geometry->bindings.push_back(lods_[draw.lod]->skin.at(ordinal));
        result.deferred_skins.push_back({result.vertices.size(),std::move(geometry)});
        result.vertices.insert(result.vertices.end(),selected.begin(),selected.end());
    }
    return result;
}
}
