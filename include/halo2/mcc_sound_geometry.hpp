#pragma once
#include "halo2/tags/types.hpp"
#include <vector>

namespace halo2 {
class Cache;
// English extra-info geometry is stored in sounds_en.dat. Native consumers
// retain their original geometry-cache publisher and 512-byte read granularity.
// Only the runtime copy receives a unique, owned file token; source MCC metadata
// and every declared payload byte remain unchanged.
struct MccSoundGeometryResource {
    TagHandle gestalt;
    std::uint32_t index{}, source_descriptor{}, source_file_offset{};
    std::uint32_t encoded_offset{}, declared_bytes{};
    std::vector<std::byte> sector;
};
std::vector<MccSoundGeometryResource> plan_mcc_sound_geometry_resources(const Cache&);
}
