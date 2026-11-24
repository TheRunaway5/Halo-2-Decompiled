#pragma once
// Native TU1.5 object disconnect, entered at BA7E0 (physics live support
// unit): an object taken out of its cluster partition.
//
//   object_disconnect::disconnect   BA7E0(object handle) -> RET 4
//
// C0F60 goes first (the handle, 1, 0, 0, 0). Then the object's partition
// references (+0x60) are removed through 1D30F0 from the partition its
// attachment flags (+4) name: bit 9 picks the one at 4EABB0, otherwise the
// one at 4EABA4. Bit 8 of the attachment flags and bit 6 of the header's
// flags (+2) are cleared.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_object_disconnect.hpp) maps the role to the original
// instruction and owns the ABI of the children; the object, its header and
// its handle the original keeps in ESI, EBX and EBP across them are a Kept,
// refreshed from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_object_disconnect {
using native_object_update::ObjectData;
using native_object_update::ObjectHeader;
using native_object_update::at;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t marked_partition=0x4eabb0u; // the partition for objects with attachment bit 9
inline constexpr std::uint32_t partition=0x4eaba4u;        // the partition for the others
}
inline constexpr std::uint32_t datum_data=0x44u,partition_references=0x60u,partition_bit=9u;
inline constexpr std::uint32_t connected=1u<<8;          // attachment flags
inline constexpr std::uint32_t header_connected=1u<<6;   // header flags

namespace object_disconnect {
enum class Access {headers,header_datums,handle,object_address,attachment,attachment_clear,header_flags,header_store};
struct Kept {std::uint32_t object,header,handle;};   // ESI, EBX, EBP
// Frame: argument() the handle's slot; prepare(kept) C0F60; remove(kept,
// partition, references) 1D30F0. Each refreshes kept.
template<class Memory,class Frame> bool disconnect(Memory &memory,Frame &frame) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto object_handle=memory.load32(handle,frame.argument());
    const auto header=datums+std::uint32_t(sizeof(ObjectHeader))*(object_handle&0xffffu);
    Kept kept{memory.load32(object_address,at(header,offsetof(ObjectHeader,object))),header,object_handle};
    frame.prepare(kept);
    const bool marked=(memory.load32(attachment,at(kept.object,offsetof(ObjectData,attachment_flags)))>>partition_bit)&1u;
    frame.remove(kept,marked?globals::marked_partition:globals::partition,at(kept.object,partition_references));
    const auto flags_address=at(kept.object,offsetof(ObjectData,attachment_flags));
    memory.store32(attachment_clear,flags_address,memory.load32(attachment_clear,flags_address)&~connected);
    const auto header_flags_address=at(kept.header,offsetof(ObjectHeader,flags));
    memory.store8(header_store,header_flags_address,memory.load8(header_flags,header_flags_address)&~header_connected&0xffu);
    return marked;
}
} // namespace object_disconnect
} // namespace halo2::native_object_disconnect
