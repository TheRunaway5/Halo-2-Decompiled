#pragma once
// Native TU1.5 cluster visibility, entered at 13B610 (physics live support
// unit): the set of clusters seen, for a game kind or the local seats.
//
//   cluster_visibility::gather   13B610(ESI = set*; view) -> RET 8
//
// The set (16 dwords) is cleared (REP STOSD, in the string direction flag's
// direction). Outside game kind 3 ([4F3218]'s +8) the PVS flood (1532A0:
// the set and the view pushed) fills it. In kind 3 the first local seat in use
// (the four seat players at [4F54F0]+0xC, not NONE), when the seats are in
// use (word [474C24] not NONE), names a player (0x358 bytes at 4F655C)
// whose cluster (word +0x10, not NONE; at 0 without a seat) adds its row of
// the structure BSP's ([4ECC18]) cluster bits (+0x58, rows of (count +0x9C
// + 31) / 32 dwords) to the set (140EC0: EAX the row, EDX the set; the set
// pushed). Then, when the game's +0x11F8 and byte [[4F5504]] are set, the
// cluster 16FD40 answers (through the dispatch) adds its row likewise.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_cluster_visibility.hpp) maps the role to the original
// instruction and owns the ABI of the children. The values the original
// keeps in callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_cluster_visibility {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t structure_bsp=0x4ecc18u,game=0x4f3218u,seats=0x4f54f0u,seats_used=0x474c24u,
    players=0x4f655cu,second=0x4f5504u;}
inline constexpr std::uint32_t set_words=0x10u,seat_count=4u,player_size=0x358u,seated_kind=3u;
namespace game {inline constexpr std::uint32_t kind=8u,second=0x11f8u;}
namespace seats {inline constexpr std::uint32_t players=0xcu;}
namespace player {inline constexpr std::uint32_t cluster=0x10u;}
namespace bsp {inline constexpr std::uint32_t bits=0x58u,count=0x9cu;}
namespace argument {inline constexpr std::uint32_t view=0u;}

namespace cluster_visibility {
enum class Access {bsp,clear,game,kind,view,seats,seat,seats_used,cluster,count,bits,second_flag,second_global,second_byte,count_again,
    bits_again};
enum class Outcome {flooded,seated,unseated};
// The original's EBX (the BSP), ESI (the set) and EDI (the game) across the children.
struct Kept {std::uint32_t bsp,set,game;};
// Frame: saving(n), step(), cleared(set), kind_tested(kind), flood(view, kept) [1532A0], seat_walk hooks,
// row(cluster, count, bits, kept) [140EC0], second_cluster(kept) [16FD40]; each names the original instructions.
template<class Memory,class Frame> Outcome gather(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    frame.saving(0u);
    kept.bsp=memory.load32(Access::bsp,globals::structure_bsp);
    frame.saving(1u);
    // The set cleared, as REP STOSD clears it.
    auto word=frame.clearing(kept.set);
    const auto step=frame.step();
    for(std::uint32_t k=0;k<set_words;++k,word+=4u*step)memory.store32(clear,word,0u);
    kept.game=memory.load32(Access::game,globals::game);
    const auto kind_value=memory.load32(kind,at(kept.game,game::kind));
    frame.kind_tested(kind_value);
    auto outcome=Outcome::unseated;
    const auto add_row=[&](std::uint32_t cluster,Access count_role,Access bits_role,unsigned n) {
        const auto count_value=memory.load32(count_role,at(kept.bsp,bsp::count));
        const auto row_words=frame.row_words(widen16(cluster),count_value);
        const auto bits_value=memory.load32(bits_role,at(kept.bsp,bsp::bits));
        frame.row(n,bits_value+4u*row_words,kept);
    };
    if(kind_value!=seated_kind) {
        frame.flood(memory.load32(view,frame.argument(argument::view)),kept);
        outcome=Outcome::flooded;
    } else {
        // The first local seat in use.
        auto slot=memory.load32(seats,globals::seats)+seats::players;
        std::uint32_t seat_index=none;
        frame.seat_walk_start(slot);
        for(std::uint32_t k=0;;) {
            const auto player=memory.load32(seat,slot);
            frame.seat_tested(player);
            if(player!=none){seat_index=k;frame.seat_found(k);break;}
            ++k;slot+=4u;
            frame.seat_next(k,slot);
            if(!(std::int32_t(k)<std::int32_t(seat_count)))break;
        }
        frame.seat_chosen(seat_index);
        std::uint32_t record=0u;
        if(seat_index!=none) {
            const auto used=memory.load16(seats_used,globals::seats_used);
            frame.used_tested(used);
            if(used!=0xffffu)record=frame.player(seat_index);
        }
        const auto cluster_value=memory.load16(cluster,at(record,player::cluster));
        frame.cluster_tested(cluster_value);
        if(cluster_value!=0xffffu){add_row(cluster_value,count,bits,0u);outcome=Outcome::seated;}
    }
    // The second cluster.
    const auto second_on=memory.load8(second_flag,at(kept.game,game::second));
    frame.tested8(second_on);
    if(second_on) {
        const auto second_byte_value=memory.load8(second_byte,memory.load32(second_global,globals::second));
        frame.byte_compared(second_byte_value);
        if(second_byte_value) {
            const auto second_cluster=frame.second_cluster(kept);
            frame.cluster_tested(second_cluster);
            if((second_cluster&0xffffu)!=0xffffu)add_row(second_cluster,count_again,bits_again,1u);
        }
    }
    frame.restoring();
    return outcome;
}
} // namespace cluster_visibility
} // namespace halo2::native_cluster_visibility
