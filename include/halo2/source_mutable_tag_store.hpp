#pragma once
#include "halo2/cache.hpp"
#include <functional>

namespace halo2 {
struct SourceMutableTagProviders {
  std::function<std::uint32_t(std::uint32_t)> address;
  std::function<std::uint32_t(std::uint16_t)> index_address;
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
};
// One retained mutable copy of each explicitly selected resident payload.
// Source addresses and all serialized bytes are preserved. Captured providers
// share this same copy across requests and retain the original cache owner.
// The owning source update thread must serialize mutation; this object does
// not initialize runtime fields, create a raw tag table or activate a map/BSP.
class SourceMutableTagStore {
public:
  SourceMutableTagStore(std::shared_ptr<const Cache>,std::vector<std::string> groups);
  SourceMutableTagProviders providers() const;
  std::size_t tag_count() const;
private:
  struct State;
  std::shared_ptr<State> state_;
};
} // namespace halo2
