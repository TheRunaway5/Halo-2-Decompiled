#pragma once
// Native TU1.5 unit countdowns, entered at C8C20 (physics live support
// unit): three countdowns of a unit ticked once.
//
//   unit_countdowns::tick   C8C20(EAX = unit handle) -> AL
//
// Two word countdowns tick while positive (as signed words). The first
// (+0x2E8) clears the float +0x2E4 when it reaches zero. The second
// (+0x2CA) reaching zero hands its pending word (+0x2C8), float (+0x2CC) and
// handle (+0x2D0) to 1D1F30 (EAX the unit, EDX the handle; the word, the
// float, 0 and 1 pushed) and clears them (0, 0.0, NONE). The byte countdown
// (+0x1F4) ticks while above zero and, reaching zero, calls D2500. The
// result is 1 when the byte countdown ticked, otherwise whether either word
// countdown did.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_unit_countdowns.hpp) maps the role to the original
// instruction and owns the ABI of the children; the unit and its handle the
// original keeps in ESI and EDI across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_unit_countdowns {
using native_object_update::at;
using native_object_update::none;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
namespace unit {
inline constexpr std::uint32_t first=0x2e8u,first_value=0x2e4u;
inline constexpr std::uint32_t second=0x2cau,pending_word=0x2c8u,pending_value=0x2ccu,pending_handle=0x2d0u;
inline constexpr std::uint32_t countdown=0x1f4u;
}
inline constexpr std::uint32_t zero_float=0u;

namespace unit_countdowns {
enum class Access {headers,header_datums,unit_address,first,first_store,first_clear,second,second_store,pending_value,
    pending_handle,pending_word,word_clear,handle_clear,value_clear,countdown,countdown_store};
struct Kept {std::uint32_t unit,handle;};   // ESI, EDI
struct Result {bool counted_down,ticked;};
// Frame: hand_over(kept, word, value, handle) 1D1F30; finish(kept) D2500;
// both refresh kept. counted() after a word countdown ticks.
template<class Memory,class Frame> Result tick(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    Kept kept{memory.load32(unit_address,datums+header_size*(handle&0xffffu)+header_address),handle};
    bool ticked=false;
    const auto first_count=std::int16_t(memory.load16(Access::first,at(kept.unit,unit::first)));
    if(first_count>0) {
        const auto left=std::uint32_t(first_count-1)&0xffffu;
        memory.store16(first_store,at(kept.unit,unit::first),left);
        if(!left)memory.store32(first_clear,at(kept.unit,unit::first_value),zero_float);
        ticked=true;
        frame.counted();
    }
    const auto second_count=std::int16_t(memory.load16(Access::second,at(kept.unit,unit::second)));
    if(second_count>0) {
        const auto left=std::uint32_t(second_count-1)&0xffffu;
        memory.store16(second_store,at(kept.unit,unit::second),left);
        if(!left) {
            const auto value=memory.load32(Access::pending_value,at(kept.unit,unit::pending_value));
            const auto target=memory.load32(Access::pending_handle,at(kept.unit,unit::pending_handle));
            const auto word=memory.load16(Access::pending_word,at(kept.unit,unit::pending_word))&0xffffu;
            frame.hand_over(kept,word,value,target);
            memory.store16(word_clear,at(kept.unit,unit::pending_word),0u);
            memory.store32(handle_clear,at(kept.unit,unit::pending_handle),none);
            memory.store32(value_clear,at(kept.unit,unit::pending_value),zero_float);
        }
        ticked=true;
        frame.counted();
    }
    const auto count=memory.load8(Access::countdown,at(kept.unit,unit::countdown))&0xffu;
    if(!count)return {false,ticked};
    const auto left=(count-1u)&0xffu;
    memory.store8(countdown_store,at(kept.unit,unit::countdown),left);
    if(!left)frame.finish(kept);
    return {true,ticked};
}
} // namespace unit_countdowns
} // namespace halo2::native_unit_countdowns
