#include "halo2/particle_material.hpp"
#include "halo2/texture.hpp"
#include <algorithm>

namespace halo2 {
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
  const auto impl=metadata_block(processed.offset, 306),bindings=metadata_block(processed.offset+8, 4),
    states=metadata_block(processed.offset+16, 5),samplers=metadata_block(processed.offset+24, 24),
    fragments=metadata_block(processed.offset+32, 4),externs=metadata_block(processed.offset+56, 4);
  if(implementation>=impl.count)throw FormatError("original particle pass implementation missing");
  const auto i=impl.offset+implementation*306;
  auto &pass=result.pass;
  pass.name=spas.name;pass.template_pass=selected;pass.implementation=implementation;
  pass.vertex_shader=find(result.vertex.vertex_datum,"vrtx").name;
  pass.vertex_code=result.vertex.program;
  pass.bitmap_remaps=remaps(mapping,maps);
  pass.pixel_remaps=remaps(mapping+2,maps);
  pass.vertex_remaps=remaps(mapping+4,maps);
  pass.pixel_externs=remaps(i+0x122,externs);
  pass.vertex_externs=remaps(i+0x124,externs);
  if(range(i+246,fragments).count() || !pass.pixel_externs.empty() || !pass.vertex_externs.empty())
    throw FormatError("particle material switches or external values require original draw state");
  for(unsigned w=0;w<60;++w)pass.definition.words[w]=b.u32(i+6+w*4);
  pass.program=pass.definition;
  remap_pixel_constants(pass.program,pc,pass.pixel_remaps,{},{});
  remap_vertex_constants(pass.constants,vc,pass.vertex_remaps);
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
    if(binding.stage>=4 || binding.flags)
      throw FormatError("particle texture requires original external binding state");
    if(binding.external) {
      if(binding.external!=23 || binding.stage!=1 || binding.parameter!=255 || pass.program.words[54]!=0xc1u)
        throw FormatError("particle texture requires original external binding state");
      const auto animation=read_shader_animation(b,pp.offset,metadata_magic_);
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
  return result;
}
} // namespace halo2
