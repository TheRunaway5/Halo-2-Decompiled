#pragma once
#include "halo2/fp_registration.hpp"
#include "halo2/unit_animation_graph_metadata.hpp"
#include <map>
#include <set>

namespace halo2::test {
// Read-only witness for the named dual action and its actual authored variant
// chain. Native1D3660/1E58E0 keeps the packed inheritance and follows +30/+32
// inside the resolved graph; choosing a different legal variant is expected.
class AuthoredFirstPersonDualVariants {
  std::uint32_t graph_;
  std::map<std::uint32_t,std::set<std::uint32_t>> activities_;
public:
  AuthoredFirstPersonDualVariants(const Cache &cache,const FirstPersonBindingDefinition &binding)
      :graph_(binding.graph) {
    std::map<std::uint32_t,UnitAnimationGraphMetadata> graphs;
    const auto metadata=[&](std::uint32_t datum)->const UnitAnimationGraphMetadata& {
      const auto found=graphs.find(datum);
      if(found!=graphs.end())return found->second;
      return graphs.emplace(datum,cache.load_source_unit_animation_graph_metadata(datum)).first->second;
    };
    for(const auto &clip:binding.dual_clock_clips) {
      auto resolved=graph_;
      if(clip.inheritance!=0xffffu) {
        const auto &manager=metadata(graph_);const Bytes header(manager.header());
        if(clip.inheritance>=header.u32(0x4c))throw FormatError("Dual variant inherited graph exceeds its authored owner");
        resolved=Bytes(manager.bytes_at(header.u32(0x50)+std::uint32_t(clip.inheritance)*32u+4u,4)).u32(0);
      }
      const auto &owner=metadata(resolved);const Bytes header(owner.header());
      const auto count=header.u32(0x2c),base=header.u32(0x30);
      const auto record=[&](std::uint32_t index) {
        if(index>=count || index>0x7fffu)throw FormatError("Dual variant index exceeds its authored graph");
        return Bytes(owner.bytes_at(base+index*0x6cu,0x6c));
      };
      const auto head=record(clip.index).u16(0x30);
      auto next=head==0xffffu?clip.index:std::uint32_t(head);
      std::set<std::uint32_t> seen;
      while(next!=0xffffu) {
        if(!seen.insert(next).second)throw FormatError("Dual action has a cyclic authored variant chain");
        const auto row=record(next);
        activities_[clip.activity].insert((next<<16)|clip.inheritance);
        next=row.u16(0x32);
      }
    }
  }
  bool has_activity(std::uint32_t activity)const {return activities_.contains(activity);}
  bool contains(std::uint32_t graph,std::uint32_t activity,std::uint32_t packed)const {
    const auto found=activities_.find(activity);
    return graph==graph_ && found!=activities_.end() && found->second.contains(packed);
  }
};
}
