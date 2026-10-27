#pragma once
#include "halo2/native_tu15_contact_buffers.hpp"
namespace halo2 {
struct NativeTu15ContactEntryBuffer {std::uint32_t identity{};};
// Actual2F33B0 under its ordinary value-argument ABI: argument/control cells
// do not alias key storage. Keys compare unsigned, indices compare signed;
// address/index arithmetic wraps at32bits. The opaque fourth argument is only
// forwarded to recursive calls by the source, never used for comparisons.
void native_tu15_sort_contact_keys(SourcePhysicsContactMemory&,std::uint32_t keys,
    std::uint32_t first,std::uint32_t last,std::uint32_t forwarded_comparator,
    const SourcePhysicsContactRecordObserver& = {});
// Actual2F3430/2F34D0 and317170. These own12-byte opaque entry backing through
// the genuine stack-block and owned-data services. They initialize only their
// verified header fields and never clear entry contents or emulate FS:[0].
NativeTu15ContactEntryBuffer native_tu15_initialize_contact_entries(
    SourcePhysicsContactMemory&,NativeTu15ContactEntryBuffer,std::uint32_t count,
    NativeTu15PhysicsFrameFp&,const NativeTu15ContactBufferServices&,
    const SourcePhysicsContactRecordObserver& = {});
void native_tu15_destroy_contact_entries(SourcePhysicsContactMemory&,
    NativeTu15ContactEntryBuffer,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactBufferServices&,
    const SourcePhysicsContactRecordObserver& = {});
void native_tu15_release_owned_contact_entries(SourcePhysicsContactMemory&,
    NativeTu15ContactEntryBuffer,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactBufferServices&);
} // namespace halo2
