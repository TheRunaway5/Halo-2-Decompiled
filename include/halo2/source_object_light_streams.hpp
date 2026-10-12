#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
// Resident geometry+3C descriptors in literal order. `data` begins at the
// address in descriptor+C. Both D31C0 and D33A0 deliberately ignore +8.
struct SourceObjectLightDataStream {
    std::uint8_t declaration{};
    std::uint32_t descriptor_offset{};
    std::vector<std::byte> data;
};
struct SourceObjectLightDataGeometry {
    std::vector<SourceObjectLightDataStream> streams;
    std::vector<std::uint16_t> indices; //geometry+24, original triangle*3.
};
//Complete D33A0/D31C0 CPU services. Scan starts at descriptor3 and uses the
//last descriptor if no requested declaration is present. A bank with at most
//three descriptors returns NONE without reading its indices or stream data.
//No clamp/normalize/pose transform is applied. Incident length is preserved.
std::optional<Vec3> source_object_light_radiance(const SourceObjectLightDataGeometry&,
    std::int32_t lightmap_triangle,float weight_b,float weight_c);
std::optional<Vec3> source_object_light_incident(const SourceObjectLightDataGeometry&,
    std::int32_t lightmap_triangle,float weight_b,float weight_c);
} // namespace halo2
