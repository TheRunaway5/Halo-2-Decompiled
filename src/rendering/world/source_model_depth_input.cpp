#include "halo2/source_model_depth_input.hpp"
#include "halo2/source_model_depth_position_contract.hpp"
#include "halo2/source_world_depth_position_contract.hpp"
#include "halo2/source_world_depth_raster_contract.hpp"
#include "halo2/pstencil/geometry.hpp"
#include "halo2/model_selection.hpp"
#include <algorithm>
#include <cstring>
#include <set>

namespace halo2 {
namespace {
SourceBspDrawKey key(const Vertex& v) {
    return {v.material,v.lightmap,v.bsp_cluster,v.source_section,v.source_part};
}
bool bytes_equal(auto a,auto b) {
    return a.size_bytes()==b.size_bytes() && (!a.size_bytes() || !std::memcmp(a.data(),b.data(),a.size_bytes()));
}
std::array<std::uint16_t,3> oriented_triangle(std::array<std::uint16_t,3> vertices) {
    // The expanded model swaps the first two vertices of odd strip faces;
    // the raw draw swaps the last two. These are cyclic presentations of the
    // same oriented triangle. Keep the reverse winding a distinct identity.
    const std::array<std::uint16_t,3> second{vertices[1],vertices[2],vertices[0]},
        third{vertices[2],vertices[0],vertices[1]};
    return std::min(vertices,std::min(second,third));
}
void material_position(const Material& m) {
    // PRT's extended bank remains native; only the live position dependency
    // is admitted here. Color/texture/basis reads need not be dead.
    if(!m.forward_pass || !m.vertex_declaration || m.vertex_declaration->class_index!=0 ||
       (m.world_vertex_input_transform!=SourceWorldVertexInputTransform::Preserve &&
        m.world_vertex_input_transform!=SourceWorldVertexInputTransform::NormalizeSkinNormal) ||
       !source_world_depth_cull_admitted(m.forward_raster.cull) ||
       !source_model_depth_private::world_position_program_admitted(m.vertex_program))
        throw FormatError("model receiver depth lacks its exact native WORLD position program: "+m.name+
            " shader="+std::to_string(m.source_shader_datum)+" lod="+std::to_string(m.selected_shader_lod)+
            " class="+(m.vertex_declaration?std::to_string(m.vertex_declaration->class_index):"NONE")+
            " cull="+std::to_string(m.forward_raster.cull)+" transform="+
            std::to_string(unsigned(m.world_vertex_input_transform)));
}
void native_prefix(const Cache& cache,const Material& m,unsigned classification,const SourceVertexDeclaration& apply) {
    material_position(m);
    if(m.forward_pass_index>=m.passes.size())throw FormatError("model receiver depth ordinary pass has no actual Cache owner");
    const auto& pass=m.passes[m.forward_pass_index];
    const auto tag=cache.query_tag({.group=tag_groups::vertex_shader,.name=pass.vertex_shader,.require_address=true});
    if(!tag || cache.load_vertex_shader_code(tag->datum,0)!=m.vertex_program)
        throw FormatError("model receiver depth WORLD program differs from its actual Cache pass");
    const auto code=cache.load_vertex_shader_code(tag->datum,classification);
    if(classification && !source_model_depth_private::ordinary_position_program_admitted(code,classification))
        throw FormatError("model receiver depth ordinary raw skin prefix differs from Apply");
    const auto declaration=cache.load_source_vertex_declaration(tag->datum,classification);
    const auto mask=source_model_depth_private::position_input_mask(classification);
    if(!mask || declaration.class_index!=classification || apply.class_index!=classification)
        throw FormatError("model receiver depth native skin declaration lost its actual class");
    for(unsigned input=0;input<16;++input)if(mask&(1u<<input))
        if(input>=declaration.semantics.size() || input>=apply.semantics.size() || declaration.semantics[input]!=apply.semantics[input])
            throw FormatError("model receiver depth ordinary/Apply live skin inputs differ");
}
}
std::vector<SourceModelDepthSelection> source_model_depth_apply_selections_tu15(
    std::span<const SourceModelDepthSelection> selections,const SourceShadowRawPlan& plan) {
    using Part=std::tuple<std::uint32_t,std::uint32_t,unsigned,unsigned>;
    std::set<Part> receiving;std::set<std::pair<std::uint32_t,std::uint32_t>> objects;
    const auto authored=[&](std::uint32_t model,unsigned section,unsigned part)->const SourceShadowModelPart& {
        if(!plan.model_parts)throw FormatError("model depth Apply selection lacks its immutable authored part bank");
        const auto at=plan.model_parts->find({model,section,part});
        if(at==plan.model_parts->end())throw FormatError("model depth Apply selection escaped its authored part bank");
        return at->second;
    };
    const auto layers=[&](const auto& part,std::uint32_t shader,unsigned lod,std::uint32_t actual) {
        if(shader!=part.shader || lod>=part.template_lod_layers.size() || actual!=part.template_lod_layers[lod])
            throw FormatError("model depth Apply command lost its authored shader/template-LOD layers");
    };
    for(const auto& root:plan.roots)for(const auto& draw:root.receivers) {
        if(draw.command.phase!=SourceShadowPhase::Apply)throw FormatError("model depth selection requires actual Apply draws");
        const auto& binding=draw.source;const auto& owner=binding.owner();
        if(owner.kind!=MenuWorldSourceOwnerKind::dynamic_object)continue;
        if(!owner.generation || *owner.generation==~0u)throw FormatError("model depth Apply selection lost its actual object generation");
        const auto section=binding.draw().geometry->section(),part=binding.draw().part;
        layers(authored(owner.scope_datum,section,part),draw.command.command[2],
            draw.command.command[5]>>8&255u,draw.command.command[4]);
        if(!(draw.command.command[4]&0x800u))throw FormatError("model depth receiver contradicts its authored Apply layer");
        receiving.emplace(*owner.generation,owner.scope_datum,section,part);
        objects.emplace(*owner.generation,owner.scope_datum);
    }
    std::vector<SourceModelDepthSelection> result;
    for(const auto& selection:selections) {
        // Preserve the original caller's whole-object prefilter: objects with
        // no Apply draw anywhere are not shadow receivers in this frame.
        if(!objects.contains({selection.object,selection.render_model}))continue;
        if(!selection.model || selection.source_vertices.size()%3 || selection.first%3)
            throw FormatError("model depth Apply selection lost its ordinary triangle span");
        std::optional<std::size_t> begin;
        const auto finish=[&](std::size_t end) {
            if(!begin)return;auto run=selection;run.first+=*begin;
            run.source_vertices=selection.source_vertices.subspan(*begin,end-*begin);result.push_back(run);begin.reset();
        };
        std::map<std::pair<unsigned,unsigned>,bool> decisions;
        for(std::size_t n=0;n<selection.source_vertices.size();n+=3) {
            const auto& origins=selection.model->vertex_origins;const auto ordinal=selection.source_vertices[n];
            if(ordinal>=origins.size())throw FormatError("model depth Apply selection escaped its original vertex owner");
            const auto& origin=origins[ordinal];
            for(unsigned lane=1;lane<3;++lane) {
                const auto index=selection.source_vertices[n+lane];
                if(index>=origins.size() || origins[index].section!=origin.section || origins[index].part!=origin.part ||
                   origins[index].vertex_class!=origin.vertex_class)
                    throw FormatError("model depth Apply selection crosses authored part/class ownership");
            }
            auto [decision,fresh]=decisions.try_emplace({origin.section,origin.part},false);
            if(fresh) {
                const auto& part=authored(selection.render_model,origin.section,origin.part);
                const bool never=part.type==0 || part.type==5 || part.shader==~0u ||
                    std::none_of(part.template_lod_layers.begin(),part.template_lod_layers.end(),[](auto mask){return mask&0x800u;});
                const bool receiver=receiving.contains({selection.object,selection.render_model,origin.section,origin.part});
                if(receiver && never)throw FormatError("model depth receiver contradicts its authored non-Apply part");
                if(!receiver && !never) {
                    bool excluded=false;
                    for(const auto& root:plan.roots)for(const auto& command:root.model_commands)
                        if(command.object==selection.object && command.render_model==selection.render_model &&
                           command.section==origin.section && command.part==origin.part) {
                            layers(part,command.shader,command.shader_lod,command.layers);
                            if((command.layers&0x800u) && !command.empty_ranges)
                                throw FormatError("model receiver depth part lacks its actual Apply owner: object="+std::to_string(selection.object)+
                                    " model="+std::to_string(selection.render_model)+" section="+std::to_string(origin.section)+" part="+std::to_string(origin.part));
                            excluded=true;
                        }
                    if(!excluded)throw FormatError("model receiver depth part lacks its actual Apply exclusion owner: object="+
                        std::to_string(selection.object)+" model="+std::to_string(selection.render_model)+
                        " section="+std::to_string(origin.section)+" part="+std::to_string(origin.part));
                }
                decision->second=receiver;
            }
            if(decision->second){if(!begin)begin=n;}else finish(n);
        }
        finish(selection.source_vertices.size());
    }
    return result;
}
SourceModelDepthInput source_model_depth_input_tu15(const SourceWorldDepthInput& depth,
    const RendererTargets::Use& use,std::span<const Vertex> vertices,
    std::span<const SourceModelDepthSelection> selections,
    std::span<const SourceShadowRawDraw* const> receivers,std::span<const Material> materials) {
    const auto& raster=depth.raster(use);
    SourceModelDepthInput output(depth);output.vertices_.assign(vertices.begin(),vertices.end());
    std::vector<bool> owned(vertices.size());
    struct Raw {const SourceShadowRawDraw* draw;std::vector<Vec3> positions;
        std::vector<std::array<std::uint16_t,3>> triangles;};
    std::vector<Raw> raw;
    for(const auto* draw:receivers) {
        if(!draw || draw->command.phase!=SourceShadowPhase::Apply)
            throw FormatError("model receiver depth requires an actual Apply draw");
        const auto& owner=draw->source.owner();
        if(owner.kind!=MenuWorldSourceOwnerKind::dynamic_object)continue;
        if(std::none_of(selections.begin(),selections.end(),[&](const auto& selection) {
            return owner.generation==selection.object && owner.scope_datum==selection.render_model;
        }))continue;
        const auto& binding=draw->source;
        if(&draw->pass.cache_owner()!=&depth.cache_owner() || !owner.generation || *owner.generation==~0u || owner.ordinal!=(*owner.generation&65535u) ||
           owner.scope_datum!=binding.draw().geometry->owner_datum() ||
           binding.draw().geometry->kind()!=MenuWorldSourceSectionKind::render_model ||
           !(binding.declaration().class_index==0?
             source_world_depth_private::position_program_admitted(binding.program_bytes()):
             source_model_depth_private::position_program_admitted(binding.program_bytes(),binding.declaration().class_index)))
            throw FormatError("model receiver depth lost its complete native class/owner/program identity");
        if(!bytes_equal(std::span(binding.constants()).first<12>(),std::span(raster.camera)) ||
           !bytes_equal(std::span(draw->clip_range),std::span(raster.clip_range)) ||
           draw->surface_size!=raster.logical_surface_size || draw->raster.surface_scale!=raster.surface_scale ||
           !draw->controls.depth_clamp)
            throw FormatError("model receiver depth camera/clip differs from its completed main owner");
        Raw retained{draw,pstencil::complete_world_positions(binding),{}};
        auto authored=binding.draw();authored.range.reset();
        for(std::size_t n=0;n<authored.triangle_count();++n) {
            const auto triangle=authored.triangle(n);
            if(triangle[0]!=triangle[1] && triangle[0]!=triangle[2] && triangle[1]!=triangle[2])
                retained.triangles.push_back(triangle);
        }
        raw.push_back(std::move(retained));
        output.receivers_.push_back({draw,binding});
    }
    std::set<std::tuple<std::uint32_t,std::uint32_t,unsigned,std::size_t>> selected;
    std::set<std::pair<unsigned,unsigned>> checked_materials;
    std::map<const RenderModel*,std::vector<std::size_t>> primitive_ordinals;
    for(const auto& selection:selections) {
        if(selection.object==~0u || selection.definition==~0u || selection.render_model==~0u || selection.lod>=6 ||
           !selection.model || selection.source_vertices.empty() || selection.source_vertices.size()%3 || selection.first%3 ||
           selection.first>vertices.size() || selection.source_vertices.size()>vertices.size()-selection.first ||
           selection.model->vertex_origins.size()!=selection.model->mesh.vertices.size() ||
           selection.model->vertex_origins.size()%3 ||
           !selected.emplace(selection.object,selection.render_model,selection.lod,selection.first).second)
            throw FormatError("model receiver depth selected span lacks its full model/LOD/source-ordinal owner");
        const auto& model=*selection.model;
        auto [primitive,first_model]=primitive_ordinals.try_emplace(&model);
        if(first_model) {
            std::map<std::tuple<unsigned,unsigned,unsigned>,std::size_t> counts;
            primitive->second.reserve(model.vertex_origins.size()/3);
            for(std::size_t at=0;at<model.vertex_origins.size();at+=3) {
                const auto& origin=model.vertex_origins[at];
                primitive->second.push_back(counts[{origin.section,origin.part,origin.vertex_class}]++);
            }
        }
        if(depth.cache_owner().load_initial_model_definition(selection.definition).render_model!=selection.render_model)
            throw FormatError("model receiver depth definition differs from its actual Cache render-model owner");
        for(std::size_t n=0;n<selection.source_vertices.size();n+=3) {
            std::array<RenderModel::VertexOrigin,3> origin;
            for(unsigned lane=0;lane<3;++lane) {
                const auto ordinal=selection.source_vertices[n+lane];
                if(ordinal>=model.vertex_origins.size() || owned[selection.first+n+lane])
                    throw FormatError("model receiver depth source ordinal is foreign or overlaps another owner");
                origin[lane]=model.vertex_origins[ordinal];
                const auto& actual=vertices[selection.first+n+lane];const auto& source=model.mesh.vertices[ordinal];
                if(actual.bsp_cluster!=-1 || actual.source_section!=origin[lane].section ||
                   actual.source_part!=origin[lane].part || actual.material>=materials.size() ||
                   std::memcmp(&actual.source_local_position,&source.source_local_position,sizeof(Vec3)))
                    throw FormatError("model receiver depth packed vertex lost its authored source ordinal");
            }
            // Cache expands authored primitives in order, skipping only their
            // degenerate faces. Original ordinals retain this occurrence even
            // when a strip revisits the same three indices. A cyclicly equal
            // face has equal winding, but ordered W reconstruction still needs
            // the EXACT raw triangle vertex order, not just that equivalence.
            const auto source_triangle=selection.source_vertices[n]/3u;
            for(unsigned lane=1;lane<3;++lane)
                if(selection.source_vertices[n+lane]/3u!=source_triangle ||
                   key(vertices[selection.first+n+lane])!=key(vertices[selection.first+n]))
                    throw FormatError("model receiver depth triangle mixed original primitive/draw owners");
            const auto native_triangle=primitive->second.at(source_triangle);
            const Raw* match{};
            for(const auto& candidate:raw) {
                const auto& b=candidate.draw->source;const auto& d=b.draw();const auto& o=b.owner();
                if(*o.generation!=selection.object || o.scope_datum!=selection.render_model ||
                   d.geometry->section()!=origin[0].section || d.part!=origin[0].part ||
                   b.declaration().class_index!=origin[0].vertex_class)continue;
                if(origin[1].section!=origin[0].section || origin[2].section!=origin[0].section ||
                   origin[1].part!=origin[0].part || origin[2].part!=origin[0].part ||
                   origin[1].vertex_class!=origin[0].vertex_class || origin[2].vertex_class!=origin[0].vertex_class)
                    throw FormatError("model receiver depth triangle crosses original part/class ownership");
                if(native_triangle>=candidate.triangles.size() ||
                   oriented_triangle(candidate.triangles[native_triangle])!=
                       oriented_triangle({origin[0].vertex,origin[1].vertex,origin[2].vertex}))continue;
                if(match)for(unsigned lane=0;lane<3;++lane)
                    if(std::memcmp(&match->positions[origin[lane].vertex],&candidate.positions[origin[lane].vertex],sizeof(Vec3)))
                        throw FormatError("model receiver depth has conflicting actual native palettes");
                match=&candidate;
            }
            if(!match) {
                std::string candidates;
                unsigned count{};
                for(const auto &candidate:raw) {
                    const auto &binding=candidate.draw->source;const auto &owner=binding.owner();
                    if(owner.generation!=selection.object || owner.scope_datum!=selection.render_model)continue;
                    if(count++<6)candidates+=" [section="+std::to_string(binding.draw().geometry->section())+
                        " part="+std::to_string(binding.draw().part)+" class="+
                        std::to_string(binding.declaration().class_index)+" triangles="+
                        std::to_string(candidate.triangles.size())+" topology="+
                        std::to_string(unsigned(binding.draw().topology))+
                        " lod="+std::to_string((candidate.draw->command.command[5]>>8)&255u)+"]";
                }
                throw FormatError("model receiver depth triangle has no matching actual native Apply owner: object="+
                    std::to_string(selection.object)+" definition="+std::to_string(selection.definition)+
                    " model="+std::to_string(selection.render_model)+" lod="+std::to_string(selection.lod)+
                    " section="+std::to_string(origin[0].section)+" part="+std::to_string(origin[0].part)+
                    " class="+std::to_string(origin[0].vertex_class)+" triangle="+std::to_string(native_triangle)+
                    " vertices="+std::to_string(origin[0].vertex)+","+std::to_string(origin[1].vertex)+","+
                    std::to_string(origin[2].vertex)+" candidates="+std::to_string(count)+candidates);
            }
            const std::array<Vertex,3> triangle{vertices[selection.first+n],vertices[selection.first+n+1],vertices[selection.first+n+2]};
            for(unsigned lane=0;lane<3;++lane) {
                const auto at=selection.first+n+lane;const auto material=vertices[at].material;
                const auto& m=materials[material];
                const auto classification=match->draw->source.declaration().class_index;
                if(checked_materials.emplace(material,classification).second)
                    native_prefix(depth.cache_owner(),m,classification,match->draw->source.declaration());
                const auto shader=match->draw->source.draw().geometry->parts().at(origin[lane].part).shader;
                if(m.source_shader_datum!=shader)
                    throw FormatError("model receiver depth material differs from its actual source part shader");
                const auto retained=output.materials_.try_emplace(material,SourceModelDepthInput::MaterialOwner{&m,m.vertex_program,
                    m.source_shader_datum,m.selected_shader_lod,m.forward_raster,m.vertex_declaration,m.world_vertex_input_transform});
                retained.first->second.classes.try_emplace(classification,match->draw->source.declaration());
                const auto native_vertex=match->triangles[native_triangle][lane];
                unsigned source_lane=0;while(source_lane<3 && origin[source_lane].vertex!=native_vertex)++source_lane;
                if(source_lane==3)throw FormatError("model receiver depth lost its exact native triangle vertex");
                output.vertices_[at]=triangle[source_lane];
                output.vertices_[at].position=match->positions.at(native_vertex);
                owned[at]=true;++output.model_vertices_;
            }
        }
    }
    // Grouping may combine disjoint runs with the same material/part. Every
    // member of an admitted key must own its actual raw source, including
    // duplicated section selections; a matching torso cannot admit a weapon.
    std::map<SourceBspDrawKey,std::pair<std::size_t,std::size_t>> counts;
    for(std::size_t n=0;n<vertices.size();++n) {auto& count=counts[key(vertices[n])];++count.first;if(owned[n])++count.second;}
    for(const auto& [identity,count]:counts)if(count.second) {
        if(count.first!=count.second)throw FormatError("model receiver depth draw mixes owned and unowned source vertices");
        output.draws_.emplace(identity,count.first);
    }
    return output;
}
bool SourceModelDepthInput::admits_draw(const SourceBspDrawKey& identity)const {return draws_.contains(identity);}
void SourceModelDepthInput::validate_vertices(const RendererTargets::Use& use,std::span<const Vertex> actual)const {
    (void)depth_->raster(use);
    if(!bytes_equal(actual,std::span<const Vertex>(vertices_)))throw FormatError("model receiver depth submitted vertex bank changed");
    for(const auto& owner:receivers_) {
        const auto& current=owner.draw->source;const auto& original=owner.source;
        const auto& a=current.draw();const auto& b=original.draw();
        if(current.owner()!=original.owner() || current.declaration()!=original.declaration() ||
           !bytes_equal(current.program_bytes(),original.program_bytes()) ||
           !bytes_equal(std::span(current.constants()),std::span(original.constants())) ||
           a.geometry!=b.geometry || a.part!=b.part || a.topology!=b.topology || a.range!=b.range ||
           !bytes_equal(std::span(a.object_rows),std::span(b.object_rows)))
            throw FormatError("model receiver depth native geometry/palette/program changed after completion");
    }
    for(const auto& [slot,owner]:materials_) {
        (void)slot;material_position(*owner.material);
        const auto& m=*owner.material;
        if(m.vertex_program!=owner.program || m.source_shader_datum!=owner.shader || m.selected_shader_lod!=owner.lod ||
           m.forward_raster!=owner.raster || m.vertex_declaration!=owner.declaration || m.world_vertex_input_transform!=owner.transform)
            throw FormatError("model receiver depth source material changed");
    }
}
void SourceModelDepthInput::validate_receivers(const SourceShadowRawPlan& plan)const {
    for(const auto& owner:receivers_) {
        bool present=false;
        for(const auto& root:plan.roots)for(const auto& draw:root.receivers)if(&draw==owner.draw) {
            validate_source_shadow_raw_draw(draw,SourceShadowPhase::Apply,root.uniforms);present=true;
        }
        if(!present)throw FormatError("model receiver depth is foreign to this actual raw Apply plan");
    }
}
void SourceModelDepthInput::validate_draw(const RendererTargets::Use& use,const SourceBspDrawKey& identity,std::size_t count)const {
    (void)depth_->raster(use);const auto found=draws_.find(identity);
    if(found==draws_.end() || found->second!=count)throw FormatError("model receiver depth final native draw range changed");
}
bool SourceModelDepthInput::admits_material(unsigned root,const Material& actual)const {
    const auto found=materials_.find(root);if(found==materials_.end())return false;
    if(&actual==found->second.material && actual.source_shader_datum!=found->second.shader)
        throw FormatError("model receiver depth expanded material lost its actual root shader");
    for(const auto& [classification,declaration]:found->second.classes)
        native_prefix(depth_->cache_owner(),actual,classification,declaration);
    return true;
}
}
