#pragma once
#include "halo2/source_geometry_resource_request.hpp"
#include "halo2/source_bitmap_async_read.hpp"

namespace halo2 {
struct SourceGeometryResourceAcquireServices {
    // Complete13EAE0 physical admission on the genuinely constructed cache.
    // This owner must preserve eviction/callbacks and failure, not select a
    // convenient handle. The threshold is source1 or5, not the IO priority.
    std::function<std::uint32_t(std::uint32_t physical,std::uint32_t bytes,
        std::int32_t threshold)> physical_allocate;
    // Complete19D8C0/1A4870 request publication, returning its actual job token.
    // File routing, owned buffer and completion-byte backing must share memory.
    std::function<std::uint32_t(const SourceBitmapReadRequest &)> async_read;
};
// Whole retail12E3A0 / TU1.1 12FB10 around the required physical and IO owners.
// Native16D390 binds the actual admitted identity into the inflight pool;
// header, linked-state, job and priority stores retain original ordering.
// A completed allocation is not assumed;12DE70 separately waits/publishes it.
bool source_geometry_resource_acquire(std::uint32_t header,bool blocking,
    bool selective,std::int32_t priority,const SourceGeometryResourceGlobals &,
    const SourceGeometryResourceMemory &,const SourceGeometryResourceAcquireServices &);
}
