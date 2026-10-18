#pragma once
#include <cstdint>

namespace halo2 {
// A verified descriptor in one owned, normalized tag allocation. offset names
// its 32-bit serialized address word, and bytes is the complete referenced
// extent. Resource file offsets, tag handles and pre-bind shape selectors are
// deliberately excluded: none is a metadata address to be relocated.
struct MccNormalizedPointerField {
    std::uint32_t offset{}, address{}, bytes{};
};
}
