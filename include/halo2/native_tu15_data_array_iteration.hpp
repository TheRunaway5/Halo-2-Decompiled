#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// 16EA60: find the next occupied actual slot from a signed starting index.
// Failure is FFFFFFFF; no salt, row or admission result is supplied.
std::uint32_t native_tu15_data_array_next(const SourcePhysicsContactMemory &,
    std::uint32_t pool,std::int32_t first);
// C1E90: the signed bit index uses arithmetic word indexing and low-five-bit
// shift selection. There is no bounds clamp or game-engine policy default.
bool native_tu15_bit_array_test(const SourcePhysicsContactMemory &,
    std::uint32_t words,std::int32_t index);
struct NativeTu15DataArrayIterator {
    std::uint32_t current,pool,datum,cursor;
};
// 6CA40 raw retained iterator preserves header/row/iterator aliases.
bool native_tu15_data_array_advance(SourcePhysicsContactMemory &,
    std::uint32_t iterator,const SourcePhysicsContactRecordObserver & = {});
// For the real callers whose iterator is a private native local, disjoint
// from retained engine storage. No source-stack record is reconstructed.
bool native_tu15_data_array_advance(SourcePhysicsContactMemory &,
    NativeTu15DataArrayIterator &);
}
