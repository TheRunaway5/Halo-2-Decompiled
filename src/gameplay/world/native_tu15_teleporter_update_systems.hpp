#pragma once
// Native TU1.5 teleporter update, entered at 1A6830 (teleporter unit): the
// teleporters' tick over the players.
//
//   teleporter_update::update   1A6830()
//
// Every player of the players array (4F54F4, walked by the linked iterator
// 1A8F10 through an iterator on the stack: the record +0, the array +4, the
// handle +8, the index +0xC) has its inside bit (0x80 of its word +2)
// cleared. Each of the session's teleporters ([4F63B8]: the count +0xBFC,
// 16-byte entries from +0xAFC) then marks the players inside it (1A6930, EAX
// the entry). Every player not inside one has its teleported bit (0x100)
// cleared. Finally each teleporter moves its players (1A6A40, the entry
// pushed), the session read again after each.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the iterator in the original's frame;
// the boundary adapter (native_tu15_teleporter_update.hpp) maps the role to
// the original instruction and owns the ABI of the children. The values the
// original keeps in callee-saved registers across them are a Kept, refreshed
// from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_teleporter_update {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
namespace globals {inline constexpr std::uint32_t players=0x4f54f4u,session=0x4f63b8u;}
namespace player {inline constexpr std::uint32_t flags=2u;inline constexpr std::uint32_t inside=0x80u,teleported=0x100u;}
namespace session {inline constexpr std::uint32_t teleporters=0xafcu,count=0xbfcu;}
inline constexpr std::uint32_t teleporter_size=16u;
// The iterator's fields, by offset from the return address.
namespace local {inline constexpr std::int32_t record=-0x10,array=-0xc,handle=-8,index=-4;}

namespace teleporter_update {
enum class Access {players_array,array_store,index_store,handle_store,record_load,inside_load,inside_store,session_address,count_load,
    count_again,array_again,index_again,handle_again,record_again,inside_test,teleported_load,teleported_store,count_final,
    session_again,count_last};
enum class Outcome {updated};
// The original's callee-saved registers across the children, each holding by turns:
struct Kept {
    std::uint32_t entry;    // EBX: NONE for the iterator, a teleporter (its address, then its offset), the inside mask (BL)
    std::uint32_t array;    // EBP: the players array, then the teleported clear mask (BP)
    std::uint32_t index;    // ESI: the teleporter's index
    std::uint32_t held;     // EDI: the inside clear mask (DI), then the session
};
// Frame:
//   local(offset)                         the iterator's fields now
//   prepared(kept), masking(kept), clearing(kept)   the registers set before each walk (and ESI, EDI saved first)
//   next(kept) -> AL                      1A8F10(ESI = the iterator), refreshes kept
//   masked(value)                         the word AND's flags
//   counting(count), advanced(kept, count), reset(kept), offsets(kept)   the teleporter loops' registers and flags
//   iterating()                           the second walk's iterator (LEA ESI)
//   touch(kept), teleport(entry, kept)    1A6930(EAX = the entry) and 1A6A40(the entry pushed), refresh kept
//   restoring()                           EDI, ESI, EBP and EBX restored
template<class Memory,class Frame> Outcome update(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    kept.array=memory.load32(players_array,globals::players);
    kept.entry=none;
    frame.prepared(kept);
    memory.store32(array_store,frame.local(local::array),kept.array);
    memory.store32(index_store,frame.local(local::index),kept.entry);
    memory.store32(handle_store,frame.local(local::handle),kept.entry);
    if(frame.next(kept)&0xffu) {
        kept.held=0xffffu&~player::inside;
        frame.masking(kept);
        do {
            const auto record=memory.load32(record_load,frame.local(local::record));
            const auto value=memory.load16(inside_load,at(record,player::flags))&kept.held&0xffffu;
            frame.masked(value);
            memory.store16(inside_store,at(record,player::flags),value);
        } while(frame.next(kept)&0xffu);
    }
    kept.held=memory.load32(session_address,globals::session);
    auto count=memory.load32(count_load,at(kept.held,session::count));
    kept.index=0u;
    frame.counting(count);
    if(signed_value(count)>0) {
        kept.entry=at(kept.held,session::teleporters);
        do {
            frame.touch(kept);
            count=memory.load32(count_again,at(kept.held,session::count));
            ++kept.index;kept.entry+=teleporter_size;
            frame.advanced(kept,count);
        } while(signed_value(kept.index)<signed_value(count));
        kept.entry=none;
        frame.reset(kept);
    }
    frame.iterating();
    memory.store32(array_again,frame.local(local::array),kept.array);
    memory.store32(index_again,frame.local(local::index),kept.entry);
    memory.store32(handle_again,frame.local(local::handle),kept.entry);
    if(frame.next(kept)&0xffu) {
        kept.entry=(kept.entry&0xffffff00u)|player::inside;
        kept.array=0xffffu&~player::teleported;
        frame.clearing(kept);
        do {
            const auto record=memory.load32(record_again,frame.local(local::record));
            if(!(memory.load8(inside_test,at(record,player::flags))&kept.entry&0xffu)) {
                const auto value=memory.load16(teleported_load,at(record,player::flags))&kept.array&0xffffu;
                frame.masked(value);
                memory.store16(teleported_store,at(record,player::flags),value);
            }
        } while(frame.next(kept)&0xffu);
    }
    count=memory.load32(count_final,at(kept.held,session::count));
    kept.index=0u;
    frame.counting(count);
    if(signed_value(count)>0) {
        kept.entry=0u;
        frame.offsets(kept);
        do {
            frame.teleport(kept.entry+kept.held+session::teleporters,kept);
            kept.held=memory.load32(session_again,globals::session);
            count=memory.load32(count_last,at(kept.held,session::count));
            ++kept.index;kept.entry+=teleporter_size;
            frame.advanced(kept,count);
        } while(signed_value(kept.index)<signed_value(count));
    }
    frame.restoring();
    return Outcome::updated;
}
} // namespace teleporter_update
} // namespace halo2::native_teleporter_update
