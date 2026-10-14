#pragma once
#include "halo2/source_draw_resources.hpp"

namespace halo2 {
struct SourceModelResourceWorld {
    SourceDrawResourceWorld banks;
    // Must expose the same current banks as read(), including aliased active,
    // cached and selected region bytes and resource/cache publication writes.
    std::function<std::span<std::byte>(std::uint32_t,std::size_t)> write;
    std::int32_t view{}; // Actual4BA050.
    std::uint32_t render_cache_pool{}; // Actual509434, including record/bit banks.
    std::uint32_t render_frame{}; // Actual4BA034.
    // Optional observation AFTER native acquisition. Supplies no state/results.
    std::function<void(std::uint32_t,std::uint32_t)> cache_acquired;
    // Caller-owned16-byte temporary bank consumed by the first-person branch.
    std::uint32_t temporary_regions{};
    std::uint32_t context_callback=0xbe8b0u;
};
// Whole3D4F0: resolve normal object model/nodes or a current cached first-person
// entry (strideD0C). Outputs are bank addresses, preserving original write order
// and aliasing. cached_models is the actual4C1BD8 owner bank, not a tag default.
void source_model_build_inputs(std::uint32_t cached,std::uint32_t cache_index,
    std::uint32_t flags,std::uint32_t source_owner,std::uint32_t model,
    std::uint32_t transforms,std::uint32_t count,std::uint32_t owner,
    std::uint32_t cached_models,const SourceModelResourceWorld&);
// Whole137650. Counts actual selected-section node-map bytes as well as the
// model/LOD nodes. This returns wrapped32-bit source arithmetic, not a count
// inferred from vertices, cached node extent or a produced Vulkan palette.
std::int32_t source_model_geometry_node_count(std::uint32_t model,std::int32_t lod,
    std::uint32_t selected,const SourceModelResourceWorld&);
// Complete4C2B0, including all initial requests before ordered cached-region
// fallback,16-bit success-mask truncation, byte sign extension and preserved
// selected bytes on failure. All addresses belong to current owned banks.
bool source_model_resource_sections(std::uint32_t model,std::uint32_t active,
    std::uint32_t cached,std::int32_t lod,std::uint32_t request,
    std::uint32_t selected,std::uint32_t changed,const SourceModelResourceWorld&);
// Whole3DDD0 over current object and shared pool banks. Matching-owner lookup
// precedes region/RGB access. Allocation/rebind joins SourceRenderCachePool;
// all resulting records, occupancy, counters, salt and object+CC stay owned
// by the caller. Unsupported region extents are rejected before memory copies.
std::uint32_t source_model_acquire_cache(std::uint32_t owner,const SourceModelResourceWorld&);
// Whole3DD10/BAD50 current-color refresh, including the force low byte and
// definition+1C flag gate. It consumes the existing object+CC cache record;
// no new cache is acquired and channels beyond the current count survive.
void source_model_refresh_colors(std::uint32_t owner,std::uint32_t force,
    const SourceModelResourceWorld&);
// Complete3D430 and4C640 around native3DDD0.
std::uint32_t source_model_cached_regions(std::uint32_t owner,const SourceModelResourceWorld&);
// The4C450/4B5D0 per-view prior-LOD acquisition and byte read.
std::int8_t source_model_prior_lod(std::uint32_t owner,const SourceModelResourceWorld&);
void source_model_cache_lod(std::uint32_t owner,std::uint32_t lod,const SourceModelResourceWorld&);
// Complete4C190 joined to native4C2B0/3D430/4C640. Region count is a32-bit
// output; current LOD is a16-bit in/out field. Prior LOD and first-person flags
// retain original low-byte semantics. Platform12DE70 publication remains an
// explicit service. This module does not create the live object/pool owners.
bool source_model_resources(std::uint32_t owner,std::uint32_t first_person,
    std::uint32_t model,std::uint32_t region_count,std::uint32_t lod,
    std::uint32_t prior_lod,std::uint32_t selected,std::uint32_t changed,
    const SourceModelResourceWorld&);
}
