#pragma once
// Native TU1.5 partition unlink, entered at C5250 (physics live support
// unit): a partitioned record taken out of the cluster partition.
//
//   partition_unlink::unlink   C5250(EAX = record handle)
//
// A record (0x110 bytes in [4ECBDC]) marked partitioned (bit 1 of +2) is
// removed through 1D30F0 (the partition [4ECBE0], the handle and the
// record's references at +0x10), and its bit 3 of +2 is cleared.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_partition_unlink.hpp) maps the role to the original
// instruction and owns the ABI of 1D30F0; the record the original keeps in
// ESI across it is a Kept, refreshed from the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_partition_unlink {
using native_object_update::at;
namespace globals {
inline constexpr std::uint32_t records=0x4ecbdcu;     // DataArray* of 0x110-byte records
inline constexpr std::uint32_t partition=0x4ecbe0u;   // the cluster partition
}
inline constexpr std::uint32_t datum_data=0x44u,record_size=0x110u,flags=2u,references=0x10u;
inline constexpr std::uint8_t partitioned=0x02u,cleared_bit=0x08u;

namespace partition_unlink {
enum class Access {table,datums,flags_read,flags_clear};
struct Kept {std::uint32_t record;};   // ESI
// Frame: remove(kept, handle, references) 1D30F0; refreshes kept.
template<class Memory,class Frame> bool unlink(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto records=memory.load32(table,globals::records);
    const auto data=memory.load32(datums,at(records,datum_data));
    Kept kept{record_size*(handle&0xffffu)+data};
    if(!(memory.load8(flags_read,at(kept.record,flags))&partitioned))return false;
    frame.remove(kept,handle,at(kept.record,references));
    memory.store8(flags_clear,at(kept.record,flags),memory.load8(flags_clear,at(kept.record,flags))&~std::uint32_t(cleared_bit));
    return true;
}
} // namespace partition_unlink
} // namespace halo2::native_partition_unlink
