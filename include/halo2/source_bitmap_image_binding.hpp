#pragma once
#include "halo2/source_bitmap_manager_transition.hpp"

namespace halo2 {
struct SourceBitmapImageBindingBank {
    SourceBitmapManagerRegion pool,rows,bits;
};
// Complete 16B910/16BA00 over separate addressed banks of the original
// 512-by-40 image pool. Overlapping bank layout is outside this proof.
// EAX is the supplied handle on success, or NONE for invalid low16 index or
// nonzero row salt. The source tests row salt, not occupancy or generation.
// Successful binding zeros the complete row, consumes the pool's generated
// next salt, then overwrites the row salt with the supplied upper16 (including
// zero/FFFF). First-free is unchanged. No ready=true or image I/O is produced.
std::uint32_t source_bitmap_bind_image(SourceBitmapImageBindingBank &,
    std::uint32_t handle);
} // namespace halo2
