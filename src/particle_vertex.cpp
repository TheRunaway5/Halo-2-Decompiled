#include "halo2/particle_vertex.hpp"
#include <algorithm>

namespace halo2 {
std::vector<std::byte> Cache::load_vertex_shader_code(std::uint32_t datum,unsigned classification) const {
  const auto tag=query_tag({.handle=TagHandle{datum}, .group=tag_groups::vertex_shader, .require_address=true});
  if(!tag)throw FormatError("original vertex shader tag missing");
  const Bytes b(storage_);const auto root=metadata_offset(tag->address,12);
  if(b.u16(root)!=1)throw FormatError("vertex shader is not original Xbox format");
  const auto count=b.u32(root+4);
  if(count>65536 || classification>=count)throw FormatError("vertex classification outside original tag");
  const auto at=metadata_offset(b.u32(root+8),count*28)+classification*28;
  const auto size=b.u32(at+12);
  if(!size)return {};
  if(size>4+136*16)throw FormatError("original vertex shader exceeds NV2A instruction memory");
  const auto raw=b.slice(metadata_offset(b.u32(at+16),size),size);
  return {raw.begin(),raw.end()};
}
ParticleVertexSelection Cache::load_particle_vertex_selection(std::uint32_t datum,
    unsigned layer,unsigned layer_pass) const {
  const Bytes b(storage_);
  const auto find=[&](std::uint32_t id,const char *group)->const Tag& {
    const auto t=query_tag({.handle=TagHandle{id}, .group=TagGroup::from_string(group), .require_address=true});
    if(!t)throw FormatError("original particle shader dependency missing");
    return *t;
  };

  const auto &particle=find(datum,"prt3");
  const auto root=metadata_offset(particle.address,0x94);
  const auto definitions=metadata_block(root+0x8c, 124);
  if(!definitions.count)throw FormatError("original particle has no shader definition");
  ParticleVertexSelection result;
  result.classification=unsigned(b.u16(root+4))+7u; // 2243C0 -> 23EA80.
  result.template_datum=b.u32(definitions.offset);
  const auto &stem=find(result.template_datum,"stem");
  const auto templates=metadata_block(metadata_offset(stem.address,0x60)+0x58, 40);
  if(!templates.count)throw FormatError("original particle template definition missing");
  const auto lods=metadata_block(templates.offset, 10),layers=metadata_block(templates.offset+8, 2),
      passes=metadata_block(templates.offset+16, 10);
  if(!lods.count)throw FormatError("original particle template LOD missing");
  const ShaderRange layer_range{b.u16(lods.offset)};
  if(layer>=layer_range.count() || layer_range.first()>layers.count ||
      layer_range.count()>layers.count-layer_range.first())
    throw FormatError("particle layer outside original first LOD");
  const ShaderRange pass_range{b.u16(layers.offset+(layer_range.first()+layer)*2)};
  if(layer_pass>=pass_range.count() || pass_range.first()>passes.count ||
      pass_range.count()>passes.count-pass_range.first())
    throw FormatError("particle pass outside original layer");
  result.pass_datum=b.u32(passes.offset+(pass_range.first()+layer_pass)*10+4);
  const auto &pass=find(result.pass_datum,"spas");
  const auto processed=metadata_block(metadata_offset(pass.address,0x24)+0x1c, 88);
  if(!processed.count)throw FormatError("original particle pass definition missing");
  const auto implementations=metadata_block(processed.offset, 306);
  if(!implementations.count)throw FormatError("original particle implementation missing");
  // 23EA80 obtains the declaration from the first pass implementation before
  // the style-specific override. Pixel implementation choice is separate.
  result.vertex_datum=b.u32(implementations.offset+256);
  result.program=load_vertex_shader_code(result.vertex_datum,result.classification);
  if(result.program.empty())throw FormatError("original particle vertex classification is not resident");
  return result;
}
} // namespace halo2
