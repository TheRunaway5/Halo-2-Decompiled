#pragma once
#include "halo2/cache.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <functional>

namespace halo2 {
struct SourceTagMetadataProviders {
  std::function<const SourceTagMetadata &(std::uint32_t datum)> definition;
  std::function<std::uint32_t(std::uint32_t datum)> address;
  // Resolve literal forwarding pointers across the explicitly owned groups.
  // Nonempty reads require unique actual payload containment.
  std::function<std::span<const std::byte>(std::uint32_t address,
                                         std::size_t extent)> read;
};
class SourceTagMetadataStore {
public:
  SourceTagMetadataStore(std::shared_ptr<const Cache>,
                         std::vector<std::string> groups);
  SourceTagMetadataProviders providers() const;
  // Literal low16 table-slot lookup used by source map owners. The selected
  // resident groups remain explicit; this does not bind a current map or pick
  // a default matg/ugh datum. The callback retains the actual cache backing.
  std::function<std::uint32_t(std::uint16_t)> index_address_provider() const;
  std::size_t tag_count() const;
private:
  struct State;
  std::shared_ptr<const State> state_;
};
} // namespace halo2
