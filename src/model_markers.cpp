#include "halo2/model_markers.hpp"
#include "halo2/first_person_ik.hpp"
#include <bit>

namespace halo2 {
std::vector<ObjectAttachmentMarker> source_model_markers(
    const SourceTagMetadata &tag,std::uint32_t name) {
  if(tag.group()!="mode")throw FormatError("Marker owner must be an authored render model");
  const Bytes root(tag.bytes_at(tag.source_address(),0x60));
  const auto nodes=root.u32(0x48),count=root.u32(0x58);
  if(count>65536)throw FormatError("Excessive authored marker groups");
  std::vector<ObjectAttachmentMarker> result;
  if(!count)return result;
  const Bytes groups(tag.bytes_at(root.u32(0x5c),std::size_t(count)*12));
  for(unsigned n=0;n<count;++n) {
    const auto at=n*12;if(groups.u32(at)!=name)continue;
    const auto entries=groups.u32(at+4);
    if(entries>65536)throw FormatError("Excessive authored marker records");
    if(!entries)return result;
    const Bytes records(tag.bytes_at(groups.u32(at+8),std::size_t(entries)*36));
    for(unsigned i=0;i<entries;++i) {
      const auto p=i*36;const auto raw=records.slice(p,3);
      ObjectAttachmentMarker m;
      m.region=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(raw[0]));
      m.permutation=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(raw[1]));
      m.node=std::to_integer<std::uint8_t>(raw[2]);
      if(m.node>=nodes)throw FormatError("Authored marker node exceeds its model");
      m.local=source_marker_transform({records.f32(p+4),records.f32(p+8),records.f32(p+12)},
          {records.f32(p+16),records.f32(p+20),records.f32(p+24),records.f32(p+28)});
      m.authored_scale=records.f32(p+32);result.push_back(m);
    }
    break; // Original first matching group, including an empty group.
  }
  return result;
}
std::vector<PosedModelMarker> pose_model_markers(
    std::span<const ObjectAttachmentMarker> markers,std::span<const Transform> nodes,
    std::span<const std::uint8_t> permutations,std::span<const std::uint32_t> remap,
    unsigned maximum) {
  std::vector<PosedModelMarker> result;
  for(const auto &m:markers) {
    if(!permutations.empty() && m.region!=-1) {
      if(m.region<0 || unsigned(m.region)>=permutations.size())
        throw FormatError("Marker region exceeds current permutation bank");
      if(permutations[unsigned(m.region)]!=std::uint8_t(m.permutation))continue;
    }
    if(result.size()>=maximum)break;
    unsigned node=m.node;
    if(!remap.empty()) {
      if(node>=remap.size())throw FormatError("Marker node exceeds current remap bank");
      node=remap[node];
    }
    if(node>=nodes.size())throw FormatError("Marker has no current posed node");
    result.push_back({node,m.local,compose_transform(nodes[node],m.local),m.authored_scale});
  }
  return result;
}
} // namespace halo2
