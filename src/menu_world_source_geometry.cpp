#include "halo2/menu_world_source_geometry.hpp"
#include "halo2/mcc_bsp_vertex_layout.hpp"
#include "halo2/mcc_model_vertex_layout.hpp"
#include "halo2/mcc_skinned_model_vertex_layout.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace halo2 {
namespace {
struct Metadata {
  explicit Metadata(SourceTagMetadata payload)
      : owned(std::move(payload)), bytes(owned.bytes_at(owned.source_address(),owned.size())) {}
  SourceTagMetadata owned;
  Bytes bytes;
  std::size_t pointer(std::uint32_t address,std::size_t extent) const {
    if(address<owned.source_address())throw FormatError("source geometry pointer precedes metadata");
    const auto at=std::size_t(address-owned.source_address());bytes.require(at,extent);return at;
  }
  std::pair<unsigned,std::size_t> block(std::size_t at,unsigned stride,unsigned limit) const {
    const auto count=bytes.u32(at);
    if(count>limit)throw FormatError("source geometry block exceeds supported bank");
    return {count,count?pointer(bytes.u32(at+4),std::size_t(count)*stride):0};
  }
};
std::array<float,12> compression(const Bytes &m,std::optional<std::size_t> bounds,unsigned flags) {
  // TU1.5 1CDB0 tests only the low position/UV bits. Authored character
  // sections also carry bit2; rejecting that unrelated bit hid valid meshes.
  flags&=3u;
  std::array<float,12> out{1,1,1,1,0,0,0,0,1,1,0,0};
  if(!flags || !bounds)return out; // Original1CDD0 null bounds leaves identity.
  std::array<float,10> b{};for(unsigned i=0;i<b.size();++i) {
    b[i]=m.f32(*bounds+i*4);
    if(!std::isfinite(b[i]))throw FormatError("nonfinite source geometry compression bound");
  }
  const auto half_range=[&](unsigned axis) {const volatile float v=b[axis+1]-b[axis];return v*.5f;};
  if(flags&1) {
    for(unsigned i=0;i<3;++i)out[i]=half_range(i*2);
    // Original X uses max+min; Y and Z use min+max, before MULSS .5.
    const volatile float x=b[1]+b[0],y=b[2]+b[3],z=b[4]+b[5];
    out[4]=x*.5f;out[5]=y*.5f;out[6]=z*.5f;
  }
  if(flags&2) {
    out[8]=half_range(6);out[9]=half_range(8);
    const volatile float u=b[6]+b[7],v=b[8]+b[9];out[10]=u*.5f;out[11]=v*.5f;
  }
  return out;
}
void finite_rows(std::span<const std::array<float,12>> rows) {
  if(rows.empty() || rows.size()>40)throw FormatError("source draw requires actual supported c50 palette range");
  for(const auto &row:rows)for(float value:row)
    if(!std::isfinite(value))throw FormatError("nonfinite source draw object rows");
}
}

SourceTagMetadata Cache::load_source_bsp_metadata(std::size_t index) const {
  if(index>=bsps_.size())throw FormatError("source BSP metadata index outside scenario");
  const auto &bsp=bsps_[index];
  if(bsp.size<16 || !bsp.memory_address || bsp.datum==0xffffffffu)
    throw FormatError("source BSP metadata mapping unavailable");
  return {bsp.datum,"sbsp",bsp.memory_address,Bytes(cache_bytes()).slice(bsp.file_offset,bsp.size)};
}

class MenuWorldSourceSectionBuilder {
public:
  static MenuWorldSourceSection decode(const Metadata &m,std::size_t at,
      std::span<const std::byte> raw,MenuWorldSourceSectionKind kind,unsigned ordinal,
      unsigned material_count,std::size_t materials,std::optional<std::size_t> bounds,bool mcc_bsp=false,
      bool mcc_model=false,unsigned model_nodes=0) {
    const bool model=kind==MenuWorldSourceSectionKind::render_model;
    const Bytes &meta=m.bytes;const Bytes b(raw);MenuWorldSourceSection out;
    out.kind_=kind;out.owner_datum_=m.owned.datum();out.section_=ordinal;
    out.vertex_count_=meta.u16(at+(model?4:0));if(model)out.model_geometry_class_=meta.u16(at);
    if(!out.vertex_count_ || (model && (*out.model_geometry_class_<1 || *out.model_geometry_class_>3)))
      throw FormatError("source world geometry requires nonempty BSP or model class1..3 section");
    out.compression_flags_=model?meta.u16(at+0x1a):0;
    out.compression_rows_=compression(meta,bounds,out.compression_flags_);
    std::size_t base{};
    if(model) {
      const auto header=meta.u32(at+0x44);
      if(std::uint64_t(header)+4>raw.size())throw FormatError("invalid source model resource base");
      base=raw.size()-header-4;
      if(base<8)throw FormatError("source model resident geometry header is truncated");
      // raw_header() is the independently proved BSP-header interface.
      // Model indexed ranges use their separate original model-range loader;
      // retaining model bytes here would falsely assign that BSP contract.
      const auto retained=b.slice(8,base-8);
      out.model_raw_header_.assign(retained.begin(),retained.end());
    } else {
      base=std::size_t(meta.u32(at+48))+8;
      const auto header=b.slice(8,base-8);
      out.raw_header_.assign(header.begin(),header.end());
    }
    const auto [resource_count,resource]=m.block(at+(model?0x48:56),16,128);
    if(!resource_count)throw FormatError("source geometry resource bank is empty");
    unsigned index_resource=~0u,descriptor_resource=~0u;
    std::array<unsigned,3> stream_resources{~0u,~0u,~0u};
    unsigned node_resource=~0u;
    for(unsigned i=0;i<resource_count;++i) {
      const auto r=resource+i*16;MenuWorldSourceResource result;
      result.type=meta.u16(r+4);result.channel=meta.u16(r+6);
      const auto offset=std::uint64_t(base)+meta.u32(r+12);
      const auto length=meta.u32(r+8);
      if(offset>std::numeric_limits<std::uint32_t>::max())throw FormatError("source resource offset overflow");
      result.raw_offset=std::uint32_t(offset);const auto slice=b.slice(std::size_t(offset),length);
      result.bytes.assign(slice.begin(),slice.end());
      const auto admit=[&](unsigned &slot) {
        if(slot!=~0u)throw FormatError("duplicate source geometry resource binding");slot=i;
      };
      if(result.type==32)admit(index_resource);
      if(result.type==56 && result.channel==32)admit(descriptor_resource);
      if(result.type==56 && result.channel<3)admit(stream_resources[result.channel]);
      if(result.type==(mcc_model?104:100)) {
        if(mcc_model && result.channel!=1)throw FormatError("MCC model node resource has an unsupported typed channel");
        admit(node_resource);
      }
      out.resources_.push_back(std::move(result));
    }
    if(index_resource==~0u || descriptor_resource==~0u ||
       std::find(stream_resources.begin(),stream_resources.end(),~0u)!=stream_resources.end())
      throw FormatError("source geometry lacks original indices or supported stream bindings");
    auto &descriptor_bytes=out.resources_[descriptor_resource].bytes;
    if(descriptor_bytes.size()<3*32 || descriptor_bytes.size()%32 || descriptor_bytes.size()>16*32)
      throw FormatError("unsupported source geometry descriptor bank");
    if(mcc_bsp) {
      const auto layout=project_mcc_bsp_vertex_layout(out.vertex_count_,descriptor_bytes,
          out.resources_[stream_resources[0]].bytes,out.resources_[stream_resources[1]].bytes,
          out.resources_[stream_resources[2]].bytes);
      std::copy(layout.descriptors.begin(),layout.descriptors.end(),descriptor_bytes.begin());
      out.resources_[stream_resources[2]].bytes=layout.basis;
    }
    if(mcc_model) {
      // An absent MCC node104 bank is the source's direct model-node domain,
      // also used by the existing palette owner. Do not invent an identity
      // map: each consumed index is validated against the actual root count.
      const std::span<const std::byte> node_bytes=node_resource==~0u?
          std::span<const std::byte>{}:out.resources_[node_resource].bytes;
      if(*out.model_geometry_class_==3) {
        const auto &parts=out.resources_.front();
        if(parts.type!=0 || parts.channel!=72)
          throw FormatError("MCC skinned model first resource is not its actual parts");
        auto layout=project_mcc_skinned_model_vertex_layout(
            std::to_integer<unsigned>(meta.slice(at+0x14,1)[0]),
            std::to_integer<unsigned>(meta.slice(at+0x15,1)[0]),model_nodes,out.vertex_count_,
            out.model_raw_header_,descriptor_bytes,out.resources_[stream_resources[0]].bytes,
            out.resources_[stream_resources[1]].bytes,out.resources_[stream_resources[2]].bytes,
            node_bytes,parts.bytes,out.resources_[index_resource].bytes);
        std::copy(layout.descriptors.begin(),layout.descriptors.end(),descriptor_bytes.begin());
        out.resources_[stream_resources[0]].bytes=std::move(layout.positions);
        out.resources_[stream_resources[2]].bytes=std::move(layout.basis);
        out.node_map_=std::move(layout.node_map);
      } else {
        auto layout=project_mcc_rigid_model_vertex_layout(*out.model_geometry_class_,
            std::to_integer<unsigned>(meta.slice(at+0x14,1)[0]),model_nodes,out.vertex_count_,out.model_raw_header_,descriptor_bytes,
            out.resources_[stream_resources[0]].bytes,out.resources_[stream_resources[1]].bytes,
            out.resources_[stream_resources[2]].bytes,node_bytes);
        std::copy(layout.descriptors.begin(),layout.descriptors.end(),descriptor_bytes.begin());
        out.resources_[stream_resources[2]].bytes=std::move(layout.basis);
        out.node_map_=std::move(layout.node_map);
      }
    }
    for(unsigned d=0;d<descriptor_bytes.size()/32;++d) {
      std::array<std::byte,32> descriptor;
      std::copy_n(descriptor_bytes.begin()+d*32,32,descriptor.begin());out.descriptors_.push_back(descriptor);
      // Additional unchanged BSP lighting descriptors are retained. This
      // bounded raw fetch adapter exposes only channels0..2; a selected
      // declaration consuming another semantic is rejected by its binding.
      if(d>=3)continue;
      const auto type=std::to_integer<unsigned>(descriptor[0]),stride=std::to_integer<unsigned>(descriptor[1]);
      // Source1C8C0 consumes buffer format/stride, offset+8 and byte offset+10.
      // Relocation words are zero in the admitted literal resource descriptors.
      for(unsigned j=2;j<32;++j)if(descriptor[j]!=std::byte{})
        throw FormatError("unsupported source geometry descriptor relocation/offset");
      const unsigned r=stream_resources[d];
      if(!stride || out.resources_[r].bytes.size()!=std::size_t(out.vertex_count_)*stride)
        throw FormatError("source geometry stream extent differs from descriptor stride");
      const auto binding=[&](SourceVertexSemantic semantic,MenuWorldSourceFormat format,unsigned offset) {
        out.streams_.push_back({semantic,format,r,offset,stride,d});
      };
      if(d==0 && type>=1 && type<=10) {
        // Whole TU1.5 1C310 and its unchanged4494C8 stream table establish
        // these byte offsets and UB_OGL counts. Preserve alignment gaps and
        // stored weights; no CPU bone remap, weight completion or repacking.
        constexpr std::array<unsigned,10> strides{12,6,16,8,16,12,20,12,20,16};
        constexpr std::array<unsigned,10> indices{0,0,12,6,12,8,12,6,12,8};
        constexpr std::array<unsigned,10> weights{0,0,0,0,14,10,16,9,16,12};
        constexpr std::array<MenuWorldSourceFormat,4> bytes{
            MenuWorldSourceFormat::unorm8x1,MenuWorldSourceFormat::unorm8x2,
            MenuWorldSourceFormat::unorm8x3,MenuWorldSourceFormat::unorm8x4};
        if(stride!=strides[type-1] || (!model && type>2) ||
            (model && ((*out.model_geometry_class_==1 && type>2) ||
                       (*out.model_geometry_class_==2 && (type<3 || type>4)) ||
                       (*out.model_geometry_class_==3 && type<3))))
          throw FormatError("source position descriptor differs from authored model class/stride");
        binding(SourceVertexSemantic::Position,(type&1)?MenuWorldSourceFormat::float32x3:MenuWorldSourceFormat::snorm16x3,0);
        if(type>=3) {
          const unsigned influences=(type-1)/2;
          binding(SourceVertexSemantic::BlendIndices,bytes[influences-1],indices[type-1]);
          if(influences>1)binding(SourceVertexSemantic::BlendWeights,bytes[influences-1],weights[type-1]);
        }
      }
      else if(d==1 && type==24 && stride==8)binding(SourceVertexSemantic::UV,MenuWorldSourceFormat::float32x2,0);
      else if(d==1 && type==25 && stride==4)binding(SourceVertexSemantic::UV,MenuWorldSourceFormat::snorm16x2,0);
      else if(d==2 && type==27 && stride==12) {
        binding(SourceVertexSemantic::Normal,MenuWorldSourceFormat::packed_signed_11_11_10,0);
        binding(SourceVertexSemantic::Tangent,MenuWorldSourceFormat::packed_signed_11_11_10,4);
        binding(SourceVertexSemantic::Binormal,MenuWorldSourceFormat::packed_signed_11_11_10,8);
      } else throw FormatError("unsupported source world geometry stream format");
    }
    const auto count=b.u16(0x28);const auto &indices=out.resources_[index_resource].bytes;
    if(indices.size()!=std::size_t(count)*2)throw FormatError("source index resource extent differs from raw header");
    const Bytes ib(indices);for(unsigned i=0;i<count;++i) {
      const auto v=ib.u16(i*2);if(v>=out.vertex_count_)throw FormatError("source index outside vertex stream");
      out.indices_.push_back(v);
    }
    const auto &parts=out.resources_.front();
    if(parts.type!=0 || parts.channel!=72 || parts.bytes.empty() || parts.bytes.size()%72)
      throw FormatError("source geometry first resource is not literal72-byte parts");
    const Bytes pb(parts.bytes);
    for(unsigned i=0;i<parts.bytes.size()/72;++i) {
      const auto p=i*72;MenuWorldSourcePart part;
      std::copy_n(parts.bytes.begin()+p,72,part.bytes.begin());part.type=pb.u16(p);part.flags=pb.u16(p+2);
      part.material=std::int16_t(pb.u16(p+4));part.index_first=pb.u16(p+6);part.index_count=pb.u16(p+8);
      if(part.index_first>count || part.index_count>count-part.index_first)
        throw FormatError("source part indices escape original bank");
      if(part.material>=0) {
        if(unsigned(part.material)>=material_count)throw FormatError("source part material escapes authored bank");
        part.shader=meta.u32(materials+unsigned(part.material)*32+12);
      }
      out.parts_.push_back(part);
    }
    if(model && !mcc_model) {
      const auto nodes=b.u16(0x6c);
      if(nodes && (node_resource==~0u || out.resources_[node_resource].bytes.size()!=nodes))
        throw FormatError("source model node-map extent differs from raw header");
      if(nodes)for(auto value:out.resources_[node_resource].bytes)out.node_map_.push_back(std::to_integer<std::uint8_t>(value));
    }
    return out;
  }
};

MenuWorldSourceSection load_menu_world_source_bsp_section(const Cache &cache,std::size_t bsp,
    MenuWorldSourceSectionKind kind,unsigned section) {
  if(kind!=MenuWorldSourceSectionKind::bsp_cluster && kind!=MenuWorldSourceSectionKind::bsp_instance_definition)
    throw FormatError("source BSP section kind is not a BSP allocation");
  const Metadata m(cache.load_source_bsp_metadata(bsp));
  const auto [clusters,cp]=m.block(16+156,176,2048);
  const auto [instances,ip]=m.block(16+312,200,2048);
  const bool instance=kind==MenuWorldSourceSectionKind::bsp_instance_definition;
  if(section>=(instance?instances:clusters))throw FormatError("source BSP section outside allocation");
  const auto [materials,mp]=m.block(16+164,32,65535);
  const auto resources=cache.load_bsp_resources(bsp);const auto r=instance?clusters+section:section;
  if(resources.size()!=std::size_t(clusters)+instances || r>=resources.size())
    throw FormatError("source BSP raw resource bank differs from metadata");
  return MenuWorldSourceSectionBuilder::decode(m,(instance?ip:cp)+section*(instance?200:176),
      resources[r],kind,section,materials,mp,std::nullopt,cache.is_mcc());
}
MenuWorldSourceSection load_menu_world_source_model_section(const Cache &cache,std::uint32_t datum,unsigned section) {
  const Metadata m(cache.load_source_tag_metadata(datum));
  if(m.owned.group()!="mode")throw FormatError("source model geometry requires actual mode datum");
  const auto [sections,sp]=m.block(0x24,0x5c,512);
  if(section>=sections)throw FormatError("source model section outside authored bank");
  const auto [materials,mp]=m.block(0x60,32,65535);
  const auto [bounds,bp]=m.block(0x14,0x38,65536);
  const auto resources=cache.load_model_resources(datum);
  if(section>=resources.size())throw FormatError("source model raw resource bank differs from metadata");
  return MenuWorldSourceSectionBuilder::decode(m,sp+section*0x5c,resources[section],
      MenuWorldSourceSectionKind::render_model,section,materials,mp,bounds?std::optional(bp):std::nullopt,
      false,cache.is_mcc(),m.bytes.u32(0x48));
}
MenuWorldSourceBspOwner load_menu_world_source_bsp_owner(const Cache &cache,std::size_t bsp,unsigned instance) {
  const Metadata m(cache.load_source_bsp_metadata(bsp));const auto [count,p]=m.block(16+320,88,2048);
  if(instance>=count)throw FormatError("source BSP owner outside instance bank");
  const auto at=p+instance*88;MenuWorldSourceBspOwner out;
  out.bsp_datum=m.owned.datum();out.instance=instance;out.section=m.bytes.u16(at+52);out.separate_scale=m.bytes.f32(at);
  const auto [sections,unused]=m.block(16+312,200,2048);(void)unused;
  if(out.section>=sections)throw FormatError("source BSP owner section outside definition bank");
  for(unsigned row=0;row<3;++row)for(unsigned column=0;column<4;++column)
    out.rows[row*4+column]=m.bytes.f32(at+4+row*4+column*12);
  finite_rows(std::span<const std::array<float,12>>(&out.rows,1));return out;
}
std::span<const std::uint16_t> MenuWorldSourceSection::part_indices(unsigned part) const {
  if(part>=parts_.size())throw FormatError("source draw part outside section");
  const auto &p=parts_[part];return std::span<const std::uint16_t>(indices_).subspan(p.index_first,p.index_count);
}
MenuWorldSourceIndexRange MenuWorldSourceDraw::index_range() const {
  if(!geometry)throw FormatError("source draw lacks owned geometry");
  if(part>=geometry->parts().size())throw FormatError("source draw part outside section");
  const auto &p=geometry->parts()[part];
  const auto selected=range.value_or(MenuWorldSourceIndexRange{p.index_first,p.index_count});
  // Subtraction after the lower-bound check avoids first+count overflow.
  if(selected.first<p.index_first || selected.first-p.index_first>p.index_count ||
     selected.count>p.index_count-(selected.first-p.index_first))
    throw FormatError("source draw index range escapes authored part");
  return selected;
}
std::span<const std::uint16_t> MenuWorldSourceDraw::indices() const {
  const auto selected=index_range();
  return geometry->indices().subspan(selected.first,selected.count);
}
std::size_t MenuWorldSourceDraw::triangle_count() const {
  const auto count=indices().size();
  if(topology==MenuWorldSourceTopology::triangle_list) {
    if(count%3)throw FormatError("source list range is not whole triangles");return count/3;
  }
  if(topology!=MenuWorldSourceTopology::triangle_strip)throw FormatError("unsupported source primitive topology");
  return count>2?count-2:0;
}
std::array<std::uint16_t,3> MenuWorldSourceDraw::triangle(std::size_t ordinal) const {
  if(ordinal>=triangle_count())throw FormatError("source triangle outside part range");
  const auto bank=indices();const auto first=topology==MenuWorldSourceTopology::triangle_list?ordinal*3:ordinal;
  if(topology==MenuWorldSourceTopology::triangle_strip && (ordinal&1))return {bank[first],bank[first+2],bank[first+1]};
  return {bank[first],bank[first+1],bank[first+2]};
}
MenuWorldSourceDraw menu_world_source_draw(std::shared_ptr<const MenuWorldSourceSection> geometry,
    unsigned part,MenuWorldSourceTopology topology,std::span<const std::array<float,12>> rows,
    std::optional<MenuWorldSourceIndexRange> range) {
  finite_rows(rows);MenuWorldSourceDraw out{std::move(geometry),part,topology,{rows.begin(),rows.end()},range};
  (void)out.triangle_count();return out;
}
MenuWorldSourceDraw menu_world_source_cluster_draw(std::shared_ptr<const MenuWorldSourceSection> geometry,
    unsigned part,MenuWorldSourceTopology topology,std::optional<MenuWorldSourceIndexRange> range) {
  if(!geometry || geometry->kind()!=MenuWorldSourceSectionKind::bsp_cluster)
    throw FormatError("source cluster draw requires owned BSP cluster geometry");
  MenuWorldSourceDraw out{std::move(geometry),part,topology,{},range};
  (void)out.triangle_count();return out;
}
} // namespace halo2
