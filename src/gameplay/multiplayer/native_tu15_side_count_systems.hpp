#pragma once
// Native TU1.5 side count, entered at 15B4D0 (multiplayer engines unit): how
// many sides a game has, teams or players.
//
//   side_count::count   15B4D0() -> EAX
//
// With a variant record for the session ([56DFA0] by [4F63B8] +0xC14) in a
// team game (bit 0 of byte +0x184 of [4F3218]), the sides are the teams 0
// to 7 whose bit is set in the session's team word (+0xE), read again for
// each team; the original walks them four at a time (two to the left of its
// index, the index, one to the right). Otherwise the sides are the players
// in use in the players array ([4F54F4]: the in-use bits +0x48 below the
// first unused index +0x38), each found from the one after the last; a
// player whose record (index times +0x24, plus +0x44, kept in the frame) is
// at address 0 ends the count.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_side_count.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_side_count {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
namespace globals {inline constexpr std::uint32_t session=0x4f63b8u,variants=0x56dfa0u,game=0x4f3218u,players=0x4f54f4u;}
namespace session {inline constexpr std::uint32_t teams=0xeu,variant=0xc14u;}
namespace game {inline constexpr std::uint32_t mode_flags=0x184u;inline constexpr std::uint32_t team_game=1u;}
namespace array {inline constexpr std::uint32_t datum_size=0x24u,first_unused=0x38u,data=0x44u,in_use=0x48u;}
inline constexpr std::uint32_t team_count=8u,group=4u;
// The frame's slots, by offset from the return address.
namespace local {inline constexpr std::int32_t team_game=-5,record=-4;}

namespace side_count {
enum class Access {session_address,variant_index,variant_entry,game_address,mode_flags,team_game_store,team_word,players_array,
    first_unused,bitmap,bitmap_word,datum_size,datum_data,record_store};
enum class Outcome {teams,players,scanned,empty_record};
// Frame:
//   local(offset)                         the frame's slots now
//   cleared(record), team_tested(flag)    XOR EAX,EAX; TEST EDX,EDX; PUSH EDI; AND BL,1
//   team(slot, team, inside, word, set, count)   one team's range test, bit test and count in the original's registers
//   grouped(index)                        ADD EDX,4; LEA ECX,[EDX-2]; CMP ECX,8
//   walking(), starting(index), limit_compared(index, limit), bit_tested(index, word), advanced(index, limit)
//   found_tested(index), record(datum), counted(count)     the players' walk
template<class Memory,class Frame> Outcome count(Memory &memory,Frame &frame,std::uint32_t &sides) {
    using enum Access;
    sides=0u;
    const auto session_value=memory.load32(session_address,globals::session);
    const auto variant=memory.load32(variant_index,at(session_value,session::variant));
    const auto record_value=memory.load32(variant_entry,globals::variants+4u*variant);
    frame.cleared(record_value);
    if(record_value) {
        const auto game_value=memory.load32(game_address,globals::game);
        const auto teams_flag=memory.load8(mode_flags,at(game_value,game::mode_flags))&game::team_game;
        frame.team_tested(teams_flag);
        memory.store8(team_game_store,frame.local(local::team_game),teams_flag);
        if(teams_flag) {
            for(std::uint32_t index=2u;signed_value(index-2u)<signed_value(team_count);index+=group) {
                for(std::uint32_t slot=0;slot<group;++slot) {
                    const auto team=index-2u+slot;
                    const bool inside=signed_value(team)>=0 && signed_value(team)<signed_value(team_count);
                    std::uint32_t word=0u;
                    bool set=false;
                    if(inside) {
                        word=memory.load16(team_word,at(session_value,session::teams));
                        set=(word&(1u<<(team&31u)))!=0u;
                        if(set)++sides;
                    }
                    frame.team(slot,team,inside,word,set,sides);
                }
                frame.grouped(index+group);
            }
            return Outcome::teams;
        }
    }
    // The players in use.
    const auto players=memory.load32(players_array,globals::players);
    frame.walking();
    std::uint32_t index=none;
    for(;;) {
        ++index;
        frame.starting(index);
        if(signed_value(index)<0)return Outcome::players;
        const auto limit=memory.load32(first_unused,at(players,array::first_unused));
        frame.limit_compared(index,limit);
        if(!(signed_value(index)<signed_value(limit)))return Outcome::players;
        const auto bits=memory.load32(bitmap,at(players,array::in_use));
        bool found=false;
        do {
            const auto word=memory.load32(bitmap_word,bits+4u*std::uint32_t(signed_value(index)>>5));
            frame.bit_tested(index,word);
            if(word&(1u<<(index&31u))){found=true;break;}
            ++index;
            frame.advanced(index,limit);
        } while(signed_value(index)<signed_value(limit));
        if(!found)return Outcome::scanned;
        frame.found_tested(index);
        if(index==none)return Outcome::players;
        const auto size=memory.load32(datum_size,at(players,array::datum_size));
        const auto datum=size*index+memory.load32(datum_data,at(players,array::data));
        frame.record(datum);
        memory.store32(record_store,frame.local(local::record),datum);
        if(!datum)return Outcome::empty_record;
        ++sides;
        frame.counted(sides);
    }
}
} // namespace side_count
} // namespace halo2::native_side_count
