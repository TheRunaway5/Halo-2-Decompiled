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
void source_bind_runtime_cache_tags_tu15(SourceRuntimeMemory &m,const SourceRuntimeCacheTagImage &image) {
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
  const auto arena=word(m.view(0x4f2950,4),0);
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
  constexpr std::array outputs{0x556d90u,0x4f0414u,0x474c20u,0x4ecc20u,0x4ecc1cu};
  for(auto p:outputs)m.mutable_view(p,4);
  for(const auto&r:image.regions)m.write(r.address,r.bytes);
  // Native cache bindings: actual TU1.5 operands125437..125474.
  // These stores are native inputs, not execution attributed to source PCs.
  put(m,0x556d90,image.index);put(m,0x4f0414,image.table);put(m,0x474c20,image.scenario);
  put(m,0x4ecc20,scenario);put(m,0x4ecc1c,globals);
}
void source_deliver_runtime_cache_bsp_tu15(SourceRuntimeMemory &m,const SourceRuntimeCacheBspImage &image) {
  const auto &r=image.region;const auto raw=std::span(r.bytes);
  const auto scenario=word(m.view(0x4ecc20,4),0);
  if(!scenario || image.index>=word(m.view(scenario+0x210,4),0) ||
      image.mapping!=word(m.view(scenario+0x214,4),0)+image.index*0x44 ||
      !std::ranges::equal(m.view(image.mapping,0x44),image.mapping_bytes) ||
      word(image.mapping_bytes,0)!=image.file_offset || word(image.mapping_bytes,4)!=r.bytes.size() ||
      word(image.mapping_bytes,8)!=r.address || word(image.mapping_bytes,0x14)!=image.bsp ||
      word(image.mapping_bytes,0x1c)!=image.lightmap || r.bytes.size()<16 ||
      word(raw,0)!=r.bytes.size() || word(raw,12)!=0x73627370u)
    throw FormatError("Native BSP resource request differs from its literal cache mapping");
  const auto table=word(m.view(0x4f0414,4),0),index=word(m.view(0x556d90,4),0);
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
  const auto meta=word(m.view(table+8,4),0),arena=word(m.view(0x4f2950,4),0);
  if(r.address<std::uint64_t(table)+table_bytes || end>meta || end>arena ||
      word(raw,4)<r.address+16u || word(raw,4)>=end ||
      (image.lightmap!=0xffffffffu && (word(raw,8)<r.address+16u || word(raw,8)>=end)))
    throw FormatError("Native BSP resource escapes its physical cache reservation");
  m.mutable_view(r.address,r.bytes.size());m.write(r.address,r.bytes);
}
// Actual TU1.5 selected-BSP operands11EA1D..11EA69.
void source_bind_runtime_cache_bsp_tu15(SourceRuntimeMemory &m,const SourceRuntimeCacheBspImage &image) {
  const auto table=word(m.view(0x4f0414,4),0);
  const auto bsp=word(m.view(table+(image.bsp&65535u)*16+8,4),0);
  const auto lightmap=image.lightmap==0xffffffffu?0u:word(m.view(table+(image.lightmap&65535u)*16+8,4),0);
  const auto raw=m.view(image.region.address,16);
  if(word(m.view(0x556d94,4),0)!=image.region.address || bsp!=word(raw,4) ||
      (image.lightmap!=0xffffffffu && lightmap!=word(raw,8)))
    throw FormatError("Native BSP binding precedes original tag-table publication");
  const auto collision=word(m.view(bsp+0x18,4),0);
  constexpr std::array outputs{0x474c24u,0x4ecc14u,0x4ecc18u,0x4ecc10u,0x4ecc0cu};
  for(auto p:outputs)m.mutable_view(p,p==0x474c24u?2:4);
  auto selected=m.mutable_view(0x474c24,2);selected[0]=std::byte(image.index);selected[1]=std::byte(image.index>>8);
  put(m,0x4ecc14,lightmap);put(m,0x4ecc18,bsp);put(m,0x4ecc10,collision);put(m,0x4ecc0c,collision);
}
} // namespace halo2
