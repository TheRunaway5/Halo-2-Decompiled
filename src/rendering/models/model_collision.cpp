#include "halo2/posed_collision.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
namespace {

std::uint8_t byte(const Bytes &b,std::size_t at) {return std::to_integer<std::uint8_t>(b.slice(at,1)[0]);}
std::int16_t short_value(const Bytes &b,std::size_t at) {return std::bit_cast<std::int16_t>(b.u16(at));}
float real(const Bytes &b,std::size_t at) {
  const auto value=b.f32(at);
  if(!std::isfinite(value))throw FormatError("Nonfinite collision data");
  return value;
}
Vec3 point(const Bytes &b,std::size_t at) {return {real(b,at),real(b,at+4),real(b,at+8)};}
std::int32_t packed_child(const Bytes &b,std::size_t at) {
  const std::uint32_t value=byte(b,at)|(std::uint32_t(byte(b,at+1))<<8)|
      (std::uint32_t(byte(b,at+2))<<16);
  return std::bit_cast<std::int32_t>(value|(value&0x800000u?0xff000000u:0));
}
RawCollisionBsp load_raw_bsp(const Bytes &b,std::size_t at,std::int64_t magic,
    std::int16_t model_node) {
  RawCollisionBsp out;out.node=model_node;
  const auto nodes=TagAddressSpace(b, magic).block(at, 8),planes=TagAddressSpace(b, magic).block(at+8, 16),
      leaves=TagAddressSpace(b, magic).block(at+0x10, 4),refs=TagAddressSpace(b, magic).block(at+0x18, 4),
      nodes2=TagAddressSpace(b, magic).block(at+0x20, 16),surfaces=TagAddressSpace(b, magic).block(at+0x28, 8),
      edges=TagAddressSpace(b, magic).block(at+0x30, 12),vertices=TagAddressSpace(b, magic).block(at+0x38, 16);
  for(std::size_t i=0;i<nodes.count;++i) {
    const auto p=nodes.offset+i*8;
    out.nodes.push_back({short_value(b,p),{packed_child(b,p+2),packed_child(b,p+5)}});
  }
  for(std::size_t i=0;i<planes.count;++i) {
    const auto p=planes.offset+i*16;out.planes.push_back({point(b,p),real(b,p+12)});
  }
  for(std::size_t i=0;i<leaves.count;++i) {
    const auto p=leaves.offset+i*4;out.leaves.push_back({byte(b,p),byte(b,p+1),b.u16(p+2)});
  }
  for(std::size_t i=0;i<refs.count;++i) {
    const auto p=refs.offset+i*4;out.references.push_back({short_value(b,p),short_value(b,p+2)});
  }
  for(std::size_t i=0;i<nodes2.count;++i) {
    const auto p=nodes2.offset+i*16;
    out.nodes2.push_back({{real(b,p),real(b,p+4)},real(b,p+8),{short_value(b,p+12),short_value(b,p+14)}});
  }
  for(std::size_t i=0;i<surfaces.count;++i) {
    const auto p=surfaces.offset+i*8;
    out.surfaces.push_back({short_value(b,p),b.u16(p+2),byte(b,p+4),byte(b,p+5),short_value(b,p+6)});
  }
  for(std::size_t i=0;i<edges.count;++i) {
    const auto p=edges.offset+i*12;
    out.edges.push_back({{b.u16(p),b.u16(p+2)},{b.u16(p+4),b.u16(p+6)},{b.u16(p+8),b.u16(p+10)}});
  }
  for(std::size_t i=0;i<vertices.count;++i) {
    out.vertices.push_back(point(b,vertices.offset+i*16));
    out.vertex_edges.push_back(b.u16(vertices.offset+i*16+12));
  }
  return out;
}
}
ModelCollision Cache::load_model_collision(std::uint32_t datum) const {
  const auto tag=[&](std::uint32_t value,const char *group)->std::size_t {
    const auto it=query_tag({.handle=TagHandle{value}, .group=TagGroup::from_string(group)});
    if(!it)throw FormatError("Missing original model collision tag");
    return metadata_offset(it->address,it->size);
  };
  const Bytes b(cache_bytes());const auto model=tag(datum,"hlmt");
  ModelCollision out;out.model=datum;out.collision=b.u32(model+0xc);
  const auto collision=tag(out.collision,"coll");
  const auto materials=TagAddressSpace(b, metadata_magic_).block(model+0x58, 0x14, 32768),
      model_regions=TagAddressSpace(b, metadata_magic_).block(model+0x70, 0x10, 16),
      nodes=TagAddressSpace(b, metadata_magic_).block(model+0x78, 0x5c, 256),
      regions=TagAddressSpace(b, metadata_magic_).block(collision+0x1c, 0xc, 16);
  for(std::size_t i=0;i<materials.count;++i) {
    const auto p=materials.offset+i*0x14;out.materials.push_back({short_value(b,p+6),short_value(b,p+0x10)});
  }
  for(std::size_t i=0;i<model_regions.count;++i) {
    const auto p=model_regions.offset+i*0x10;ModelCollision::Region region{byte(b,p+4),{}};
    const auto perms=TagAddressSpace(b, metadata_magic_).block(p+8, 8, 128);
    for(std::size_t j=0;j<perms.count;++j)region.permutations.push_back(byte(b,perms.offset+j*8+5));
    out.model_regions.push_back(std::move(region));
  }
  for(std::size_t i=0;i<nodes.count;++i) {
    const auto p=nodes.offset+i*0x5c+0x28;
    out.authored_inverse.push_back({real(b,p),{point(b,p+4),point(b,p+16),point(b,p+28)},point(b,p+40)});
  }
  for(std::size_t i=0;i<regions.count;++i) {
    const auto perms=TagAddressSpace(b, metadata_magic_).block(regions.offset+i*0xc+4, 0x14, 128);
    std::vector<ModelCollision::Permutation> region;
    for(std::size_t j=0;j<perms.count;++j) {
      const auto bsps=TagAddressSpace(b, metadata_magic_).block(perms.offset+j*0x14+4, 0x44, 256);
      ModelCollision::Permutation permutation;
      for(std::size_t k=0;k<bsps.count;++k) {
        const auto p=bsps.offset+k*0x44;
        auto bsp=load_raw_bsp(b,p+4,metadata_magic_,short_value(b,p));
        if(bsp.node<0 || std::size_t(bsp.node)>=nodes.count)throw FormatError("Collision node outside model");
        permutation.push_back(std::move(bsp));
      }
      region.push_back(std::move(permutation));
    }
    out.regions.push_back(std::move(region));
  }
  return out;
}
TerrainCollision Cache::load_raw_terrain_collision(std::size_t index) const {
  if(index>=bsps_.size())throw FormatError("Raw terrain BSP index outside scenario");
  const Bytes data(cache_bytes());const auto &bsp=bsps_[index];
  const Bytes meta(data.slice(bsp.file_offset,bsp.size));
  const auto magic=std::int64_t(bsp.memory_address);
  const auto root=TagAddressSpace(meta,magic).offset(meta.u32(4),0x40);
  const auto records=TagAddressSpace(meta, magic).block(std::size_t(root)+0x14, 0x40);
  const auto materials=TagAddressSpace(meta, magic).block(std::size_t(root)+0xc, 0x14);
  const auto leaves=TagAddressSpace(meta, magic).block(std::size_t(root)+0x2c, 8);
  TerrainCollision out;
  for(std::size_t i=0;i<records.count;++i)
    out.bsps.push_back(load_raw_bsp(meta,records.offset+i*0x40,magic,-1));
  for(std::size_t i=0;i<materials.count;++i)
    out.global_materials.push_back(short_value(meta,materials.offset+i*0x14+8));
  for(std::size_t i=0;i<leaves.count;++i)
    out.leaf_clusters.push_back(short_value(meta,leaves.offset+i*8));
  return out;
}
}
