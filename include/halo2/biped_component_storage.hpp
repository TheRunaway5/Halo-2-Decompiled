#pragma once
#include "halo2/raw_datum_pool.hpp"

namespace halo2 {
// Complete1D2620 uses the actual46FBF8 allocator record and method table.
// Its platform owner must provide real requested storage. Pool validity is
// owned by the later map reset; construction does not mark it ready.
void source_initialize_biped_component_storage_1_1(
    const SourceRawDatumPoolCreationServices &,
    const SourceRawDatumPoolObserver & = {});
} // namespace halo2
