#pragma once
// Native TU1.5 item notice, entered at 1D9F10 (death cycle and physics live
// support units): each of a record's items noticed.
//
//   item_notice::notify   1D9F10(ESI = record)
//
// A record whose flags (+4) carry bit 5 hands each of its items (the
// count +0x74, read again after each; the 0x60-byte items at +0x70, each
// one's +0x40) to 315CB0 (ECX the item).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_item_notice.hpp) maps the role to the original instruction
// and owns the ABI of the child; the record, the index and the item's
// offset, which the original keeps in ESI, EDI and EBX across it, are a
// Kept, refreshed from the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_item_notice {
using native_object_update::at;
using native_object_update::signed_value;
namespace record {inline constexpr std::uint32_t flags=4u,items=0x70u,count=0x74u;}
inline constexpr std::uint32_t noticed=1u<<5,item_size=0x60u,item_target=0x40u;

namespace item_notice {
enum class Access {flags,count,items,item,count_again};
enum class Outcome {quiet,empty,noticed};
struct Kept {std::uint32_t record,index,offset;};   // ESI, EDI, EBX
// Frame: each(kept, item) 315CB0(ECX = item), refreshes kept.
template<class Memory,class Frame> Outcome notify(Memory &memory,Frame &frame,std::uint32_t record_address) {
    using enum Access;
    if(!(memory.load32(Access::flags,at(record_address,record::flags))&noticed))return Outcome::quiet;
    auto count_value=memory.load32(count,at(record_address,record::count));
    if(signed_value(count_value)<=0)return Outcome::empty;
    Kept kept{record_address,0u,0u};
    do {
        const auto item_value=memory.load32(item,memory.load32(items,at(kept.record,record::items))+kept.offset+item_target);
        frame.each(kept,item_value);
        count_value=memory.load32(count_again,at(kept.record,record::count));
        ++kept.index;kept.offset+=item_size;
    } while(signed_value(kept.index)<signed_value(count_value));
    return Outcome::noticed;
}
} // namespace item_notice
} // namespace halo2::native_item_notice
