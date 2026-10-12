#pragma once
#include "halo2/cache.hpp"
#include <functional>

namespace halo2 {
struct SourceSoundMetadataProviders {
  std::function<std::array<std::byte,20>(std::uint32_t)> definition;
  std::function<std::array<std::byte,92>(std::int8_t)> sound_class;
};
// Owns the actual map cache and its resident records for retained source sound
// consumers. Callbacks retain the backing independently of this facade.
class SoundMetadataStore {
public:
  explicit SoundMetadataStore(std::shared_ptr<const Cache>);
  SourceSoundMetadataProviders providers() const;
  std::size_t sound_count() const;
  std::size_t class_count() const;
private:
  struct State;
  std::shared_ptr<const State> state_;
};
} // namespace halo2
