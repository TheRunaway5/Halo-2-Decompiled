#pragma once
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Complete original 1303C0/12EBE0 CPU bodies already belong to the shared
// physics_live_support_tu15 lowering. These declarations supply only their
// authenticated cold bindings and literal names; they construct no pool.
void mount_native_tu15_renderer_cache_pools_data(SourceRuntimeMemory&);

struct NativeTu15RendererCachePoolViews {
    NativeRuntimeAddress geometry{}, predicted_geometry{}, geometry_cache{};
    NativeRuntimeAddress texture{}, predicted_texture{}, texture_cache{};
};
// The six complete constructor allocations remain owned by the shared native
// pool allocator. Borrowed wide views must be rebound after bank replacement.
// Retained upload/destroy/lookup callback identities are not executed here.
NativeTu15RendererCachePoolViews native_tu15_renderer_cache_pool_views(const SourceRuntimeMemory&);
}
