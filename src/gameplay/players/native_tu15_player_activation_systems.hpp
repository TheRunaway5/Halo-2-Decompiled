#pragma once
// Native TU1.5 player activation, entered at 14FDC0 (live support unit):
// each player's active bit kept in step with its machine.
//
//   player_activation::update   14FDC0()
//
// Outside game kind 4 (byte +0xC of [4F3218]), every player in use in the
// players array ([4F54F4]: the in-use bits +0x48 below the first unused
// index +0x38; datums of +0x24 bytes at +0x44, a salt word at +0), in index
// order, whose byte +2 lacks bit 1, is asked whether it is active (69AD0,
// EAX its +0x14). A player whose active bit (bit 0 of its word +2) differs
// from the answer byte takes the answer: set, the session's variant record
// ([56DFA0] by [4F63B8] +0xC14), when there is one, hears of it (its method
// +0x18, ECX the record, the handle pushed); set or cleared, a player with a
// variant record and a network slot (the session's +0x2C by the handle's
// index, not NONE) has the slot published dirty (B7E50, the slot and 0x200
// pushed).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the variant's method; the boundary
// adapter (native_tu15_player_activation.hpp) maps the role to the original
// instruction and owns the ABI of the children. The values the original
// keeps in callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_player_activation {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t game=0x4f3218u,players=0x4f54f4u,session=0x4f63b8u,variants=0x56dfa0u;}
namespace game {inline constexpr std::uint32_t kind=0xcu;inline constexpr std::uint32_t skipped_kind=4u;}
namespace array {inline constexpr std::uint32_t datum_size=0x24u,first_unused=0x38u,data=0x44u,in_use=0x48u;}
namespace player {inline constexpr std::uint32_t flags=2u,machine=0x14u;inline constexpr std::uint32_t active=1u,passed_over=2u;}
namespace session {inline constexpr std::uint32_t variant=0xc14u,slots=0x2cu;}
inline constexpr std::uint32_t joined_method=0x18u,dirty_mask=0x200u;

namespace player_activation {
enum class Access {game_address,game_kind,players_array,first_unused,bitmap,bitmap_word,datum_size,datum_data,salt,flags_byte,
    flags_word,session_address,set_store,clear_store,variant_index,variant_entry,variant_vtable,variant_method,session_again,
    variant_index_again,variant_entry_again,session_slots,slot_load};
enum class Outcome {skipped,walked};
// The original's EBX (the players array), EBP (the last index found), ESI (the player) and EDI (its handle; the
// scan's bit) across the children.
struct Kept {std::uint32_t array,index,player,handle;};
// Frame:
//   kind_tested(kind), saving(), walking(kept), restoring()   CMP byte [EAX+C],4; the saved registers; OR EBP,-1
//   starting(index), limit_compared(index, limit), bit_tested(index, word), advanced(index, limit), found_tested(index)
//   player_found(kept, salt)                the handle's SHL and OR, MOV EBP,EAX
//   skip_tested(byte), compared(flags, answer), activity_tested(answer), flagged(value), variant_tested(record), slot_tested(slot)
//   active(machine, kept) -> AL             69AD0(EAX = machine), refreshes kept
//   handing(kept), joined(record, method, kept)   PUSH EDI (the handle); the variant's method (ECX = record), refreshes kept
//   dirty(slot, kept)                       B7E50(the slot, 0x200), refreshes kept
template<class Memory,class Frame> Outcome update(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    const auto game_value=memory.load32(game_address,globals::game);
    const auto kind=memory.load8(game_kind,at(game_value,game::kind));
    frame.kind_tested(kind);
    if(kind==game::skipped_kind)return Outcome::skipped;
    frame.saving();
    kept.array=memory.load32(players_array,globals::players);
    kept.index=none;
    frame.walking(kept);
    for(;;) {
        auto index=kept.index+1u;
        frame.starting(index);
        if(signed_value(index)<0)break;
        const auto limit=memory.load32(first_unused,at(kept.array,array::first_unused));
        frame.limit_compared(index,limit);
        if(!(signed_value(index)<signed_value(limit)))break;
        const auto bits=memory.load32(bitmap,at(kept.array,array::in_use));
        bool found=false;
        do {
            const auto word=memory.load32(bitmap_word,bits+4u*std::uint32_t(signed_value(index)>>5));
            frame.bit_tested(index,word);
            if(word&(1u<<(index&31u))){found=true;break;}
            ++index;
            frame.advanced(index,limit);
        } while(signed_value(index)<signed_value(limit));
        if(!found)break;
        frame.found_tested(index);
        if(index==none)break;
        const auto size=memory.load32(datum_size,at(kept.array,array::datum_size));
        kept.player=size*index+memory.load32(datum_data,at(kept.array,array::data));
        const auto salt_value=widen16(memory.load16(salt,kept.player));
        kept.handle=(salt_value<<16)|index;
        kept.index=index;
        frame.player_found(kept,salt_value);
        const auto byte=memory.load8(flags_byte,at(kept.player,player::flags));
        frame.skip_tested(byte);
        if(byte&player::passed_over)continue;
        const auto answer=frame.active(at(kept.player,player::machine),kept)&0xffu;
        const auto flags=memory.load16(flags_word,at(kept.player,player::flags));
        frame.compared(flags,answer);
        if((flags&player::active)==answer)continue;
        frame.activity_tested(answer);
        if(answer) {
            const auto session_value=memory.load32(session_address,globals::session);
            const auto now=flags|player::active;
            frame.flagged(now);
            memory.store16(set_store,at(kept.player,player::flags),now);
            const auto variant=memory.load32(variant_index,at(session_value,session::variant));
            const auto record=memory.load32(variant_entry,globals::variants+4u*variant);
            frame.variant_tested(record);
            if(record) {
                const auto vtable=memory.load32(variant_vtable,record);
                frame.handing(kept);
                frame.joined(record,memory.load32(variant_method,at(vtable,joined_method)),kept);
            }
        } else {
            const auto now=flags&(0xffffu&~player::active);
            frame.flagged(now);
            memory.store16(clear_store,at(kept.player,player::flags),now);
        }
        const auto variant=memory.load32(variant_index_again,at(memory.load32(session_again,globals::session),session::variant));
        const auto record=memory.load32(variant_entry_again,globals::variants+4u*variant);
        frame.variant_tested(record);
        if(!record)continue;
        const auto slots=memory.load32(session_slots,globals::session);
        const auto slot=memory.load32(slot_load,slots+session::slots+4u*widen16(kept.handle&0xffffu));
        frame.slot_tested(slot);
        if(slot==none)continue;
        frame.dirty(slot,kept);
    }
    frame.restoring();
    return Outcome::walked;
}
} // namespace player_activation
} // namespace halo2::native_player_activation
