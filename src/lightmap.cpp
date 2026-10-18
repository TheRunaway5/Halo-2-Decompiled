#include "halo2/cache.hpp"
#include "halo2/bsp_lighting.hpp"
#include <cmath>
#include <map>

namespace halo2 {
namespace {

} // namespace
BspLighting Cache::load_bsp_lighting(const Bytes &metadata,
                                     std::int64_t magic) const {
  BspLighting result;
  const auto pointer = metadata.u32(8);
  if (pointer == 0 || pointer == 0xffffffffu)
    return result;
  const auto base = TagAddressSpace(metadata,magic).offset(pointer,136);
  const auto groups = TagAddressSpace(metadata, magic).block(base + 128, 104);
  if (!groups.count)
    return result;
  // The default lighting group is explicit. Alternate lighting groups require
  // the retail scenario lighting state and must not be merged into this one.
  const auto group = groups.offset;
  // Original 2D000 rejects static lighting from a stale geometry checksum.
  // The supplied Forerunner Ship BSP6 retains an older group; its bucket
  // extents and instance count cannot be applied to the current geometry.
  result.structure_matches=metadata.u32(group+4)==metadata.u32(16+8);
  const auto palettes = TagAddressSpace(metadata, magic).block(group + 8, 1024);
  const auto clusters = TagAddressSpace(metadata, magic).block(group + 40, 4);
  const auto instances = TagAddressSpace(metadata, magic).block(group + 72, 4);
  const auto bitmap = metadata.u32(group + 28);
  result.bitmap_datum=bitmap;
  std::map<std::pair<unsigned, unsigned>, std::uint32_t> images;
  auto bindings = [&](TagBlockRange records, std::vector<std::uint32_t> &output) {
    output.resize(records.count);
    for (std::size_t i = 0; i < records.count; ++i) {
      const auto at = records.offset + i * 4;
      const auto index = static_cast<std::int16_t>(metadata.u16(at));
      if (index == -1)
        continue; // Unbound; the independent authored policy selects pixel/vertex lighting.
      if (index < 0)
        throw FormatError("invalid lightmap bitmap index");
      const auto palette =
          std::to_integer<unsigned>(metadata.slice(at + 2, 1)[0]);
      if (palette >= palettes.count)
        throw FormatError("lightmap palette outside group");
      const auto key = std::pair{unsigned(index), palette};
      auto found = images.find(key);
      if (found == images.end()) {
        auto image =
            load_bitmap(bitmap, index,
                        metadata.slice(palettes.offset + palette * 1024, 1024));
        result.images.push_back(std::move(image));
        found =
            images
                .emplace(key, static_cast<std::uint32_t>(result.images.size()))
                .first;
      }
      output[i] = found->second;
    }
  };
  bindings(clusters, result.clusters);
  bindings(instances, result.instances);

  // Retail 4D300 binds the incident stream from the instance's geometry
  // bucket, independently of its bitmap/palette binding. 4DC60 binds the
  // optional incident-radiance stream from the same bucket. These resources
  // belong to the instance, not to its reusable sbsp geometry definition.
  const auto buckets=TagAddressSpace(metadata, magic).block(group+64, 56);
  result.buckets.resize(buckets.count);
  for(std::size_t i=0;i<buckets.count;++i) {
    const auto at=buckets.offset+i*56;
    const auto flags=metadata.u16(at);
    if(!(flags&3))continue;
    const auto resources=TagAddressSpace(metadata, magic).block(at+28, 16);
    const auto raw=raw_data(metadata.u32(at+12),metadata.u32(at+16));
    const Bytes bytes(raw);
    const auto raw_base=std::size_t(metadata.u32(at+20))+8;
    bytes.require(raw_base,0);
    bool declaration=false;
    for(std::size_t j=0;j<resources.count;++j) {
      const auto resource=resources.offset+j*16;
      if(std::to_integer<unsigned>(metadata.slice(resource,1)[0])!=0 ||
         metadata.u16(resource+4)!=0 || metadata.u16(resource+6)!=32)continue;
      if(declaration || metadata.u32(resource+8)!=64)
        throw FormatError("invalid lightmap bucket vertex declaration");
      declaration=true;
      const auto source=raw_base+std::size_t(metadata.u32(resource+12));
      const auto descriptor=bytes.slice(source,64);
      // Retail 4D300/4DC60 bind these two 32-byte stream descriptors.
      // Every supplied MP bucket declares 48/NORMPACKED3 (4 bytes),
      // followed by 47/RGB8 (3 bytes) when incident radiance is present.
      if((flags&1) && (descriptor[0]!=std::byte(pc_geometry()?47:48) || descriptor[1]!=std::byte(pc_geometry()?12:4)))
        throw FormatError("unsupported lightmap incident vertex declaration");
      if((flags&2) && (descriptor[32]!=std::byte(pc_geometry()?46:47) || descriptor[33]!=std::byte(pc_geometry()?4:3)))
        throw FormatError("unsupported lightmap radiance vertex declaration");
    }
    if(!declaration)throw FormatError("lightmap bucket vertex declaration missing");
    auto& bucket=result.buckets[i];
    bool direction=false,color=false;
    for(std::size_t j=0;j<resources.count;++j) {
      const auto resource=resources.offset+j*16;
      if(std::to_integer<unsigned>(metadata.slice(resource,1)[0])!=2 ||
         metadata.u16(resource+4)!=0)continue;
      const auto channel=metadata.u16(resource+6);
      const auto size=metadata.u32(resource+8);
      const auto source=raw_base+std::size_t(metadata.u32(resource+12));
      bytes.require(source,size);
      if(channel==0 && (flags&1)) {
        const unsigned stride=pc_geometry()?12:4;
        if(direction || size%stride)throw FormatError("invalid incident-light bucket stream");
        direction=true;
        if(pc_geometry())for(std::size_t k=0;k<size;k+=12) {
          Vec3 value{bytes.f32(source+k),bytes.f32(source+k+4),bytes.f32(source+k+8)};
          if(!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z))
            throw FormatError("nonfinite Vista incident-light vector");
          if(is_mcc())bucket.incident.push_back(pack_bsp_incident_tu15(value));
          else bucket.incident_vectors.push_back(value);
        } else {
          bucket.incident.reserve(size/4);
          for(std::size_t k=0;k<size;k+=4)bucket.incident.push_back(bytes.u32(source+k));
        }
      } else if(channel==1 && (flags&2)) {
        // Format 47 is packed RGB8, not the four-byte format 46. The original
        // 4DC60 changes the offset multiplier according to this declaration.
        const unsigned stride=pc_geometry()?4:3;
        if(color || size%stride)throw FormatError("invalid incident-radiance bucket stream");
        color=true;
        bucket.radiance.resize(size/stride);
        for(std::size_t k=0;k<size/stride;++k)for(unsigned component=0;component<3;++component)
          bucket.radiance[k][component]=std::to_integer<std::uint8_t>(bytes.slice(source+k*stride+component,1)[0]);
      }
    }
    if(bool(flags&1)!=direction || bool(flags&2)!=color)
      throw FormatError("lightmap bucket is missing an authored vertex stream");
    if(direction && color && bsp_incident_count(bucket)!=bucket.radiance.size())
      throw FormatError("lightmap bucket stream vertex counts differ");
  }
  const auto reference=[&](std::size_t at) {
    BspLightingReference ref;
    ref.bucket=static_cast<std::int16_t>(metadata.u16(at+2));
    if(ref.bucket < -1 || (ref.bucket>=0 && std::size_t(ref.bucket)>=buckets.count))
      throw FormatError("geometry references an invalid lightmap bucket");
    const auto offsets=TagAddressSpace(metadata, magic).block(at+4, 2);
    ref.section_offsets.reserve(offsets.count);
    for(std::size_t k=0;k<offsets.count;++k)
      ref.section_offsets.push_back(metadata.u16(offsets.offset+k*2));
    return ref;
  };
  const auto refs=TagAddressSpace(metadata, magic).block(group+80, 12);
  result.instance_buckets.reserve(refs.count);
  for(std::size_t i=0;i<refs.count;++i)
    result.instance_buckets.push_back(reference(refs.offset+i*12));
  const auto scenery=TagAddressSpace(metadata, magic).block(group+88, 12);
  const auto scenery_refs=TagAddressSpace(metadata, magic).block(group+96, 12);
  if(scenery.count!=scenery_refs.count)
    throw FormatError("scenery lightmap identity/reference counts differ");
  result.scenery.reserve(scenery.count);
  for(std::size_t i=0;i<scenery.count;++i) {
    const auto at=scenery.offset+i*12;
    BspLightingScenery info;
    info.unique_id=metadata.u32(at);
    info.origin_bsp=static_cast<std::int16_t>(metadata.u16(at+4));
    info.type=static_cast<std::int8_t>(std::to_integer<unsigned>(metadata.slice(at+6,1)[0]));
    info.source=static_cast<std::int8_t>(std::to_integer<unsigned>(metadata.slice(at+7,1)[0]));
    info.checksum=metadata.u32(at+8);
    info.reference=reference(scenery_refs.offset+i*12);
    result.scenery.push_back(std::move(info));
  }
  return result;
}
} // namespace halo2
