#include "halo2/menu_world_source_binding.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace halo2 {
namespace {
void validate_owner(const Cache &cache,const MenuWorldSourceOwner &owner,
                    const MenuWorldSourceSection &section) {
  if(owner.scope_datum==0xffffffffu || owner.ordinal==0xffffffffu ||
     (owner.generation && *owner.generation==0xffffffffu))
    throw FormatError("source world draw requires actual owner identity");
  if(owner.kind==MenuWorldSourceOwnerKind::scenery_placement) {
    if(section.kind()!=MenuWorldSourceSectionKind::render_model)
      throw FormatError("source scenery owner requires render-model geometry");
    const auto scope=cache.query_tag({.handle=TagHandle{owner.scope_datum}, .group=tag_groups::scenario});
    if(!scope)throw FormatError("source scenery owner scope is not a scenario");
    const auto metadata=cache.load_source_tag_metadata(owner.scope_datum);
    const Bytes scenario(metadata.bytes_at(metadata.source_address(),metadata.size()));
    if(owner.ordinal>=scenario.u32(0x50))
      throw FormatError("source scenery owner ordinal escapes the scenery placement block");
    // The caller's placement/generation relation is live state, not inferred
    // from a mode resource or static material. Preserve it without invention.
    return;
  }
  if(owner.kind!=MenuWorldSourceOwnerKind::bsp_cluster && owner.kind!=MenuWorldSourceOwnerKind::bsp_instance)
    throw FormatError("unsupported source world owner kind");
  if(owner.scope_datum!=section.owner_datum())
    throw FormatError("source BSP draw scope differs from owned section");
  if(owner.kind==MenuWorldSourceOwnerKind::bsp_cluster) {
    if(section.kind()!=MenuWorldSourceSectionKind::bsp_cluster || owner.ordinal!=section.section())
      throw FormatError("source cluster owner differs from owned section");
    return;
  }
  if(section.kind()!=MenuWorldSourceSectionKind::bsp_instance_definition)
    throw FormatError("source instance owner requires an instance-definition section");
  const auto bsp=std::find_if(cache.bsps().begin(),cache.bsps().end(),[&](const auto &b) {
    return b.datum==owner.scope_datum;
  });
  if(bsp==cache.bsps().end())throw FormatError("source instance owner scope is not resident");
  const auto definition=load_menu_world_source_bsp_owner(cache,std::size_t(bsp-cache.bsps().begin()),owner.ordinal);
  if(definition.section!=section.section())
    throw FormatError("source instance owner selects a different definition section");
}
void validate_input(const MenuWorldSourceSection &section,const MenuWorldSourceStream &stream) {
  unsigned width{},channel{},offset{},stride{};
  switch(stream.semantic) {
  case SourceVertexSemantic::Position:
    channel=0;
    if(stream.format==MenuWorldSourceFormat::float32x3)width=stride=12;
    else if(stream.format==MenuWorldSourceFormat::snorm16x3)width=stride=6;
    else throw FormatError("unsupported source raw position format");
    break;
  case SourceVertexSemantic::UV:
    channel=1;
    if(stream.format==MenuWorldSourceFormat::float32x2)width=stride=8;
    else if(stream.format==MenuWorldSourceFormat::snorm16x2)width=stride=4;
    else throw FormatError("unsupported source raw UV format");
    break;
  case SourceVertexSemantic::Normal:case SourceVertexSemantic::Tangent:case SourceVertexSemantic::Binormal:
    channel=2;stride=12;width=4;
    offset=stream.semantic==SourceVertexSemantic::Tangent?4:
        stream.semantic==SourceVertexSemantic::Binormal?8:0;
    if(stream.format!=MenuWorldSourceFormat::packed_signed_11_11_10)
      throw FormatError("unsupported source raw packed basis format");
    break;
  default:throw FormatError("unsupported source raw stream semantic");
  }
  if(stream.resource>=section.resources().size() || stream.descriptor>=section.descriptors().size())
    throw FormatError("source raw input escapes resource/declaration bank");
  const auto &resource=section.resources()[stream.resource];
  if(resource.type!=56 || resource.channel!=channel || stream.byte_offset!=offset ||
     stream.byte_stride!=stride || stream.byte_offset+width>stride ||
     resource.bytes.size()!=std::size_t(section.vertex_count())*stride)
    throw FormatError("unsupported source raw resource/stride/offset binding");
}
void reads(const VertexProgram &program,std::array<bool,16> &registers,
           std::array<bool,192> &constants) {
  for(unsigned i=0;i<program.count;++i) {
    const auto &t=program.instructions[i];const auto mac=(t[1]>>21)&15,ilu=(t[1]>>25)&7;
    const auto operand=[&](unsigned which) {
      const auto mux=which==0?(t[2]>>26)&3:which==1?(t[2]>>11)&3:(t[3]>>28)&3;
      const auto temporary=which==0?t[2]>>28:which==1?(t[2]>>13)&15:
          (t[2]&3)*4+(t[3]>>30);
      if(mux==0)throw FormatError("invalid source raw operand register type");
      if(mux==1 && temporary>12)
        throw FormatError("source raw temporary outside register file");
      if(mux==2)registers[(t[1]>>9)&15]=true;
      if(mux==3) {
        const auto index=(t[1]>>13)&255;
        if(index>=constants.size())throw FormatError("source raw constant read escapes c192");
        constants[index]=true;
      }
    };
    if(mac)operand(0);
    if(mac==2 || (mac>=4 && mac<=12))operand(1);
    if(ilu || mac==3 || mac==4)operand(2);
  }
}
}
MenuWorldSourceBinding menu_world_source_binding(const Cache &cache,MenuWorldSourceOwner owner,
    MenuWorldSourceDraw draw,std::uint32_t vertex_shader,const MenuWorldSourceConstants &constants) {
  return menu_world_source_binding_class(cache,owner,std::move(draw),vertex_shader,1,constants);
}
MenuWorldSourceBinding menu_world_source_binding_class(const Cache &cache,MenuWorldSourceOwner owner,
    MenuWorldSourceDraw draw,std::uint32_t vertex_shader,unsigned vertex_class,
    const MenuWorldSourceConstants &constants) {
  if(!draw.geometry)throw FormatError("source world binding lacks owned geometry");
  (void)draw.triangle_count();
  if(vertex_class>1)throw FormatError("source raw binding requires explicit class0 or class1");
  if(vertex_class==0 && (draw.geometry->kind()!=MenuWorldSourceSectionKind::bsp_cluster ||
      owner.kind!=MenuWorldSourceOwnerKind::bsp_cluster || !draw.object_rows.empty()))
    throw FormatError("source class0 raw binding requires BSP cluster without a palette write");
  if(vertex_class==1 && (draw.object_rows.empty() || draw.object_rows.size()>40))
    throw FormatError("source world binding requires actual c50 palette rows");
  for(const auto &matrix:draw.object_rows)for(float value:matrix)
    if(!std::isfinite(value))throw FormatError("source world binding has nonfinite object rows");
  validate_owner(cache,owner,*draw.geometry);
  MenuWorldSourceBinding result;result.owner_=owner;result.draw_=std::move(draw);result.vertex_shader_=vertex_shader;
  result.program_bytes_=cache.load_vertex_shader_code(vertex_shader,vertex_class);
  result.program_=decode_vertex_program(result.program_bytes_);
  result.declaration_=cache.load_source_vertex_declaration(vertex_shader,vertex_class);
  if(result.program_.relative_constants)
    throw FormatError("source raw binding requires an explicit nonrelative program");
  if(result.declaration_.class_index!=vertex_class || result.declaration_.semantics.empty() ||
     result.declaration_.semantics.size()>16)
    throw FormatError("unsupported source raw register declaration");
  std::array<bool,16> consumed{};std::array<bool,192> constant_reads{};
  reads(result.program_,consumed,constant_reads);
  for(unsigned reg=0;reg<result.declaration_.semantics.size();++reg) {
    const auto semantic=SourceVertexSemantic(result.declaration_.semantics[reg]);
    const auto &streams=result.draw_.geometry->streams();
    const auto stream=std::find_if(streams.begin(),streams.end(),[&](const auto &s){return s.semantic==semantic;});
    if(stream==streams.end() || std::count_if(streams.begin(),streams.end(),[&](const auto &s){return s.semantic==semantic;})!=1)
      throw FormatError("source raw declaration has an unsupported/missing/duplicate semantic");
    validate_input(*result.draw_.geometry,*stream);
    result.inputs_.push_back({reg,*stream,consumed[reg]});
  }
  for(unsigned reg=0;reg<consumed.size();++reg)if(consumed[reg]) {
    result.consumed_registers_.push_back(reg);
    if(reg>=result.declaration_.semantics.size())result.external_attributes_.push_back(reg);
  }
  for(unsigned index=0;index<constant_reads.size();++index)if(constant_reads[index])result.consumed_constants_.push_back(index);
  std::memcpy(result.constants_.data(),constants.data(),sizeof(constants));
  for(unsigned matrix=0;matrix<result.draw_.object_rows.size();++matrix)
    for(unsigned row=0;row<3;++row)
      std::copy_n(result.draw_.object_rows[matrix].begin()+row*4,4,result.constants_[50+matrix*3+row].begin());
  const auto &compression=result.draw_.geometry->compression_rows();
  for(unsigned row=0;row<3;++row)
    std::copy_n(compression.begin()+row*4,4,result.constants_[170+row].begin());
  return result;
}
std::span<const std::byte> MenuWorldSourceBinding::resource_bytes(unsigned input) const {
  if(input>=inputs_.size())throw FormatError("source raw input outside binding");
  return draw_.geometry->resources().at(inputs_[input].stream.resource).bytes;
}
} // namespace halo2
