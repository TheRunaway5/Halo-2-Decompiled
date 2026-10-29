#include "halo2/source_world_depth_input.hpp"
#include "halo2/source_world_depth_position_contract.hpp"
#include "halo2/source_world_depth_scope.hpp"
#include "halo2/source_world_depth_raster_contract.hpp"
#include "halo2/menu_world_source_pass.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_shader_lod.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/pstencil/detail.hpp"
#include "halo2/pstencil/environment.hpp"
#include <vulkan/vulkan_core.h>
#include <type_traits>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <map>
#include <set>

namespace halo2 {
namespace {
template<class T>T read(const SourceRuntimeMemory& memory,std::uint32_t address) {
    T result;std::memcpy(&result,memory.view(address,sizeof(result)).data(),sizeof(result));return result;
}
template<class T>bool exact(const T& a,const T& b) {return std::memcmp(&a,&b,sizeof(T))==0;}
bool texture_metadata_exact(const Texture& a,const Texture& b) {
    if(a.width!=b.width || a.height!=b.height || a.depth!=b.depth || a.layers!=b.layers ||
       a.format!=b.format || a.mips.size()!=b.mips.size())return false;
    for(unsigned i=0;i<a.mips.size();++i) {
        const auto& x=a.mips[i];const auto& y=b.mips[i];
        if(x.width!=y.width || x.height!=y.height || x.depth!=y.depth || x.offset!=y.offset || x.size!=y.size)return false;
    }return true;
}
bool texture_exact(const Texture& a,const Texture& b) {
    return texture_metadata_exact(a,b) && a.pixels==b.pixels;
}
bool bitmap_animations_exact(std::span<const BitmapFrameAnimation> a,
    std::span<const BitmapFrameAnimation> b) {
    if(a.size()!=b.size())return false;
    for(unsigned n=0;n<a.size();++n) {
        const auto& x=a[n];const auto& y=b[n];
        if(x.stage!=y.stage || x.image_count!=y.image_count || x.tables!=y.tables ||
           x.function.input!=y.function.input || x.function.range!=y.function.range ||
           !exact(x.function.period,y.function.period) || x.function.curve!=y.function.curve)return false;
    }
    return true;
}

// Same physical-surface admission as the immutable live-memory branch. Logical
// completed-camera authority alone cannot stand in for initialized attachments.
void completed_attachments(const RendererTargets::Use& use) {
    if(!use.valid() || use.device_address().is_null())
        throw FormatError("completed main depth requires a real Renderer target use");
    const auto& bank=use.bank();
    const auto valid=[](const RendererTargetImage& image,std::uint32_t format,std::uint32_t usage) {
        return image.image && image.view && !image.image_backing.is_null() && !image.view_backing.is_null() &&
            image.format==format && image.width>0 && image.height>0 && image.samples==1 &&
            image.mips==1 && image.layers==1 && (image.usage&usage)==usage;
    };
    if(!bank.generation || bank.color.empty() ||
       !valid(bank.depth_stencil,VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ||
       std::any_of(bank.color.begin(),bank.color.end(),[&](const auto& image) {
           return !valid(image,VK_FORMAT_B8G8R8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
       }))
        throw FormatError("completed main depth requires actual single-sample BGRA8/integerD24S8 attachments");
}
std::array<ShaderVector,12> completed_camera(const SourceMainCameraOperandsTu15& data,
    const RendererTargets::Use& use) {
    completed_attachments(use);
    if(data.view>=4 || data.local_players<0 || data.local_players>1 || data.submitting_mode ||
       data.submitting_scaled || data.source_aa[0] || data.source_aa[1] ||
       data.viewport[0]!=0 || data.viewport[1]!=0 || data.viewport[2]<=0 || data.viewport[3]<=0 ||
       std::any_of(data.timed_effect_overrides.begin(),data.timed_effect_overrides.end(),[](auto word){return word!=0;}))
        throw FormatError("completed main depth lacks its ordinary full viewport/AA/timed-boundary scope");
    if(!data.timed_effect_source || data.timed_effect_source==0xffffffffu || !data.game_arena_source ||
       data.timed_effect_source<data.game_arena_source || data.timed_effect_backing.is_null() ||
       std::uint64_t(data.timed_effect_source)+0x3fcu>std::uint64_t(data.game_arena_source)+data.game_arena_used)
        throw FormatError("completed main depth lost its issued native timed-effect owner");
    // Global camera aliases are the COMPLETED main words, not mutable late-FP
    // globals. The opaque issuer proved the corresponding submitting aliases.
    if(std::memcmp(reinterpret_cast<const std::byte*>(data.raw_camera_words.data())+0x30,
                   data.viewport.data(),sizeof(data.viewport)) ||
       std::memcmp(reinterpret_cast<const std::byte*>(data.raw_camera_words.data())+0x40,
                   data.near_far_words.data(),sizeof(data.near_far_words)))
        throw FormatError("completed main depth retained raw viewport/plane aliases differ");
    for(unsigned n=0;n<9;++n)if(!std::isfinite(std::bit_cast<float>(data.raw_camera_words[n])))
        throw FormatError("completed main depth retained camera vectors are not finite");
    for(auto word:data.projection_words)if(!std::isfinite(std::bit_cast<float>(word)))
        throw FormatError("completed main depth retained projection is not finite");
    SourceViewProjectionBank bank;
    std::memcpy(&bank,data.projection_words.data(),sizeof(bank));
    const Vec3 eye{std::bit_cast<float>(data.raw_camera_words[0]),
        std::bit_cast<float>(data.raw_camera_words[1]),std::bit_cast<float>(data.raw_camera_words[2])};
    const float near=std::bit_cast<float>(data.near_far_words[0]),far=std::bit_cast<float>(data.near_far_words[1]);
    if(!std::isfinite(near) || !std::isfinite(far) || near<=0 || far<=near || !std::isfinite(1.f/far) ||
       !exact(bank.camera_to_world.position,eye))
        throw FormatError("completed main depth retained projection/eye/planes differ");
    const auto camera=source_view_shader_bank(bank,far,eye,
        RenderViewport{0,0,float(data.viewport[3]),float(data.viewport[2]),1,false,false,0},false);
    static_assert(sizeof(camera)==sizeof(data.camera_words));
    for(const auto& vector:camera)for(float f:vector)if(!std::isfinite(f))
        throw FormatError("completed main depth retained camera bank is not finite");
    if(std::memcmp(camera.data(),data.camera_words.data(),sizeof(camera)))
        throw FormatError("completed main depth camera bank differs from unchanged source products");
    // Original1345B..134BA ordinary boundary0, ordered f32 DIV/MUL/MUL.
    const float reciprocal=1.f/far,maximum=16777215.f;
    const std::array<float,4> clip{0,maximum,std::clamp((reciprocal*near)*maximum,0.f,maximum),
        std::clamp((reciprocal*far)*maximum,0.f,maximum)};
    if(std::memcmp(clip.data(),data.clip_words.data(),sizeof(clip)))
        throw FormatError("completed main depth clip bank differs from original ordered operands");
    return camera;
}
using GeometryKey=std::tuple<int,unsigned,unsigned,unsigned>;
GeometryKey key(const Vertex& v){return {v.bsp_cluster,v.source_section,v.source_part,v.material};}
GeometryKey key(const SourceBspShaderPart& p){return {p.geometry,p.section,p.part,p.material};}
using Identity=SourceWorldDepthMaterialIdentity;
std::map<Identity,const Material*> selected_materials(const Mesh& mesh,
    std::span<const SourceBspShaderPart> parts) {
    std::set<unsigned> roots;
    for(const auto& part:parts) {
        if(part.material>=mesh.materials.size() || !part.instance_alpha ||
           part.shader_lod!=mesh.materials[part.material].selected_shader_lod)
            throw FormatError("ordinary depth selected part has unowned material/LOD/fade input");
        roots.insert(part.material);
    }
    std::map<Identity,const Material*> result;
    for(unsigned root:roots) {
        const auto& m=mesh.materials[root];result.emplace(Identity{root,"root"},&m);
        const auto add=[&](const std::shared_ptr<Material>& child,std::string path) {
            if(!child)throw FormatError("ordinary depth has a null selected child owner");
            if(!result.emplace(Identity{root,std::move(path)},child.get()).second)
                throw FormatError("ordinary depth has duplicate child identity");
        };
        if(m.environment_pass)add(m.environment_pass,"environment");
        for(unsigned n=0;n<m.world_overlays_before.size();++n)add(m.world_overlays_before[n],"before"+std::to_string(n));
        for(unsigned n=0;n<m.world_overlays_after.size();++n)add(m.world_overlays_after[n],"after"+std::to_string(n));
        for(unsigned n=0;n<m.world_bloom_passes.size();++n)add(m.world_bloom_passes[n],"bloom"+std::to_string(n));
        if(m.planar_fog_pass)add(m.planar_fog_pass,"planar");
        for(unsigned n=0;n<3;++n)if(m.world_planar_passes[n])add(m.world_planar_passes[n],"world_planar"+std::to_string(n));
    }
    return result;
}
void state_ranges(const Cache& cache,const MenuWorldSourcePass& source,std::uint32_t expected_cull) {
    const Bytes impl(source.implementation_record());
    if(ShaderRange{impl.u16(0x11e)}.count())
        throw FormatError("ordinary depth selected state extern lacks an original owner");
    const auto block=read_tag_block_descriptor(
        Bytes(cache.read_tag_bytes(source.pass_postprocess_address()+16,8)),0);
    const auto bytes=block.count?cache.read_tag_bytes(block.address,block.extent(5,65536)):
        std::span<const std::byte>{};
    for(unsigned offset:{2u,0x11cu}) {
        //16B70 applies current+2 after restoring previous+11C. They are
        // distinct lifetimes: a two-sided draw must not use its retirement900.
        SourceWorldDepthCullState cull(offset==2u?0x900u:expected_cull);
        const ShaderRange range{impl.u16(offset)};
        const auto count=std::size_t(block.count);
        if(range.first()>count || range.count()>count-range.first())
            throw FormatError("ordinary depth original state range escapes its Cache owner");
        for(unsigned n=0;n<range.count();++n) {
            const auto at=(range.first()+n)*5;
            const unsigned state=std::to_integer<unsigned>(bytes[at]);
            if(!cull.apply(state,Bytes(bytes).u32(at+1)))
                throw FormatError("ordinary depth original pass changes front/cull/bias/AA contract");
        }
        if(cull.cull()!=(offset==2u?expected_cull:0x900u))
            throw FormatError("ordinary depth original apply/restore cull differs from its completed material");
    }
}
void authored_pass(const Cache& cache,const Material& root,const Material& material) {
    if(!material.forward_pass || material.forward_pass_index>=material.passes.size() ||
       !material.vertex_declaration || material.vertex_declaration->class_index!=0 ||
       material.vertex_declaration->semantics.empty() || material.vertex_declaration->semantics[0]!=0 ||
       !source_world_vertex_declaration_supported(*material.vertex_declaration) || material.vertex_bank ||
       material.world_vertex_input_transform!=SourceWorldVertexInputTransform::Preserve ||
       !source_world_depth_cull_admitted(material.forward_raster.cull) ||
       !source_world_depth_private::position_program_admitted(material.vertex_program))
        throw FormatError("ordinary depth material lacks its admitted original class0 position/input contract");
    const auto program=decode_vertex_program(material.vertex_program);
    if(program.relative_constants)throw FormatError("ordinary depth cannot borrow relative constants");
    const auto& pass=material.passes.at(material.forward_pass_index);
    if(pass.vertex_code!=material.vertex_program || !pass.state_externs.empty())
        throw FormatError("ordinary depth lost its actual selected original pass/extern identity");
    SourceWorldDepthCullState completed_cull;
    for(const auto& state:pass.states)
        if(!completed_cull.apply(state.index,state.value))
            throw FormatError("ordinary depth completed pass changes front/cull/bias/AA contract");
    if(completed_cull.cull()!=material.forward_raster.cull)
        throw FormatError("ordinary depth completed pass cull differs from its authored material");
    const auto authored=cache.load_material(root.source_shader_datum);
    if(root.selected_shader_lod>=authored.lods.size())throw FormatError("ordinary depth main LOD is outside original Cache");
    const auto& lod=authored.lods[root.selected_shader_lod];bool matched=false;
    for(unsigned layer=0;layer<lod.layers.count();++layer) {
        if(!(lod.available_layers&(1u<<layer)))continue;
        const auto range=authored.layers.at(lod.layers.first()+layer);
        if(pass.template_pass<range.first() || pass.template_pass-range.first()>=range.count())continue;
        const auto source=load_menu_world_source_pass(cache,root.source_shader_datum,
            {root.selected_shader_lod,layer,unsigned(pass.template_pass-range.first()),pass.implementation});
        if(cache.load_vertex_shader_code(source.vertex_shader(),0)!=material.vertex_program)
            throw FormatError("ordinary depth compiled tokens differ from actual Cache-selected pass");
        state_ranges(cache,source,material.forward_raster.cull);matched=true;
    }
    if(!matched)throw FormatError("ordinary depth selected child cannot join original main LOD/pass");
}
}
SourceWorldDepthInput source_world_depth_input_tu15(const SourceRuntimeMemory& memory,
    const RendererTargets::Use& use,const Cache& cache,const Mesh& mesh,
    const SourceBspResidentPlan& resident,const SourceBspResidentViewPlan& selection,
    std::array<std::uint32_t,2> logical_surface) {
    const SourceLiveShadowPhaseResidueTu15 unused{};
    SourceWorldDepthInput output{source_live_shadow_phase_inputs_tu15(memory,use,unused,logical_surface)};
    output.raster_.camera=output.camera(use);
    output.memory_=&memory;output.eye_=read<std::array<float,3>>(memory,0x4c65c0);
    output.forward_=read<std::array<float,3>>(memory,0x4c65cc);
    output.up_=read<std::array<float,3>>(memory,0x4c65d8);
    const auto planes=read<std::array<float,2>>(memory,0x4922f4);
    const float reciprocal=1.f/planes[1],maximum=16777215.f;
    output.raster_.clip_range={0,maximum,std::clamp((reciprocal*planes[0])*maximum,0.f,maximum),
        std::clamp((reciprocal*planes[1])*maximum,0.f,maximum)};
    const auto viewport=read<std::array<std::int16_t,4>>(memory,0x4c65f0u);
    output.raster_.logical_surface_size={float(viewport[3]-viewport[1]),float(viewport[2]-viewport[0])};
    return SourceWorldDepthInput::complete_owners(std::move(output),use,cache,mesh,resident,selection);
}
SourceWorldDepthInput source_world_depth_input_completed_main_tu15(
    SourceMainCameraRasterUseTu15 checked_use,const RendererTargets::Use& use,const Cache& cache,
    const Mesh& mesh,const SourceBspResidentPlan& resident,const SourceBspResidentViewPlan& selection) {
    // Own a weak, checked consumer capability, never a raw pointer to the
    // callback token. Every subsequent validation rechecks actual borrow life.
    SourceWorldDepthInput output{std::move(checked_use)};
    const auto& data=std::get<SourceMainCameraRasterUseTu15>(output.camera_owner_).operands();
    output.raster_.camera=completed_camera(data,use);
    for(unsigned n=0;n<3;++n) {
        output.eye_[n]=std::bit_cast<float>(data.raw_camera_words[n]);
        output.forward_[n]=std::bit_cast<float>(data.raw_camera_words[n+3]);
        output.up_[n]=std::bit_cast<float>(data.raw_camera_words[n+6]);
    }
    std::memcpy(output.raster_.clip_range.data(),data.clip_words.data(),sizeof(data.clip_words));
    output.raster_.logical_surface_size={float(data.viewport[3]-data.viewport[1]),float(data.viewport[2]-data.viewport[0])};
    return SourceWorldDepthInput::complete_owners(std::move(output),use,cache,mesh,resident,selection);
}
std::array<ShaderVector,12> SourceWorldDepthInput::camera(const RendererTargets::Use& use)const {
    return std::visit([&](const auto& owner) {
        if constexpr(std::is_same_v<std::decay_t<decltype(owner)>,SourceLiveShadowPhaseInputsTu15>)
            return owner.camera(use);
        else return completed_camera(owner.operands(),use);
    },camera_owner_);
}
SourceWorldDepthInput SourceWorldDepthInput::complete_owners(SourceWorldDepthInput output,
    const RendererTargets::Use& use,const Cache& cache,const Mesh& mesh,
    const SourceBspResidentPlan& resident,const SourceBspResidentViewPlan& selection) {
    output.cache_=&cache;
    output.mesh_=&mesh;output.generation_=use.bank().generation;output.device_=use.device_address();
    auto parts=std::span(selection.selected_parts);
    output.parts_.assign(parts.begin(),parts.end());output.resident_=&resident;
    output.resident_atlas_=resident.resident_vertices();
    output.bindings_.assign(selection.bindings.begin(),selection.bindings.end());
    output.expected_bsp_=resident.merge(output.bindings_,{},unsigned(mesh.materials.size()),
        unsigned(mesh.lightmaps.size()+1));
    output.lightmaps_=mesh.lightmaps;

    // Independent water owns its separate source program, reflection/copy
    // inputs and later schedule. Retain its exact authored role and geometry
    // rather than routing it through the ordinary position/depth adapter.
    std::set<GeometryKey> independent;
    std::set<unsigned> water_roots;
    for(const auto& part:output.parts_) {
        const auto& material=mesh.materials.at(part.material);
        if(!source_world_depth_independent_water(material))continue;
        if(!part.instance_alpha || part.shader_lod!=material.selected_shader_lod)
            throw FormatError("independent water part changed its selected source LOD/fade owner");
        independent.insert(key(part));
        output.independent_parts_.push_back(part);
        if(!water_roots.insert(part.material).second)continue;
        const auto& pass=material.passes.at(material.forward_pass_index);
        const auto authored=cache.load_material(material.source_shader_datum);
        const auto actual=std::find_if(authored.passes.begin(),authored.passes.end(),[&](const auto& candidate) {
            return candidate.template_pass==pass.template_pass && candidate.implementation==pass.implementation;
        });
        if(actual==authored.passes.end() || actual->vertex_shader!=pass.vertex_shader ||
           material.selected_shader_lod>=authored.lods.size() ||
           actual->vertex_code!=material.vertex_program)
            throw FormatError("independent water depth scope lost its original Cache/pass/program owner");
        const auto& lod=authored.lods.at(material.selected_shader_lod);
        const auto layer=material.forward_vertex==3?6u:material.forward_vertex==7?15u:3u;
        if(layer>=lod.layers.count() || !(lod.available_layers&(1u<<layer)))
            throw FormatError("independent water pass is absent from its selected original layer");
        const auto range=authored.layers.at(lod.layers.first()+layer);
        if(pass.template_pass<range.first() || pass.template_pass-range.first()>=range.count())
            throw FormatError("independent water pass differs from its original selected LOD/layer");
        output.independent_water_.push_back({part.material,&material,material.source_shader_datum,
            material.selected_shader_lod,pass.template_pass,pass.implementation,material.forward_vertex,
            pass.vertex_shader,material.vertex_program});
    }
    std::erase_if(output.parts_,[&](const auto& part){return independent.contains(key(part));});
    parts=std::span(output.parts_);

    std::set<GeometryKey> selected;
    for(const auto& p:parts)if(p.geometry==-1 || !selected.insert(key(p)).second)
        throw FormatError("ordinary depth duplicate/unowned selected BSP part");
    std::set<SourceBspDrawKey> independent_draws;
    for(const auto& v:resident.expand(selection.bindings)) {
        if(selected.contains(key(v)) || independent.contains(key(v)))output.selected_geometry_.push_back(v);
        if(independent.contains(key(v)))independent_draws.insert({v.material,v.lightmap,v.bsp_cluster,v.source_section,v.source_part});
    }
    output.independent_draws_.assign(independent_draws.begin(),independent_draws.end());
    std::set<SourceBspDrawKey> draw_keys;
    for(const auto& v:output.selected_geometry_)if(selected.contains(key(v)))
        draw_keys.insert({v.material,v.lightmap,v.bsp_cluster,v.source_section,v.source_part});
    output.selected_draws_.assign(draw_keys.begin(),draw_keys.end());
    // The retained logical source viewport/AA is admitted by camera_owner_.
    // c8..11 are regenerated for this full physical target before the ordinary
    // draw;402840 separately owns scissor and inclusive full-window behavior.
    const auto& surface=use.bank().depth_stencil;
    output.raster_.surface_size={float(surface.width),float(surface.height)};
    output.raster_.surface_clip={0,0,surface.width,surface.height};
    output.raster_.window_inclusive={0,std::int32_t(surface.width),0,std::int32_t(surface.height)};
    output.raster_.surface_scale={1,1};
    // Ordinary44550→4D9B0→1BBD0 performs3FFA90(0),900 cull/front.
    // Actual children retain their separately proved147=0/900 apply state;
    // each original+11C retirement list must restore this ordinary900 state.
    output.raster_.factor_offset={-0.f,-0.f};output.raster_.cull=output.raster_.front_face=0x900;
    output.raster_.w_clamp=false; // Original ordinary13430 method1D78=0.
    for(const auto& [identity,material]:selected_materials(mesh,parts)) {
        authored_pass(cache,mesh.materials.at(identity.root_slot),*material);
        const auto& p=material->passes.at(material->forward_pass_index);
        output.identities_.push_back(identity);
        output.materials_.push_back({identity,material,material->source_shader_datum,material->selected_shader_lod,
            p.template_pass,p.implementation,material->forward_vertex,material->forward_blend,
            material->forward_raster,decode_vertex_program(material->vertex_program),material->vertex_program,
            pack_source_vertex_declaration(material->vertex_declaration),material->program_constants,material->texture_states,
            material->texture_frames,{material->diffuse,material->detail,material->bump,material->auxiliary},
            material->animated_textures,bitmap_frame_animations(*material),material->forward_program,
            {material->accumulation_texture,material->baked_lightmap_texture,material->scope_texture,
             material->water_reflection_texture,material->water_refraction_texture}});
    }
    output.validate_source_owners(use);output.validate_geometry(use,output.selected_geometry_);return output;
}
void SourceWorldDepthInput::validate_lifetime(const RendererTargets::Use& use)const {
    const auto camera=this->camera(use);
    if(!mesh_ || !use.valid() || use.bank().generation!=generation_ || use.device_address()!=device_ ||
       !exact(camera,raster_.camera))throw FormatError("ordinary depth source camera/target owner changed");
}
void SourceWorldDepthInput::validate_source_owners(const RendererTargets::Use& use)const {
    validate_lifetime(use);
    if(mesh_->lightmaps.size()!=lightmaps_.size())throw FormatError("ordinary depth immutable lightmap bank changed");
    for(unsigned n=0;n<lightmaps_.size();++n)if(!texture_exact(mesh_->lightmaps[n],lightmaps_[n]))
        throw FormatError("ordinary depth immutable lightmap image changed");
    const auto current=selected_materials(*mesh_,parts_);
    if(current.size()!=materials_.size())throw FormatError("ordinary depth complete root/child topology changed");
    for(const auto& owner:independent_water_) {
        const auto& material=mesh_->materials.at(owner.root);
        if(&material!=owner.material || !source_world_depth_independent_water(material) ||
           material.source_shader_datum!=owner.shader || material.selected_shader_lod!=owner.lod ||
           material.forward_vertex!=owner.forward_vertex || material.vertex_program!=owner.tokens)
            throw FormatError("independent water source role/owner changed during ordinary depth submission");
        const auto& pass=material.passes.at(material.forward_pass_index);
        if(pass.template_pass!=owner.pass || pass.implementation!=owner.implementation || pass.vertex_shader!=owner.vertex_shader)
            throw FormatError("independent water selected pass changed during ordinary depth submission");
    }
    for(const auto& owner:materials_) {
        const auto it=current.find(owner.identity);
        if(it==current.end() || it->second!=owner.material)
            throw FormatError("ordinary depth complete root/child pointer owner changed");
        const auto& m=*it->second;
        if(!m.forward_pass || m.forward_pass_index>=m.passes.size())
            throw FormatError("ordinary depth selected material pass changed");
        const auto& p=m.passes[m.forward_pass_index];
        if(m.source_shader_datum!=owner.shader || m.selected_shader_lod!=owner.lod ||
           p.template_pass!=owner.template_pass || p.implementation!=owner.implementation ||
           m.vertex_program!=owner.tokens || m.vertex_bank ||
           pack_source_vertex_declaration(m.vertex_declaration,m.world_vertex_input_transform)!=owner.declaration ||
           m.forward_raster!=owner.raster || m.forward_vertex!=owner.forward_vertex || m.forward_blend!=owner.blend ||
           !exact(m.program_constants,owner.constants) || m.texture_states!=owner.textures ||
           m.texture_frames!=owner.frames || m.forward_program.words!=owner.pixel.words ||
           m.forward_program.fragment_offsets!=owner.pixel.fragment_offsets ||
           std::array<bool,5>{m.accumulation_texture,m.baked_lightmap_texture,m.scope_texture,
               m.water_reflection_texture,m.water_refraction_texture}!=owner.target_bindings)
            throw FormatError("ordinary depth immutable material/input/raster owner changed");
        if(!bitmap_animations_exact(bitmap_frame_animations(m),owner.bitmap_animations))
            throw FormatError("ordinary depth immutable bitmap animation owner changed");
        const Texture* images[]{&m.diffuse,&m.detail,&m.bump,&m.auxiliary};
        for(unsigned stage=0;stage<4;++stage) {
            if(!texture_exact(*images[stage],owner.images[stage]) ||
               m.animated_textures[stage].size()!=owner.animated_images[stage].size())
                throw FormatError("ordinary depth immutable texture/frame owner changed");
            for(unsigned frame=0;frame<m.animated_textures[stage].size();++frame)
                if(!texture_exact(m.animated_textures[stage][frame],owner.animated_images[stage][frame]))
                    throw FormatError("ordinary depth immutable animated image owner changed");
        }
    }
}
const SourceWorldDepthRaster& SourceWorldDepthInput::raster(const RendererTargets::Use& use)const {
    validate_lifetime(use);return raster_;
}
void SourceWorldDepthInput::validate_geometry(const RendererTargets::Use& use,
    std::span<const Vertex> geometry)const {
    validate_lifetime(use);
    if(geometry.size()!=selected_geometry_.size() ||
       std::memcmp(geometry.data(),selected_geometry_.data(),geometry.size_bytes()))
        throw FormatError("ordinary depth immutable selected Cache geometry/payload/order changed");
    using Key=GeometryKey;std::map<Key,std::uint32_t> expected;std::set<Key> seen;
    const auto expect=[&](const auto& p) {
        const Key key{p.geometry,p.section,p.part,p.material};
        const auto level=source_shader_instance_stipple(p.instance_alpha).level;
        const auto [it,inserted]=expected.emplace(key,level);
        if(!inserted && it->second!=level)throw FormatError("ordinary depth selected part stipple ownership conflicts");
    };
    for(const auto& p:parts_)expect(p);
    for(const auto& p:independent_parts_)expect(p);
    if((geometry.empty() && (!parts_.empty() || !independent_parts_.empty())) || geometry.size()%3)
        throw FormatError("ordinary depth requires complete ordinary BSP triangles");
    for(const auto& v:geometry) {
        const Key key{v.bsp_cluster,v.source_section,v.source_part,v.material};const auto it=expected.find(key);
        if(it==expected.end() || v.source_stipple_level!=it->second)
            throw FormatError("ordinary depth geometry changed actual selected part/stipple identity");
        seen.insert(key);
    }
    if(seen.size()!=expected.size())throw FormatError("ordinary depth geometry omitted an actual selected part");
}
void SourceWorldDepthInput::validate_resident_atlas(const RendererTargets::Use& use,
    const SourceBspResidentPlan& resident,std::span<const Vertex> uploaded)const {
    validate_lifetime(use);
    if(&resident!=resident_ || uploaded.size()!=resident_atlas_.size() ||
       std::memcmp(uploaded.data(),resident_atlas_.data(),uploaded.size_bytes()))
        throw FormatError("ordinary depth immutable resident atlas/upload provenance changed");
}
bool SourceWorldDepthInput::admits_draw(const SourceBspDrawKey& key)const {
    return std::binary_search(selected_draws_.begin(),selected_draws_.end(),key);
}
bool SourceWorldDepthInput::independent_draw(const SourceBspDrawKey& key)const {
    return std::binary_search(independent_draws_.begin(),independent_draws_.end(),key);
}
void SourceWorldDepthInput::validate_camera(const RendererTargets::Use& use,
    std::array<float,3> eye,std::array<float,3> forward,std::array<float,3> up)const {
    validate_lifetime(use);
    const bool live_alias_changed=std::holds_alternative<SourceLiveShadowPhaseInputsTu15>(camera_owner_) &&
        (!memory_ || !exact(read<std::array<float,3>>(*memory_,0x4c65c0),eye_) ||
         !exact(read<std::array<float,3>>(*memory_,0x4c65cc),forward_) ||
         !exact(read<std::array<float,3>>(*memory_,0x4c65d8),up_));
    if(!exact(eye,eye_) || !exact(forward,forward_) || !exact(up,up_) || live_alias_changed)
        throw FormatError("ordinary depth actual view camera differs from retained native owner");
}
void SourceWorldDepthInput::validate_camera_push(const RendererTargets::Use& use,
    ShaderVector forward,ShaderVector eye,ShaderVector viewport)const {
    validate_camera(use,{eye[0],eye[1],eye[2]},
        {forward[0],forward[1],forward[2]},up_);
    if(eye[3]!=1.f || forward[3]!=1.f ||
       viewport!=ShaderVector{0,0,raster_.surface_size[0],raster_.surface_size[1]})
        throw FormatError("ordinary depth actual camera push/viewport differs from native world scope");
}
void SourceWorldDepthInput::validate_resident_view(const RendererTargets::Use& use,
    const SourceBspResidentPlan& resident,std::span<const SourceBspPartBinding> bindings,
    const SourceBspMergedDrawPlan& actual)const {
    validate_lifetime(use);
    if(&resident!=resident_)throw FormatError("ordinary depth current resident owner changed");
    if(bindings.size()!=bindings_.size())throw FormatError("ordinary depth current BSP binding owner changed");
    for(unsigned n=0;n<bindings.size();++n)
        if(bindings[n].material!=bindings_[n].material || bindings[n].stipple!=bindings_[n].stipple)
            throw FormatError("ordinary depth current BSP material/stipple binding changed");
    std::vector<const SourceBspDrawToken*> current;
    for(const auto& token:actual.draws)if(token.key.geometry!=-1)current.push_back(&token);
    if(current.size()!=expected_bsp_.draws.size())
        throw FormatError("ordinary depth current BSP merged token coverage changed");
    for(unsigned n=0;n<current.size();++n) {
        const auto& a=*current[n];const auto& e=expected_bsp_.draws[n];
        if(a.key!=e.key || a.count!=e.count || a.storage!=e.storage || a.run_count!=e.run_count)
            throw FormatError("ordinary depth current BSP merged token/range/storage changed");
        if(a.storage==SourceBspVertexStorage::resident) {
            if(a.first!=e.first || a.first>resident_atlas_.size() || a.count>resident_atlas_.size()-a.first)
                throw FormatError("ordinary depth current BSP resident range changed");
        } else if(a.storage==SourceBspVertexStorage::dynamic) {
            if(a.first>actual.dynamic_vertices.size() || a.count>actual.dynamic_vertices.size()-a.first ||
               e.first>expected_bsp_.dynamic_vertices.size() || e.count>expected_bsp_.dynamic_vertices.size()-e.first ||
               std::memcmp(actual.dynamic_vertices.data()+a.first,
                   expected_bsp_.dynamic_vertices.data()+e.first,a.count*sizeof(Vertex)))
                throw FormatError("ordinary depth current BSP dynamic geometry/stipple payload changed");
        } else throw FormatError("ordinary depth BSP key collided with unowned object geometry");
    }
}
void SourceWorldDepthInput::validate_resident_visibility(const RendererTargets::Use& use,
    const SourceBspAdmission& admission)const {
    validate_lifetime(use);std::vector<SourceBspDrawKey> visible;
    for(const auto& token:expected_bsp_.draws)
        if(admission.geometry_visible(token.key.geometry) && !independent_draw(token.key))visible.push_back(token.key);
    if(visible!=selected_draws_)
        throw FormatError("ordinary depth actual BSP visible selection differs from owned main parts");
}

namespace {
std::array<std::uint32_t,60> normalized_controls(std::array<std::uint32_t,60> words) {
    std::fill(words.begin()+10,words.begin()+26,0u);words[43]=words[44]=0;return words;
}
bool native_controls_match(const Material& material,const std::array<std::uint32_t,60>& controls) {
    if(controls==normalized_controls(material.forward_program.words))return true;
    // pStencil keeps the source vertex/depth kernel while using its existing
    // explicit presentation adapters. Admit those exact CPU-produced programs,
    // rather than accepting an arbitrary replacement final combiner.
    auto enhanced=material.forward_program;
    if(const auto detail=pstencil::reference_neutral_detail_stage(material))
        enhanced=pstencil::neutral_detail_response(enhanced,*detail);
    if(pstencil::supports_environment_response(material))
        enhanced=pstencil::adapt_environment_response(enhanced);
    return controls==normalized_controls(enhanced.words);
}
bool native_attributes_match(const Material& material,const std::array<ShaderVector,16>& attributes) {
    if(exact(attributes,material.program_constants[0]))return true;
    if(!pstencil::supports_environment_response(material))return false;
    const auto* reference=pstencil::reference_environment_parameters(material.name);
    if(!reference)return false;
    auto enhanced=material.program_constants[0];auto program=material.forward_program;
    pstencil::apply_reference_environment(*reference,program,enhanced);
    return exact(attributes,enhanced);
}
std::uint64_t pixel_fingerprint(const Texture& image) {
    std::uint64_t h=14695981039346656037ull;
    for(const auto b:image.pixels){h^=std::to_integer<unsigned char>(b);h*=1099511628211ull;}return h;
}
void uploaded_image(const Texture& expected,const SourceWorldDepthImageUpload& actual,bool source_image) {
    if(!actual.shape || actual.mutated || actual.source_image!=source_image ||
       actual.pixel_fingerprint!=pixel_fingerprint(expected))
        throw FormatError("ordinary depth constructor image/upload witness changed");
    if(!actual.shape->pixels.empty() || !texture_metadata_exact(expected,*actual.shape) ||
       actual.mip_count!=(expected.mips.empty()?1:expected.mips.size()/expected.layers))
        throw FormatError("ordinary depth constructor image/mip upload shape changed");
}
Texture fallback_image(bool normal=false) {
    return {1,1,11,{std::byte(normal?128u:255u),std::byte(normal?128u:255u),
        std::byte{255},std::byte{255}}, {}};
}
}
void SourceWorldDepthInput::validate_draw(const RendererTargets::Use& use,
    const SourceBspDrawKey& key,std::span<const Vertex> payload,bool resident)const {
    validate_lifetime(use);
    if(!admits_draw(key))throw FormatError("ordinary depth actual BSP draw key is unowned");
    std::vector<Vertex> expected;
    if(resident) {
        const auto it=std::find_if(expected_bsp_.draws.begin(),expected_bsp_.draws.end(),[&](const auto& t){return t.key==key;});
        if(it==expected_bsp_.draws.end() || it->storage!=SourceBspVertexStorage::resident)
            throw FormatError("ordinary depth actual draw changed resident storage");
        const auto bank=std::span(resident_atlas_).subspan(it->first,it->count);
        expected.assign(bank.begin(),bank.end());
    } else for(const auto& v:selected_geometry_)
        if(SourceBspDrawKey{v.material,v.lightmap,v.bsp_cluster,v.source_section,v.source_part}==key)expected.push_back(v);
    if(payload.size()!=expected.size() || std::memcmp(payload.data(),expected.data(),payload.size_bytes()))
        throw FormatError("ordinary depth actual draw range/buffer/payload changed");
}
void SourceWorldDepthInput::validate_uploaded_material(const RendererTargets::Use& use,
    const SourceWorldDepthMaterialIdentity& identity,const Material* material,
    const std::array<std::uint32_t,60>& actual_pipeline_controls,
    std::span<const SourceWorldDepthImageUpload,4> images,
    const std::array<ShaderTextureState,4>& samplers,const std::array<std::uint32_t,4>& frames,
    const SourceWorldDepthFrameReceipt* receipt,unsigned expanded_slot)const {
    validate_lifetime(use);
    const auto it=std::find_if(materials_.begin(),materials_.end(),[&](const auto& m){return m.identity==identity;});
    // The retained owner follows completed CPU animate_material_bitmaps.
    // Equal stale wanted/bound GPU frames do not prove that owner selection.
    if(it==materials_.end() || it->material!=material ||
       material->forward_program.words!=it->pixel.words ||
       !native_controls_match(*it->material,actual_pipeline_controls) || samplers!=it->textures || frames!=it->frames)
        throw FormatError("ordinary depth constructor material/pipeline/sampler witness changed");
    if(receipt) {
        if(receipt->epoch_.expired() || receipt->owner_!=this || receipt->device_!=use.device_address() ||
           receipt->generation_!=use.bank().generation || receipt->identity_!=identity ||
           receipt->material_!=material || receipt->expanded_slot_!=expanded_slot ||
           !receipt->descriptor_ || receipt->wanted_!=receipt->bound_ || receipt->bound_!=frames ||
           receipt->samplers_!=samplers)
            throw FormatError("ordinary depth current bound-frame receipt owner/epoch/slot/frame changed");
        if(!bitmap_animations_exact(receipt->animations_,it->bitmap_animations))
            throw FormatError("ordinary depth bound-frame receipt changed its authored bitmap functions");
        for(unsigned stage=0;stage<4;++stage) {
            const auto& current=images[stage];const auto& issued=receipt->images_[stage];
            if(current.shape!=issued.shape || current.pixel_fingerprint!=issued.pixel_fingerprint ||
               current.mip_count!=issued.mip_count || current.source_image!=issued.source_image ||
               current.mutated!=issued.mutated || !receipt->gpu_samplers_[stage] ||
               std::any_of(receipt->views_[stage].begin(),receipt->views_[stage].end(),[](auto view){return !view;}))
                throw FormatError("ordinary depth bound-frame receipt page/stage/descriptor changed");
        }
    }
    for(unsigned stage=0;stage<4;++stage) {
        const auto& animated=it->animated_images[stage];
        if(!animated.empty() && !receipt)
            throw FormatError("ordinary depth animated uploaded page lacks a frame receipt: root="+
                std::to_string(identity.root_slot)+" child="+identity.child_path+" material="+material->name+
                " stage="+std::to_string(stage)+" frame="+std::to_string(frames[stage])+
                " pages="+std::to_string(animated.size()));
        if(animated.empty() && frames[stage]!=it->frames[stage])
            throw FormatError("ordinary depth static uploaded page changed its literal frame");
        if(!animated.empty() && frames[stage]>=animated.size())
            throw FormatError("ordinary depth bound animation frame is outside its immutable page bank");
        const auto& source=animated.empty()?it->images[stage]:animated[frames[stage]];
        const auto fallback=fallback_image(stage==2);
        const auto& expected=source.pixels.empty()?fallback:source;
        uploaded_image(expected,images[stage],!source.pixels.empty());
    }
}
void SourceWorldDepthInput::validate_uploaded_lightmaps(const RendererTargets::Use& use,
    std::span<const SourceWorldDepthImageUpload> images)const {
    validate_lifetime(use);
    if(images.size()!=lightmaps_.size()+1)throw FormatError("ordinary depth constructor lightmap coverage changed");
    uploaded_image(fallback_image(),images[0],true);
    for(unsigned n=0;n<lightmaps_.size();++n)uploaded_image(lightmaps_[n],images[n+1],!lightmaps_[n].pixels.empty());
}

void SourceWorldDepthInput::validate_completed(const RendererTargets::Use& use,
    std::span<const SourceWorldDepthCompletedMaterial> actual)const {
    validate_source_owners(use);
    if(actual.size()!=materials_.size())throw FormatError("ordinary depth requires complete root/child final uploads");
    std::map<Identity,const SourceWorldDepthCompletedMaterial*> by_owner;std::set<unsigned> slots;
    for(const auto& row:actual)if(!by_owner.emplace(row.identity,&row).second || !slots.insert(row.expanded_slot).second)
        throw FormatError("ordinary depth has duplicate final material owner or slot");
    for(const auto& owner:materials_) {
        const auto it=by_owner.find(owner.identity);
        if(it==by_owner.end())throw FormatError("ordinary depth actual final child owner is missing");
        const auto& row=*it->second;
        if(row.source_material!=owner.material)
            throw FormatError("ordinary depth actual Renderer material owner differs from producer Mesh");
        if(row.material_uniform.size()!=sizeof(WorldMaterialUniform) || row.vertex_bank_uniform.size()!=sizeof(PackedVertexProgramBank))
            throw FormatError("ordinary depth completed upload changed its actual ABI");
        WorldMaterialUniform u;PackedVertexProgramBank b;
        std::memcpy(&u,row.material_uniform.data(),sizeof(u));std::memcpy(&b,row.vertex_bank_uniform.data(),sizeof(b));
        const auto* native=row.native_kernel;
        if(!native || native->extended_vertex_bank || native->source_depth_vertex_path.empty() ||
           native->declaration!=owner.declaration ||
           std::as_bytes(native->vertex_program).size()!=owner.tokens.size() ||
           !std::ranges::equal(std::as_bytes(native->vertex_program),owner.tokens) ||
           u.flags[3] || std::ranges::any_of(u.vertex_instructions,[](const auto& word){
               return std::ranges::any_of(word,[](auto value){return value!=0;});}) ||
           u.vertex_declaration!=PackedSourceVertexDeclaration{} || !exact(u.vertex_constants,owner.constants[1]) ||
           !native_attributes_match(*owner.material,u.vertex_attributes) || b.range!=std::array<std::uint32_t,4>{0,0,0,0})
            throw FormatError("ordinary depth completed source vertex/input bank differs from actual owner");
        if(!u.source_camera_control[0] || !std::equal(u.source_camera_world.begin(),u.source_camera_world.end(),raster_.camera.begin()))
            throw FormatError("ordinary depth completed c0..6 differs from retained native camera");
        // Color constants remain current per-view inputs. Shader operations,
        // texture modes and register routing must match the exact authored
        // program used to create the source pipeline specialization.
        const auto controls=[](std::array<std::uint32_t,60> words) {
            std::fill(words.begin()+10,words.begin()+26,0u);
            words[43]=words[44]=0u;return words;
        };
        if(controls(u.program)!=std::array<std::uint32_t,60>{} || !native_controls_match(*owner.material,row.native_controls))
            throw FormatError("ordinary depth completed authored pixel controls differ from actual owner");
        const auto& m=*owner.material;
        if(u.alpha_test[0]!=owner.raster.alpha_test || u.alpha_test[1]!=owner.raster.alpha_function ||
           u.alpha_test[2]!=owner.raster.alpha_reference || !u.flags[1] ||
           u.flags[2]!=(m.per_vertex_lighting?4u:m.forward_vertex))
            throw FormatError("ordinary depth completed material altered authored alpha/pass state");
    }
}
} // namespace halo2
