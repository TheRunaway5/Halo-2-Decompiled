#include "halo2/source_runtime_cache_tags.hpp"
#include "halo2/cache.hpp"
#include <array>
#include <algorithm>

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> b,std::size_t at) {
  if(at>b.size() || b.size()-at<4)throw FormatError("Cache runtime word escapes its owner");
  std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[at+i])<<(8*i);return v;
}
void put(SourceRuntimeMemory &m,std::uint32_t p,std::uint32_t v) {
  std::array<std::byte,4> b;for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(p,b);
}
}
SourceRuntimeCacheTagView Cache::view_source_runtime_cache_tags() const {
  if(mcc_)throw FormatError("MCC gameplay tags require verified Xbox TU1.5 layout/semantic conversion before original-ABI delivery");
  if(vista_)throw FormatError("Original Xbox runtime tag delivery requires the Xbox cache owner");
  const Bytes bytes(cache_bytes());const auto index=bytes.u32(16),index_bytes=bytes.u32(20),metadata_bytes=bytes.u32(24);
  bytes.require(index,index_bytes);bytes.require(std::size_t(index)+index_bytes,metadata_bytes);
  if(index_bytes<32 || !metadata_bytes)throw FormatError("Original cache runtime regions are unavailable");
  const auto header_magic=std::int64_t(bytes.u32(index))-(std::int64_t(index)+32);
  const auto index_address=std::int64_t(index)+header_magic;
  const auto metadata_address=std::int64_t(index)+index_bytes+metadata_magic_;
  if(index_address<=0 || metadata_address<=0 || index_address+index_bytes>0x100000000ll ||
      metadata_address+metadata_bytes>0x100000000ll)
    throw FormatError("Original cache runtime regions wrap source addresses");
  SourceRuntimeCacheTagView out;
  out.index=std::uint32_t(index_address);out.table=bytes.u32(index+8);out.count=bytes.u32(index+24);
  out.scenario=bytes.u32(index+12);out.globals=bytes.u32(index+16);
  out.reservation_bytes=bytes.u32(0x1c);
  out.regions={SourceRuntimeCacheTagView::Region{out.index,bytes.slice(index,index_bytes)},
               SourceRuntimeCacheTagView::Region{std::uint32_t(metadata_address),
                 bytes.slice(std::size_t(index)+index_bytes,metadata_bytes)}};
  return out;
}
SourceRuntimeCacheTagImage Cache::load_source_runtime_cache_tag_image() const {
  const auto view=view_source_runtime_cache_tags();
  SourceRuntimeCacheTagImage out;
  out.index=view.index;out.table=view.table;out.count=view.count;
  out.scenario=view.scenario;out.globals=view.globals;
  out.reservation_bytes=view.reservation_bytes;
  for(const auto &region:view.regions)
    out.regions.push_back({region.address,{region.bytes.begin(),region.bytes.end()}});
  return out;
}
SourceRuntimeCacheBspImage Cache::load_source_runtime_cache_bsp_image(std::size_t selected) const {
  if(mcc_)throw FormatError("MCC compiled collision/physics requires verified Xbox TU1.5 conversion before original-ABI BSP delivery");
  if(vista_ || selected>=bsps_.size() || selected>32767)
    throw FormatError("Original runtime BSP cache selection is unavailable");
  const Bytes data(cache_bytes());const auto index=data.u32(16),scenario=data.u32(index+12);
  const auto* tag=find_tag({scenario}, tag_groups::scenario);
  if(!tag)throw FormatError("Original runtime BSP has no scenario");
  const auto scnr=metadata_offset(tag->address,tag->size);
  if(selected>=data.u32(scnr+0x210))throw FormatError("Original runtime BSP mapping is unavailable");
  const auto mapping=data.u32(scnr+0x214)+selected*0x44;
  const auto file=metadata_offset(std::uint32_t(mapping),0x44);
  const auto &bsp=bsps_[selected];
  if(data.u32(file)!=bsp.file_offset || data.u32(file+4)!=bsp.size ||
      data.u32(file+8)!=bsp.memory_address || data.u32(file+0x14)!=bsp.datum)
    throw FormatError("Original runtime BSP mapping differs from its cache owner");
  SourceRuntimeCacheBspImage out;
  out.mapping=std::uint32_t(mapping);out.index=std::uint32_t(selected);out.file_offset=bsp.file_offset;
  out.bsp=bsp.datum;out.lightmap=data.u32(file+0x1c);
  const auto bytes=data.slice(file,0x44);std::copy(bytes.begin(),bytes.end(),out.mapping_bytes.begin());
  const auto bsp_bytes=data.slice(bsp.file_offset,bsp.size);
  out.region={bsp.memory_address,{bsp_bytes.begin(),bsp_bytes.end()}};
  return out;
}
void source_bind_runtime_cache_tags_1_1(SourceRuntimeMemory &m,const SourceRuntimeCacheTagImage &image) {
  if(image.regions.size()!=2 || !image.count || image.count>65536 || image.scenario==0xffffffffu || image.globals==0xffffffffu)
    throw FormatError("Native source cache tag delivery shape is unavailable");
  const auto read=[&](std::uint32_t p,std::size_t bytes)->std::span<const std::byte> {
    for(const auto&r:image.regions)if(p>=r.address && std::uint64_t(p)-r.address<=r.bytes.size() &&
        bytes<=r.bytes.size()-(std::uint64_t(p)-r.address))return std::span(r.bytes).subspan(p-r.address,bytes);
    throw FormatError("Native cache table points outside delivered cache bytes");
  };
  const auto header=read(image.index,32);
  if(word(header,8)!=image.table || word(header,12)!=image.scenario || word(header,16)!=image.globals || word(header,24)!=image.count)
    throw FormatError("Native cache runtime header binding differs from its literal bytes");
  const auto table=read(image.table,std::size_t(image.count)*16);
  const auto tag=[&](std::uint32_t datum,std::uint32_t group) {
    const auto rank=datum&65535u;
    if(rank>=image.count || word(table,rank*16+4)!=datum || word(table,rank*16)!=group)
      throw FormatError("Native current-map cache tag identity is unavailable");
    const auto address=word(table,rank*16+8),size=word(table,rank*16+12);
    if(!size)throw FormatError("Native current-map tag has no resident payload");
    read(address,size);return address;
  };
  const auto scenario=tag(image.scenario,0x73636e72u),globals=tag(image.globals,0x6d617467u);
  // Require the original physical allocation/arena owners. Cache delivery
  // cannot overwrite the source-created game arena or mount a parallel bank.
  const auto arena=word(m.view(0x4ed650,4),0);
  if(!arena)throw FormatError("Native cache delivery precedes original physical/arena startup");
  for(std::size_t i=0;i<image.regions.size();++i) {
    const auto&r=image.regions[i];const auto end=std::uint64_t(r.address)+r.bytes.size();
    if(r.bytes.empty() || r.address<0x80061000u || end>arena)
      throw FormatError("Native cache delivery overlaps the source-created game arena");
    m.mutable_view(r.address,r.bytes.size());
    for(std::size_t j=0;j<i;++j) {
      const auto&other=image.regions[j];
      if(r.address<std::uint64_t(other.address)+other.bytes.size() && other.address<end)
        throw FormatError("Native delivered cache regions overlap");
    }
  }
  constexpr std::array outputs{0x54fa50u,0x4eb114u,0x46fb60u,0x4e7920u,0x4e791cu};
  for(auto p:outputs)m.mutable_view(p,4);
  for(const auto&r:image.regions)m.write(r.address,r.bytes);
  // Declared native cache bindings, not stores attributed to an original PC.
  put(m,0x54fa50,image.index);put(m,0x4eb114,image.table);put(m,0x46fb60,image.scenario);
  put(m,0x4e7920,scenario);put(m,0x4e791c,globals);
}
void source_deliver_runtime_cache_bsp_1_1(SourceRuntimeMemory &m,const SourceRuntimeCacheBspImage &image) {
  const auto &r=image.region;const auto raw=std::span(r.bytes);
  const auto scenario=word(m.view(0x4e7920,4),0);
  if(!scenario || image.index>=word(m.view(scenario+0x210,4),0) ||
      image.mapping!=word(m.view(scenario+0x214,4),0)+image.index*0x44 ||
      !std::ranges::equal(m.view(image.mapping,0x44),image.mapping_bytes) ||
      word(image.mapping_bytes,0)!=image.file_offset || word(image.mapping_bytes,4)!=r.bytes.size() ||
      word(image.mapping_bytes,8)!=r.address || word(image.mapping_bytes,0x14)!=image.bsp ||
      word(image.mapping_bytes,0x1c)!=image.lightmap || r.bytes.size()<16 ||
      word(raw,0)!=r.bytes.size() || word(raw,12)!=0x73627370u)
    throw FormatError("Native BSP resource request differs from its literal cache mapping");
  const auto table=word(m.view(0x4eb114,4),0),index=word(m.view(0x54fa50,4),0);
  const auto count=word(m.view(index+24,4),0);
  const auto identity=[&](std::uint32_t datum,std::uint32_t group) {
    if(datum==0xffffffffu || (datum&65535u)>=count)throw FormatError("Native BSP tag identity unavailable");
    const auto row=m.view(table+(datum&65535u)*16,16);
    if(word(row,0)!=group || word(row,4)!=datum)throw FormatError("Native BSP tag salt/group differs");
  };
  identity(image.bsp,0x73627370u);
  if(image.lightmap!=0xffffffffu)identity(image.lightmap,0x6c746d70u);
  const auto end=std::uint64_t(r.address)+r.bytes.size();
  const auto table_bytes=std::uint64_t(count)*16;
  const auto meta=word(m.view(table+8,4),0),arena=word(m.view(0x4ed650,4),0);
  if(r.address<std::uint64_t(table)+table_bytes || end>meta || end>arena ||
      word(raw,4)<r.address+16u || word(raw,4)>=end ||
      (image.lightmap!=0xffffffffu && (word(raw,8)<r.address+16u || word(raw,8)>=end)))
    throw FormatError("Native BSP resource escapes its physical cache reservation");
  m.mutable_view(r.address,r.bytes.size());m.write(r.address,r.bytes);
}
void source_bind_runtime_cache_bsp_1_1(SourceRuntimeMemory &m,const SourceRuntimeCacheBspImage &image) {
  const auto table=word(m.view(0x4eb114,4),0);
  const auto bsp=word(m.view(table+(image.bsp&65535u)*16+8,4),0);
  const auto lightmap=image.lightmap==0xffffffffu?0u:word(m.view(table+(image.lightmap&65535u)*16+8,4),0);
  const auto raw=m.view(image.region.address,16);
  if(word(m.view(0x54fa54,4),0)!=image.region.address || bsp!=word(raw,4) ||
      (image.lightmap!=0xffffffffu && lightmap!=word(raw,8)))
    throw FormatError("Native BSP binding precedes original tag-table publication");
  const auto clusters=word(m.view(bsp+0x18,4),0);
  constexpr std::array outputs{0x46fb64u,0x4e7914u,0x4e7918u,0x4e7910u,0x4e790cu};
  for(auto p:outputs)m.mutable_view(p,p==0x46fb64u?2:4);
  auto selected=m.mutable_view(0x46fb64,2);selected[0]=std::byte(image.index);selected[1]=std::byte(image.index>>8);
  put(m,0x4e7914,lightmap);put(m,0x4e7918,bsp);put(m,0x4e7910,clusters);put(m,0x4e790c,clusters);
}
} // namespace halo2
