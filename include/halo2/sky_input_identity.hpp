#pragma once
// Offline audit identities. Floating point inputs retain their literal words;
// map-local handles are resolved to names rather than compared across caches.
#include "halo2/cache.hpp"
#include "halo2/source_render_policy.hpp"
#include "halo2/texture_program.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <openssl/evp.h>
#include <iomanip>
#include <sstream>

namespace halo2::sky_audit {
using json=nlohmann::json;
inline json words(float value){return std::bit_cast<std::uint32_t>(value);}
template<class T> json words(const T& range) {json out=json::array();for(const auto& value:range)out.push_back(words(value));return out;}
inline std::string sha256(std::span<const std::byte> bytes) {
    std::array<unsigned char,32> digest{};unsigned length{};
    if(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&length,EVP_sha256(),nullptr)!=1 || length!=digest.size())
        throw FormatError("Cannot fingerprint complete sky input bytes");
    std::ostringstream out;out<<std::hex<<std::setfill('0');
    for(auto value:digest)out<<std::setw(2)<<unsigned(value);return out.str();
}
inline std::string tag_name(const Cache& cache,std::uint32_t handle) {
    const auto* tag=cache.find_tag({handle});return tag?tag->name:"<NONE>";
}
inline json vector(Vec3 value){return {words(value.x),words(value.y),words(value.z)};}
inline json fog(const AtmosphericFog& value) {
    return {vector(value.color),words(value.maximum_density),words(value.start),words(value.opaque)};
}
inline json sky(const SkyDefinition& value,const Cache& cache) {
    return {{"model",tag_name(cache,value.model)},{"scale",words(value.scale)},
        {"movement",words(value.movement)},{"yaw",words(value.yaw)},
        {"clear",value.clear?vector(*value.clear):json(nullptr)},
        {"fog",value.atmospheric_fog?fog(*value.atmospheric_fog):json(nullptr)}};
}
inline json sampler(const ShaderTextureState& v) {
    return {v.address_u,v.address_v,v.address_w,v.mag_filter,v.min_filter,v.mip_filter,
        v.lod_bias,v.max_mip,v.anisotropy,v.color_key_op,v.color_sign,v.alpha_kill,v.border_color,v.key_color};
}
inline json program(const PixelProgram& v){return {{"words",v.words},{"fragment_offsets",v.fragment_offsets}};}
inline json remaps(const std::vector<ShaderRemap>& values) {
    json out=json::array();for(const auto& v:values)out.push_back({v.destination,v.component,v.source});return out;
}
inline json ranges(const std::vector<ShaderRange>& values) {
    json out=json::array();for(const auto& v:values)out.push_back(v.packed);return out;
}
inline json pass(const MaterialPass& v) {
    json textures=json::array(),states=json::array(),samplers=json::array(),fragments=json::array(),permutations=json::array(),combiners=json::array();
    for(const auto& t:v.textures)textures.push_back({t.parameter,t.external,t.stage,t.flags});
    for(const auto& s:v.states)states.push_back({s.index,s.value});
    for(const auto& s:v.texture_states)samplers.push_back(sampler(s));
    for(const auto& f:v.fragments)fragments.push_back({f.switch_index,f.permutations.packed});
    for(const auto& p:v.permutations)permutations.push_back({p.value,p.flags,p.combiners.packed});
    for(const auto& c:v.combiners)combiners.push_back(c.words);
    return {{"implementation",v.implementation},{"template_pass",v.template_pass},{"textures",textures},{"states",states},
        {"samplers",samplers},{"bitmap_remaps",remaps(v.bitmap_remaps)},{"pixel_remaps",remaps(v.pixel_remaps)},
        {"vertex_remaps",remaps(v.vertex_remaps)},{"pixel_externs",remaps(v.pixel_externs)},
        {"vertex_externs",remaps(v.vertex_externs)},{"state_externs",remaps(v.state_externs)},
        {"constants",words(v.constants)},{"program",program(v.program)},{"definition",program(v.definition)},
        {"fragments",fragments},{"permutations",permutations},{"combiners",combiners},
        {"vertex_code_sha256",sha256(v.vertex_code)},
        {"declaration",pack_source_vertex_declaration(v.vertex_declaration,SourceWorldVertexInputTransform::Preserve)}};
}
inline json animation(const ShaderAnimation& v) {
    json lods=json::array(),implementations=json::array(),overlays=json::array(),references=json::array();
    for(const auto& l:v.lods)lods.push_back({l.available_layers,l.layers.packed});
    for(const auto& i:v.implementations)implementations.push_back({i.bitmap_transforms.packed,i.pixel_constants.packed,i.vertex_constants.packed});
    for(const auto& o:v.overlays)overlays.push_back({o.input,o.range,words(o.period),sha256(o.curve)});
    for(const auto& r:v.references)references.push_back({r.function,r.transform});
    json tables=nullptr;
    if(v.tables)tables={{"periodic",sha256(std::as_bytes(std::span(v.tables->periodic)))},
        {"transition",sha256(std::as_bytes(std::span(v.tables->transition)))}};
    return {{"lods",lods},{"layers",ranges(v.layers)},{"passes",ranges(v.passes)},
        {"parameters",ranges(v.parameters)},{"implementations",implementations},{"overlays",overlays},
        {"references",references},{"mappings",remaps(v.mappings)},{"tables",tables}};
}
inline json semantic(const Material& v,const Cache& cache) {
    json samplers=json::array(),bitmaps=json::array(),lods=json::array(),passes=json::array();
    for(const auto& s:v.texture_states)samplers.push_back(sampler(s));
    for(const auto& b:v.bitmaps)bitmaps.push_back({tag_name(cache,b.datum),b.index,words(b.log_dimension)});
    for(const auto& l:v.lods)lods.push_back({l.layers.packed,l.available_layers,words(l.projected_height)});
    for(const auto& p:v.passes)passes.push_back(pass(p));
    const auto& r=v.forward_raster;
    json bank=nullptr;if(v.vertex_bank)bank={{"first",v.vertex_bank->first_constant},
        {"constants",words(v.vertex_bank->constants)},{"prt",v.vertex_bank->prt_inputs},{"camera",v.vertex_bank->camera_inputs}};
    const auto layer=source_material_layer(v);
    return {{"template",v.template_name},{"layer",layer?json(*layer):json(nullptr)},
        {"forward_pass",v.forward_pass},{"forward_pass_index",v.forward_pass_index},
        {"forward_vertex",v.forward_vertex},{"blend",v.forward_blend},{"blend_color",v.forward_blend_color},
        {"raster",{r.color_write,r.depth_enable,r.depth_write,r.depth_function,r.cull,r.alpha_test,r.alpha_function,r.alpha_reference}},
        {"vertex_program_sha256",sha256(v.vertex_program)}, {"vertex_program_bytes",v.vertex_program.size()},
        {"declaration",pack_source_vertex_declaration(v.vertex_declaration,v.world_vertex_input_transform)},
        {"vertex_bank",bank},{"program",program(v.forward_program)},{"constants",words(v.program_constants)},
        {"lightmap_program",program(v.lightmap_program)},{"selected_shader_lod",v.selected_shader_lod},
        {"features",{{"static_lightmap",v.static_lightmap_pass},{"hud",v.hud_pass},{"ui_full_surface",v.ui_full_surface},
            {"light_volume",v.light_volume_pass},{"accumulation",v.accumulation_texture},{"world",v.world_pass},
            {"per_vertex_lighting",v.per_vertex_lighting},{"baked_lightmap",v.baked_lightmap_texture},
            {"bloom",v.world_bloom_pass},{"water_reflection",v.water_reflection_texture},
            {"water_refraction",v.water_refraction_texture},{"water",v.water_pass},{"water_definition",v.water_definition},
            {"scope",v.scope_texture},{"camouflage_scene",v.camouflage_scene_texture},{"camouflage_group",v.camouflage_group_pass},
            {"ui_emblem",v.ui_emblem_pass},{"ui_widescreen",v.ui_widescreen_background},{"ui_center",v.ui_frame_center},
            {"ui_frame_extension",unsigned(v.ui_frame_extension)}}},
        {"samplers",samplers},{"texture_transforms",words(v.texture_transforms)},
        {"forward_transforms",words(v.forward_transforms)},{"diffuse",words(v.forward_diffuse)},
        {"specular",words(v.forward_specular)},{"texture_frames",v.texture_frames},
        {"bitmaps",bitmaps},{"pixel_constants",v.pixel_constants},{"vertex_constants",words(v.vertex_constants)},
        {"switches",v.shader_switches},{"brightness",{words(v.specular_brightness),words(v.ambient_brightness)}},
        {"lods",lods},{"layers",ranges(v.layers)},{"implementations",ranges(v.pass_implementations)},
        {"passes",passes},{"animation",animation(v.animation)},{"gaps",v.rendering_gaps}};
}
inline json texture(const Texture& v) {
    json mips=json::array();for(const auto& m:v.mips) {
        if(m.offset>v.pixels.size() || m.size>v.pixels.size()-m.offset)throw FormatError("Sky mip leaves its complete pixel owner");
        mips.push_back({{"width",m.width},{"height",m.height},{"depth",m.depth},{"offset",m.offset},{"size",m.size},
            {"sha256",sha256({v.pixels.data()+m.offset,m.size})}});
    }
    return {{"width",v.width},{"height",v.height},{"depth",v.depth},{"layers",v.layers},{"format",v.format},
        {"bytes",v.pixels.size()},{"sha256",sha256({v.pixels.data(),v.pixels.size()})},{"mips",mips}};
}
inline bool texture_equal(const Texture& a,const Texture& b) {
    // The acceptance gate compares every byte directly. Digests are diagnostics.
    if(a.width!=b.width || a.height!=b.height || a.depth!=b.depth || a.layers!=b.layers || a.format!=b.format ||
       a.pixels!=b.pixels || a.mips.size()!=b.mips.size())return false;
    for(unsigned i=0;i<a.mips.size();++i) {
        const auto& x=a.mips[i];const auto& y=b.mips[i];
        if(x.width!=y.width || x.height!=y.height || x.depth!=y.depth || x.offset!=y.offset || x.size!=y.size)return false;
    }
    return true;
}
inline json complete_semantic(const Material& value,const Cache& cache) {
    auto out=semantic(value,cache);
    out["bound_textures"]={texture(value.diffuse),texture(value.detail),texture(value.bump),texture(value.auxiliary)};
    out["animated_textures"]=json::array();
    for(const auto& stage:value.animated_textures) {
        json frames=json::array();for(const auto& frame:stage)frames.push_back(texture(frame));
        out["animated_textures"].push_back(std::move(frames));
    }
    out["environment_pass"]=value.environment_pass?complete_semantic(*value.environment_pass,cache):json(nullptr);
    out["planar_fog_pass"]=value.planar_fog_pass?complete_semantic(*value.planar_fog_pass,cache):json(nullptr);
    out["overlays_before"]=json::array();out["overlays_after"]=json::array();out["bloom_passes"]=json::array();
    out["world_planar_passes"]=json::array();
    for(const auto& pass:value.world_overlays_before)out["overlays_before"].push_back(pass?complete_semantic(*pass,cache):json(nullptr));
    for(const auto& pass:value.world_overlays_after)out["overlays_after"].push_back(pass?complete_semantic(*pass,cache):json(nullptr));
    for(const auto& pass:value.world_bloom_passes)out["bloom_passes"].push_back(pass?complete_semantic(*pass,cache):json(nullptr));
    for(const auto& pass:value.world_planar_passes)out["world_planar_passes"].push_back(pass?complete_semantic(*pass,cache):json(nullptr));
    out["object_render_colors"]=nullptr;
    if(value.object_render_colors) {
        out["object_render_colors"]=json::array();
        for(const auto& color:*value.object_render_colors)out["object_render_colors"].push_back(vector(color));
    }
    return out;
}
inline std::string semantic_digest(const Material& value,const Cache& cache) {
    const auto serialized=complete_semantic(value,cache).dump();
    return sha256(std::as_bytes(std::span(serialized)));
}
} // namespace halo2::sky_audit
