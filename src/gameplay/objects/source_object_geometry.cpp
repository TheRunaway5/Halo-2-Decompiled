#include "gameplay/objects/source_object_geometry.hpp"
#include "halo2/native_parallel.hpp"
#include "halo2/world_material.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/math.hpp"
#include "halo2/content.hpp"
#include "halo2/source_model_regions.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>
#include <cstdlib>
#include <cstring>

namespace halo2 {
void SourceObjectGeometry::load(const Cache&cache,const ScenarioObject&object,
    const SourceInitialModelDefinition&initial,const SourceInitialModelSelection&selection,
    std::uint32_t render_model,bool prepare_region_variants){
    load_impl(cache,object,initial,&selection,render_model,prepare_region_variants);
}
void SourceObjectGeometry::load_union(const Cache &cache,const ScenarioObject &object,
    const SourceInitialModelDefinition &initial,std::uint32_t render_model) {
    load_impl(cache,object,initial,nullptr,render_model,true);
}
void SourceObjectGeometry::load_impl(const Cache&cache,const ScenarioObject&object,
    const SourceInitialModelDefinition&initial,const SourceInitialModelSelection *selection,
    std::uint32_t render_model,bool prepare_region_variants){
    invalidate_views();
    if(selection && !selection->resolved())throw FormatError("object geometry requires resolved source region bytes");
    lod_definition=cache.load_source_geometry_lod_definition(object.definition);
    prt=cache.load_source_prt_definition(render_model);
    if(prepare_region_variants)region_definition=initial;else region_definition.reset();
    //Keep the legacy diagnostic's sourceLOD5 slots first. Every other level
    //is decoded explicitly; an empty original collector stays empty.
    for(unsigned lod:{5u,0u,1u,2u,3u,4u}){
        auto&g=lods[lod];active_sections[lod]=selection?source_initial_model_sections(initial,*selection,lod):std::vector<unsigned>{};
        g.sections=active_sections[lod];
        if(prepare_region_variants){
            // Resource union only. Active submission below retains original
            // region order and duplicate collector entries after every change.
            if(selection)g.sections=source_model_region_resource_sections(initial,*selection,lod);
            else for(const auto &region:initial.render_sections)for(const auto &permutation:region) {
                const auto section=permutation[lod];
                if(section>=0 && std::find(g.sections.begin(),g.sections.end(),unsigned(section))==g.sections.end())
                    g.sections.push_back(unsigned(section));
            }
            g.model=cache.load_model_sections(object.definition,lod==5,object.variant,lod,g.sections);
        }else g.model=cache.load_model(object.definition,lod==5,object.variant,*selection,lod);
        g.authored_parts=cache.load_source_transparent_model_parts(render_model,g.sections);
        if(g.model.vertex_origins.size()!=g.model.mesh.vertices.size())
            throw FormatError("resident object geometry lacks source vertex identities: "+g.model.name);
        for(const auto&origin:g.model.vertex_origins)
            if(std::find(g.sections.begin(),g.sections.end(),origin.section)==g.sections.end())
                throw FormatError("resident object geometry differs from source collector: "+g.model.name);
        if(prt && !g.model.mesh.vertices.empty() && !cache.apply_source_prt_vertices(g.model,render_model,lod))
            throw FormatError("resident PRT geometry has no exact source LOD stream: "+g.model.name);
        if(lod!=5){
            if(g.model.bones.size()!=lods[5].model.bones.size() ||
                g.model.checksum!=lods[5].model.checksum || g.model.animation!=lods[5].model.animation)
                throw FormatError("resident object LOD changed its source hierarchy: "+g.model.name);
            //OnlyLOD5 owns the authored material table. Lower levels retain
            //source material indices in vertices and use those same entries.
            g.model.mesh.materials.clear();
        }
    }
}
void SourceObjectGeometry::regions(const SourceInitialModelSelection &selection){
    if(!region_definition)throw FormatError("object geometry has no prepared region transition bank");
    bool changed=false;
    for(unsigned lod=0;lod<6;++lod){auto sections=source_initial_model_sections(*region_definition,selection,lod);
        for(auto section:sections)if(std::find(lods[lod].sections.begin(),lods[lod].sections.end(),section)==lods[lod].sections.end())
            throw FormatError("active model region selected an unprepared resident section");
        changed|=sections!=active_sections[lod];active_sections[lod]=std::move(sections);
    }
    if(changed)invalidate_views();
}
std::vector<SourceObjectGeometry::PartRun> SourceObjectGeometry::active_runs(unsigned lod)const{
    if(!region_definition)return part_runs.at(lod);
    std::vector<PartRun> result;
    for(auto section:active_sections.at(lod))for(const auto &run:part_runs.at(lod))
        if(parts.at(lod).at(run.part).section==section)result.push_back(run);
    return result;
}
SourceObjectGeometry::ViewDraw SourceObjectGeometry::active_draw(unsigned lod)const{
    ViewDraw result;const auto &g=lods.at(lod);
    for(const auto &run:active_runs(lod)){
        const auto vertices=std::span(g.vertices).subspan(run.first,run.count);
        result.vertices.insert(result.vertices.end(),vertices.begin(),vertices.end());
        result.ordinary_lighting|=parts[lod].at(run.part).ordinary.at(0);
    }
    return result;
}
SourceObjectGeometry::Stats SourceObjectGeometry::prepare(const Cache&cache,const ObjectColors&colors,
    std::span<const Transform>world_nodes,const SourcePrtLighting&sh,Mesh&mesh,
    std::optional<float> camouflage,std::optional<Vec3> retained_incident){
    invalidate_views();
    Stats counts;std::array<std::vector<SourcePrtSectionBank>,6>banks;
    for(unsigned lod=0;lod<6;++lod)if(prt && !lods[lod].sections.empty()){
        banks[lod]=source_prt_section_banks(cache,lods[lod].model,*prt,lod,lods[lod].sections,world_nodes,sh,retained_incident);
        counts.union_section_banks+=unsigned(banks[lod].size());
    }
    counts.section_banks=unsigned(banks[5].size());
    std::shared_ptr<const VertexProgramBank>first_bank;
    for(unsigned lod:{5u,0u,1u,2u,3u,4u})if(!banks[lod].empty()){first_bank=banks[lod].front().bank;break;}
    const auto&source=lods[5].model.mesh.materials;
    shader_definitions.clear();shader_definitions.reserve(source.size());
    for(auto material:source){
        if(material.source_shader_datum==0xffffffffu)
            shader_definitions.push_back({});
        else {
            shader_definitions.push_back(cache.load_source_shader_lod_definition(material.source_shader_datum));
            if(shader_definitions.back().thresholds.size()!=material.lods.size())
                throw FormatError("object shader LOD bank differs from authored template: "+material.name);
        }
        if(!material.lods.empty()){
            if(prepare_dynamic_material(cache,material,{},true,camouflage))++counts.dynamic_materials;
            else{
                if(!prt || !first_bank || !prepare_prt_material(cache,material,first_bank))
                    throw FormatError("object authored shader requires unresolved source inputs: "+material.name);
                ++counts.prt_materials;
            }
        }
    }
    first_material=unsigned(mesh.materials.size());
    std::map<std::pair<unsigned,unsigned>,unsigned>ordinary;
    for(unsigned lod:{5u,0u,1u,2u,3u,4u}){
        auto&g=lods[lod];std::map<std::tuple<unsigned,unsigned,unsigned>,unsigned>projected_slots;
        std::map<std::pair<unsigned,unsigned>,unsigned>part_indices;
        for(unsigned i=0;i<g.model.mesh.vertices.size();++i){
            const auto source_material=g.model.mesh.vertices[i].material;
            if(source_material>=source.size() || source[source_material].lods.empty())
                throw FormatError("resident object face has no authored shader: "+g.model.name);
            const auto &origin=g.model.vertex_origins[i];const unsigned section=origin.section;
            auto [part_index,new_part]=part_indices.try_emplace(std::pair{section,unsigned(origin.part)},unsigned(parts[lod].size()));
            if(new_part) {
                const auto section_bank=std::find_if(g.authored_parts.sections.begin(),g.authored_parts.sections.end(),
                    [&](const auto&s){return s.section==section;});
                if(section_bank==g.authored_parts.sections.end() || origin.part>=section_bank->parts.size())
                    throw FormatError("resident object face has no literal source part: "+g.model.name);
                const auto &authored=section_bank->parts[origin.part];
                if(authored.material!=int(source_material) || authored.shader!=source[source_material].source_shader_datum)
                    throw FormatError("resident object part shader differs from expanded face: "+g.model.name);
                Part part{section,origin.part,source_material,authored.lod_scale>0?authored.lod_scale:1,{}, {}};
                for(unsigned shader_lod=0;shader_lod<source[source_material].lods.size();++shader_lod) {
                    const bool is_prt=bool(source[source_material].lods[shader_lod].available_layers&(1u<<5));
                    unsigned target{};
                    if(is_prt) {
                        auto[at,inserted]=projected_slots.try_emplace(std::tuple{section,source_material,shader_lod});
                        if(inserted) {
                            const auto bank=std::find_if(banks[lod].begin(),banks[lod].end(),[&](const auto&b){return b.section==section;});
                            if(bank==banks[lod].end())throw FormatError("resident PRT face has no exact section/LOD bank: "+g.model.name);
                            auto material=source[source_material];material.selected_shader_lod=shader_lod;
                            if(!prepare_prt_material(cache,material,bank->bank))
                                throw FormatError("resident PRT shader LOD requires unresolved source pass: "+material.name);
                            apply_object_colors(material,colors);at->second=unsigned(mesh.materials.size());
                            mesh.materials.push_back(std::move(material));slots.push_back({at->second,section,lod,true,shader_lod});
                            ++counts.union_prt_materials;if(lod==5 && shader_lod==0)++counts.resident_prt_materials;
                        }
                        target=at->second;
                    } else {
                        union_ordinary_lighting=true;
                        auto[at,inserted]=ordinary.try_emplace(std::pair{source_material,shader_lod});
                        if(inserted) {
                            auto material=source[source_material];material.selected_shader_lod=shader_lod;
                            if(!prepare_dynamic_material(cache,material,{},true,camouflage))
                                throw FormatError("resident dynamic shader LOD requires unresolved source pass: "+material.name);
                            apply_object_colors(material,colors);at->second=unsigned(mesh.materials.size());
                            mesh.materials.push_back(std::move(material));slots.push_back({at->second,section,lod,false,shader_lod});
                            ++counts.union_dynamic_materials;if(lod==5 && shader_lod==0)++counts.resident_dynamic_materials;
                        }
                        target=at->second;
                    }
                    part.materials.push_back(target);part.ordinary.push_back(!is_prt);
                }
                parts[lod].push_back(std::move(part));
            }
            const auto &part=parts[lod].at(part_index->second);
            if(part.source_material!=source_material)throw FormatError("source object part changed shader within expanded vertices");
            vertex_parts[lod].push_back(part_index->second);
            g.vertex_materials.push_back(part.materials.at(0));
            g.ordinary_lighting=g.ordinary_lighting || part.ordinary.at(0);
        }
        // Retain the exact expanded encounter order, including disjoint runs
        // of the same part. pose_world validates that this topology is stable.
        auto &runs=part_runs[lod];runs.clear();
        for(std::size_t i=0;i<vertex_parts[lod].size();) {
            const auto part=vertex_parts[lod][i];auto end=i+1;
            while(end<vertex_parts[lod].size() && vertex_parts[lod][end]==part)++end;
            runs.push_back({i,end-i,part});i=end;
        }
    }material_count=unsigned(mesh.materials.size())-first_material;
    for(auto&g:lods){if(g.vertices.size()!=g.vertex_materials.size())throw FormatError("resident geometry pose missing");
        for(unsigned i=0;i<g.vertices.size();++i)g.vertices[i].material=g.vertex_materials[i];}
    return counts;
}
void SourceObjectGeometry::pose_world(const std::function<std::vector<Vertex>(const RenderModel&)>&pose){
    invalidate_views();
    for(auto&g:lods){g.vertices=pose(g.model);
        if(g.vertices.size()!=g.model.mesh.vertices.size())throw FormatError("resident WORLD pose changed source vertex order");
        if(!g.vertex_materials.empty())
            for(unsigned i=0;i<g.vertices.size();++i)g.vertices[i].material=g.vertex_materials.at(i);
    }
}
void SourceObjectGeometry::pose_world_nodes(std::span<const Transform> nodes){
    std::array<std::vector<Vertex>,6> posed;
    std::array<std::exception_ptr,6> errors;
    native_parallel_for(lods.size(),[&](std::size_t lod) {
        try{posed[lod]=skin_model_world_nodes(lods[lod].model,nodes);}
        catch(...){errors[lod]=std::current_exception();}
    });
    // Publish exactly as pose_world: a failing LOD leaves earlier LODs posed.
    unsigned next=0;
    pose_world([&](const RenderModel&) {
        const auto lod=next++;
        if(errors[lod])std::rethrow_exception(errors[lod]);
        return std::move(posed[lod]);
    });
}
std::vector<SourcePrtMaterialBank> SourceObjectGeometry::owner_banks(const Cache&cache,
    std::span<const Transform>world_nodes,const SourcePrtLighting&sh,unsigned lod,std::optional<Vec3> retained_incident)const {
    const auto &g=lods.at(lod);std::vector<SourcePrtMaterialBank> result;
    if(!prt || active_sections[lod].empty())return result;
    const auto banks=source_prt_section_banks(cache,g.model,*prt,lod,active_sections[lod],world_nodes,sh,retained_incident);
    for(const auto &slot:slots)if(slot.prt && slot.lod==lod) {
        const auto bank=std::find_if(banks.begin(),banks.end(),[&](const auto &b){return b.section==slot.section;});
        if(bank!=banks.end())result.push_back({slot.material,bank->bank,slot.section});
    }
    return result;
}
void SourceObjectGeometry::update_banks(const Cache&cache,std::span<const Transform>world_nodes,
    const SourcePrtLighting&sh,Mesh&mesh){
    if(!prt)return;
    for(unsigned lod=0;lod<6;++lod){const auto&g=lods[lod];
        if(g.sections.empty())continue;
        const auto banks=source_prt_section_banks(cache,g.model,*prt,lod,g.sections,world_nodes,sh);
        for(const auto&slot:slots)if(slot.prt && slot.lod==lod){
            const auto bank=std::find_if(banks.begin(),banks.end(),[&](const auto&b){return b.section==slot.section;});
            if(bank==banks.end())throw FormatError("current resident PRT section/LOD bank missing");
            auto&material=mesh.materials.at(slot.material);
            if(!material.environment_pass || material.environment_pass->forward_vertex!=9)
                throw FormatError("resident PRT material slot was replaced");
            material.environment_pass->vertex_bank=bank->bank;
        }
    }
}
SourceObjectGeometry::ViewDraw SourceObjectGeometry::selected_section(unsigned lod,unsigned section,
    const SourceModelShaderCollector& collector,const SourceModelShaderView& view)const {
    return selected_section_vertices(lod,section,collector,view,lods.at(lod).vertices);
}
SourceObjectGeometry::ViewDraw SourceObjectGeometry::selected_section_world(unsigned lod,unsigned section,
    const SourceModelShaderCollector& collector,const SourceModelShaderView& view,
    std::span<const Transform> nodes)const {
    const auto vertices=skin_model_world_nodes(lods.at(lod).model,nodes);
    return selected_section_vertices(lod,section,collector,view,vertices);
}
SourceObjectGeometry::ViewDraw SourceObjectGeometry::selected_section_pose(unsigned lod,unsigned section,
    const SourceModelShaderCollector& collector,const SourceModelShaderView& view,
    std::span<const Vertex> vertices)const {
    if(vertices.size()!=lods.at(lod).model.mesh.vertices.size())
        throw FormatError("frame-owned WORLD pose changed source vertex order");
    return selected_section_vertices(lod,section,collector,view,vertices);
}
SourceObjectGeometry::ViewDraw SourceObjectGeometry::selected_section_vertices(unsigned lod,unsigned section,
    const SourceModelShaderCollector& collector,const SourceModelShaderView& view,
    std::span<const Vertex> vertices)const {
    const auto& geometry=lods.at(lod);ViewDraw result;
    if(vertices.size()!=geometry.model.mesh.vertices.size())
        throw FormatError("source-selected model WORLD pose changed its complete vertex order");
    if(std::find(geometry.sections.begin(),geometry.sections.end(),section)==geometry.sections.end())
        throw FormatError("completed source model selected an unprepared section/LOD");
    for(const auto& run:part_runs.at(lod)) {
        const auto& part=parts.at(lod).at(run.part);if(part.section!=section)continue;
        const auto choice=source_model_shader_part(collector,part.lod_scale,shader_definitions.at(part.source_material),view).choice;
        if(!choice.accepted || choice.index<0)continue;
        const auto selected=unsigned(choice.index);const auto first=result.vertices.size();
        const auto source=vertices.subspan(run.first,run.count);
        result.vertices.insert(result.vertices.end(),source.begin(),source.end());
        for(auto n=first;n<result.vertices.size();++n)result.vertices[n].material=part.materials.at(selected);
        result.ordinary_lighting|=part.ordinary.at(selected);
        result.prt_lighting|=std::any_of(slots.begin(),slots.end(),[&](const auto& slot){return slot.material==part.materials.at(selected) && slot.prt;});
    }
    return result;
}
std::vector<SourcePrtMaterialBank> SourceObjectGeometry::selected_prt_banks(unsigned lod,unsigned section,
    std::shared_ptr<const VertexProgramBank> bank)const {
    std::vector<SourcePrtMaterialBank> result;
    for(const auto& slot:slots)if(slot.prt && slot.lod==lod && slot.section==section)
        result.push_back({slot.material,bank,section});
    if(!result.empty() && !bank)throw FormatError("completed PRT source draw has no original coefficient bank");
    return result;
}
SourceObjectViewSelection SourceObjectGeometry::select(std::size_t object,std::uint32_t handle,Vec3 center,
    float radius,const SourceObjectViewLod&view){
    return select(object,handle,center,radius,view,previous_lod);
}
SourceObjectViewSelection SourceObjectGeometry::select(std::size_t object,std::uint32_t handle,Vec3 center,
    float radius,const SourceObjectViewLod&view,std::array<std::int8_t,4>&owner_previous_lod){
    if(view.view_index>=owner_previous_lod.size() || !view.input)
        throw FormatError("source object view requires a valid player bank and explicit distance producer");
    const auto input=view.input(object,handle,center,radius);
    if(!std::isfinite(input.draw_metric) || input.draw_metric<0 || !std::isfinite(radius) || radius<0)
        throw FormatError("source object view distance/sphere is invalid");
    float metric=input.draw_metric;
    if(view.source_4ba019)metric*=.3f;
    if(input.control_flags&4u)metric*=1.7f;
    const auto choice=source_geometry_lod_choice(lod_definition,metric,owner_previous_lod[view.view_index]);
    const auto lod=unsigned(source_geometry_lod_cap(choice,view.detail_policy.value_or(source_geometry_lod_policy(view.local_players)),input.object_flags,bool(prt)));
    auto &draw=views[view.view_index];
    auto &retained=view_selection[view.view_index];
    const auto &geometry=lods[lod];
    std::vector<unsigned> selected;
    if(view.shader && !geometry.vertices.empty()) {
        const auto collector=source_model_shader_collector(center,radius,*view.shader);
        selected.reserve(parts[lod].size());
        for(const auto &part:parts[lod]) {
            const auto shader=source_model_shader_part(collector,part.lod_scale,shader_definitions.at(part.source_material),*view.shader);
            if(!shader.choice.accepted || shader.choice.index<0)selected.push_back(0xffffffffu);
            else {
                const auto index=unsigned(shader.choice.index);
                if(index>=part.materials.size())throw FormatError("selected object shader LOD outside resident part bank");
                selected.push_back(index);
            }
        }
    }
    const auto *reference=std::getenv("HALO2_REFERENCE_VERTEX_OBJECT_SELECTION");
    const auto *rebuild=std::getenv("HALO2_REFERENCE_REBUILD_OBJECT_VIEWS");
    // Selection, history and lighting still run for every current owner/view.
    // A draw depends only on this immutable posed LOD and its exact part
    // bindings. load/prepare/pose_world/reset_history invalidate every view.
    const bool reuse=!(reference && *reference) && !(rebuild && *rebuild) &&
        retained.valid && retained.lod==lod && retained.shader==bool(view.shader) && retained.parts==selected;
    const auto *verify=std::getenv("HALO2_VERIFY_OBJECT_VIEW_REUSE");
    if(!reuse || (verify && *verify)) {
        ViewDraw previous;
        if(reuse)previous=draw;
        retained.valid=false;
        draw.vertices.clear();draw.ordinary_lighting=false;
        if(!view.shader) {draw=active_draw(lod);}
        else if(!geometry.vertices.empty()) {
            draw.vertices.reserve(geometry.vertices.size());
            if(reference && *reference) {
                for(const auto &run:active_runs(lod))for(std::size_t i=run.first;i<run.first+run.count;++i) {
                    const auto part_index=vertex_parts[lod].at(i),shader_lod=selected.at(part_index);
                    if(shader_lod==0xffffffffu)continue;
                    const auto &part=parts[lod].at(part_index);
                    auto vertex=geometry.vertices[i];vertex.material=part.materials.at(shader_lod);
                    draw.vertices.push_back(vertex);draw.ordinary_lighting=draw.ordinary_lighting || part.ordinary.at(shader_lod);
                }
            } else for(const auto &run:active_runs(lod)) {
                const auto shader_lod=selected.at(run.part);if(shader_lod==0xffffffffu)continue;
                const auto &part=parts[lod].at(run.part);const auto material=part.materials.at(shader_lod);
                const auto first=draw.vertices.size();const auto source=std::span{geometry.vertices}.subspan(run.first,run.count);
                draw.vertices.insert(draw.vertices.end(),source.begin(),source.end());
                for(std::size_t i=first;i<draw.vertices.size();++i)draw.vertices[i].material=material;
                draw.ordinary_lighting=draw.ordinary_lighting || part.ordinary.at(shader_lod);
            }
        }
        if(reuse && (previous.ordinary_lighting!=draw.ordinary_lighting || previous.vertices.size()!=draw.vertices.size() ||
            (!draw.vertices.empty() && std::memcmp(previous.vertices.data(),draw.vertices.data(),draw.vertices.size()*sizeof(Vertex)))))
            throw FormatError("retained object view differs from rebuilt complete source vertices");
        retained={true,bool(view.shader),lod,std::move(selected)};
    }
    if(!draw.vertices.empty())owner_previous_lod[view.view_index]=std::int8_t(lod);
    return {object,lod,choice.transition,draw.vertices.size(),input.draw_metric};
}
void SourceObjectGeometry::reset_history() noexcept {invalidate_views();previous_lod.fill(-1);for(auto&v:views){v.vertices.clear();v.ordinary_lighting=v.prt_lighting=false;}}
bool SourceObjectGeometry::ordinary_lighting()const{
    return union_ordinary_lighting;
}
}
