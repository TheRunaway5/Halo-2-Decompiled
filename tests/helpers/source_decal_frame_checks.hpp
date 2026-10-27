#pragma once
#include "halo2/source_decal_frame_tu15.hpp"
#include <bit>
#include <optional>
namespace halo2::test {
inline bool same_source_decal_float(float a,float b) {
 return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);
}
inline bool same_source_decal_frame(const std::optional<SourceDecalFrameTu15>& expected,
 const std::optional<SourceDecalFrameTu15>& actual) {
 if(expected.has_value()!=actual.has_value())return false;if(!expected)return true;
 const auto& a=*expected;const auto& b=*actual;
 if(a.tick!=b.tick || a.records.size()!=b.records.size() || a.allocations.size()!=b.allocations.size() || a.heads.size()!=b.heads.size())return false;
 for(std::size_t n=0;n<a.records.size();++n) {
  const auto& x=a.records[n];const auto& y=b.records[n];
  if(x.datum!=y.datum || x.definition!=y.definition || x.cache_datum!=y.cache_datum || x.tick!=y.tick ||
     x.tint!=y.tint || x.cluster!=y.cluster || x.layer!=y.layer || x.flags!=y.flags ||
     !same_source_decal_float(x.lifetime,y.lifetime) || !same_source_decal_float(x.decay,y.decay) ||
     x.primary_datum!=y.primary_datum || x.next_cluster_datum!=y.next_cluster_datum ||
     x.bitmap_index!=y.bitmap_index || x.quad_count!=y.quad_count)return false;
 }
 for(std::size_t n=0;n<a.allocations.size();++n) {
  const auto& x=a.allocations[n];const auto& y=b.allocations[n];
  if(x.cache_datum!=y.cache_datum || x.quads.size()!=y.quads.size())return false;
  for(std::size_t q=0;q<x.quads.size();++q)for(unsigned v=0;v<4;++v) {
   const auto& p=x.quads[q][v];const auto& r=y.quads[q][v];
   if(p.packed_projection!=r.packed_projection || !same_source_decal_float(p.position.x,r.position.x) ||
      !same_source_decal_float(p.position.y,r.position.y) || !same_source_decal_float(p.position.z,r.position.z))return false;
   for(unsigned c=0;c<3;++c)if(!same_source_decal_float(p.projection[c],r.projection[c]))return false;
  }
 }
 for(std::size_t n=0;n<a.heads.size();++n)
  if(a.heads[n].cluster!=b.heads[n].cluster || a.heads[n].layer!=b.heads[n].layer || a.heads[n].datum!=b.heads[n].datum)return false;
 return true;
}
}
