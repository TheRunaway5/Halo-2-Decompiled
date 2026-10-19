#include "halo2/source_tag_metadata_store.hpp"
#include <algorithm>
#include <map>
#include <set>

namespace halo2 {
SourceTagMetadata Cache::load_source_tag_metadata(std::uint32_t datum) const {
  const auto* tag = find_tag({datum});
  const auto payload=tag_payload({datum});
  return {datum, tag->group, tag->address,
          payload};
}

struct SourceTagMetadataStore::State {
  State(std::shared_ptr<const Cache> owned, std::vector<std::string> selected)
      : cache(std::move(owned)) {
    if (!cache || selected.empty())
      throw FormatError("Source tag providers require owned cache and explicit groups");
    std::set<std::string> groups(selected.begin(), selected.end());
    for (const auto &group : groups)
      if (group.size() != 4 || group == "sbsp" || group == "ltmp")
        throw FormatError("Invalid resident source tag group selection");
    for (const auto &tag : cache->select_tags()) {
      if (!tag.address || !groups.contains(tag.group))
        continue;
      if(!tag.size && !cache->query_tag({.group=tag.type(),.address=tag.address,.minimum_size=1}))continue;
      if (!tags.emplace(tag.datum, cache->load_source_tag_metadata(tag.datum)).second)
        throw FormatError("Duplicate resident source tag datum");
    }
    // Moving a canonical MCC definition can leave literal forwarding pointers
    // in other selected tags. These allocations retain their salted identity,
    // but must never replace a canonical definition or resurrect an unresolved
    // program. Admit their bytes only to the same bounded group-scoped reader.
    for (const auto &tag : cache->retained_tag_allocations()) {
      if (!groups.contains(tag.group))
        continue;
      const auto *current = cache->find_tag(tag.handle(), tag.type());
      if (!current || current->group != tag.group || current->name != tag.name ||
          !tag.address || !tag.size)
        throw FormatError("Retained source tag allocation has no typed identity");
      if (!tags.contains(tag.datum))
        continue;
      retained.emplace_back(tag.datum, tag.group, tag.address,
                            cache->read_tag_bytes(tag.address, tag.size));
    }
  }
  const SourceTagMetadata &definition(std::uint32_t datum) const {
    const auto found = tags.find(datum);
    if (found == tags.end())
      throw FormatError("Source tag is outside explicitly owned groups");
    return found->second;
  }
  std::uint32_t index_address(std::uint16_t index) const {
    const auto &slot = cache->tag_at_index(index);
    const auto &owned = definition(slot.datum);
    if (owned.group() != slot.group || owned.source_address() != slot.address)
      throw FormatError("Source tag table slot is outside explicitly owned backing");
    return owned.source_address();
  }
  std::span<const std::byte> read(std::uint32_t address, std::size_t bytes) const {
    const SourceTagMetadata *owner = nullptr;
    const auto consider = [&](const SourceTagMetadata &tag) {
      if (address < tag.source_address())
        return;
      const auto offset = std::uint64_t(address) - tag.source_address();
      if (offset > tag.size() || bytes > tag.size() - offset)
        return;
      if (owner && bytes && (owner->source_address()!=tag.source_address() || owner->size()!=tag.size() || owner->group()!=tag.group()))
        throw FormatError("Ambiguous resident source tag backing");
      owner = &tag;
    };
    for (const auto &[_, tag] : tags)
      consider(tag);
    for (const auto &tag : retained)
      consider(tag);
    if (!owner)
      throw FormatError("Source tag pointer escapes explicitly owned payloads");
    return owner->bytes_at(address, bytes);
  }
  std::shared_ptr<const Cache> cache;
  std::map<std::uint32_t, SourceTagMetadata> tags;
  std::vector<SourceTagMetadata> retained;
};

SourceTagMetadataStore::SourceTagMetadataStore(
    std::shared_ptr<const Cache> cache, std::vector<std::string> groups)
    : state_(std::make_shared<State>(std::move(cache),std::move(groups))) {}

SourceTagMetadataProviders SourceTagMetadataStore::providers() const {
  const auto state = state_;
  return {
      [state](std::uint32_t datum) -> const SourceTagMetadata & {
        return state->definition(datum);
      },
      [state](std::uint32_t datum) {
        return state->definition(datum).source_address();
      },
      [state](std::uint32_t address,std::size_t bytes) {
        return state->read(address,bytes);
      }};
}
std::size_t SourceTagMetadataStore::tag_count() const { return state_->tags.size(); }
std::function<std::uint32_t(std::uint16_t)>
SourceTagMetadataStore::index_address_provider() const {
  const auto state = state_;
  return [state](std::uint16_t index) { return state->index_address(index); };
}
} // namespace halo2
