#include "halo2/source_hologram_depth_input.hpp"
#include "halo2/source_model_depth_position_contract.hpp"
#include "halo2/source_world_depth_position_contract.hpp"
#include "halo2/pstencil/geometry.hpp"
#include "halo2/model.hpp"
#include "halo2/render_view.hpp"
#include <algorithm>
#include <cstring>
#include <set>

namespace halo2 {
namespace {
bool bytes_equal(auto a,auto b) {
    return a.size_bytes()==b.size_bytes() &&
        (!a.size_bytes() || !std::memcmp(a.data(),b.data(),a.size_bytes()));
}
SourceBspDrawKey key(const Vertex& v) {
    return {v.material,v.lightmap,v.bsp_cluster,v.source_section,v.source_part};
}
std::array<std::uint16_t,3> oriented(std::array<std::uint16_t,3> a) {
    return std::min({a,std::array<std::uint16_t,3>{a[1],a[2],a[0]},
        std::array<std::uint16_t,3>{a[2],a[0],a[1]}});
}
MenuWorldSourceBinding native_body(const SourceHologramGroupDraw& group,
    const SourceHologramGroupPart& part) {
    if(!group.cache || !part.pass || &part.pass->cache_owner()!=group.cache || !part.body_source ||
       !part.body_geometry || part.layer==21 || part.object==~0u || part.geometry_lod>=6 ||
       part.body_source->range || !part.body_source->geometry ||
       part.body_source->geometry->kind()!=MenuWorldSourceSectionKind::render_model ||
       part.body_source->geometry->section()!=part.section || part.body_source->part!=part.part ||
       part.pass->choice().layer!=part.layer)
        throw FormatError("hologram depth body lost its exact Cache/pass/LOD/geometry owner");
    const auto& geometry=*part.body_source->geometry;
    if(part.part>=geometry.parts().size() || geometry.parts()[part.part].shader!=part.pass->shader())
        throw FormatError("hologram depth body pass differs from its actual source part shader");
    if(&part.body_geometry->cache_owner()!=group.cache ||
       part.body_geometry->render_model()!=geometry.owner_datum() ||
       part.body_geometry->section()!=part.section || part.body_geometry->lod()!=part.geometry_lod)
        throw FormatError("hologram depth source ordinal witness belongs to another Cache/model/LOD/section");
    auto binding=menu_world_source_binding_class(*group.cache,
        {MenuWorldSourceOwnerKind::dynamic_object,geometry.owner_datum(),part.object&65535u,part.object},
        *part.body_source,part.pass->vertex_shader(),part.vertex_class,group.constants);
    if(!source_model_depth_private::ordinary_position_program_admitted(binding.program_bytes(),part.vertex_class) ||
       !source_world_depth_private::position_program_admitted(
           group.cache->load_vertex_shader_code(part.pass->vertex_shader(),0)))
        throw FormatError("hologram depth lacks its complete original skin/WORLD position proof");
    return binding;
}
}
SourceHologramDepthInput source_hologram_depth_input(const SourceHologramGroupDraw& group,
    std::uint32_t object,std::span<const Vertex> input,std::array<std::uint32_t,2> surface) {
    if(!group.cache || !group.camera_owner || !group.camera_owner->matches(std::span(group.constants).first<12>()) ||
       group.command.callback!=0x423b0u || object==~0u || input.empty() || input.size()%3 ||
       std::ranges::count(group.owners,object)!=1 || group.raster.window_exclusive ||
       group.raster.depth_factor_offset!=std::array<float,2>{0,0} ||
       group.raster.surface_scale!=std::array<std::int32_t,2>{1,1} || !surface[0] || !surface[1] ||
       group.raster.stencil.enabled || group.clip_range[0]!=0 || group.clip_range[1]!=16777215.f ||
       !(group.surface_size[0]>0) || !(group.surface_size[1]>0) ||
       !std::ranges::any_of(group.raster.regions,[&](const auto& r) {
           return r[0]<=0 && r[1]<=0 && r[2]>=group.surface_size[0] && r[3]>=group.surface_size[1];
       }))
        throw FormatError("hologram depth requires its complete current group/body/raster owner");
    SourceHologramDepthInput out(group,object,surface);out.vertices_.assign(input.begin(),input.end());
    out.source_vertices_.resize(input.size(),~0u);
    std::vector<bool> claimed(input.size());
    std::set<std::pair<unsigned,unsigned>> parts;
    for(const auto& part:group.parts) {
        if(part.layer==21 || part.object!=object)continue;
        const auto binding=native_body(group,part);
        for(const auto& overlay:group.parts)if(overlay.layer==21 && overlay.object==part.object &&
            overlay.section==part.section && overlay.part==part.part) {
            for(const auto& range:overlay.hologram_ranges)
                if(overlay.vertex_class!=part.vertex_class || range.geometry!=part.body_source->geometry ||
                   range.part!=part.part ||
                   !bytes_equal(std::span(range.object_rows),std::span(part.body_source->object_rows)))
                    throw FormatError("hologram depth body and override lost their common source geometry/palette/class");
        }
        const auto positions=pstencil::complete_world_positions(binding);
        const auto& model=part.body_geometry->model();
        if(model.vertex_origins.size()!=model.mesh.vertices.size() || model.vertex_origins.size()%3)
            throw FormatError("hologram depth original primitive witness is incomplete");
        std::vector<std::size_t> ordinals,actual;
        for(std::size_t n=0;n<model.vertex_origins.size();++n) {
            const auto& o=model.vertex_origins[n];
            if(o.section==part.section && o.part==part.part) {
                if(o.vertex_class!=part.vertex_class)
                    throw FormatError("hologram depth source declaration differs from its original vertex ordinal");
                ordinals.push_back(n);
            }
        }
        for(std::size_t n=0;n<input.size();++n)
            if(input[n].source_section==part.section && input[n].source_part==part.part)actual.push_back(n);
        if(actual.size()!=ordinals.size() || actual.size()!=part.body_vertices || actual.size()%3)
            throw FormatError("hologram depth body lost its exact whole-part primitive occurrence count");
        std::vector<std::array<std::uint16_t,3>> triangles;
        for(std::size_t n=0;n<binding.draw().triangle_count();++n) {
            const auto t=binding.draw().triangle(n);
            if(t[0]!=t[1] && t[0]!=t[2] && t[1]!=t[2])triangles.push_back(t);
        }
        if(triangles.size()*3!=actual.size())
            throw FormatError("hologram depth raw topology differs from its source primitive witness");
        const bool first=parts.emplace(part.section,part.part).second;
        for(std::size_t n=0;n<actual.size();n+=3) {
            std::array<std::uint16_t,3> source;
            for(unsigned lane=0;lane<3;++lane) {
                const auto at=actual[n+lane],ordinal=ordinals[n+lane];
                const auto& v=input[at];const auto& origin=model.vertex_origins[ordinal];
                if(at%3!=lane || ordinal/3!=ordinals[n]/3 || v.bsp_cluster!=-1 ||
                   key(v)!=key(input[actual[n]]) ||
                   std::memcmp(&v.source_local_position,&model.mesh.vertices[ordinal].source_local_position,sizeof(Vec3)))
                    throw FormatError("hologram depth CPU input lost its original primitive/vertex owner");
                source[lane]=origin.vertex;
            }
            const auto native=triangles[n/3];
            if(oriented(source)!=oriented(native))
                throw FormatError("hologram depth changed an original primitive occurrence or winding");
            for(unsigned lane=0;lane<3;++lane) {
                const auto from=std::find(source.begin(),source.end(),native[lane]);
                if(from==source.end())throw FormatError("hologram depth original vertex ordinal disappeared");
                auto vertex=input[actual[n+std::size_t(from-source.begin())]];
                vertex.position=positions.at(native[lane]);
                auto& target=out.vertices_[actual[n+lane]];
                if(first) {
                    if(claimed[actual[n+lane]])throw FormatError("hologram depth body primitive owners overlap");
                    target=vertex;out.source_vertices_[actual[n+lane]]=native[lane];claimed[actual[n+lane]]=true;
                } else if(std::memcmp(&target,&vertex,sizeof(Vertex)))
                    throw FormatError("hologram depth body passes disagree on exact native pose/primitive inputs");
            }
        }
    }
    if(std::ranges::find(claimed,false)!=claimed.end())
        throw FormatError("hologram depth body contains geometry absent from its actual source group");
    for(const auto& vertex:out.vertices_)++out.draws_[key(vertex)];
    return out;
}
bool SourceHologramDepthInput::admits_draw(const SourceBspDrawKey& identity)const {
    return draws_.contains(identity);
}
void SourceHologramDepthInput::validate_vertices(std::span<const Vertex> actual)const {
    validate_group(*group_);
    if(!bytes_equal(actual,std::span<const Vertex>(vertices_)))
        throw FormatError("hologram depth uploaded body geometry changed after admission");
}
void SourceHologramDepthInput::validate_group(const SourceHologramGroupDraw& group)const {
    if(&group!=group_ || group.cache!=expected_.cache || group.camera_owner!=expected_.camera_owner ||
       !group.camera_owner || !group.camera_owner->matches(std::span(group.constants).first<12>()) ||
       group.owners!=expected_.owners ||
       group.command.callback!=expected_.command.callback || group.command.priority!=expected_.command.priority ||
       group.command.payload!=expected_.command.payload ||
       !bytes_equal(std::span(&group.command.depth,1),std::span(&expected_.command.depth,1)) ||
       !bytes_equal(std::span(&group.command.center,1),std::span(&expected_.command.center,1)) ||
       !bytes_equal(std::span(group.constants),std::span(expected_.constants)) ||
       !bytes_equal(std::span(&group.shader_clock,1),std::span(&expected_.shader_clock,1)) ||
       !bytes_equal(std::span(group.clip_range),std::span(expected_.clip_range)) ||
       !bytes_equal(std::span(group.fog_parameters),std::span(expected_.fog_parameters)) ||
       !bytes_equal(std::span(group.surface_size),std::span(expected_.surface_size)) ||
       group.raster.depth_factor_offset!=expected_.raster.depth_factor_offset ||
       group.raster.surface_scale!=expected_.raster.surface_scale ||
       group.raster.regions!=expected_.raster.regions ||
       group.raster.window_exclusive!=expected_.raster.window_exclusive ||
       group.raster.source_stencil_mode!=expected_.raster.source_stencil_mode ||
       group.raster.stencil.enabled!=expected_.raster.stencil.enabled ||
       group.raster.stencil.values!=expected_.raster.stencil.values ||
       group.parts.size()!=expected_.parts.size())
        throw FormatError("hologram depth is foreign to the current group/camera/raster scope");
    for(std::size_t n=0;n<group.parts.size();++n) {
        const auto& a=group.parts[n];const auto& b=expected_.parts[n];
        if(a.object!=b.object || a.section!=b.section || a.part!=b.part || a.layer!=b.layer ||
           a.vertex_class!=b.vertex_class || a.command!=b.command || a.sorted!=b.sorted ||
           a.base_ordinal!=b.base_ordinal || a.geometry_lod!=b.geometry_lod || a.body_vertices!=b.body_vertices ||
           a.pass!=b.pass || a.body_geometry!=b.body_geometry || bool(a.body_source)!=bool(b.body_source) ||
           a.hologram_ranges.size()!=b.hologram_ranges.size())
            throw FormatError("hologram depth current descriptor/pass/origin owner changed");
        if(a.body_source) {
            const auto& x=*a.body_source;const auto& y=*b.body_source;
            if(x.geometry!=y.geometry || x.part!=y.part || x.topology!=y.topology || x.range!=y.range ||
               !bytes_equal(std::span(x.object_rows),std::span(y.object_rows)))
                throw FormatError("hologram depth current source geometry/palette changed");
        }
        for(std::size_t r=0;r<a.hologram_ranges.size();++r) {
            const auto& x=a.hologram_ranges[r];const auto& y=b.hologram_ranges[r];
            if(x.geometry!=y.geometry || x.part!=y.part || x.topology!=y.topology || x.range!=y.range ||
               !bytes_equal(std::span(x.object_rows),std::span(y.object_rows)))
                throw FormatError("hologram depth current override geometry/palette/range changed");
        }
    }
}
void SourceHologramDepthInput::validate_draw(const SourceHologramGroupDraw& group,
    const SourceHologramGroupPart& part,const SourceBspDrawKey& identity,std::size_t count,
    const Material& material,const WorldMaterialUniform& uniform,const PackedVertexProgramBank& bank,
    ShaderVector forward,ShaderVector eye,ShaderVector viewport,
    const ForwardRasterState& actual_state,unsigned actual_blend,bool actual_stencil)const {
    validate_group(group);
    const auto& camera=group.camera_owner->pose();
    const ShaderVector expected_forward{camera.forward.x,camera.forward.y,camera.forward.z,1};
    const ShaderVector expected_eye{camera.position.x,camera.position.y,camera.position.z,1};
    if(!bytes_equal(std::span(forward),std::span(expected_forward)) ||
       !bytes_equal(std::span(eye),std::span(expected_eye)))
        throw FormatError("hologram depth actual camera push differs from its submitted raw pose/ordinary selectors");
    const auto found=draws_.find(identity);
    if(found==draws_.end() || found->second!=count || part.object!=object_ || part.section!=identity.section ||
       part.part!=identity.part || part.layer==21 || !part.pass ||
       std::none_of(group.parts.begin(),group.parts.end(),[&](const auto& p){return &p==&part;}))
        throw FormatError("hologram depth final body draw escaped its source descriptor/geometry range");
    const auto& pass=*part.pass;
    if(!material.forward_pass || !material.vertex_declaration || material.vertex_declaration->class_index!=0 ||
       (material.world_vertex_input_transform!=SourceWorldVertexInputTransform::Preserve &&
        material.world_vertex_input_transform!=SourceWorldVertexInputTransform::NormalizeSkinNormal) ||
       material.source_shader_datum!=pass.shader() || material.selected_shader_lod!=pass.choice().shader_lod ||
       material.forward_pass_index>=material.passes.size() ||
       material.passes[material.forward_pass_index].template_pass!=pass.template_pass() ||
       material.passes[material.forward_pass_index].implementation!=pass.choice().implementation ||
       material.vertex_program!=group.cache->load_vertex_shader_code(pass.vertex_shader(),0) ||
       !source_world_depth_private::position_program_admitted(material.vertex_program) ||
       actual_state!=material.forward_raster || actual_blend!=material.forward_blend ||
       actual_stencil!=group.raster.stencil.enabled || actual_state.depth_enable!=2 ||
       (actual_state.cull!=0 && actual_state.cull!=0x900 && actual_state.cull!=0x901) ||
       uniform.source_camera_control[0]!=1 || viewport[0]!=0 || viewport[1]!=0 ||
       viewport[2]!=float(physical_surface_[0]) || viewport[3]!=float(physical_surface_[1]))
        throw FormatError("hologram depth final native material/pass/state/camera differs from its source owner");
    auto expected=menu_world_source_resized_viewport(native_body(group,part),
        {viewport[2]/group.surface_size[0],viewport[3]/group.surface_size[1]});
    if(bank.range[3])
        throw FormatError("hologram depth body bank overrides its original main camera");
    const auto actual_viewport=viewport_constants({viewport[0],viewport[1],viewport[2],viewport[3]});
    if(!bytes_equal(std::span(uniform.source_camera_world),std::span(expected.constants()).first<7>()) ||
       !bytes_equal(std::span(actual_viewport),std::span(expected.constants()).subspan<8,4>()))
        throw FormatError("hologram depth actual body/view projection differs from its native group");
}
}
