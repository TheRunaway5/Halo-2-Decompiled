#include "halo2/particle_material.hpp"
#include "halo2/texture.hpp"
#include "halo2/mcc_particle_alpha_multiply_add.hpp"
#include "halo2/mcc_particle_pc_projection.hpp"
#include "halo2/compiled_particle_vertex_signature.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
struct PcParticleProfile {
  std::string_view shader_template,shader_pass;
  unsigned implementation;
  std::string_view vertex;
  unsigned classifications;
  std::array<std::size_t,3> constant_counts;
  std::vector<ShaderRemap> bitmaps,pixels,vertices;
  std::vector<ShaderState> states;
  std::vector<unsigned> samplers,defaults;
  std::vector<ShaderRemap> custom_pixels;
  std::vector<ShaderState> custom_states;
  std::vector<unsigned> custom_samplers;
  std::array<std::uint32_t,60> custom_definition;
};
#include "mcc_particle_pc_profiles.inc"
const PcParticleProfile* pc_profile(std::string_view stem,std::string_view pass,unsigned implementation) {
  for(const auto& profile:pc_particle_profiles)
    if(profile.shader_template==stem && profile.shader_pass==pass && profile.implementation==implementation)return &profile;
  return nullptr;
}
bool equal_remaps(const std::vector<ShaderRemap>& left,const std::vector<ShaderRemap>& right) {
  return std::ranges::equal(left,right,[](const auto& a,const auto& b) {
    return a.destination==b.destination && a.component==b.component && a.source==b.source;
  });
}
bool equal_bytes(std::span<const std::byte> left,const std::vector<unsigned>& right) {
  return std::ranges::equal(left,right,[](auto a,auto b){return std::to_integer<unsigned>(a)==b;});
}
}
bool mcc_particle_pc_projection_names(std::string_view stem,std::string_view pass,unsigned implementation) {
  return pc_profile(stem,pass,implementation)!=nullptr;
}
std::span<const std::byte> mcc_particle_pc_vertex_signature(std::string_view stem,
    std::string_view pass,unsigned implementation,unsigned classification,
    std::string_view source_vertex,unsigned source_classifications) {
  const auto* profile=pc_profile(stem,pass,implementation);
  if(!profile || source_vertex!=profile->vertex || source_classifications!=profile->classifications ||
      (implementation==1?classification!=1:classification<7 || classification>9))
    throw FormatError("MCC particle custom vertex interface differs from its authored PC owner");
  if(profile->vertex=="rasterizer\\vertex_shaders_dx9\\particle_distortion") {
    if(classification!=7)throw FormatError("MCC particle distortion requires its compiled class7 interface");
    return compiled_particle_distortion_vertex_signature();
  }
  return compiled_particle_vertex_signature(classification);
}
MaterialPass project_mcc_particle_pc_pass(std::string_view stem,const MaterialPass& source,
    std::array<std::size_t,3> constant_counts,std::span<const std::byte> samplers,
    std::span<const std::byte> defaults) {
  const auto* profile=pc_profile(stem,source.name,source.implementation);
  if(!profile || constant_counts!=profile->constant_counts ||
      !std::ranges::all_of(source.definition.words,[](auto v){return !v;}) ||
      !std::ranges::all_of(source.program.words,[](auto v){return !v;}) ||
      !source.fragments.empty() || !source.permutations.empty() || !source.combiners.empty() ||
      !source.pixel_externs.empty() || !source.vertex_externs.empty() || !source.state_externs.empty() ||
      !equal_remaps(source.bitmap_remaps,profile->bitmaps) || !equal_remaps(source.pixel_remaps,profile->pixels) ||
      !equal_remaps(source.vertex_remaps,profile->vertices) || !equal_bytes(samplers,profile->samplers) ||
      !equal_bytes(defaults,profile->defaults) ||
      !std::ranges::equal(source.states,profile->states,[](const auto& a,const auto& b) {
        return a.index==b.index && a.value==b.value;
      }))throw FormatError("MCC particle cooked pass/remap/state/sampler contract differs");
  const unsigned stages=profile->samplers.size()/24;
  if(source.textures.size()!=stages)throw FormatError("MCC particle cooked texture bindings differ");
  for(unsigned stage=0;stage<stages;++stage) {
    const auto& binding=source.textures[stage];
    const bool distortion=profile->vertex=="rasterizer\\vertex_shaders_dx9\\particle_distortion" && stage==1;
    const unsigned parameter=distortion?255:profile->constant_counts[0]==2?stage:0;
    if(binding.parameter!=parameter || binding.external!=(distortion?23:0) ||
        binding.stage!=stage || binding.flags)
      throw FormatError("MCC particle cooked texture binding owner differs");
  }
  auto result=source;
  result.definition.words=profile->custom_definition;result.program=result.definition;
  result.pixel_remaps=profile->custom_pixels;result.states=profile->custom_states;
  result.texture_states.clear();
  for(unsigned at=0;at<profile->custom_samplers.size();at+=24) {
    std::array<std::byte,24> packed{};
    for(unsigned byte=0;byte<24;++byte)packed[byte]=std::byte(profile->custom_samplers[at+byte]);
    result.texture_states.push_back(decode_texture_state(packed));
  }
  if(source.implementation==1)result.vertex_declaration=SourceVertexDeclaration{1,{0,3,14}};
  return result;
}
MaterialPass project_mcc_particle_alpha_multiply_add_pass(const MaterialPass& source,
    std::span<const std::byte> packed_samplers,
    std::span<const std::byte> packed_default_states) {
  constexpr std::array<ShaderState,11> pc_states{{{168,7},{27,1},{19,2},{20,6},{171,1},
      {14,0},{175,0},{15,1},{24,1},{25,5},{22,1}}};
  constexpr std::array<std::byte,24> pc_sampler{std::byte{0x20},std::byte{0x22},
      std::byte{1},std::byte{1},std::byte{1},std::byte{1}};
  constexpr std::array<std::byte,15> pc_defaults{std::byte{15},
      std::byte{0},std::byte{0},std::byte{0},std::byte{0},std::byte{24},
      std::byte{0},std::byte{0},std::byte{0},std::byte{0},std::byte{22},
      std::byte{2},std::byte{0},std::byte{0},std::byte{0}};
  const auto remap=[](const ShaderRemap& r,unsigned destination,unsigned source) {
    return r.destination==destination && r.source==source && r.component==0;
  };
  bool valid=source.implementation==0 && source.name==
      "shaders\\shader_passes\\transparent\\effect_shader_alpha_multiply_add" &&
      std::ranges::all_of(source.definition.words,[](auto word){return word==0;}) &&
      std::ranges::all_of(source.program.words,[](auto word){return word==0;}) &&
      source.fragments.empty() && source.pixel_externs.empty() && source.vertex_externs.empty() &&
      source.state_externs.empty() && source.permutations.empty() && source.combiners.empty() &&
      source.bitmap_remaps.size()==1 && remap(source.bitmap_remaps[0],0,0) &&
      source.pixel_remaps.empty() && source.vertex_remaps.size()==2 &&
      remap(source.vertex_remaps[0],432,0) && remap(source.vertex_remaps[1],7279,1) &&
      source.textures.size()==2 && source.states.size()==pc_states.size() &&
      packed_samplers.size()==48 && std::ranges::equal(packed_default_states,pc_defaults);
  if(valid)for(unsigned stage=0;stage<2;++stage) {
    const auto& texture=source.textures[stage];
    valid&=texture.parameter==0 && texture.external==0 && texture.stage==stage && texture.flags==0 &&
        std::ranges::equal(packed_samplers.subspan(stage*24,24),pc_sampler);
  }
  if(valid)for(unsigned n=0;n<pc_states.size();++n)
    valid&=source.states[n].index==pc_states[n].index && source.states[n].value==pc_states[n].value;
  if(!valid)throw FormatError("MCC alpha-multiply-add particle pass contract differs");

  auto result=source;
  // Complete fixed fragment identity already present in the custom Vulkan
  // particle catalog. The source color constants are supplied by the particle
  // draw; no D3D bytecode or reference-game asset is loaded here.
  result.definition.words={873993241u,404029440u,0,0,0,0,0,0,135593984u,6272u,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2048u,128u,0,0,0,0,0,0,
      872944649u,134545408u,0,0,0,0,0,0,0,0,0,2048u,128u,0,0,0,0,0,0,69890u,33u,0,0,0,0,0};
  result.program=result.definition;
  result.states={{67,0x10101u},{59,1},{62,1},{63,0x303u},{74,0x8006u},
      {64,0},{60,1},{61,1},{58,0x204u},{147,0}};
  constexpr std::array<std::byte,24> sampler{std::byte{0x20},std::byte{0x22},
      std::byte{0},std::byte{0},std::byte{1},std::byte{1},std::byte{1}};
  result.texture_states.assign(2,decode_texture_state(sampler));
  return result;
}
ParticleMaterial Cache::load_particle_material(std::uint32_t datum,unsigned layer,
    unsigned layer_pass,unsigned implementation) const {
  const Bytes b(cache_bytes());
  const auto find=[&](std::uint32_t id,const char *group)->const Tag& {
    const auto tag=query_tag({.handle=TagHandle{id}, .group=TagGroup::from_string(group), .require_address=true});
    if(!tag)throw FormatError("original particle material dependency missing");
    return *tag;
  };

  const auto range=[&](std::size_t at,TagBlockRange all) {
    const ShaderRange r{b.u16(at)};
    if(r.first()>all.count || r.count()>all.count-r.first())
      throw FormatError("original particle material range outside block");
    return r;
  };
  const auto remaps=[&](std::size_t at,TagBlockRange all) {
    const auto r=range(at,all);std::vector<ShaderRemap> result;
    for(std::size_t n=0;n<r.count();++n) {
      const auto i=all.offset+(r.first()+n)*4;
      result.push_back({b.u16(i),std::to_integer<std::uint8_t>(b.slice(i+2,1)[0]),
        std::to_integer<std::uint8_t>(b.slice(i+3,1)[0])});
    }
    return result;
  };
  ParticleMaterial result;
  result.vertex=load_particle_vertex_selection(datum,layer,layer_pass);
  const auto particle=query_tag({.handle=TagHandle{datum},.require_address=true});
  if(!particle || (particle->group!="prt3" && particle->group!="PRTM"))
    throw FormatError("original particle material owner missing");
  const bool model=particle->group=="PRTM";
  if(model && implementation!=1)throw FormatError("Particle model requires original implementation1");
  const auto root=metadata_offset(particle->address,model?0xe0:0x94);
  const auto shader=model?metadata_offset(find(b.u32(root+0x1c),"shad").address,0x28):root;
  const auto pp=metadata_block(shader+(model?0x20:0x8c),124);
  if(!pp.count)throw FormatError("original particle material definition missing");
  const auto bitmaps=metadata_block(pp.offset+4, 12),pixel=metadata_block(pp.offset+12, 4),
    vertex=metadata_block(pp.offset+20, 16);
  std::vector<std::uint32_t> pc(pixel.count);
  for(std::size_t n=0;n<pixel.count;++n)pc[n]=b.u32(pixel.offset+n*4);
  std::vector<ShaderVector> vc(vertex.count);
  for(std::size_t n=0;n<vertex.count;++n)for(unsigned c=0;c<4;++c)
    vc[n][c]=b.f32(vertex.offset+n*16+c*4);

  const auto &stem=find(result.vertex.template_datum,"stem");
  const auto templates=metadata_block(metadata_offset(stem.address,0x60)+0x58, 40);
  if(!templates.count)throw FormatError("original particle template missing");
  const auto lods=metadata_block(templates.offset, 10),layers=metadata_block(templates.offset+8, 2),
    passes=metadata_block(templates.offset+16, 10),maps=metadata_block(templates.offset+32, 4),
    implementations=metadata_block(templates.offset+24, 6);
  if(!lods.count)throw FormatError("original particle template LOD missing");
  const auto lr=range(lods.offset,layers);
  if(layer>=lr.count())throw FormatError("original particle layer missing");
  const auto pr=range(layers.offset+(lr.first()+layer)*2,passes);
  if(layer_pass>=pr.count())throw FormatError("original particle layer pass missing");
  const auto selected=pr.first()+layer_pass,entry=passes.offset+selected*10;
  const auto ir=range(entry+8,implementations);
  if(implementation>=ir.count())throw FormatError("original particle implementation missing");
  const auto mapping=implementations.offset+(ir.first()+implementation)*6;
  const auto &spas=find(result.vertex.pass_datum,"spas");
  const auto processed=metadata_block(metadata_offset(spas.address,0x24)+0x1c, 88);
  if(!processed.count)throw FormatError("original particle pass definition missing");
  const bool pc_pass=is_mcc() && !is_preserved_native_shader(spas);
  const bool pc_projection=pc_pass && mcc_particle_pc_projection_names(stem.name,spas.name,implementation);
  const unsigned implementation_stride=pc_pass?330:306;
  const auto impl=metadata_block(processed.offset,implementation_stride),bindings=metadata_block(processed.offset+8, 4),
    states=metadata_block(processed.offset+16, 5),samplers=metadata_block(processed.offset+24, 24),
    fragments=metadata_block(processed.offset+32, 4),externs=metadata_block(processed.offset+56, 4);
  if(implementation>=impl.count)throw FormatError("original particle pass implementation missing");
  const auto i=impl.offset+implementation*implementation_stride;
  auto &pass=result.pass;
  pass.name=shader_semantic_name(spas);pass.template_pass=selected;pass.implementation=implementation;
  const bool builtin_particle_program=is_mcc() && result.vertex.vertex_datum==0xffffffffu &&
      pc_pass && (pc_projection || (!model && mcc_particle_alpha_multiply_add_names(stem.name,spas.name)));
  pass.vertex_shader=builtin_particle_program?"custom_vulkan\\particle":find(result.vertex.vertex_datum,"vrtx").name;
  pass.vertex_code=result.vertex.program;
  pass.bitmap_remaps=remaps(mapping,maps);
  pass.pixel_remaps=remaps(mapping+2,maps);
  pass.vertex_remaps=remaps(mapping+4,maps);
  pass.state_externs=remaps(i+0x11e,externs);
  pass.pixel_externs=remaps(i+0x122,externs);
  pass.vertex_externs=remaps(i+0x124,externs);
  if(range(i+246,fragments).count() || !pass.pixel_externs.empty() || !pass.vertex_externs.empty())
    throw FormatError("particle material switches or external values require original draw state");
  for(unsigned w=0;w<60;++w)pass.definition.words[w]=b.u32(i+6+w*4);
  pass.program=pass.definition;
  const auto sr=range(i+2,states);
  for(std::size_t n=0;n<sr.count();++n) {
    const auto at=states.offset+(sr.first()+n)*5;
    pass.states.push_back({std::to_integer<std::uint8_t>(b.slice(at,1)[0]),b.u32(at+1)});
  }
  const auto tr=range(i,bindings),tsr=range(i+4,samplers);
  if(tr.count()>4 || tsr.count()>4)throw FormatError("particle textures exceed NV2A stages");
  for(std::size_t n=0;n<tsr.count();++n)
    pass.texture_states.push_back(decode_texture_state(b.slice(samplers.offset+(tsr.first()+n)*24,24)));
  for(std::size_t n=0;n<tr.count();++n) {
    const auto raw=b.slice(bindings.offset+(tr.first()+n)*4,4);
    const ShaderTextureBinding binding{std::to_integer<std::uint8_t>(raw[0]),
      std::to_integer<std::uint8_t>(raw[1]),std::to_integer<std::uint8_t>(raw[2]),
      std::to_integer<std::uint8_t>(raw[3])};
    pass.textures.push_back(binding);
  }
  if(pc_projection) {
    if(layer!=15 || layer_pass!=0)throw FormatError("MCC particle cooked projection requires its authored effect layer");
    const auto defaults=range(i+0x11c,states);
    pass=project_mcc_particle_pc_pass(stem.name,pass,{bitmaps.count,pixel.count,vertex.count},
        b.slice(samplers.offset+tsr.first()*24,tsr.count()*24),
        b.slice(states.offset+defaults.first()*5,defaults.count()*5));
  }
  remap_pixel_constants(pass.program,pc,pass.pixel_remaps,{},{});
  remap_vertex_constants(pass.constants,vc,pass.vertex_remaps);
  for(const auto& binding:pass.textures) {
    if(binding.stage>=4 || binding.flags)
      throw FormatError("particle texture requires original external binding state");
    if(binding.external) {
      if(binding.external!=23 || binding.stage!=1 || binding.parameter!=255 || pass.program.words[54]!=0xc1u)
        throw FormatError("particle texture requires original external binding state");
      auto animation=read_shader_animation(b,pp.offset,metadata_magic_);
      // MCC's D3D distortion definition retains its authored scalar but has
      // no NV2A sampler animation bank. Project only the proved constant
      // contract of the authenticated original template/pass. In particular,
      // original18540 writes property6 twice and leaves SDK24 untouched.
      // Reference: docs/MCC_OPENING_DECODER_20261019.md and the complete
      // unchanged18540 replay in original-particle-sampler/report.json.
      if(is_mcc() && !model && layer==15 && layer_pass==0 && implementation==0 &&
          stem.name=="shaders\\shader_templates\\particle\\particle_alpha_blend_distortion" &&
          spas.name=="shaders\\shader_passes\\transparent\\effect_shader_alphablend_distortion" &&
          (pc_projection || (is_preserved_native_shader(stem) && is_preserved_native_shader(spas))) &&
          animation.lods.size()==1 && !animation.lods[0].available_layers &&
          animation.layers.size()==25 && animation.passes.size()==1 &&
          animation.implementations.size()==2 && animation.overlays.empty() &&
          animation.references.empty() && animation.parameters.empty() && animation.mappings.empty()) {
        const auto authored=metadata_block(root+0x14,40);
        if(authored.count!=2)throw FormatError("MCC distortion authored parameter contract differs");
        const auto bitmap=authored.offset,scalar=bitmap+40;
        if(string_id(b.u32(bitmap))!="bump_map" || b.u16(bitmap+4)!=0 || b.u16(bitmap+6) ||
            string_id(b.u32(scalar))!="distortion_scale" || b.u16(scalar+4)!=1 || b.u16(scalar+6) ||
            b.u32(scalar+12)!=0xffffffffu || b.u32(bitmap+32) || b.u32(scalar+32) ||
            !std::isfinite(b.f32(scalar+16)))
          throw FormatError("MCC distortion requires its original constant parameter owner");
        // The two original scalar curves are authored distortion_scale and
        // zero; preserve their raw float words through the existing producer.
        const auto constant=[](std::uint32_t value) {
          ShaderOverlay overlay;overlay.period=1;
          for(auto word:std::array<std::uint32_t,7>{1,value,value,0,0,0x3f800000u,0x3f800000u})
            for(unsigned byte=0;byte<4;++byte)overlay.curve.push_back(std::byte(word>>(byte*8)));
          return overlay;
        };
        animation.overlays={constant(b.u32(scalar+16)),constant(0)};
        animation.references={{0,0},{1,0}};
        animation.parameters={ShaderRange{0x200},ShaderRange{0x201}};
        animation.mappings={{0x401,1,0},{0x501,1,1},{0x601,1,1},{0x601,1,0}};
        result.sampler_writes=source_constant_particle_sampler_writes(animation,ShaderRange{0x800});
        continue;
      }

      if(animation.lods.empty() || layer>=32 || !(animation.lods[0].available_layers&(1u<<layer)))
        throw FormatError("particle sampler animation LOD/layer missing");
      const auto layers=animation.lods[0].layers;
      if(layers.first()>animation.layers.size() || layer>=layers.count() || layers.count()>animation.layers.size()-layers.first())
        throw FormatError("particle sampler animation layers escape definition");
      const auto passes=animation.layers[layers.first()+layer];
      if(passes.first()>animation.passes.size() || layer_pass>=passes.count() || passes.count()>animation.passes.size()-passes.first())
        throw FormatError("particle sampler animation passes escape definition");
      const auto implementations=animation.passes[passes.first()+layer_pass];
      const auto bank=metadata_block(pp.offset+52,10);
      if(implementations.first()>bank.count || implementation>=implementations.count() || implementations.count()>bank.count-implementations.first())
        throw FormatError("particle sampler animation implementation missing");
      result.sampler_writes=source_constant_particle_sampler_writes(animation,
          ShaderRange{b.u16(bank.offset+(implementations.first()+implementation)*10+4)});
      if(result.sampler_writes.empty())throw FormatError("particle external23 has no original sampler producer");
      continue;
    }
    const auto remap=std::find_if(pass.bitmap_remaps.begin(),pass.bitmap_remaps.end(),
      [&](const auto &r){return (r.destination&255)==binding.parameter;});
    if(remap==pass.bitmap_remaps.end() || remap->source>=bitmaps.count)
      throw FormatError("original particle bitmap remap missing");
    const auto at=bitmaps.offset+remap->source*12;
    const ShaderBitmap bitmap{b.u32(at),b.u32(at+4),b.f32(at+8)};
    if(result.bitmaps[binding.stage])throw FormatError("duplicate particle texture stage");
    result.bitmaps[binding.stage]=bitmap;
    const auto texture=load_bitmap(bitmap.datum,bitmap.index);
    if(texture.layers!=1 || texture.depth)
      throw FormatError("particle texture requires the original non-2D binding owner");
    // Uncompressed authored bitmaps are already decoded RGBA8. Only BC
    // payloads require expansion; frag flares and impact smoke use both.
    result.textures[binding.stage]=texture.format==11?texture:prepare_xemu_bc_texture(texture);
  }
  if(is_mcc() && !model && !is_preserved_native_shader(spas) &&
      mcc_particle_alpha_multiply_add_names(stem.name,spas.name)) {
    if(layer!=15 || layer_pass!=0 || implementation!=0 || bitmaps.count!=1 ||
       pixel.count || vertex.count!=2 || result.vertex.classification<7 || result.vertex.classification>9)
      throw FormatError("MCC alpha-multiply-add particle authoring contract differs");
    const auto parameters=metadata_block(root+0x14,40);
    if(parameters.count!=1 || string_id(b.u32(parameters.offset))!="alpha_map_texture" ||
       b.u16(parameters.offset+4) || b.u16(parameters.offset+6) ||
       b.u32(parameters.offset+8)!=tag_groups::bitmap.value ||
       !result.bitmaps[0] || b.u32(parameters.offset+12)!=result.bitmaps[0]->datum)
      throw FormatError("MCC alpha-multiply-add authored alpha-map contract differs");
    for(unsigned field=16;field<40;field+=4)if(b.u32(parameters.offset+field))
      throw FormatError("MCC alpha-multiply-add alpha-map requires another authored parameter owner");
    // Both stages retain the same imported authored alpha-map handle/frame.
    // General remapping above preserves the particle's current c0/c111 data.
    const auto defaults=range(i+0x11c,states);
    pass=project_mcc_particle_alpha_multiply_add_pass(pass,
        b.slice(samplers.offset+tsr.first()*24,tsr.count()*24),
        b.slice(states.offset+defaults.first()*5,defaults.count()*5));
  }
  return result;
}
} // namespace halo2
