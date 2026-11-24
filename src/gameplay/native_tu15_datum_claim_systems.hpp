#pragma once
// Native TU1.5 datum claim, entered at 16E7F0 (UI script startup and campaign
// runtime units): a data array's datum claimed at a given index.
//
//   datum_claim::claim   16E7F0(EDX = array, EDI = index) -> EAX = handle
//
// An index within the array's limit (+0x20) whose datum (the data +0x44,
// datums of +0x24 bytes) is free (its salt word zero) is claimed: its bit is
// set in the array's bitmap (+0x48), the count (+0x3C) grows, the high mark
// (+0x38) passes the index, and 16E860 sets the datum up. The handle is the
// datum's salt over the index; NONE where the index cannot be claimed.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_datum_claim.hpp) maps the role to the original instruction
// and owns the ABI of the child; the datum and the index, which the original
// keeps in ESI and EDI across it, are a Kept, refreshed from the registers
// it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_datum_claim {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace array {inline constexpr std::uint32_t limit=0x20u,datum_size=0x24u,high=0x38u,count=0x3cu,data=0x44u,bitmap=0x48u;}

namespace datum_claim {
enum class Access {limit,data,datum_size,salt,bitmap,bits,bits_store,count,high,count_store,high_store,salt_again};
enum class Outcome {out_of_range,taken,claimed};
struct Kept {std::uint32_t datum,index;};   // ESI, EDI
// Frame: set_up(kept) 16E860, refreshes kept.
template<class Memory,class Frame> std::uint32_t claim(Memory &memory,Frame &frame,std::uint32_t array_address,std::uint32_t index,Outcome &outcome) {
    using enum Access;
    outcome=Outcome::out_of_range;
    if(signed_value(index)<0 || signed_value(index)>=signed_value(memory.load32(Access::limit,at(array_address,array::limit))))return none;
    const auto data_value=memory.load32(data,at(array_address,array::data));
    Kept kept{memory.load32(Access::datum_size,at(array_address,array::datum_size))*index+data_value,index};
    outcome=Outcome::taken;
    if(memory.load16(salt,kept.datum))return none;
    const auto word=memory.load32(Access::bitmap,at(array_address,array::bitmap))+4u*std::uint32_t(signed_value(index)>>5);
    memory.store32(bits_store,word,memory.load32(bits,word)|(1u<<(index&31u)));
    const auto count_value=memory.load32(Access::count,at(array_address,array::count));
    const auto high_value=memory.load32(Access::high,at(array_address,array::high));
    memory.store32(count_store,at(array_address,array::count),count_value+1u);
    if(signed_value(index)>=signed_value(high_value))memory.store32(high_store,at(array_address,array::high),index+1u);
    frame.set_up(kept);
    outcome=Outcome::claimed;
    return (widen16(memory.load16(salt_again,kept.datum))<<16)|kept.index;
}
} // namespace datum_claim
} // namespace halo2::native_datum_claim
