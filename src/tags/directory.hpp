#pragma once
#include "halo2/tags/types.hpp"
#include <cstddef>
#include <utility>
#include <vector>

namespace halo2 {
class Bytes;
// Internal cache-format seam shared by Xbox, Vista and MCC imports.
std::vector<TagGroupDefinition> read_tag_group_definitions(
    const Bytes&, std::size_t index, std::int64_t header_magic);
enum class SharedTagLoadPolicy { unloaded_only, unloaded_or_empty };
// Preserve salted slots and serialized zero-size aliases. The returned flags
// identify which source owns each imported address, including empty locations.
std::vector<bool> merge_shared_tag_directory(std::vector<Tag>&,
    std::span<const Tag>, SharedTagLoadPolicy);
// Checked payload allocations and uncovered zero-length tag locations. Empty
// locations must not shadow a containing payload or acquire invented bytes.
// Retained allocations keep forwarding pointers valid after a tag is moved.
std::vector<std::pair<std::uint32_t,std::uint32_t>> tag_metadata_regions(
    std::span<const Tag>,
    std::span<const std::pair<std::uint32_t,std::uint32_t>> retained = {});
}
