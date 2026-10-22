#include "halo2/source_mutable_tag_store.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <map>
#include <set>

namespace halo2 {
struct SourceMutableTagStore::State {
  struct Payload {
    std::string group;
    std::uint32_t address;
    std::vector<std::byte> bytes;
  };
  struct Interval {
    std::uint32_t start;
    std::uint64_t end,prefix_end;
    Payload *payload;
  };
  State(std::shared_ptr<const Cache> owner,std::vector<std::string> selected)
      :cache(std::move(owner)) {
    if(!cache||selected.empty())
      throw FormatError("Mutable source tags require owned cache and explicit groups");
    const std::set<std::string> groups(selected.begin(),selected.end());
    for(const auto &group:groups)
      if(group.size()!=4||group=="sbsp"||group=="ltmp")
        throw FormatError("Mutable source tag group requires another mapping");
    for(const auto &tag:cache->select_tags()) {
      if(!tag.address||!tag.size||!groups.contains(tag.group))continue;
      const auto original=cache->load_source_tag_metadata(tag.datum);
      const auto bytes=original.bytes_at(original.source_address(),original.size());
      Payload payload{tag.group,original.source_address(),{bytes.begin(),bytes.end()}};
      if(!payloads.emplace(tag.datum,std::move(payload)).second)
        throw FormatError("Duplicate mutable source tag datum");
    }
    for(auto &[_,payload]:payloads)
      intervals.push_back({payload.address,std::uint64_t(payload.address)+payload.bytes.size(),0,&payload});
    std::ranges::sort(intervals,{},&Interval::start);
    std::uint64_t maximum_end=0;
    for(auto &interval:intervals) {
      maximum_end=std::max(maximum_end,interval.end);interval.prefix_end=maximum_end;
    }
  }
  Payload &definition(std::uint32_t datum) {
    const auto found=payloads.find(datum);
    if(found==payloads.end())throw FormatError("Mutable source tag is outside owned groups");
    return found->second;
  }
  std::uint32_t index_address(std::uint16_t index) {
    const auto &slot=cache->tag_at_index(index);const auto &payload=definition(slot.datum);
    if(payload.group!=slot.group||payload.address!=slot.address)
      throw FormatError("Mutable source slot has no matching owned payload");
    return payload.address;
  }
  std::span<std::byte> memory(std::uint32_t address,std::size_t extent) {
    if(!extent||std::uint64_t(extent)>(std::uint64_t{1}<<32)-address)
      throw FormatError("Mutable source access requires a bounded nonempty interval");
    const auto end=std::uint64_t(address)+extent;
    Payload *owner=nullptr;
    auto candidate=std::upper_bound(intervals.begin(),intervals.end(),address,
        [](std::uint32_t at,const Interval &interval){return at<interval.start;});
    while(candidate!=intervals.begin()) {
      --candidate;
      // No earlier interval can contain the requested end once the retained
      // prefix maximum is smaller. Overlapping owners are still rejected.
      if(candidate->prefix_end<end)break;
      if(candidate->end>=end) {
        if(owner)throw FormatError("Mutable source interval has ambiguous backing");
        owner=candidate->payload;
      }
    }
    if(!owner)throw FormatError("Mutable source interval escapes owned payloads");
    return std::span(owner->bytes).subspan(std::size_t(address-owner->address),extent);
  }
  std::shared_ptr<const Cache> cache;
  std::map<std::uint32_t,Payload> payloads;
  std::vector<Interval> intervals;
};
SourceMutableTagStore::SourceMutableTagStore(std::shared_ptr<const Cache> cache,
                                           std::vector<std::string> groups)
    :state_(std::make_shared<State>(std::move(cache),std::move(groups))) {}
SourceMutableTagProviders SourceMutableTagStore::providers() const {
  const auto state=state_;
  return {
    [state](std::uint32_t datum){return state->definition(datum).address;},
    [state](std::uint16_t index){return state->index_address(index);},
    [state](std::uint32_t address,std::size_t extent){return state->memory(address,extent);}
  };
}
std::size_t SourceMutableTagStore::tag_count() const {return state_->payloads.size();}
} // namespace halo2
