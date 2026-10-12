#include "halo2/source_bsp_resident_plan.hpp"
#include "halo2/world_material.hpp"
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <set>

namespace halo2 {
namespace {
SourceBspDrawKey draw_key(const Vertex &v) {
    return {v.material,v.lightmap,v.bsp_cluster,v.source_section,v.source_part};
}
std::uint32_t narrowed(std::size_t value) {
    if(value>std::numeric_limits<std::uint32_t>::max())throw FormatError("BSP draw plan exceeds 32-bit range");
    return std::uint32_t(value);
}
}
SourceBspResidentPlan::SourceBspResidentPlan(const Cache &cache,Mesh &mesh,std::size_t bsp,
    SourceBspResidentLighting lighting)
    :geometry_(cache.load_source_bsp_admission(bsp)),
     parts_(cache.load_source_transparent_bsp_parts(bsp)),materials_(mesh.materials.size()),
     source_shaders_(mesh.materials.size(),0xffffffffu),raw_vertices_(mesh.vertices) {
    if(lighting!=SourceBspResidentLighting::static_lightmap &&
       lighting!=SourceBspResidentLighting::classification3)
        throw FormatError("unsupported explicit BSP resident lighting classification");
    // Literal resident-bank construction from the unchanged ordinary scene.
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
        if(lighting==SourceBspResidentLighting::classification3 &&
           !raw.lods.empty() && (raw.lods[0].available_layers&(1u<<3))) {
            auto material=raw;material.selected_shader_lod=0;
            if(!prepare_authored_lighting_material(cache,material))
                throw FormatError("BSP classification3 root requires unresolved source pass: "+raw.name);
            mesh.materials[root]=std::move(material);
        }
        auto &slots=materials_[root];slots.push_back(root);
        for(unsigned lod=1;lod<raw.lods.size();++lod) {
            auto material=raw;material.selected_shader_lod=lod;
            const bool prepared=lighting==SourceBspResidentLighting::classification3
                ?prepare_authored_lighting_material(cache,material)
                :prepare_world_material(cache,material,per_vertex);
            if(!prepared)
                throw FormatError("BSP shader LOD requires unresolved source pass: "+raw.name+" LOD"+std::to_string(lod));
            slots.push_back(unsigned(mesh.materials.size()));mesh.materials.push_back(std::move(material));
        }
    }
    std::set<std::array<std::uint32_t,2>> resident_lightmaps;
    for(const auto &v:mesh.vertices) {
        if(v.bsp_cluster==-1)continue;
        if(v.source_part==0xffffffffu || v.source_section==0xffffffffu)
            throw FormatError("BSP shader selection requires authored part identity");
        auto [it,inserted]=source_materials_.try_emplace(std::pair{v.bsp_cluster,v.source_part},v.material);
        if(!inserted && it->second!=v.material)throw FormatError("BSP authored part changed its material binding");
        if(v.lightmap)for(const auto slot:materials_.at(v.material))resident_lightmaps.insert({slot,v.lightmap});
    }
    mesh.resident_material_lightmaps.assign(resident_lightmaps.begin(),resident_lightmaps.end());
    std::map<SourceBspDrawKey,unsigned> identities;
    for(const auto &v:raw_vertices_) {
        const auto [it,inserted]=identities.try_emplace(draw_key(v),unsigned(resident_parts_.size()));
        if(inserted)resident_parts_.push_back({it->first,0,0,v.source_stipple_level});
        auto &part=resident_parts_[it->second];
        if(part.uniform_stipple && *part.uniform_stipple!=v.source_stipple_level)part.uniform_stipple.reset();
        ++part.count;raw_part_.push_back(it->second);
    }
    std::vector<std::uint32_t> cursor;
    std::size_t first{};
    for(auto &part:resident_parts_) {
        part.first=narrowed(first);cursor.push_back(part.first);first+=part.count;
    }
    packed_vertices_.resize(first);
    for(unsigned i=0;i<raw_vertices_.size();++i)packed_vertices_[cursor[raw_part_[i]]++]=raw_vertices_[i];
}
SourceBspResidentViewPlan SourceBspResidentPlan::select(const SourceBspShaderView &view) const {
    return select(view,{});
}
SourceBspResidentViewPlan SourceBspResidentPlan::select(const SourceBspShaderView &view,
    SourceBspShaderLodAuxiliary auxiliary) const {
    const auto camera=source_shader_coverage_camera(view.camera);
    const auto shader_view=source_shader_lod_view(view.outer_top,view.outer_bottom,
        auxiliary.present,auxiliary.byte5);
    const auto multiplier=view.detail_multiplier.value_or(source_shader_lod_player_multiplier(view.local_players));
    SourceBspResidentViewPlan result;
    struct Decision {SourceBspPartBinding binding;SourceBspPartDecisionRecord record;};
    std::map<std::pair<int,unsigned>,Decision> selected;
    const auto section=[&](int owner,unsigned section_id,const SourceTransparentPartSection &bank,
                           std::uint8_t coverage,std::uint8_t alpha) {
        for(const auto &part:bank.parts) {
            const auto key=std::pair{owner,unsigned(part.ordinal)};
            const auto source=source_materials_.find(key);
            if(source==source_materials_.end())continue;
            if(!alpha) {
                selected.emplace(key,Decision{{{},{}},{SourceBspPartDecision::suppressed,
                    SourceBspPartSuppression::zero_alpha,{}}});continue;
            }
            const auto root=source->second;
            if(root>=materials_.size())throw FormatError("BSP shader root outside resident bank");
            const auto &slots=materials_[root];if(slots.empty())continue;
            if(part.shader!=source_shaders_[root])throw FormatError("BSP authored part differs from its native shader root");
            const auto shader=shaders_.find(part.shader);
            if(shader==shaders_.end())throw FormatError("BSP part shader differs from resident root");
            const auto part_geometry=source_transparent_cluster_geometry(part);
            const auto metric=source_shader_part_metric(coverage,part_geometry.lod_scale,multiplier);
            const auto choice=source_shader_lod_choice(shader->second.thresholds,shader->second.bias,
                metric,shader_view,view.view_modifier);
            if(!choice.accepted || choice.index<0) {
                selected.emplace(key,Decision{{{},{}},{SourceBspPartDecision::suppressed,
                    SourceBspPartSuppression::rejected_lod,{}}});continue;
            }
            const auto lod=unsigned(choice.index);
            if(lod>=slots.size())throw FormatError("BSP source shader LOD outside resident bank");
            SourceBspShaderPart accepted{owner,section_id,part.ordinal,lod,slots[lod],coverage,alpha};
            selected.emplace(key,Decision{{slots[lod],source_shader_instance_stipple(alpha).level},
                {SourceBspPartDecision::accepted,SourceBspPartSuppression::none,accepted}});
            // Preserve the original result push even if a duplicate admission
            // key keeps the earlier emplaced vertex binding.
            result.selected_parts.push_back(accepted);
        }
    };
    for(const auto &record:view.admission.clusters) {
        if(record.id>=geometry_.clusters.size() || record.id>=parts_.clusters.size())
            throw FormatError("BSP shader cluster outside authored bank");
        const auto coverage=source_shader_coverage_byte(source_shader_cluster_coverage(geometry_.clusters[record.id].bounds,camera));
        section(int(record.id),record.id,parts_.clusters[record.id],coverage,255);
    }
    for(const auto &record:view.admission.instances) {
        if(record.id>=geometry_.instances.size())throw FormatError("BSP shader instance outside authored bank");
        const auto &instance=geometry_.instances[record.id];
        if(instance.definition<0 || unsigned(instance.definition)>=parts_.instance_definitions.size())
            throw FormatError("BSP shader instance definition outside authored bank");
        const auto actual_coverage=source_shader_instance_coverage(instance.center,instance.radius,camera);
        section(-2-int(record.id),unsigned(instance.definition),parts_.instance_definitions[unsigned(instance.definition)],
            source_shader_coverage_byte(actual_coverage),source_shader_instance_alpha(actual_coverage));
    }
    for(const auto &part:resident_parts_) {
        const auto found=selected.find({part.original.geometry,part.original.part});
        if(found==selected.end()) {
            result.bindings.push_back({part.original.material,{}});result.decisions.push_back({});
        } else {
            result.bindings.push_back(found->second.binding);result.decisions.push_back(found->second.record);
        }
    }
    return result;
}
std::vector<Vertex> SourceBspResidentPlan::expand(std::span<const SourceBspPartBinding> bindings) const {
    if(bindings.size()!=resident_parts_.size())throw FormatError("BSP view bindings differ from resident catalogue");
    std::vector<Vertex> result;result.reserve(raw_vertices_.size());
    for(unsigned i=0;i<raw_vertices_.size();++i) {
        const auto &binding=bindings[raw_part_[i]];if(!binding.material)continue;
        auto v=raw_vertices_[i];v.material=*binding.material;
        if(binding.stipple)v.source_stipple_level=*binding.stipple;result.push_back(v);
    }
    return result;
}
SourceBspMergedDrawPlan SourceBspResidentPlan::merge(std::span<const SourceBspPartBinding> bindings,
    std::span<const Vertex> objects,unsigned material_count,unsigned lightmap_count,
    std::uint32_t dynamic_first,std::uint32_t logical_first) const {
    SourceBspMergedDrawPlan result;
    merge_into(result,bindings,objects,material_count,lightmap_count,dynamic_first,logical_first);
    return result;
}
void SourceBspResidentPlan::merge_into(SourceBspMergedDrawPlan &result,
    std::span<const SourceBspPartBinding> bindings,std::span<const Vertex> objects,
    unsigned material_count,unsigned lightmap_count,
    std::uint32_t dynamic_first,std::uint32_t logical_first,bool borrow_objects) const {
    const auto *reference=std::getenv("HALO2_REFERENCE_MAP_BSP_GROUPING");
    if(reference && *reference) {
        result.borrowed_runs.clear();
        result.draws.clear();result.descriptors.clear();result.dynamic_vertices.clear();result.logical_vertices=0;result.borrowed_object_vertices=0;
        if(bindings.size()!=resident_parts_.size())throw FormatError("BSP view bindings differ from resident catalogue");
        struct Run {std::size_t first{},count{};};
        struct Entry {std::vector<unsigned> parts;std::vector<Run> object_runs;std::size_t object_count{};};
        std::map<SourceBspDrawKey,Entry> entries;
        const auto validate=[&](const SourceBspDrawKey &key) {
            if(key.material>=material_count || key.lightmap>=lightmap_count)
                throw FormatError("invalid dynamic material/lightmap index");
        };
        for(unsigned i=0;i<resident_parts_.size();++i)if(bindings[i].material) {
            auto key=resident_parts_[i].original;key.material=*bindings[i].material;validate(key);entries[key].parts.push_back(i);
        }
        for(std::size_t i=0;i<objects.size();) {
            const auto key=draw_key(objects[i]);validate(key);auto end=i+1;
            while(end<objects.size() && draw_key(objects[end])==key)++end;
            auto &entry=entries[key];entry.object_runs.push_back({i,end-i});entry.object_count+=end-i;i=end;
        }
        const auto resident_entry=[&](const Entry &entry) {
            if(entry.parts.size()!=1 || !entry.object_runs.empty())return false;
            const auto p=entry.parts.front();
            return resident_parts_[p].original.geometry!=-1 &&
                (!bindings[p].stipple || (resident_parts_[p].uniform_stipple &&
                    *bindings[p].stipple==*resident_parts_[p].uniform_stipple));
        };
        const auto borrowed_entry=[&](const Entry &entry) {
            return borrow_objects && entry.parts.empty() && !entry.object_runs.empty();
        };
        std::size_t dynamic_count{};
        for(const auto &[key,entry]:entries)if(!resident_entry(entry) && !borrowed_entry(entry)) {
            dynamic_count+=entry.object_count;for(const auto p:entry.parts)dynamic_count+=resident_parts_[p].count;
        }
        narrowed(std::size_t(dynamic_first)+dynamic_count);
        result.dynamic_vertices.reserve(dynamic_count);result.draws.reserve(entries.size());result.descriptors.reserve(entries.size());
        std::size_t logical=logical_first;
        for(const auto &[key,entry]:entries) {
            std::size_t count=entry.object_count;for(const auto p:entry.parts)count+=resident_parts_[p].count;
            SourceBspDrawToken token{key,SourceBspVertexStorage::dynamic,0,narrowed(count),narrowed(logical)};logical+=count;
            result.descriptors.push_back({key.material,key.lightmap});
            const auto p=entry.parts.empty()?0u:entry.parts.front();
            // Raw material is not a world vertex attribute. A stipple override is;
            // only an unchanged exact stream may address its resident range.
            if(resident_entry(entry)) {
                token.storage=SourceBspVertexStorage::resident;token.first=resident_parts_[p].first;
            } else if(borrowed_entry(entry)) {
                if(entry.object_runs.size()==1) {
                    token.storage=SourceBspVertexStorage::objects;token.first=narrowed(entry.object_runs.front().first);
                } else {
                    token.storage=SourceBspVertexStorage::object_runs;
                    token.first=narrowed(result.borrowed_runs.size());token.run_count=narrowed(entry.object_runs.size());
                    for(const auto &run:entry.object_runs)result.borrowed_runs.push_back({narrowed(run.first),narrowed(run.count)});
                }
                result.borrowed_object_vertices+=token.count;
            } else {
                token.first=narrowed(std::size_t(dynamic_first)+result.dynamic_vertices.size());
                const auto append=[&](const Vertex &raw,const SourceBspPartBinding &binding) {
                    auto v=raw;v.material=*binding.material;if(binding.stipple)v.source_stipple_level=*binding.stipple;
                    result.dynamic_vertices.push_back(v);
                };
                if(entry.parts.size()==1) {
                    const auto &part=resident_parts_[p];
                    for(const auto &raw:std::span{packed_vertices_}.subspan(part.first,part.count))append(raw,bindings[p]);
                } else if(!entry.parts.empty()) {
                    // Explicit colliding-binding control: preserve raw global
                    // input order when several catalogue keys collapse together.
                    for(unsigned i=0;i<raw_vertices_.size();++i) {
                        const auto part=raw_part_[i];if(!bindings[part].material)continue;
                        auto actual=resident_parts_[part].original;actual.material=*bindings[part].material;
                        if(actual==key)append(raw_vertices_[i],bindings[part]);
                    }
                }
                // The ordinary caller appends dynamic objects after the BSP base.
                // A key collision stays one draw, with BSP vertices first.
                for(const auto &run:entry.object_runs) {
                    const auto span=objects.subspan(run.first,run.count);
                    result.dynamic_vertices.insert(result.dynamic_vertices.end(),span.begin(),span.end());
                }
            }
            result.draws.push_back(token);
        }
        narrowed(logical);result.logical_vertices=narrowed(logical-std::size_t(logical_first));
        return;
    }
    result.borrowed_runs.clear();
    result.draws.clear();result.descriptors.clear();result.dynamic_vertices.clear();result.logical_vertices=0;result.borrowed_object_vertices=0;
    if(bindings.size()!=resident_parts_.size())throw FormatError("BSP view bindings differ from resident catalogue");
    struct Run {std::size_t first{},count{};};
    struct InputRun {SourceBspDrawKey key;unsigned part{};Run object;bool resident{};};
    struct Entry {std::span<const InputRun> parts,object_runs;std::size_t object_count{};};
    std::vector<InputRun> inputs;inputs.reserve(resident_parts_.size());
    std::vector<std::pair<SourceBspDrawKey,Entry>> entries;
    const auto validate=[&](const SourceBspDrawKey &key) {
        if(key.material>=material_count || key.lightmap>=lightmap_count)
            throw FormatError("invalid dynamic material/lightmap index");
    };
    for(unsigned i=0;i<resident_parts_.size();++i)if(bindings[i].material) {
        auto key=resident_parts_[i].original;key.material=*bindings[i].material;validate(key);
        inputs.push_back({key,i,{},true});
    }
    for(std::size_t i=0;i<objects.size();) {
        const auto key=draw_key(objects[i]);validate(key);auto end=i+1;
        while(end<objects.size() && draw_key(objects[end])==key)++end;
        inputs.push_back({key,0,{i,end-i},false});i=end;
    }
    // Stable grouping preserves resident catalogue order and the original
    // encounter order of every object run. Colliding BSP bindings still use
    // the raw global scatter path below.
    std::stable_sort(inputs.begin(),inputs.end(),[](const auto &a,const auto &b){return a.key<b.key;});
    entries.reserve(inputs.size());
    for(std::size_t first=0;first<inputs.size();) {
        auto last=first+1;while(last<inputs.size() && inputs[last].key==inputs[first].key)++last;
        auto split=first;while(split<last && inputs[split].resident)++split;
        std::size_t objects{};for(auto i=split;i<last;++i)objects+=inputs[i].object.count;
        entries.push_back({inputs[first].key,{std::span{inputs}.subspan(first,split-first),
            std::span{inputs}.subspan(split,last-split),objects}});
        first=last;
    }
    const auto resident_entry=[&](const Entry &entry) {
        if(entry.parts.size()!=1 || !entry.object_runs.empty())return false;
        const auto p=entry.parts.front().part;
        return resident_parts_[p].original.geometry!=-1 &&
            (!bindings[p].stipple || (resident_parts_[p].uniform_stipple &&
                *bindings[p].stipple==*resident_parts_[p].uniform_stipple));
    };
    const auto borrowed_entry=[&](const Entry &entry) {
        return borrow_objects && entry.parts.empty() && !entry.object_runs.empty();
    };
    std::size_t dynamic_count{};
    for(const auto &[key,entry]:entries)if(!resident_entry(entry) && !borrowed_entry(entry)) {
        dynamic_count+=entry.object_count;for(const auto &part:entry.parts)dynamic_count+=resident_parts_[part.part].count;
    }
    narrowed(std::size_t(dynamic_first)+dynamic_count);
    result.dynamic_vertices.reserve(dynamic_count);result.draws.reserve(entries.size());result.descriptors.reserve(entries.size());
    std::size_t logical=logical_first;
    for(const auto &[key,entry]:entries) {
        std::size_t count=entry.object_count;for(const auto &part:entry.parts)count+=resident_parts_[part.part].count;
        SourceBspDrawToken token{key,SourceBspVertexStorage::dynamic,0,narrowed(count),narrowed(logical)};logical+=count;
        result.descriptors.push_back({key.material,key.lightmap});
        const auto p=entry.parts.empty()?0u:entry.parts.front().part;
        // Raw material is not a world vertex attribute. A stipple override is;
        // only an unchanged exact stream may address its resident range.
        if(resident_entry(entry)) {
            token.storage=SourceBspVertexStorage::resident;token.first=resident_parts_[p].first;
        } else if(borrowed_entry(entry)) {
            if(entry.object_runs.size()==1) {
                token.storage=SourceBspVertexStorage::objects;token.first=narrowed(entry.object_runs.front().object.first);
            } else {
                token.storage=SourceBspVertexStorage::object_runs;
                token.first=narrowed(result.borrowed_runs.size());token.run_count=narrowed(entry.object_runs.size());
                for(const auto &input:entry.object_runs)result.borrowed_runs.push_back({narrowed(input.object.first),narrowed(input.object.count)});
            }
            result.borrowed_object_vertices+=token.count;
        } else {
            token.first=narrowed(std::size_t(dynamic_first)+result.dynamic_vertices.size());
            const auto append=[&](const Vertex &raw,const SourceBspPartBinding &binding) {
                auto v=raw;v.material=*binding.material;if(binding.stipple)v.source_stipple_level=*binding.stipple;
                result.dynamic_vertices.push_back(v);
            };
            if(entry.parts.size()==1) {
                const auto &part=resident_parts_[p];
                for(const auto &raw:std::span{packed_vertices_}.subspan(part.first,part.count))append(raw,bindings[p]);
            } else if(!entry.parts.empty()) {
                // Explicit colliding-binding control: preserve raw global
                // input order when several catalogue keys collapse together.
                for(unsigned i=0;i<raw_vertices_.size();++i) {
                    const auto part=raw_part_[i];if(!bindings[part].material)continue;
                    auto actual=resident_parts_[part].original;actual.material=*bindings[part].material;
                    if(actual==key)append(raw_vertices_[i],bindings[part]);
                }
            }
            // The ordinary caller appends dynamic objects after the BSP base.
            // A key collision stays one draw, with BSP vertices first.
            for(const auto &input:entry.object_runs) {
                const auto span=objects.subspan(input.object.first,input.object.count);
                result.dynamic_vertices.insert(result.dynamic_vertices.end(),span.begin(),span.end());
            }
        }
        result.draws.push_back(token);
    }
    narrowed(logical);result.logical_vertices=narrowed(logical-std::size_t(logical_first));
}
}
