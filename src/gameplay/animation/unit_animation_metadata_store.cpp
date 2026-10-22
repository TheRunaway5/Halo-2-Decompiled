#include "halo2/unit_animation_metadata_store.hpp"
#include "halo2/unit_animation_graph_metadata.hpp"
#include <map>
#include <mutex>

namespace halo2 {
struct UnitAnimationMetadataStore::State {
  explicit State(std::shared_ptr<const Cache> owned) : cache(std::move(owned)) {
    if (!cache)
      throw FormatError("Source animation metadata requires an owned cache");
    if(cache->is_mcc()) {
      if(cache->string_id(0x04000557u)!="dual")
        throw FormatError("MCC animation dual class differs from its named namespace");
      dual_weapon_class=0x04000557u;
    }
    for (const auto &tag : cache->select_tags()) {
      if (tag.group != "jmad" || !tag.address)
        continue;
      if (!graphs.emplace(tag.datum,
                          cache->load_source_unit_animation_graph_metadata(
                              tag.datum)).second)
        throw FormatError("Duplicate source animation graph datum");
    }
  }

  const UnitAnimationDefinition &definition(std::uint32_t graph,
                                             std::uint32_t packed) {
    const auto key = std::pair(graph, packed);
    std::lock_guard lock(definition_mutex);
    if (const auto found = definitions.find(key); found != definitions.end())
      return found->second;
    auto actual = cache->load_source_unit_animation_definition(graph, packed);
    return definitions.emplace(key, std::move(actual)).first->second;
  }

  std::span<const std::byte> read(std::uint32_t address, std::size_t bytes) const {
    const UnitAnimationGraphMetadata *owner = nullptr;
    for (const auto &[_, graph] : graphs) {
      if (address < graph.source_address())
        continue;
      const auto offset = std::uint64_t(address) - graph.source_address();
      if (offset > graph.size() || bytes > graph.size() - offset)
        continue;
      // Adjacent zero-length end/start reads contain no ambiguous bytes.
      if (owner && bytes)
        throw FormatError("Ambiguous source animation graph backing");
      owner = &graph;
    }
    if (!owner)
      throw FormatError("Source animation pointer escapes owned graph backing");
    return owner->bytes_at(address, bytes);
  }

  std::shared_ptr<const Cache> cache;
  std::uint32_t dual_weapon_class=0x0400054bu;
  std::map<std::uint32_t, UnitAnimationGraphMetadata> graphs;
  std::mutex definition_mutex;
  std::map<std::pair<std::uint32_t, std::uint32_t>, UnitAnimationDefinition>
      definitions;
};

UnitAnimationMetadataStore::UnitAnimationMetadataStore(
    std::shared_ptr<const Cache> cache)
    : state_(std::make_shared<State>(std::move(cache))) {}

SourceAnimationGraphMemory UnitAnimationMetadataStore::graph_memory() const {
  const auto state = state_;
  return {
      [state](std::uint32_t datum) {
        const auto found = state->graphs.find(datum);
        if (found == state->graphs.end())
          throw FormatError("Source animation graph is unavailable");
        return found->second.source_address();
      },
      [state](std::uint32_t address, std::size_t bytes) {
        return state->read(address, bytes);
      },state->dual_weapon_class};
}

std::function<const UnitAnimationDefinition &(std::uint32_t, std::uint32_t)>
UnitAnimationMetadataStore::definition_provider() const {
  const auto state = state_;
  return [state](std::uint32_t graph,
                 std::uint32_t packed) -> const UnitAnimationDefinition & {
    return state->definition(graph, packed);
  };
}

const UnitAnimationDefinition &UnitAnimationMetadataStore::definition(
    std::uint32_t graph, std::uint32_t packed) const {
  return state_->definition(graph, packed);
}

std::size_t UnitAnimationMetadataStore::graph_count() const {
  return state_->graphs.size();
}
} // namespace halo2
