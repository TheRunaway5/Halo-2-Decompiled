#include "halo2/pstencil/material.hpp"
#include "halo2/pstencil/adaptation.hpp"
#include "halo2/pstencil/specular_mask.hpp"
#include "halo2/pstencil/light_response_2003.hpp"
#include "halo2/pstencil/light_response_material.hpp"
#include "halo2/pstencil/normal_accumulation_2003.hpp"
#include "halo2/pstencil/texture_accumulation_2003.hpp"
#include "halo2/pstencil/reference_light_materials.hpp"
#include "halo2/render_program.hpp"
#include "halo2/object_lighting.hpp"
#include <algorithm>
#include <cstdlib>
#include <map>
#include <tuple>

namespace halo2::pstencil {
namespace {
std::optional<Material> prepare(const Cache& cache,Material authored,unsigned index,
    const EffectLightUniforms& uniforms,bool load_images=true) {
    auto& pass=authored.passes.at(index);
    authored.shader_switches[0]=authored.shader_switches[1]=std::int32_t(uniforms.color_shift);
    pass.program=assemble_pixel_program(pass.definition,pass.fragments,pass.permutations,
        pass.combiners,authored.shader_switches);
    remap_pixel_constants(pass.program,authored.pixel_constants,pass.pixel_remaps,
        pass.fragments,authored.shader_switches);
    std::array<float,256> scalars{};scalars[21]=authored.specular_brightness;
    scalars[22]=authored.ambient_brightness;std::vector<ShaderRemap> brightness;
    for(const auto& r:pass.pixel_externs)if(r.source==21 || r.source==22)brightness.push_back(r);
    remap_pixel_externs(pass.program,scalars,{},brightness,pass.fragments,authored.shader_switches);
    return load_images?bind_light_pass(cache,authored,index,uniforms):complete_light_pass(cache,authored,index,uniforms);
}
struct ResponseContext {
    bool enabled{};
    unsigned geometry{},diffuse{},specular{},mask{},gel=0xffffffffu;
    auto operator<=>(const ResponseContext&)const=default;
};
ResponseContext response_context(const EffectLightDefinition& definition) {
    const auto* legacy=std::getenv("HALO2_PSTENCIL_LEGACY_LIGHT_RESPONSE");
    if(legacy && *legacy)return {};
    const Bytes light(definition.bytes);
    const unsigned geometry=light.u16(4),falloff=light.u16(0x8e);
    // Sphere inputs use their verified crosswalk. Static projective inputs
    // use the separately checked original A3CF0 producer. Original180810
    // supplies V9.xy=(0,0) for these static, non-scrolling gel inputs.
    if((geometry!=0 && geometry!=2) ||
       (geometry==0 ? (falloff!=0 && falloff!=2) : (falloff!=1 && falloff!=2)) ||
       (geometry==0 && light.u32(0x7c)!=0xffffffffu) || (definition.flags&0x20u) ||
       (geometry==2 && ((definition.flags&0x200u) || light.u32(0xe0)!=0xffffffffu)) ||
       light.u32(0xc4) || light.u32(0xcc) || light.u32(0xd4))return {};
    const unsigned diffuse=light.u16(0x8a),specular=light.u16(0x8c),mask=light.u16(0x80);
    if(diffuse>3 || specular>3 || mask>3)
        throw FormatError("pStencil light response has an unknown current contrast or specular mask");
    return {true,geometry,diffuse,specular,mask,light.u32(0x7c)};
}
void adapter_metadata(Material& material) {
    // Completed per-draw source UV/material attributes and light constants
    // are supplied separately. The retail pixel-remap offsets do not name
    // registers in the recovered 2003 program.
    auto& pass=material.passes.at(material.forward_pass_index);
    pass.pixel_externs.clear();pass.pixel_remaps.clear();pass.fragments.clear();
    material.animation={};material.lods.clear();material.layers.clear();
    material.pass_implementations.clear();material.environment_pass.reset();
    material.planar_fog_pass.reset();material.world_overlays_before.clear();
    material.world_overlays_after.clear();material.world_bloom_passes.clear();
    material.world_planar_passes={};material.baked_lightmap_texture=false;
    material.forward_raster.alpha_test=0;
}
ShaderTextureState attenuation_sampler(const Material& authored) {
    const auto& lod=authored.lods.at(authored.selected_shader_lod);
    const auto range=authored.layers.at(lod.layers.first()+12);
    for(const auto& pass:authored.passes) {
        if(pass.implementation!=1 || pass.template_pass<range.first() ||
           pass.template_pass-range.first()>=range.count())continue;
        for(const auto& texture:pass.textures)if(texture.external==9 && texture.stage<4)
            return pass.texture_states.at(texture.stage);
    }
    throw FormatError("pStencil utility has no authored current sphere attenuation sampler");
}
ShaderTextureState projected_gel_sampler(const Material& authored) {
    const auto& lod=authored.lods.at(authored.selected_shader_lod);
    const auto range=authored.layers.at(lod.layers.first()+12);
    for(const auto& pass:authored.passes) {
        if(pass.implementation!=3 || pass.template_pass<range.first() ||
           pass.template_pass-range.first()>=range.count())continue;
        for(const auto& texture:pass.textures)if(texture.external==10 && texture.stage==3)
            return pass.texture_states[3];
    }
    throw FormatError("pStencil projected response has no authored gel sampler");
}
std::array<float,3> rgb(const ShaderVector& value) {return {value[0],value[1],value[2]};}
}
bool supports_projected_light_response_2003(const EffectLightDefinition& definition) {
    const auto context=response_context(definition);
    return context.enabled && context.geometry==2;
}
struct MaterialBank::Impl {
    struct Pass {unsigned slot,index;std::shared_ptr<const Material> authored;bool response;bool normal{};};
    using Key=std::tuple<std::uint32_t,unsigned,ResponseContext>;
    const Cache& cache;Mesh& mesh;
    std::map<Key,std::vector<Pass>> passes;
    std::map<std::pair<std::uint32_t,unsigned>,std::shared_ptr<const Material>> sources;
    // Contrast/mask/color words are completed per draw. They do not change
    // resident images, the vertex family or the two attachment pipelines.
    std::map<std::tuple<std::uint32_t,unsigned,unsigned,unsigned,unsigned>,std::pair<unsigned,bool>> response_slots;
    std::map<std::uint32_t,ResponseContext> contexts;
    std::map<std::uint32_t,bool> texture_masks;
    Impl(const Cache& c,Mesh& m):cache(c),mesh(m){}
};
MaterialBank::MaterialBank(const Cache& cache,Mesh& mesh):impl_(std::make_unique<Impl>(cache,mesh)){}
MaterialBank::~MaterialBank()=default;
void MaterialBank::preload(const Material& authored,std::uint32_t light,const EffectLightDefinition& definition) {
    auto& bank=*impl_;
    auto context=response_context(definition);
    const auto [where,inserted]=bank.contexts.emplace(light,context);
    if(!inserted && where->second!=context)throw FormatError("pStencil light definition changed its response context");
    if(authored.lods.empty())return;
    const auto& lod=authored.lods.at(authored.selected_shader_lod);
    if(!(lod.available_layers&(1u<<13)))return;
    const auto range=authored.layers.at(lod.layers.first()+13);
    auto [mask,first_mask]=bank.texture_masks.try_emplace(authored.source_shader_datum,false);
    if(first_mask) {
        auto texture=authored;texture.selected_shader_lod=0;
        const auto mapping=bind_texture_mapping_pass(bank.cache,texture);
        if(mapping && texture_mask_pass(*mapping)) {
            mask->second=true;
            bank.mesh.materials.push_back(!std::getenv("HALO2_PSTENCIL_CURRENT_TEXTURE_ACCUMULATION") &&
                supports_texture_accumulation_2003(*mapping)?adapt_texture_accumulation_2003(*mapping):*mapping);
        } else if(prepare_dynamic_material(bank.cache,texture,{},true,0.f) && texture_mask_pass(texture)) {
            mask->second=true;
            if(!std::getenv("HALO2_PSTENCIL_CURRENT_TEXTURE_ACCUMULATION") && supports_texture_accumulation_2003(texture))
                texture=adapt_texture_accumulation_2003(std::move(texture));
            bank.mesh.materials.push_back(std::move(texture));
        }
    }
    // The final source program consumes complete texture-target RGBA.
    context.enabled=context.enabled && mask->second;
    const Impl::Key key{authored.source_shader_datum,authored.selected_shader_lod,context};
    if(bank.passes.contains(key))return;
    auto& out=bank.passes[key];
    const auto source_key=std::pair{authored.source_shader_datum,authored.selected_shader_lod};
    auto source_at=bank.sources.find(source_key);
    if(source_at==bank.sources.end()) {
        auto source=std::make_shared<Material>(authored);
        source->diffuse=source->detail=source->bump=source->auxiliary={};
        source->animated_textures={};source->environment_pass.reset();source->planar_fog_pass.reset();
        source->world_overlays_before.clear();source->world_overlays_after.clear();
        source->world_bloom_passes.clear();source->world_planar_passes={};
        source_at=bank.sources.emplace(source_key,std::move(source)).first;
    }
    const auto& source=source_at->second;
    EffectLightUniforms uniforms{};
    for(unsigned i=0;i<authored.passes.size();++i) {
        const auto& pass=authored.passes[i];
        if(pass.template_pass<range.first() || pass.template_pass-range.first()>=range.count() || pass.implementation!=0)continue;
        if(!pass.vertex_shader.ends_with("\\stencil_light_specular_tangentspace_sphere"))
            throw FormatError("pStencil specular pass requires its authored sphere program");
        const auto resident_key=std::tuple{authored.source_shader_datum,authored.selected_shader_lod,context.geometry,context.gel,i};
        if(context.enabled)if(const auto at=bank.response_slots.find(resident_key);at!=bank.response_slots.end()) {
            out.push_back({at->second.first,i,source,true,at->second.second});continue;
        }
        auto material=prepare(bank.cache,authored,i,uniforms);
        if(!material)throw FormatError("pStencil specular material has unresolved authored bindings: "+authored.name);
        if(material->forward_blend!=8)throw FormatError("pStencil specular pass changed its destination-alpha blend");
        const auto format=material->bump.format;
        const bool normalized_bump=format==11 || (format>=14 && format<=16);
        if(context.enabled && normalized_bump && !material->bump.depth && material->bump.layers==1 && material->detail.layers==6) {
            const auto* reference=reference_light_material(authored.name);
            const bool dull=reference && reference->utility_builtin==10;
            // Original names retain the primary-checked layer2 settings.
            // Compatible later assets adapt that same producer/consumer using
            // their authored normalized bump image and posed tangent basis.
            // This switch is a diagnostic comparison, outside the game UI.
            const bool normal=std::getenv("HALO2_PSTENCIL_TANGENT_LIGHT_RESPONSE")==nullptr;
            auto utility=*material;adapter_metadata(utility);
            utility.forward_blend=normal?normal_light_utility_blend_2003:light_utility_blend_2003;
            utility.accumulation_texture=false;
            if(normal) {
                utility.texture_states[0]=normal_target_sampler_2003();
                utility.texture_states[1]=normalization_cube_sampler_2003();
            }
            utility.forward_program=light_utility_2003(context.geometry,context.diffuse,context.specular,0,dull);
            utility.auxiliary=bank.cache.load_effect_light_attenuation_bitmap();
            utility.animated_textures[3].clear();utility.texture_states[3]=attenuation_sampler(authored);
            auto dependent=*material;adapter_metadata(dependent);
            dependent.forward_blend=dependent_light_blend_2003;dependent.accumulation_texture=true;
            dependent.forward_program=dependent_light_2003(context.geometry,context.mask,0,0,0);
            const Texture zero{1,1,11,std::vector<std::byte>(4),{}};
            dependent.detail=dependent.bump=dependent.auxiliary=zero;
            if(context.geometry) {
                dependent.auxiliary=context.gel==0xffffffffu?
                    Texture{1,1,11,std::vector<std::byte>(4,std::byte{255}),{}}:bank.cache.load_bitmap(context.gel,0);
                if(dependent.auxiliary.depth || dependent.auxiliary.layers!=1)
                    throw FormatError("2003 static projected response requires its actual 2D gel image");
                dependent.texture_states[3]=projected_gel_sampler(authored);
            }
            dependent.animated_textures={};
            const auto slot=unsigned(bank.mesh.materials.size());
            if(normal) {
                auto producer=*material;adapter_metadata(producer);
                producer.forward_blend=normal_accumulation_blend_2003;
                producer.accumulation_texture=false;producer.forward_program=normal_accumulation_2003();
                producer.texture_states[2]=normal_bump_sampler_2003(!dull);
                producer.texture_states[1]=normalization_cube_sampler_2003();
                // Current base geometry has already filled depth. Preserve that
                // exact clip/depth while reproducing layer2 color ownership.
                producer.forward_raster.depth_enable=1;producer.forward_raster.depth_write=0;
                producer.forward_raster.depth_function=0x202;producer.forward_raster.color_write=0x01010101;
                bank.mesh.materials.push_back(std::move(producer));
            }
            bank.mesh.materials.push_back(std::move(utility));bank.mesh.materials.push_back(std::move(dependent));
            bank.response_slots.emplace(resident_key,std::pair{slot,normal});
            out.push_back({slot,i,source,true,normal});
        } else {
            if(context.geometry)continue;
            material->forward_program=texture_masked_specular(material->forward_program);
            if(mask->second){material->forward_blend=3;material->accumulation_texture=true;}
            const auto slot=unsigned(bank.mesh.materials.size());bank.mesh.materials.push_back(std::move(*material));
            out.push_back({slot,i,source,false});
        }
    }
}
std::shared_ptr<const std::vector<EffectLightDraw>> MaterialBank::complete(
    std::uint32_t shader,unsigned lod,std::uint32_t light,const EffectLightUniforms& uniforms) const {
    const auto& bank=*impl_;const auto found=bank.contexts.find(light);
    if(found==bank.contexts.end())return {};
    auto context=found->second;
    const auto mask=bank.texture_masks.find(shader);
    context.enabled=context.enabled && mask!=bank.texture_masks.end() && mask->second;
    const auto at=bank.passes.find({shader,lod,context});if(at==bank.passes.end() || at->second.empty())return {};
    auto out=std::make_shared<std::vector<EffectLightDraw>>();
    for(const auto& pass:at->second) {
        auto material=prepare(bank.cache,*pass.authored,pass.index,uniforms,false);
        if(!material || !material->vertex_bank)throw FormatError("pStencil specular bank failed to complete");
        const auto append=[&](unsigned slot,PixelProgram program,const Material& completed) {
            out->push_back({slot,{},std::move(program),completed.program_constants,
                pack_vertex_program_bank(*completed.vertex_bank)});
        };
        if(pass.response) {
            const auto* reference=reference_light_material(pass.authored->name);
            if(reference) {
                material->program_constants[0][10]=reference->v10;
                material->program_constants[0][11]=reference->v11;
            }
            if(pass.normal)append(pass.slot,normal_accumulation_2003(),*material);
            append(pass.slot+unsigned(pass.normal),light_utility_2003(context.geometry,context.diffuse,context.specular,0,
                reference && reference->utility_builtin==10),*material);
            append(pass.slot+1+unsigned(pass.normal),dependent_light_2003(context.geometry,context.mask,uniforms.color_shift,
                pack_light_rgb_2003(rgb(uniforms.constants[6])),pack_light_rgb_2003(rgb(uniforms.constants[7]))),*material);
        } else append(pass.slot,texture_masked_specular(material->forward_program),*material);
        if(arbiter_metal(pass.authored->name)) {
            auto polish=uniforms;
            for(unsigned c=0;c<3;++c)polish.constants[7][c]*=arbiter_specular_companion;
            auto extra=prepare(bank.cache,*pass.authored,pass.index,polish,false);
            if(!extra || !extra->vertex_bank)throw FormatError("Arbiter armor polish has unresolved source bindings");
            if(pass.response)if(const auto* reference=reference_light_material(pass.authored->name)) {
                extra->program_constants[0][10]=reference->v10;extra->program_constants[0][11]=reference->v11;
            }
            for(unsigned n=0;n<arbiter_specular_copies;++n) {
                if(pass.response)append(pass.slot+1+unsigned(pass.normal),dependent_light_2003(context.geometry,context.mask,polish.color_shift,
                    0,pack_light_rgb_2003(rgb(polish.constants[7]))),*extra);
                else append(pass.slot,texture_masked_specular(extra->forward_program),*extra);
            }
        }
    }
    return out;
}
}
