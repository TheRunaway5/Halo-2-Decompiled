#include "halo2/world_material.hpp"
#include "halo2/source_shader_texture.hpp"
#include "halo2/source_vertex_extern.hpp"
#include "halo2/source_shader_state.hpp"
#include "halo2/water.hpp"
#include "halo2/texture.hpp"
#include "halo2/appearance.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/effect_light.hpp"
#include "halo2/pstencil/material.hpp"
#include "halo2/pstencil/projected_light_material.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
namespace {
std::vector<std::size_t> layer_passes(const Material &m, unsigned layer,
                                    unsigned implementation) {
  std::vector<std::size_t> result;
  if (m.lods.empty()) return result;
  if(m.selected_shader_lod>=m.lods.size())
    throw FormatError("selected world shader LOD outside authored template");
  const auto &lod = m.lods[m.selected_shader_lod];
  if (!(lod.available_layers & (1u << layer))) return result;
  if (layer >= lod.layers.count() || lod.layers.first() + layer >= m.layers.size())
    throw FormatError("world layer outside authored LOD");
  const auto range = m.layers[lod.layers.first() + layer];
  for (std::size_t i = 0; i < m.passes.size(); ++i) {
    const auto &p = m.passes[i];
    if (p.template_pass >= range.first() &&
        p.template_pass - range.first() < range.count() &&
        p.implementation == implementation)
      result.push_back(i);
  }
  return result;
}

bool lighting_vertex(const MaterialPass &p, bool per_vertex) {
  const auto leaf = p.vertex_shader.substr(p.vertex_shader.find_last_of('\\') + 1);
  return leaf.starts_with(per_vertex ? "lightmap_pervertex" : "lightmap_perpixel");
}

bool environment_vertex(const MaterialPass& p) {
  // Both authored families execute their own original compiled program.
  // Cairo's fast/masked pass selects stencil_env, not the bumped variant.
  return p.vertex_shader.ends_with("\\stencil_env_bumped_tangentspace") ||
      p.vertex_shader.ends_with("\\stencil_env");
}

bool volume_addressing_supported(const PixelProgram &program,unsigned stage) {
  const auto mode=(program.words[54]>>(stage*5))&31;
  // The production texture interpreter preserves literal PROJECT2D (R=0)
  // and PROJECT3D (S/T/R divided by Q). Bit100h requires original runtime
  // mode rewriting in 3FA3D0/3FFA80 and remains outside this binder.
  return (mode==1 || mode==2) && !(program.words[59]&0x100);
}

std::shared_ptr<Material> bind_pass(const Cache &cache, const Material &m,
                                  std::size_t selected, unsigned vertex,
                                  bool per_vertex,
                                  const ObjectLightingState *lighting=nullptr,
                                  bool volume_textures=false,
                                  const float *shadow_opacity=nullptr,
                                  const float *camouflage=nullptr,
                                  const EffectLightUniforms *effect_light=nullptr,
                                  const EffectLightDefinition *projected_definition=nullptr,
                                  bool load_images=true) {
  const auto &p = m.passes.at(selected);
  // No unresolved uniform is silently substituted. Pixel brightness externs
  // 21/22 were resolved from the shader by Cache::load_material (1BBF0).
  const auto lighting_source=[](unsigned source) {
    return (source>=14 && source<=17) || source==23 || source==24 || source==35;
  };
  if (std::any_of(p.vertex_externs.begin(), p.vertex_externs.end(), [&](const auto &r) {
        // The world overlay uses the default ObjectChangePrimary RGB.
        // 19BE0 encodes component/mode in destination, not byte +2.
        return !(r.destination==0x1c8a && r.source==3) &&
            !(lighting && lighting_source(r.source) && ((r.destination>>5)&31)==4);
      }) ||
      std::any_of(p.pixel_externs.begin(), p.pixel_externs.end(), [&](const auto &r) {
        return r.source != 21 && r.source != 22 &&
            !((r.source==3 || r.source==4) && (r.component&7)==4) &&
            !(lighting && lighting_source(r.source) && (r.component&7)==4) &&
            !(lighting && r.source==23 && (r.component&7)<4) &&
            !(shadow_opacity && r.source==17 && (r.component&7)<4) &&
            !(camouflage && r.source==37 && (r.component&7)<4) &&
            !(effect_light && (r.source==5 || r.source==6) && (r.component&7)==4) &&
            !(r.source==0 && r.component==4 && r.destination==0x800);
      }))
    return {};
  auto pass = std::make_shared<Material>();
  pass->name = m.name;
  pass->source_shader_datum=m.source_shader_datum;
  pass->selected_shader_lod=m.selected_shader_lod;
  pass->template_name = m.template_name;
  pass->bitmaps = m.bitmaps;
  pass->pixel_constants = m.pixel_constants;
  pass->vertex_constants = m.vertex_constants;
  pass->lods = m.lods;
  pass->layers = m.layers;
  pass->pass_implementations = m.pass_implementations;
  pass->passes = m.passes;
  pass->animation = m.animation;
  pass->shader_switches = m.shader_switches;
  pass->specular_brightness = m.specular_brightness;
  pass->ambient_brightness = m.ambient_brightness;
  pass->world_pass = true;
  pass->forward_pass = true;
  pass->forward_pass_index = unsigned(selected);
  pass->forward_vertex = vertex;
  pass->per_vertex_lighting = per_vertex;
  pass->forward_program = p.program;
  pass->vertex_program = p.vertex_code;
  pass->vertex_declaration = p.vertex_declaration;
  pass->program_constants = p.constants;
  // 33980 color slot3 ->339AE, with a null BSP object context, reads
  // RGB(1,1,1) through468710 ->4409E4. Apply the actual19BE0 destination
  // encoding; this is distinct from17000's parameter-vector remapper.
  std::array<ShaderVector,4> change_colors{};change_colors[3]={1,1,1,0};
  std::vector<ShaderRemap> change_color_mappings;
  for(const auto &r:p.vertex_externs)if(r.source==3)change_color_mappings.push_back(r);
  (void)remap_source_vertex_externs(pass->program_constants,{},change_colors,change_color_mappings);
  // Animation starts from the already-resolved object-context bindings, as
  // 19BE0 does after the overlay constants in the original submission.
  pass->passes[selected].constants = pass->program_constants;
  pass->forward_diffuse = p.constants[0][10];
  pass->forward_specular = p.constants[0][11];
  std::copy_n(p.constants[1].begin(), 8, pass->forward_transforms.begin());
  std::copy(p.texture_states.begin(), p.texture_states.end(), pass->texture_states.begin());
  const Texture zero{1, 1, 11, std::vector<std::byte>(4), {}};
  if(load_images)pass->diffuse = pass->detail = pass->bump = pass->auxiliary = zero;
  Texture *stages[]{&pass->diffuse, &pass->detail, &pass->bump, &pass->auxiliary};
  if(load_images)for (const auto &binding : p.textures) {
    if (binding.stage >= 4) throw FormatError("world texture stage outside NV2A slots");
    if (binding.external == 3 && binding.stage == 0) {
      pass->accumulation_texture = true;
      continue;
    }
    if (binding.external == 11 && binding.stage == 3) {
      pass->baked_lightmap_texture = true;
      continue;
    }
    if (binding.external == 19 && binding.stage == 1) {
      pass->water_reflection_texture = true;
      continue;
    }
    if (binding.external == 20 && binding.stage == 2) {
      pass->water_refraction_texture = true;
      continue;
    }
    if (binding.external == 27 && binding.stage == 0) {
      pass->scope_texture = true;
      // 19530 binds target16 during first person and the authored matg
      // rasterizer default outside it. The renderer overrides only FP draws.
      *stages[binding.stage]=cache.load_scope_default_bitmap();
      continue;
    }
    if(shadow_opacity && binding.external==8 && binding.stage==0)continue;
    if(effect_light && binding.external==9) {
      *stages[binding.stage]=cache.load_effect_light_attenuation_bitmap();
      continue;
    }
    if(effect_light && binding.external==10) {
      //19533/19671/19699 ->144D0. The ordinary non-animated gel reads
      //ligh+7C, with the original rasterizer's default2D bitmap for NONE.
      //The runtime-shader and shadow-target branches need separate owners.
      //The projected binder validates those before this binding is admitted.
      if(!projected_definition)return {};
      const Bytes light(projected_definition->bytes);
      *stages[binding.stage]=light.u32(0x7c)==0xffffffffu?
          cache.load_scope_default_bitmap():cache.load_bitmap(light.u32(0x7c),0);
      continue;
    }
    if (binding.external == 1) {
      const auto bitmap=source_shader_texture_reference(cache,m.source_shader_datum,1);
      *stages[binding.stage] = cache.load_bitmap(bitmap.bitmap,bitmap.frame);
      if (stages[binding.stage]->layers != 6)
        throw FormatError("world normalization texture is not a cube");
      continue;
    }
    if (binding.external) return {};
    if (binding.parameter == 255) {
      // No-bump alpha-test variants use PASSTHRU (4): the coordinates,
      // rather than a bitmap fetch, supply that stage's combiner input.
      const auto mode = (p.program.words[54] >> (binding.stage * 5)) & 31;
      if (mode != 0 && mode != 4 && mode != 5 && mode != 17) return {};
      continue;
    }
    const auto remap = std::find_if(p.bitmap_remaps.begin(), p.bitmap_remaps.end(),
                                  [&](const auto &r) {
      return (r.destination & 255) == binding.parameter;
    });
    if (remap == p.bitmap_remaps.end() || remap->source >= m.bitmaps.size())
      throw FormatError("world bitmap remap outside shader");
    const auto &bitmap = m.bitmaps[remap->source];
    *stages[binding.stage] = cache.load_bitmap(bitmap.datum, bitmap.index);
    if(stages[binding.stage]->depth) {
      if(!volume_textures)throw FormatError("unsupported bitmap dimensionality");
      if(!volume_addressing_supported(p.program,binding.stage))
        throw FormatError("unsupported volume texture addressing");
    }
  }
  // Retail 1BBF0 resets these before 18900 applies each authored pass.
  unsigned source = 1, destination = 0;
  unsigned blend_operation=0x8006,blend_color=0;
  bool blend = false;
  for (const auto &state : p.states) {
    switch (state.index) {
    case 57: pass->forward_raster.depth_function = state.value; break;
    case 58: pass->forward_raster.alpha_function = state.value; break;
    case 59: blend = state.value != 0; break;
    case 60: pass->forward_raster.alpha_test = state.value; break;
    case 61: pass->forward_raster.alpha_reference = state.value; break;
    case 62: source = state.value; break;
    case 63: destination = state.value; break;
    case 64: pass->forward_raster.depth_write = state.value; break;
    case 67: pass->forward_raster.color_write = state.value; break;
    case 74: blend_operation=state.value;break;
    case 75: blend_color=state.value;break;
    case 143: pass->forward_raster.depth_enable = state.value; break;
    case 147: pass->forward_raster.cull = state.value; break;
    default: break;
    }
  }
  pass->forward_blend_color=blend_color;
  if (blend) {
    if(blend_operation!=0x8006)return {};
    if (source == 1 && destination == 1) pass->forward_blend = 3;
    else if (source == 1 && destination == 0x303) pass->forward_blend = 2;
    // Authored cinematic alpha_detail_masked: constant00FFFFFF multiplies
    // source RGB by one and source alpha by zero. Mode5 retains that RGBA
    // contract; ordinary ONE/INV_SRC_ALPHA would add source alpha instead.
    else if(source==0x8001 && destination==0x303 && blend_color==0x00ffffff && p.state_externs.empty())
      pass->forward_blend=5;
    else if (source == 0x302 && destination == 0x303) pass->forward_blend = 1;
    else if (source == 0x306 && destination == 0x300) pass->forward_blend = 6;
    else if (source == 0x305 && destination == 0x304) pass->forward_blend = 7;
    else if (source == 0x304 && destination == 1) pass->forward_blend = 8;
    else if (shadow_opacity && source == 0x306 && destination == 0) pass->forward_blend = 9;
    // Sphere light passes use ONE + CONSTANT_COLOR with literal00FFFFFF:
    // RGB accumulates, while destination alpha contributes zero.
    else if(effect_light && source==1 && destination==0x8001 && blend_color==0x00ffffff)
      pass->forward_blend=10;
    else if(source==1 && destination==0x8002)pass->forward_blend=11;
    else return {};
  }
  // 33980's null object context supplies white for change-color slots.
  // Static scenery replaces this with its placement's authored object colors.
  if(std::any_of(p.pixel_externs.begin(),p.pixel_externs.end(),[](const auto& r) {
    return (r.source==3 || r.source==4) && (r.component&7)==4;
  })) apply_object_colors(*pass,{{{1,1,1},{1,1,1},{1,1,1},{1,1,1}}});
  if(lighting)apply_object_lighting(*pass,*lighting);
  if(shadow_opacity) {
    std::array<float,256> scalars{};scalars[17]=*shadow_opacity;
    std::vector<ShaderRemap> mappings;
    for(const auto&r:p.pixel_externs)if(r.source==17 && (r.component&7)<4)mappings.push_back(r);
    remap_pixel_externs(pass->forward_program,scalars,{},mappings,p.fragments,m.shader_switches);
  }
  if(camouflage)bind_object_camouflage(*pass,*camouflage);
  if(effect_light) {
    std::array<ShaderVector,256> colors{};
    colors[5]=effect_light->constants[6];colors[6]=effect_light->constants[7];
    std::vector<ShaderRemap> mappings;
    for(const auto&r:p.pixel_externs)if(r.source==5 || r.source==6)mappings.push_back(r);
    remap_pixel_externs(pass->forward_program,{},colors,mappings,p.fragments,m.shader_switches);
    pass->passes[selected].program=pass->forward_program;
    // The production VP conduit stores c18..33 in the material UBO and c34+
    // in the extended bank. 3A550 overwrites the overlapping c31..33 words.
    std::copy_n(effect_light->constants.begin(),3,pass->program_constants[1].begin()+13);
    pass->passes[selected].constants=pass->program_constants;
    auto bank=std::make_shared<VertexProgramBank>();bank->first_constant=34;
    bank->constants.assign(effect_light->constants.begin()+3,effect_light->constants.end());
    pass->vertex_bank=std::move(bank);
  }
  if(load_images)prepare_bitmap_animation(cache, *pass);
  if(load_images)for(unsigned stage=0;stage<4;++stage) {
    // Animated frame selections retain the same image dimensionality
    // requirements as the initially bound bitmap.
    for(const auto &frame:pass->animated_textures[stage]) {
      if(bool(frame.depth)!=bool(stages[stage]->depth))
        throw FormatError("animated bitmap changes volume dimensionality");
      if(!frame.depth)continue;
      if(!volume_textures)throw FormatError("unsupported bitmap dimensionality");
      if(!volume_addressing_supported(p.program,stage))
        throw FormatError("unsupported volume texture addressing");
    }
    const auto sign=pass->texture_states[stage].color_sign;
    if(!sign)continue;
    *stages[stage]=prepare_signed_texture(std::move(*stages[stage]),sign);
    for(auto &frame:pass->animated_textures[stage])
      frame=prepare_signed_texture(std::move(frame),sign);
  }
  return pass;
}
}

std::optional<Material> pstencil::bind_light_pass(const Cache& cache,const Material& authored,
    unsigned index,const EffectLightUniforms& uniforms) {
  auto result=bind_pass(cache,authored,index,6,false,nullptr,true,nullptr,nullptr,&uniforms);
  if(!result)return {};return std::move(*result);
}
std::optional<Material> pstencil::complete_light_pass(const Cache& cache,const Material& authored,
    unsigned index,const EffectLightUniforms& uniforms) {
  auto result=bind_pass(cache,authored,index,6,false,nullptr,true,nullptr,nullptr,&uniforms,nullptr,false);
  if(!result)return {};return std::move(*result);
}
std::optional<Material> pstencil::bind_texture_mapping_pass(const Cache& cache,const Material& authored) {
  const auto selected=layer_passes(authored,1,0);
  if(selected.size()!=1 || !authored.passes[selected[0]].vertex_shader.ends_with("\\stencil_texture"))return {};
  auto result=bind_pass(cache,authored,selected[0],3,false,nullptr,true);
  if(!result)return {};return std::move(*result);
}

void bind_object_camouflage(Material &material,float camouflage,std::uint16_t control) {
  if(!std::isfinite(camouflage) || camouflage<0 || camouflage>1)
    throw FormatError("Camouflage material context outside finite unit range");
  const auto child=[&](std::shared_ptr<Material> &pass) {
    if(!pass)return;
    if(!pass.unique())pass=std::make_shared<Material>(*pass);
    bind_object_camouflage(*pass,camouflage,control);
  };
  child(material.environment_pass);child(material.planar_fog_pass);
  for(auto &pass:material.world_overlays_before)child(pass);
  for(auto &pass:material.world_overlays_after)child(pass);
  for(auto &pass:material.world_bloom_passes)child(pass);
  for(auto &pass:material.world_planar_passes)child(pass);
  if(!material.forward_pass)return;
  if(material.forward_pass_index>=material.passes.size())
    throw FormatError("Camouflage selected pass outside material bank");
  auto &pass=material.passes[material.forward_pass_index];
  std::vector<ShaderRemap> mappings;
  for(const auto &mapping:pass.pixel_externs)
    if(mapping.source==37 && (mapping.component&7)<4)mappings.push_back(mapping);
  std::array<float,256> scalars{};scalars[37]=camouflage;
  // Keep the live bank for a pass with no animated overlays, and update the
  // retained base from which17420 reconstructs the next animated bank.
  remap_pixel_externs(material.forward_program,scalars,{},mappings,pass.fragments,material.shader_switches);
  remap_pixel_externs(pass.program,scalars,{},mappings,pass.fragments,material.shader_switches);
  mappings.clear();
  for(const auto &mapping:pass.vertex_externs)
    if(mapping.source==37 && ((mapping.destination>>5)&31)<4)mappings.push_back(mapping);
  (void)remap_source_vertex_externs(material.program_constants,scalars,{},mappings);
  (void)remap_source_vertex_externs(pass.constants,scalars,{},mappings);
  mappings.clear();
  for(const auto &mapping:pass.state_externs)
    if((mapping.destination>>8)!=1 && mapping.source==40)mappings.push_back(mapping);
  if(!mappings.empty()) {
    std::array<std::uint32_t,256> states{};
    states[75]=material.forward_blend_color;
    std::array<ShaderVector,41> colors{};colors[40]={camouflage,camouflage,camouflage,0};
    for(const auto &write:remap_source_shader_states(states,{},colors,mappings,control)) {
      if(write.index!=75)throw FormatError("Camouflage color requires its separate raster state consumer");
      material.forward_blend_color=write.value;
    }
  }
}

Texture Cache::load_scope_default_bitmap()const {
  const auto globals=query_tag({.group=tag_groups::globals});
  if(!globals)throw FormatError("scope rasterizer globals missing");
  const Bytes data(cache_bytes());const auto root=metadata_offset(globals->address,globals->size);
  data.require(root+0x108,8);
  if(data.u32(root+0x108)!=1)throw FormatError("scope rasterizer globals block count changed");
  const auto rasterizer=metadata_offset(data.u32(root+0x10c),0x68);
  if(data.u32(rasterizer+0x60)!=0x6269746du)throw FormatError("scope default reference is not bitmap");
  return load_bitmap(data.u32(rasterizer+0x64),0);
}

Texture Cache::load_effect_light_attenuation_bitmap()const {
  const auto globals=query_tag({.group=tag_groups::globals});
  if(!globals)throw FormatError("effect-light rasterizer globals missing");
  const Bytes data(cache_bytes());const auto root=metadata_offset(globals->address,globals->size);
  data.require(root+0x108,8);
  if(data.u32(root+0x108)!=1)throw FormatError("effect-light rasterizer globals block count changed");
  const auto rasterizer=metadata_offset(data.u32(root+0x10c),8);
  if(data.u32(rasterizer)!=0x6269746du)throw FormatError("effect-light attenuation reference is not bitmap");
  auto texture=load_bitmap(data.u32(rasterizer+4),0);
  if(!texture.depth || texture.layers!=1)throw FormatError("effect-light attenuation is not a volume");
  return texture;
}

std::vector<SourceShadowMaterial> prepare_shadow_material(const Cache &cache,
    const Material &authored,SourceShadowPhase phase,bool model_geometry,
    bool geometry_shadow_modifier,float opacity) {
  if(phase==SourceShadowPhase::Generate && geometry_shadow_modifier)return {};
  const unsigned layer=phase==SourceShadowPhase::Generate?10:11;
  const unsigned implementation=phase==SourceShadowPhase::Generate?(model_geometry?0:2):
      (geometry_shadow_modifier?2:4);
  const auto selected=layer_passes(authored,layer,implementation);
  if(selected.empty())return {};
  if(phase==SourceShadowPhase::Apply && implementation!=4)
    throw FormatError("unresolved source shadow application implementation");
  std::vector<SourceShadowMaterial> result;
  for(const auto index:selected) {
    const auto &source=authored.passes[index];
    const auto vertex=phase==SourceShadowPhase::Generate?"\\shadow_buffer_generation":
        "\\shadow_buffer_application_lightmap";
    if(!source.vertex_shader.ends_with(vertex))throw FormatError("unexpected source shadow vertex program");
    // Shadow combiners consume authored oD0, including receiver attenuation.
    // Category3 is stencil_texture's separate substituted diffuse consumer.
    auto material=bind_pass(cache,authored,index,10,false,nullptr,false,&opacity);
    if(!material)throw FormatError("unresolved source shadow material binding");
    SourceShadowMaterial pass;pass.material=std::move(*material);
    for(const auto&texture:source.textures)if(texture.external==8)pass.external_targets.at(texture.stage)=10;
    result.push_back(std::move(pass));
  }
  return result;
}

std::vector<Material> prepare_effect_light_diffuse_material(const Cache &cache,
    const Material &authored,const EffectLightDefinition &definition,
    const EffectLightUniforms &uniforms) {
  const Bytes light(definition.bytes);
  //497A0 only takes its alternate projection branch for authored kind1.
  //The actual frag definition is kind2; rejecting every nonzero kind dropped
  //the source grenade's diffuse pass even though it uses the ordinary bank.
  if(light.u16(4)!=0 || (light.u16(0x8e)!=0 && light.u16(0x8e)!=2) || (definition.flags&0x20))
    throw FormatError("Unresolved effect-light diffuse shape/falloff/gel owner");
  if(uniforms.color_shift>2)throw FormatError("Invalid source effect-light color shift");
  const auto selected=layer_passes(authored,12,1);
  std::vector<Material> result;
  for(auto index:selected) {
    auto context=authored;auto &source=context.passes[index];
    context.shader_switches[0]=context.shader_switches[1]=std::int32_t(uniforms.color_shift);
    context.shader_switches[2]=light.u16(0x8a);context.shader_switches[3]=light.u16(0x80);
    context.shader_switches[7]=!(definition.flags&2);
    for(const auto&f:source.fragments)if(f.switch_index==6)
      throw FormatError("Effect-light material requires source pass specular-mask context");
    source.program=assemble_pixel_program(source.definition,source.fragments,source.permutations,
        source.combiners,context.shader_switches);
    remap_pixel_constants(source.program,context.pixel_constants,source.pixel_remaps,
        source.fragments,context.shader_switches);
    std::array<float,256> scalars{};scalars[21]=context.specular_brightness;
    scalars[22]=context.ambient_brightness;std::vector<ShaderRemap> brightness;
    for(const auto&r:source.pixel_externs)if(r.source==21 || r.source==22)brightness.push_back(r);
    remap_pixel_externs(source.program,scalars,{},brightness,source.fragments,context.shader_switches);
    auto material=bind_pass(cache,context,index,6,false,nullptr,true,nullptr,nullptr,&uniforms);
    if(!material)throw FormatError("Unresolved effect-light diffuse material binding");
    result.push_back(std::move(*material));
  }
  return result;
}

static std::vector<Material> projected_light_material(const Cache &cache,
    const Material &authored,const EffectLightDefinition &definition,
    const EffectLightUniforms &uniforms,unsigned implementation,std::uint32_t feature_mask,bool load_images) {
  const Bytes light(definition.bytes);
  // Only the literal projected/static-gel branches are admitted here. The
  // caller supplies whole49790 constants and the original31150 feature mask.
  if(light.u16(4)==0 || implementation>4 || implementation==1 ||
      bool(definition.flags&0x20u)!=(implementation==4) ||
      (definition.flags&0x80000u) || light.u32(0xe0)!=0xffffffffu ||
      light.u32(0xc4) || light.u32(0xcc) || light.u32(0xd4))
    throw FormatError("Projected light material requires its static original shape/gel branch");
  if(implementation!=pstencil::projected_light_implementation(definition.flags,
      feature_mask,authored.shader_switches[4]!=0))
    throw FormatError("Projected light implementation differs from original30D80 selection");
  if(uniforms.color_shift>2)throw FormatError("Invalid source effect-light color shift");
  const auto selected=layer_passes(authored,12,implementation);
  std::vector<Material> result;
  for(auto index:selected) {
    auto context=authored;auto &source=context.passes[index];
    context.shader_switches[0]=context.shader_switches[1]=std::int32_t(uniforms.color_shift);
    context.shader_switches[2]=light.u16(0x8a);context.shader_switches[3]=light.u16(0x80);
    context.shader_switches[7]=!(definition.flags&2);
    context.shader_switches[6]=pstencil::projected_light_specular_mask(definition.flags,
        feature_mask,context.lods.at(context.selected_shader_lod).available_layers);
    source.program=assemble_pixel_program(source.definition,source.fragments,source.permutations,
        source.combiners,context.shader_switches);
    remap_pixel_constants(source.program,context.pixel_constants,source.pixel_remaps,
        source.fragments,context.shader_switches);
    std::array<float,256> scalars{};scalars[21]=context.specular_brightness;
    scalars[22]=context.ambient_brightness;std::vector<ShaderRemap> brightness;
    for(const auto&r:source.pixel_externs)if(r.source==21 || r.source==22)brightness.push_back(r);
    remap_pixel_externs(source.program,scalars,{},brightness,source.fragments,context.shader_switches);
    auto material=bind_pass(cache,context,index,6,false,nullptr,true,nullptr,nullptr,&uniforms,&definition,load_images);
    if(!material)throw FormatError("Unresolved effect-light diffuse material binding");
    result.push_back(std::move(*material));
  }
  return result;
}
std::vector<Material> pstencil::prepare_projected_light_material(const Cache& cache,
    const Material& authored,const EffectLightDefinition& definition,
    const EffectLightUniforms& uniforms,unsigned implementation,std::uint32_t features) {
  return projected_light_material(cache,authored,definition,uniforms,implementation,features,true);
}
std::vector<Material> pstencil::complete_projected_light_constants(const Cache& cache,
    const Material& authored,const EffectLightDefinition& definition,
    const EffectLightUniforms& uniforms,unsigned implementation,std::uint32_t features) {
  return projected_light_material(cache,authored,definition,uniforms,implementation,features,false);
}

static bool prepare_world_lighting(const Cache &,Material &,unsigned,const float * = nullptr,
                                  const ObjectLightingState * = nullptr,bool = false);

bool prepare_shield_overlay_material(const Cache &cache,Material &material) {
  const auto selected=layer_passes(material,15,0);
  if(selected.size()!=1)return false;
  const auto &pass=material.passes[selected.front()];
  const bool plain=pass.name=="shaders\\shader_passes\\plasma\\plasma_shield" && pass.pixel_externs.empty();
  // Authored TU1.5 Elite/Arbiter pass: two ObjectChangePrimary RGB
  // remaps. bind_pass and apply_object_render_colors retain the original
  // compiled pixel program and19CA0 remapping; no color result is substituted.
  const bool change_color=pass.name=="shaders\\shader_passes\\plasma\\plasma_shield_change_color" &&
      pass.pixel_externs.size()==2 &&
      pass.pixel_externs[0].source==3 && pass.pixel_externs[0].component==4 && pass.pixel_externs[0].destination==0x00c0 &&
      pass.pixel_externs[1].source==3 && pass.pixel_externs[1].component==4 && pass.pixel_externs[1].destination==0x04c0;
  if((!plain && !change_color) || !pass.vertex_shader.ends_with("\\transparent_generic_offset") ||
     !pass.vertex_externs.empty())return false;
  auto bound=bind_pass(cache,material,selected.front(),1,false,nullptr,true);
  if(!bound)return false;
  material=std::move(*bound);return true;
}
bool prepare_simple_world_material(const Cache& cache,Material& material) {
  if(material.selected_shader_lod!=0)return false;
  const auto selected=layer_passes(material,15,0);
  if(selected.size()!=1)return false;
  const auto& pass=material.passes[selected.front()];
  if(!pass.vertex_shader.ends_with("\\transparent_simple_world") ||
      !pass.vertex_externs.empty() || !pass.pixel_externs.empty())return false;
  auto bound=bind_pass(cache,material,selected.front(),12,false);
  if(!bound)return false;
  material=std::move(*bound);return true;
}

bool prepare_dynamic_material(const Cache &cache,Material &material,
                              const ObjectLightingState &lighting,bool volume_textures,
                              std::optional<float> camouflage) {
  if(camouflage && (!std::isfinite(*camouflage) || *camouflage<0 || *camouflage>1))
    throw FormatError("object camouflage context must be source-clamped");
  const auto selected=layer_passes(material,3,2);
  // Object classification controls layer3 only. Transparent/emissive-only
  // materials retain implementation0 in their authored independent layers.
  if(selected.empty())return prepare_world_lighting(cache,material,2,
      camouflage ? &*camouflage : nullptr,&lighting,volume_textures);
  if(selected.size()!=1)return false;
  const auto& p=material.passes[selected[0]];
  const auto leaf=p.vertex_shader.substr(p.vertex_shader.find_last_of('\\')+1);
  if(!leaf.starts_with("lightmap_dynamic"))return false;
  auto illuminated=bind_pass(cache,material,selected[0],6,false,&lighting,volume_textures);
  if(!illuminated)return false;
  std::shared_ptr<Material> result;
  if(illuminated->accumulation_texture) {
    const auto base=layer_passes(material,1,0);
    if(base.size()!=1)return false;
    const auto& vertex=material.passes[base[0]].vertex_shader;
    const auto name=vertex.substr(vertex.find_last_of('\\')+1);
    if(!name.starts_with("stencil_texture") && !name.starts_with("lightmap_dynamic"))return false;
    result=bind_pass(cache,material,base[0],name.starts_with("stencil_texture")?3:6,false,&lighting,volume_textures);
    if(!result)return false;
    result->environment_pass=std::move(illuminated);
  } else result=std::move(illuminated);
  for(const auto index:layer_passes(material,2,0)) {
    auto overlay=bind_pass(cache,material,index,3,false,&lighting,volume_textures);
    if(!overlay)return false;
    result->world_overlays_before.push_back(std::move(overlay));
  }
  // Original2BCD0/404B0 submit independent environment layer8 before7.
  // Chief FP arms' separate captured895/903 draws use this exact pass.
  for(const auto index:layer_passes(material,8,0)) {
    const auto &vertex=material.passes[index].vertex_shader;
    if(!vertex.ends_with("\\stencil_env") &&
       !vertex.ends_with("\\stencil_env_bumped_tangentspace") &&
       !vertex.ends_with("\\stencil_env_super_fast"))return false;
    auto environment=bind_pass(cache,material,index,4,false,&lighting,volume_textures);
    if(!environment)return false;
    result->world_overlays_after.push_back(std::move(environment));
  }
  for(const auto index:layer_passes(material,7,0)) {
    auto emissive=bind_pass(cache,material,index,2,false,&lighting,volume_textures);
    if(!emissive)return false;
    result->world_overlays_after.push_back(std::move(emissive));
  }
  material=std::move(*result);return true;
}

bool prepare_prt_material(const Cache&cache,Material&material,std::shared_ptr<const VertexProgramBank> bank){
  if(!bank || !bank->prt_inputs || bank->first_constant!=68 ||
      (bank->constants.size()!=42 && bank->constants.size()!=82))return false;
  const auto projected=layer_passes(material,5,0),base=layer_passes(material,1,0);
  // This bounded path covers the original supplied PRT-only machine family.
  // Mixed layer3/5 materials require both independently scheduled passes.
  if(projected.size()!=1 || base.size()!=1 || !layer_passes(material,3,2).empty())return false;
  const auto& projected_vertex=material.passes[projected[0]].vertex_shader;
  if((!projected_vertex.ends_with("\\prt") && !projected_vertex.ends_with("\\prt_lightmap")) ||
      !material.passes[base[0]].vertex_shader.ends_with("\\stencil_texture"))return false;
  auto illuminated=bind_pass(cache,material,projected[0],9,false);
  auto result=bind_pass(cache,material,base[0],3,false);
  if(!illuminated || !result || !illuminated->accumulation_texture)return false;
  illuminated->vertex_bank=std::move(bank);result->environment_pass=std::move(illuminated);
  for(const auto index:layer_passes(material,2,0)){
    auto overlay=bind_pass(cache,material,index,3,false);if(!overlay)return false;
    result->world_overlays_before.push_back(std::move(overlay));
  }
  //2BCD0: layer5 follows3 and precedes8/7. The base accumulation
  //pass and layer2 have already run before this environment child.
  for(const auto index:layer_passes(material,8,0)){
    if(!material.passes[index].vertex_shader.ends_with("\\stencil_env_bumped_tangentspace"))return false;
    auto environment=bind_pass(cache,material,index,4,false);if(!environment)return false;
    result->world_overlays_after.push_back(std::move(environment));
  }
  for(const auto index:layer_passes(material,7,0)){
    auto emissive=bind_pass(cache,material,index,2,false);if(!emissive)return false;
    result->world_overlays_after.push_back(std::move(emissive));
  }
  for(const auto index:layer_passes(material,17,0)){
    if(!material.passes[index].vertex_shader.ends_with("\\stencil_texture"))return false;
    auto bloom=bind_pass(cache,material,index,3,false);if(!bloom)return false;
    bloom->world_bloom_pass=true;result->world_bloom_passes.push_back(std::move(bloom));
  }
  material=std::move(*result);return true;
}

static bool prepare_world_lighting(const Cache &cache, Material &material, unsigned implementation,
                                   const float *camouflage,const ObjectLightingState *object_lighting,
                                   bool volume_textures) {
  const bool per_vertex = implementation == 1;
  if (prepare_water_material(cache,material)) return true;
  const auto edges=layer_passes(material,6,0);
  if(edges.size()==1 && material.passes[edges[0]].vertex_shader.ends_with("\\water_edge_blend")) {
    if(auto edge=bind_pass(cache,material,edges[0],3,false)) {
      edge->water_pass=true;
      material=std::move(*edge);return true;
    }
    return false;
  }
  // 16B90 resolves template LOD -> layer -> pass -> implementation with
  // independent packed ranges. Do not search a lower LOD for a familiar name.
  const auto lighting = layer_passes(material, 3, implementation);
  const auto emissives = layer_passes(material, 7, 0);
  const auto overlays = layer_passes(material, 2, 0);
  const auto late = layer_passes(material,15,0);
  std::shared_ptr<Material> result;
  if (lighting.empty()) {
    // The authored Ascension jump-pad is emissive-only in its highest LOD.
    // It still needs its original additive/color/depth raster state.
    if (emissives.size() == 1) result = bind_pass(cache, material, emissives[0], 2, false);
    else if (overlays.size() == 1) result = bind_pass(cache, material, overlays[0], 3, false);
    else if(late.size()==1) {
      const auto &p=material.passes[late[0]];
      const auto leaf=p.vertex_shader.substr(p.vertex_shader.find_last_of('\\')+1);
      if(leaf.starts_with("transparent_generic") &&
          (p.name.starts_with("shaders\\shader_passes\\transparent\\") ||
           p.name=="shaders\\shader_passes\\plasma\\plasma_alpha" ||
           p.name=="shaders\\shader_passes\\plasma\\plasma_shield" ||
           (camouflage && p.name=="shaders\\shader_passes\\plasma\\plasma_alpha_active_camo")))
        result=bind_pass(cache,material,late[0],1,false,object_lighting,volume_textures,nullptr,camouflage);
      else if(leaf=="stencil_emissive")result=bind_pass(cache,material,late[0],2,false);
    }
    else return false;
    if (!result) return false;
  } else {
    if (lighting.size() != 1)
      return false;
    const auto &selected = material.passes[lighting[0]];
    if (implementation == 3) {
      const auto leaf = selected.vertex_shader.substr(selected.vertex_shader.find_last_of('\\') + 1);
      // Default lighting may consume object colors. bind_pass validates each
      // extern; no dynamic-lighting bank is substituted for classification3.
      if (!leaf.starts_with("lightmap_dynamic")) return false;
    } else if (!lighting_vertex(selected, per_vertex)) return false;
    auto illuminated = bind_pass(cache, material, lighting[0], 6, per_vertex);
    if (!illuminated) return false;
    const auto base = layer_passes(material, 1, 0);
    if (illuminated->accumulation_texture) {
      if (base.size() != 1) return false;
      const auto &vertex_name = material.passes[base[0]].vertex_shader;
      const auto leaf = vertex_name.substr(vertex_name.find_last_of('\\') + 1);
      if (!leaf.starts_with("stencil_texture")) return false;
      result = bind_pass(cache, material, base[0], 3, false);
      if (!result) return false;
      result->environment_pass = std::move(illuminated);
    } else {
      result = std::move(illuminated);
      if (implementation < 2) {
        result->static_lightmap_pass = true;
        result->lightmap_program = result->forward_program;
      }
    }
  }
  for (const auto index : overlays) {
    if (lighting.empty()) continue; // Standalone layer 2 material.
    if (auto overlay = bind_pass(cache, material, index, 3, false))
      result->world_overlays_before.push_back(std::move(overlay));
    else result->rendering_gaps.push_back("world overlay requires unresolved externs: " + material.passes[index].name);
  }
  // Retail 2BCD0 submits environment layer 8 before self-illumination 7.
  for(const auto index:layer_passes(material,8,0)) {
    if(environment_vertex(material.passes[index])) {
      if(auto environment=bind_pass(cache,material,index,4,false))
        result->world_overlays_after.push_back(std::move(environment));
      else result->rendering_gaps.push_back("world environment requires unresolved externs: "+material.passes[index].name);
    } else result->rendering_gaps.push_back("world environment vertex program has no source execution: "+material.passes[index].name);
  }
  for (const auto index : emissives) {
    if (lighting.empty()) continue; // Already the main material pass.
    if (auto emissive = bind_pass(cache, material, index, 2, false))
      result->world_overlays_after.push_back(std::move(emissive));
    else result->rendering_gaps.push_back("world self-illumination requires unresolved externs: " + material.passes[index].name);
  }
  for(const auto index:layer_passes(material,17,0)) {
    if(material.passes[index].vertex_shader.ends_with("\\stencil_texture")) {
      if(auto bloom=bind_pass(cache,material,index,3,false)) {
        bloom->world_bloom_pass=true;result->world_bloom_passes.push_back(std::move(bloom));
      } else result->rendering_gaps.push_back("world bloom requires unresolved externs: "+material.passes[index].name);
    } else result->rendering_gaps.push_back("world bloom vertex program has no source execution: "+material.passes[index].name);
  }
  for (const unsigned layer : {6u})
    if (!layer_passes(material, layer, 0).empty())
      result->rendering_gaps.push_back("world layer " + std::to_string(layer) + " requires separate authored target scheduling");
  // Retain concrete asset failures while replacing stale collapsed-pass gaps.
  for (const auto &gap : material.rendering_gaps)
    if (!gap.starts_with("surface program bound;") &&
        !gap.starts_with("fog and complete pass scheduling") &&
        gap != "material uses an unsupported rendering pass")
      result->rendering_gaps.push_back(gap);
  material = std::move(*result);
  return true;
}
bool prepare_world_material(const Cache &cache, Material &material, bool per_vertex) {
  return prepare_world_lighting(cache, material, per_vertex ? 1u : 0u);
}
bool prepare_authored_lighting_material(const Cache &cache, Material &material) {
  if (layer_passes(material, 3, 3).empty()) return false;
  return prepare_world_lighting(cache, material, 3);
}
bool prepare_default_object_material(const Cache &cache,Material &material,
                                     std::optional<float> camouflage) {
  if(camouflage && (!std::isfinite(*camouflage) || *camouflage<0 || *camouflage>1))
    throw FormatError("object camouflage context must be source-clamped");
  return prepare_world_lighting(cache,material,3,camouflage ? &*camouflage : nullptr);
}
bool prepare_independent_object_material(const Cache &cache,Material &material) {
  constexpr auto supported=(1u<<2)|(1u<<7)|(1u<<15)|(1u<<17);
  if(material.selected_shader_lod>=material.lods.size() ||
      (material.lods[material.selected_shader_lod].available_layers&~supported))return false;
  const auto colors=material.object_render_colors;
  if(!prepare_world_lighting(cache,material,3))return false;
  if(colors)apply_object_render_colors(material,*colors);
  return true;
}
bool prepare_water_material(const Cache &cache,Material &material,bool infinite) {
  const auto passes=layer_passes(material,infinite?3u:15u,0);
  if(passes.size()!=1)return false;
  const auto &p=material.passes[passes[0]];
  const auto leaf=p.vertex_shader.substr(p.vertex_shader.find_last_of('\\')+1);
  if(infinite ? leaf!="water" && leaf!="water_static" : leaf!="water_pool" && leaf!="water_pool_static")return false;
  auto result=bind_pass(cache,material,passes[0],infinite?8u:7u,false);
  if(!result)return false;
  result->water_pass=true;
  for(const auto &gap:material.rendering_gaps)
    if(gap.starts_with("bitmap "))result->rendering_gaps.push_back(gap);
  material=std::move(*result);return true;
}
}
