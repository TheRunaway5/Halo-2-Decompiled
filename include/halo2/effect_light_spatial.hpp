#pragma once
#include "halo2/source_object_spatial_pool.hpp"

namespace halo2 {
// Retail1CABC0("light",0): both independent2048-record pools have stride12.
// Object registries carry an additional primitive payload; light lists do not.
struct SourceEffectLightSpatialPool {
    std::vector<std::uint32_t> heads;
    SourceObjectSpatialDatumPool entries,references;
    explicit SourceEffectLightSpatialPool(std::size_t clusters);
    void activate(bool debug=false);
    // Ordered14A5B0/14A550 membership is supplied by the registration owner.
    // Independent allocator failures preserve the source's partial writes.
    void insert(std::uint32_t identity,std::uint32_t& reference_head,
        std::span<const std::int16_t> memberships);
    void erase(std::uint32_t identity,std::uint32_t& reference_head);
    std::vector<std::uint32_t> cluster(std::int16_t) const;
};
}
