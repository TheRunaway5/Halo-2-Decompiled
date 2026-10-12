#pragma once
#include "halo2/source_geometry_resource_request.hpp"
#include "halo2/raw_datum_pool.hpp"

namespace halo2 {
struct SourceGeometryRetirementGlobals {
    SourceGeometryResourceGlobals resource;
    std::uint32_t async_mutex,async_head,async_count;
    static constexpr SourceGeometryRetirementGlobals retail() {
        return {SourceGeometryResourceGlobals::retail(),0x4e035c,0x4e28ec,0x4e28f0};
    }
    static constexpr SourceGeometryRetirementGlobals map_pack_1_1() {
        return {SourceGeometryResourceGlobals::map_pack_1_1(),0x4e792c,0x4e9ebc,0x4e9ec0};
    }
};
struct SourceGeometryRetirementServices {
    // Original D3D resource busy/wait operations (3FB710/3FAD50). A Vulkan
    // adapter must own actual submitted-resource completion. Missing ownership
    // throws at the reached operation, never supplies an idle result.
    std::function<std::int32_t(std::uint32_t resource)> resource_busy;
    std::function<void(std::uint32_t resource)> wait_resource;
    // Native platform adaptation of actual async queue mutex ownership.
    std::function<void(std::uint32_t handle,std::uint32_t milliseconds)> wait_mutex;
    std::function<void(std::uint32_t handle)> release_mutex;
};
// Whole223D80/223E00 (TU1.1 225E90/225F10) descriptor walks. Only type2
// resources participate. Both direct and indexed resource locations consume
// current published banks; query results use original signed >0 semantics.
bool source_geometry_resource_in_use(std::uint32_t header,std::uint32_t allocation,
    const SourceGeometryResourceMemory &,const SourceGeometryRetirementServices &);
void source_geometry_resource_wait(std::uint32_t header,std::uint32_t allocation,
    const SourceGeometryResourceMemory &,const SourceGeometryRetirementServices &);
// Whole12E210/12E290 (TU1.1 12F980/12FA00) physical-cache callbacks. They share
// actual inflight/physical/header/async banks, including original queue-count
// traversal, completion loop, pointer unpublication and raw-datum retirement.
bool source_geometry_cache_busy(std::uint32_t datum,
    const SourceGeometryRetirementGlobals &,const SourceGeometryResourceMemory &,
    const SourceGeometryRetirementServices &);
void source_geometry_cache_deleted(std::uint32_t datum,
    const SourceGeometryRetirementGlobals &,const SourceGeometryResourceMemory &,
    const SourceGeometryRetirementServices &,const SourceRawDatumPoolObserver & = {});
}
