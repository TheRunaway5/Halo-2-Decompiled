#include "halo2/source_bsp_shader_scene.hpp"
#include "halo2/world_material.hpp"
#include <algorithm>
#include <set>

namespace halo2 {
SourceBspShaderScene::SourceBspShaderScene(const Cache &cache,Mesh &mesh,std::size_t bsp)
    :geometry_(cache.load_source_bsp_admission(bsp)),
     parts_(cache.load_source_transparent_bsp_parts(bsp)),materials_(mesh.materials.size()),
     source_shaders_(mesh.materials.size(),0xffffffffu) {
    const unsigned roots=unsigned(mesh.materials.size());
    for(unsigned root=0;root<roots;++root) {
        const auto &original=mesh.materials[root];
        if(original.source_shader_datum==0xffffffffu ||
            (original.water_pass && original.forward_vertex==8))continue;
        const auto shader=original.source_shader_datum;
        source_shaders_[root]=shader;
        auto [definition,inserted]=shaders_.try_emplace(shader);
        if(inserted)definition->second=cache.load_source_shader_lod_definition(shader);
        const auto raw=cache.load_material(shader);
        if(raw.lods.size()!=definition->second.thresholds.size())
            throw FormatError("BSP shader template differs from source LOD bank");
        const bool per_vertex=original.per_vertex_lighting ||
            (original.environment_pass && original.environment_pass->per_vertex_lighting);
        auto &slots=materials_[root];slots.push_back(root);
        for(unsigned lod=1;lod<raw.lods.size();++lod) {
            auto material=raw;material.selected_shader_lod=lod;
            if(!prepare_world_material(cache,material,per_vertex))
                throw FormatError("BSP shader LOD requires unresolved source pass: "+
                    raw.name+" LOD"+std::to_string(lod));
            slots.push_back(unsigned(mesh.materials.size()));
            mesh.materials.push_back(std::move(material));
        }
    }
    std::set<std::array<std::uint32_t,2>> resident_lightmaps;
    for(const auto &vertex:mesh.vertices) {
        if(vertex.bsp_cluster==-1)continue;
        if(vertex.source_part==0xffffffffu || vertex.source_section==0xffffffffu)
            throw FormatError("BSP shader selection requires authored part identity");
        auto [it,inserted]=source_materials_.try_emplace(
            std::pair{vertex.bsp_cluster,vertex.source_part},vertex.material);
        if(!inserted && it->second!=vertex.material)
            throw FormatError("BSP authored part changed its material binding");
        if(vertex.lightmap)for(const auto slot:materials_.at(vertex.material))
            resident_lightmaps.insert({slot,vertex.lightmap});
    }
    mesh.resident_material_lightmaps.assign(resident_lightmaps.begin(),resident_lightmaps.end());
}
std::vector<SourceBspShaderPart> SourceBspShaderScene::select(
    std::vector<Vertex> &vertices,const SourceBspShaderView &view) const {
    const auto camera=source_shader_coverage_camera(view.camera);
    const auto shader_view=source_shader_lod_view(view.outer_top,view.outer_bottom,false,false);
    const auto multiplier=view.detail_multiplier.value_or(source_shader_lod_player_multiplier(view.local_players));
    std::vector<SourceBspShaderPart> result;
    struct Binding {unsigned material;std::uint8_t alpha;std::uint8_t stipple_level{};};
    std::map<std::pair<int,unsigned>,Binding> selected;
    const auto section=[&](int owner,unsigned section_id,
        const SourceTransparentPartSection &bank,std::uint8_t coverage,std::uint8_t alpha) {
        for(const auto &part:bank.parts) {
            const auto key=std::pair{owner,unsigned(part.ordinal)};
            const auto source=source_materials_.find(key);
            if(source==source_materials_.end())continue; // No emitted visible triangles.
            if(!alpha) {selected.emplace(key,Binding{0xffffffffu,0});continue;}
            const auto root=source->second;
            if(root>=materials_.size())throw FormatError("BSP shader root outside resident bank");
            const auto &slots=materials_[root];
            if(slots.empty())continue; // Independent infinite-water/absent shader service.
            if(part.shader!=source_shaders_[root])
                throw FormatError("BSP authored part differs from its native shader root");
            const auto shader=shaders_.find(part.shader);
            if(shader==shaders_.end())throw FormatError("BSP part shader differs from resident root");
            const auto part_geometry=source_transparent_cluster_geometry(part);
            const auto metric=source_shader_part_metric(coverage,part_geometry.lod_scale,multiplier);
            const auto choice=source_shader_lod_choice(shader->second.thresholds,
                shader->second.bias,metric,shader_view,view.view_modifier);
            if(!choice.accepted || choice.index<0) {
                selected.emplace(key,Binding{0xffffffffu,alpha});continue;
            }
            const auto lod=unsigned(choice.index);
            if(lod>=slots.size())throw FormatError("BSP source shader LOD outside resident bank");
            // Alpha is constant for this selected authored part. Retain the
            // complete source helper's level once, before visiting its vertices.
            selected.emplace(key,Binding{slots[lod],alpha,source_shader_instance_stipple(alpha).level});
            result.push_back({owner,section_id,part.ordinal,lod,slots[lod],coverage,alpha});
        }
    };
    for(const auto &record:view.admission.clusters) {
        if(record.id>=geometry_.clusters.size() || record.id>=parts_.clusters.size())
            throw FormatError("BSP shader cluster outside authored bank");
        const auto coverage=source_shader_coverage_byte(
            source_shader_cluster_coverage(geometry_.clusters[record.id].bounds,camera));
        section(int(record.id),record.id,parts_.clusters[record.id],coverage,255);
    }
    for(const auto &record:view.admission.instances) {
        if(record.id>=geometry_.instances.size())throw FormatError("BSP shader instance outside authored bank");
        const auto &instance=geometry_.instances[record.id];
        if(instance.definition<0 || unsigned(instance.definition)>=parts_.instance_definitions.size())
            throw FormatError("BSP shader instance definition outside authored bank");
        const auto actual_coverage=source_shader_instance_coverage(instance.center,instance.radius,camera);
        const auto coverage=source_shader_coverage_byte(actual_coverage);
        section(-2-int(record.id),unsigned(instance.definition),
            parts_.instance_definitions[unsigned(instance.definition)],coverage,
            source_shader_instance_alpha(actual_coverage));
    }
    vertices.erase(std::remove_if(vertices.begin(),vertices.end(),[&](Vertex &vertex) {
        const auto found=selected.find({vertex.bsp_cluster,vertex.source_part});
        if(found==selected.end())return false;
        if(found->second.material==0xffffffffu)return true;
        vertex.material=found->second.material;
        vertex.source_stipple_level=found->second.stipple_level;
        return false;
    }),vertices.end());
    return result;
}
}
