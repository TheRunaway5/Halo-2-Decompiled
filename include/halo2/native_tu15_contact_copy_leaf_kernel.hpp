#pragma once
#include "halo2/native_tu15_contact_records.hpp"

namespace halo2::native_contact_copy_leaf_detail {
// Actual303BA0 DATA policy, extracted from the existing typed public copy.
// Each field stays lazy so overlapping records retain authored feedback.
// Bytes28..2F and7E..7F belong to their original destination storage slots.
template<class Storage>NativeTu15ContactRecord copy(Storage&storage,
    NativeTu15ContactRecord destination,NativeTu15ContactRecord source){
    using Phase=NativeTu15ContactRecordPublication;
    storage.vector(destination,0,storage.vector(source,0),Phase::copy);
    storage.vector(destination,0x10,storage.vector(source,0x10),Phase::copy);
    storage.word(destination,0x20,storage.word(source,0x20),Phase::copy);
    storage.word(destination,0x24,storage.word(source,0x24),Phase::copy);
    for(unsigned field:{0x30u,0x40u,0x50u,0x60u})
        storage.vector(destination,field,storage.vector(source,field),Phase::copy);
    for(unsigned field:{0x70u,0x74u,0x78u})
        storage.word(destination,field,storage.word(source,field),Phase::copy);
    storage.byte(destination,0x7c,storage.byte(source,0x7c),Phase::copy);
    storage.byte(destination,0x7d,storage.byte(source,0x7d),Phase::copy);
    return destination;
}
} // namespace halo2::native_contact_copy_leaf_detail
