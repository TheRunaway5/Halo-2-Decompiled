#pragma once
#include "halo2/cache.hpp"
#include "halo2/unit_animation_selection.hpp"
#include <memory>

namespace halo2 {
// Retained, read-only map metadata providers. Returned callbacks retain their
// cache and graph backing independently of this facade's lifetime. Definitions
// have stable references even after additional definitions are imported.
// Mutable resource descriptors, RNG and runtime admission are separate owners.
class UnitAnimationMetadataStore {
public:
  explicit UnitAnimationMetadataStore(std::shared_ptr<const Cache>);
  SourceAnimationGraphMemory graph_memory() const;
  std::function<const UnitAnimationDefinition &(std::uint32_t, std::uint32_t)>
  definition_provider() const;
  const UnitAnimationDefinition &definition(std::uint32_t graph,
                                             std::uint32_t packed) const;
  std::size_t graph_count() const;

private:
  struct State;
  std::shared_ptr<State> state_;
};
} // namespace halo2
