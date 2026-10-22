#include "halo2/source_runtime_cache_tags.hpp"
#include "halo2/cache.hpp"
#include <array>
#include <algorithm>
#include <limits>

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
namespace {
std::uint32_t native_cache_base(SourceRuntimeMemory& memory,SourceRuntimeNativeCacheRange range) {
  if(!range.bytes || range.bytes%4096 || range.allocation.is_null())
    throw FormatError("Native cache range requires a complete aligned allocation");
  const auto base=memory.source_address(range.allocation,range.bytes);
  if(base<0x90000000u || base%4096 || std::uint64_t(base)+range.bytes>0x100000000ull)
    throw FormatError("Native cache range escapes its separate logical allocation domain");
  // Admission authenticates the complete backing, including static tag
  // pages which11EF70 legitimately made read-only. Each actual publication
  // below separately validates its exact mutable destination before writing.
  (void)memory.view(base,range.bytes);
  return base;
}
template<class Image>
void bind_cache_tags_tu15(SourceRuntimeMemory &m,const Image &image,bool independent_source,
    const SourceRuntimeNativeCacheRange* native=nullptr) {
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
  // Require original physical/arena startup in both paths. The physical API
  // cannot overwrite the source-created game arena; the explicit native proof
  // admits a separate metadata allocation without replacing that arena owner.
  const auto arena=word(m.view(0x4f2950,4),0);
  if(!arena)throw FormatError("Native cache delivery precedes original physical/arena startup");
  const auto native_base=native?native_cache_base(m,*native):0;
  if(native && (image.index!=native_base || image.reservation_bytes!=native->bytes))
    throw FormatError("Native cache image differs from its complete allocation owner");
  std::array<std::span<std::byte>,2> destinations{};
  for(std::size_t i=0;i<image.regions.size();++i) {
    const auto&r=image.regions[i];const auto end=std::uint64_t(r.address)+r.bytes.size();
    if(r.bytes.empty() || (native?
        (r.address<native_base || end>std::uint64_t(native_base)+native->bytes):
        (r.address<0x80061000u || end>arena)))
      throw FormatError("Native cache delivery overlaps the source-created game arena");
    destinations[i]=m.mutable_view(r.address,r.bytes.size());
    for(std::size_t j=0;j<i;++j) {
      const auto&other=image.regions[j];
      if(r.address<std::uint64_t(other.address)+other.bytes.size() && other.address<end)
        throw FormatError("Native delivered cache regions overlap");
    }
  }
  constexpr std::array outputs{0x556d90u,0x4f0414u,0x474c20u,0x4ecc20u,0x4ecc1cu};
  std::array<std::span<std::byte>,outputs.size()> output_destinations{};
  std::size_t output_index{};
  for(auto p:outputs)output_destinations[output_index++]=m.mutable_view(p,4);
  if(independent_source) {
    const auto overlaps=[](std::span<const std::byte> source,std::span<const std::byte> destination) {
      const auto s=reinterpret_cast<std::uintptr_t>(source.data());
      const auto d=reinterpret_cast<std::uintptr_t>(destination.data());
      if(source.size()>std::numeric_limits<std::uintptr_t>::max()-s ||
          destination.size()>std::numeric_limits<std::uintptr_t>::max()-d)
        throw FormatError("Native cache delivery host span wraps");
      return s<d+destination.size() && d<s+source.size();
    };
    // After exactly the old admission/observer sequence and before any copy,
    // reject writable aliases of the immutable Cache owner. Never mount it as
    // the bank, split a source write, or replace physical allocation ownership.
    for(const auto &source:image.regions) {
      for(const auto destination:destinations)
        if(overlaps(source.bytes,destination))
          throw FormatError("Native cache source aliases its physical destination");
      for(const auto destination:output_destinations)if(overlaps(source.bytes,destination))
        throw FormatError("Native cache source aliases its publication outputs");
    }
  }
  for(const auto&r:image.regions)m.write(r.address,r.bytes);
  // Native cache bindings: actual TU1.5 operands125437..125474.
  // These stores are native inputs, not execution attributed to source PCs.
  put(m,0x556d90,image.index);put(m,0x4f0414,image.table);put(m,0x474c20,image.scenario);
  put(m,0x4ecc20,scenario);put(m,0x4ecc1c,globals);
}
} // namespace
void source_bind_runtime_cache_tags_tu15(SourceRuntimeMemory &m,const SourceRuntimeCacheTagImage &image) {
  bind_cache_tags_tu15(m,image,false);
}
void source_bind_runtime_cache_tags_tu15(SourceRuntimeMemory &m,const SourceRuntimeCacheTagImage &image,
    SourceRuntimeNativeCacheRange native) {
  bind_cache_tags_tu15(m,image,true,&native);
}
void source_bind_runtime_cache_tags_tu15(SourceRuntimeMemory &m,const SourceRuntimeCacheTagView &image) {
  bind_cache_tags_tu15(m,image,true);
}
void source_bind_runtime_cache_sound_gestalt_tu15(SourceRuntimeMemory &m) {
  const auto get=[&](std::uint32_t p){return word(m.view(p,4),0);};
  const auto globals=get(0x4ecc1cu),table=get(0x4f0414u),index=get(0x556d90u);
  if(!globals || !table || !index || !get(globals+0xc0u))
    throw FormatError("TU1.5 sound metadata requires resident authored globals");
  const auto sound_globals=get(globals+0xc4u),datum=get(sound_globals+0x20u);
  const auto rank=datum&65535u;
  if(datum==0xffffffffu || rank>=get(index+24u))
    throw FormatError("TU1.5 sound gestalt datum escapes the resident cache");
  const auto row=table+rank*16u;
  if(get(row)!=0x75676821u || get(row+4u)!=datum || !get(row+12u))
    throw FormatError("TU1.5 sound gestalt salt/group/payload differs");
  const auto address=get(row+8u);m.view(address,get(row+12u));
  m.mutable_view(0x52d664u,4);put(m,0x52d664u,address);
}
namespace {
void deliver_cache_bsp_tu15(SourceRuntimeMemory &m,const SourceRuntimeCacheBspImage &image,
    const SourceRuntimeNativeCacheRange* native) {
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
  const auto native_base=native?native_cache_base(m,*native):0;
  if(native && (index!=native_base || r.address<native_base ||
      end>std::uint64_t(native_base)+native->bytes || meta<native_base ||
      meta>=std::uint64_t(native_base)+native->bytes))
    throw FormatError("Native BSP resource escapes its complete allocation owner");
  if(r.address<std::uint64_t(table)+table_bytes || end>meta || (!native && end>arena) ||
      word(raw,4)<r.address+16u || word(raw,4)>=end ||
      (image.lightmap!=0xffffffffu && (word(raw,8)<r.address+16u || word(raw,8)>=end)))
    throw FormatError("Native BSP resource escapes its physical cache reservation");
  m.mutable_view(r.address,r.bytes.size());m.write(r.address,r.bytes);
}
} // namespace
void source_deliver_runtime_cache_bsp_tu15(SourceRuntimeMemory &m,const SourceRuntimeCacheBspImage &image) {
  deliver_cache_bsp_tu15(m,image,nullptr);
}
void source_deliver_runtime_cache_bsp_tu15(SourceRuntimeMemory &m,const SourceRuntimeCacheBspImage &image,
    SourceRuntimeNativeCacheRange native) {
  deliver_cache_bsp_tu15(m,image,&native);
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
