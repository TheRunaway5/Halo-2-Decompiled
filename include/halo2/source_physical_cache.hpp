#pragma once
#include "halo2/raw_datum_pool.hpp"
namespace halo2 {
struct SourcePhysicalCacheServices {
    SourceRawDatumPoolServices storage;
    // Actual original allocator+24/+20 callback identities. Reached callbacks
    // must consume this same current bank and own their real mutations; they
    // are neither default-evictable slots nor supplied cache decisions.
    std::function<bool(std::uint32_t callback,std::uint32_t datum)> busy;
    std::function<void(std::uint32_t callback,std::uint32_t datum)> deleted;
};
// Whole retail13D370 / TU1.1 13EAE0, including13EA90 window selection,
//13EFA0 ordered eviction and native raw-pool allocation/retirement. The
// constructed physical allocator, page/bitmap/list banks and actual callback
// owners must exist. This does not construct backing or submit/complete IO.
// Observer instruction identities use the authenticated TU1.1 correspondence.
std::uint32_t source_physical_cache_allocate(std::uint32_t allocator,
    std::uint32_t bytes,std::int32_t threshold,const SourcePhysicalCacheServices &,
    const SourceRawDatumPoolObserver & = {});
void source_physical_cache_release(std::uint32_t allocator,std::uint32_t datum,
    const SourcePhysicalCacheServices &,const SourceRawDatumPoolObserver & = {});
}
