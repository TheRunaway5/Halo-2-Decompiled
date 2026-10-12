#pragma once
#include "halo2/tags/types.hpp"
#include <cstddef>
#include <vector>

namespace halo2 {
class Bytes;
// Internal cache-format seam shared by Xbox, Vista and Vista shared imports.
std::vector<TagGroupDefinition> read_tag_group_definitions(
    const Bytes&, std::size_t index, std::int64_t header_magic);
}
