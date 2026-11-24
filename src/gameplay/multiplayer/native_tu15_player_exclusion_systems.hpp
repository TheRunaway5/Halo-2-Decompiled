#pragma once
// Native TU1.5 player exclusion, entered at 1617A0 (physics live support
// unit): each player's waiting bit kept in step with the unit cap.
//
//   player_exclusion::update   1617A0()
//
// Outside game kind 4 (byte +0xC of [4F3218]), every player of the players
// array (4F54F4, walked by the filtered iterator 1A8E50 through an iterator
// on the stack: the record +0, the array +4, the handle +8, the index +0xC)
// is excluded when the unit cap keeps it out (1601B0) or 160360 says so
// (EAX the handle for both). A player whose waiting bit (0x4000 of its flags
// word +2) differs takes it; then, with a variant record for the session
// ([56DFA0] by [4F63B8] +0xC14), a network slot for the player (the session's
// +0x2C by the handle's index, not NONE) and the network in mode 4 ([4DC04C]
// +8), a slot whose flags (byte +0x44 of its 8-byte entry in the table +0x20A4
// of the network's state +4) carry bit 2 has its 36-byte record (+0x2098 +
// 0x14 of the state) marked: bit 0x400 of its +0xC.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the iterator in the original's frame;
// the boundary adapter (native_tu15_player_exclusion.hpp) maps the role to
// the original instruction and owns the ABI of the children (1A8E50, 1601B0,
// 160360). The values the original keeps in callee-saved registers across
// them are a Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_player_exclusion {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t game=0x4f3218u,players=0x4f54f4u,session=0x4f63b8u,variants=0x56dfa0u,network=0x4dc04cu;}
namespace game {inline constexpr std::uint32_t kind=0xcu;inline constexpr std::uint32_t skipped_kind=4u;}
namespace player {inline constexpr std::uint32_t flags=2u;inline constexpr std::uint32_t waiting=0x4000u,waiting_shift=14u;}
namespace session {inline constexpr std::uint32_t variant=0xc14u,slots=0x2cu;}
namespace network {inline constexpr std::uint32_t state=4u,mode=8u;inline constexpr std::uint32_t marking_mode=4u;}
namespace state {inline constexpr std::uint32_t records=0x2098u,slot_table=0x20a4u;}
inline constexpr std::uint32_t slot_mask=0x3ffu,slot_entry=8u,slot_flags_offset=0x44u,slot_replicated=4u,record_size=36u,record_first=0x14u,
    record_flags=0xcu,record_dirty=0x400u;
// The iterator's fields, by offset from the return address.
namespace local {inline constexpr std::int32_t record=-0x10,array=-0xc,handle=-8,index=-4;}

namespace player_exclusion {
enum class Access {game_address,game_kind,players_array,array_store,index_store,handle_store,session_address,handle_load,record_load,
    flags_load,flags_store,variant_index,variant_entry,slot_load,network_address,network_mode,network_state,slot_table,slot_flags,
    record_flags_load,record_flags_store};
enum class Outcome {skipped,no_players,walked};
// The original's BL (excluded), EBP (the session) and EDI (the handle) across the children.
struct Kept {std::uint32_t excluded,session,handle;};
// Frame:
//   local(offset)                          the frame's slots now
//   kind_tested(kind), iterating()         SUB ESP,10; CMP CL,4; OR EAX,-1; PUSH ESI; LEA ESI
//   next(kept) -> AL                       1A8E50(ESI = the iterator), refreshes kept
//   saving(), saved(), restoring()         EBX, EBP and EDI around the walk
//   testing(kept), over_cap(kept), waiting(kept)   EAX = EDI, XOR BL,BL; 1601B0 and 160360 (EAX the handle), refresh kept
//   excluded(kept)                         MOV BL,1
//   compared(flags, was, kept), flagged(kept, flags)   the waiting bit's test and its OR or AND
//   variant_tested(record), slot_tested(slot), mode_tested(mode), slot_indexed(slot), marked(value)
template<class Memory,class Frame> Outcome update(Memory &memory,Frame &frame) {
    using enum Access;
    const auto game_value=memory.load32(game_address,globals::game);
    const auto kind=memory.load8(game_kind,at(game_value,game::kind));
    frame.kind_tested(kind);
    if(kind==game::skipped_kind)return Outcome::skipped;
    const auto array=memory.load32(players_array,globals::players);
    frame.iterating();
    memory.store32(array_store,frame.local(local::array),array);
    memory.store32(index_store,frame.local(local::index),none);
    memory.store32(handle_store,frame.local(local::handle),none);
    Kept kept{};
    if(!(frame.next(kept)&0xffu))return Outcome::no_players;
    frame.saving();
    kept.session=memory.load32(session_address,globals::session);
    frame.saved();
    do {
        kept.handle=memory.load32(handle_load,frame.local(local::handle));
        kept.excluded=0u;
        frame.testing(kept);
        if(frame.over_cap(kept)&0xffu)kept.excluded=1u;
        else if(frame.waiting(kept)&0xffu)kept.excluded=1u;
        if(kept.excluded)frame.excluded(kept);
        const auto record=memory.load32(record_load,frame.local(local::record));
        const auto flags=memory.load16(flags_load,at(record,player::flags));
        const auto was=(flags>>player::waiting_shift)&1u;
        frame.compared(flags,was,kept);
        if((kept.excluded&0xffu)==was)continue;
        const auto now=(kept.excluded&0xffu)?flags|player::waiting:flags&(0xffffu&~player::waiting);
        frame.flagged(kept,now);
        memory.store16(flags_store,at(record,player::flags),now);
        const auto variant=memory.load32(variant_index,at(kept.session,session::variant));
        const auto variant_record=memory.load32(variant_entry,globals::variants+4u*variant);
        frame.variant_tested(variant_record);
        if(!variant_record)continue;
        const auto slot=memory.load32(slot_load,kept.session+session::slots+4u*widen16(kept.handle&0xffffu));
        frame.slot_tested(slot);
        if(slot==none)continue;
        const auto network_value=memory.load32(network_address,globals::network);
        const auto mode=memory.load32(network_mode,at(network_value,network::mode));
        frame.mode_tested(mode);
        if(mode!=network::marking_mode)continue;
        const auto state_value=memory.load32(network_state,at(network_value,network::state));
        const auto table=memory.load32(slot_table,at(state_value,state::slot_table));
        frame.slot_indexed(state_value,slot);
        const auto entry=slot&slot_mask;
        if(!(memory.load8(slot_flags,table+slot_entry*entry+slot_flags_offset)&slot_replicated))continue;
        const auto marked=state_value+state::records+record_first+record_size*entry+record_flags;
        const auto value=memory.load32(record_flags_load,marked)|record_dirty;
        frame.marked(value);
        memory.store32(record_flags_store,marked,value);
    } while(frame.next(kept)&0xffu);
    frame.restoring();
    return Outcome::walked;
}
} // namespace player_exclusion
} // namespace halo2::native_player_exclusion
