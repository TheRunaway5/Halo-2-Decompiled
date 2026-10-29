#include "halo2/particle_vertex.hpp"
#include "halo2/mcc_particle_alpha_multiply_add.hpp"
#include "halo2/mcc_particle_pc_projection.hpp"
#include "halo2/compiled_particle_vertex_signature.hpp"
#include <algorithm>

namespace halo2 {
std::vector<std::byte> Cache::load_vertex_shader_code(std::uint32_t datum,unsigned classification) const {
  const auto tag=query_tag({.handle=TagHandle{datum}, .group=tag_groups::vertex_shader, .require_address=true});
  if(!tag)throw FormatError("original vertex shader tag missing");
  const Bytes b(cache_bytes());const auto root=metadata_offset(tag->address,12);
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
  const Bytes b(cache_bytes());
  const auto find=[&](std::uint32_t id,const char *group)->const Tag& {
    const auto t=query_tag({.handle=TagHandle{id}, .group=TagGroup::from_string(group), .require_address=true});
    if(!t)throw FormatError("original particle shader dependency missing");
    return *t;
  };

  const auto particle=query_tag({.handle=TagHandle{datum},.require_address=true});
  if(!particle || (particle->group!="prt3" && particle->group!="PRTM"))
    throw FormatError("original particle shader owner missing");
  const bool model=particle->group=="PRTM";
  const auto root=metadata_offset(particle->address,model?0xe0:0x94);
  const auto shader=model?metadata_offset(find(b.u32(root+0x1c),"shad").address,0x28):root;
  const auto definitions=metadata_block(shader+(model?0x20:0x8c),124);
  if(!definitions.count)throw FormatError("original particle has no shader definition");
  ParticleVertexSelection result;
  result.classification=model?1u:unsigned(b.u16(root+4))+7u; //224DB0 /2243C0.
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
  const bool pc_pass=is_mcc() && !is_preserved_native_shader(pass);
  // MCC keeps an additional24-byte D3D pixel descriptor tail. Only the first
  //306 bytes share the native field layout; implementation1 begins at330.
  const unsigned implementation_stride=pc_pass?330:306;
  const auto implementations=metadata_block(processed.offset,implementation_stride);
  if(!implementations.count)throw FormatError("original particle implementation missing");
  // 23EA80 obtains the declaration from the first pass implementation before
  // the style-specific override. Pixel implementation choice is separate.
  if(model && implementations.count<2)throw FormatError("Particle model requires authored implementation1");
  result.vertex_datum=b.u32(implementations.offset+(model?implementation_stride:0)+256);
  if(pc_pass && mcc_particle_pc_projection_names(stem.name,pass.name,model?1:0)) {
    if(layer!=15 || layer_pass!=0)throw FormatError("MCC particle custom projection requires its authored effect pass");
    const auto& pc=find(result.vertex_datum,"vrtx");
    const auto program=mcc_particle_pc_vertex_signature(stem.name,pass.name,model?1:0,
        result.classification,pc.name,b.u32(metadata_offset(pc.address,0x18)));
    result.vertex_datum=0xffffffffu;
    result.program.assign(program.begin(),program.end());
    return result;
  }
  if(is_mcc() && !model && !is_preserved_native_shader(pass) &&
      mcc_particle_alpha_multiply_add_names(stem.name,pass.name)) {
    // This PC-only pass retains a D3D vertex reference. Its sprite stream uses
    // the same already-compiled custom Vulkan particle program as the other
    // particle passes; its separate pixel/raster projection is qualified by
    // load_particle_material. Never relabel the PC bytes as Xbox microcode.
    const auto &pc=find(result.vertex_datum,"vrtx");
    if(layer!=15 || layer_pass!=0 || result.classification<7 || result.classification>9 ||
       pc.name!="rasterizer\\vertex_shaders_dx9\\particle" ||
       b.u16(metadata_offset(pc.address,12))!=12)
      throw FormatError("MCC alpha-multiply-add particle vertex contract differs");
    const auto* native=query_tag({.group=tag_groups::vertex_shader,
        .name="rasterizer\\vertex_shaders_xbox\\particle",.require_address=true});
    if(native && is_preserved_native_shader(*native))result.vertex_datum=native->datum;
    else {
      // PC-only levels have no preserved vertex-tag namespace. The custom
      // Vulkan kernel still has its built-in validation signature. A null
      // datum records that no source tag/declaration owner was fabricated.
      result.vertex_datum=0xffffffffu;
      const auto signature=compiled_particle_vertex_signature(result.classification);
      result.program.assign(signature.begin(),signature.end());
      return result;
    }
  }
  result.program=load_vertex_shader_code(result.vertex_datum,result.classification);
  if(result.program.empty())throw FormatError("original particle vertex classification is not resident");
  return result;
}
} // namespace halo2
