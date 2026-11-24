#pragma once
// Native TU1.5 cluster visibility update, entered at 13B6F0 (live support
// unit): the game's cluster sets recomputed, and their listeners told of a
// change.
//
//   cluster_visibility_update::update   13B6F0()
//
// The game's ([4F3218]) seen set (+0x1138) and reachable set (+0x11B8), 16
// dwords each, are copied to the stack (REP MOVSD, in the string direction
// flag's direction). The seen set and the local set (+0x1178) are gathered
// again (13B610: for the view 0, then for the local seats). 1D1E00 (EBX the
// stack's cluster buffer; the structure BSP's [4ECC18] cluster count, word
// +0x9C) either answers clusters, which 140EC0 adds to the seen set into the
// reachable set, or the reachable set becomes the seen set ((count + 31) /
// 32 dwords). A cluster from 13B590 (not NONE) adds its row of the BSP's
// cluster bits (+0x58) to the reachable set. A set that differs from its
// copy (REPE CMPSD) is announced to each listener (0x44 of 0x24 bytes at
// 44AEEC: the seen set's callback +0, the reachable set's +4, called with
// the copy, the set and the count).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_cluster_visibility_update.hpp) maps the role to the original
// instruction and owns the ABI of the children. The values the original
// keeps in callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_cluster_visibility_update {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t game=0x4f3218u,structure=0x4ecc18u,listeners=0x44aeecu;}
namespace game {inline constexpr std::uint32_t seen=0x1138u,local=0x1178u,reachable=0x11b8u;}
namespace structure {inline constexpr std::uint32_t cluster_bits=0x58u,cluster_count=0x9cu;}
namespace listener {inline constexpr std::uint32_t seen=0u,reachable=4u,size=0x24u,end=0x990u;}
inline constexpr std::uint32_t set_words=16u,cluster_none=0xffffu;
// The frame below the saved registers: the reachable set's copy, the seen set's copy, the cluster buffer.
namespace local {inline constexpr std::uint32_t reachable_before=0x10u,seen_before=0x50u,clusters=0x90u,size=0xc0u;}

namespace cluster_visibility_update {
enum class Access {game_load,structure_load,seen_copy,seen_copy_store,reachable_copy,reachable_copy_store,game_again,count_word,
    count_load,game_found,game_copied,copy_word,copy_word_store,copy_byte,copy_byte_store,game_after,count_again,bits_load,
    seen_now,seen_before,listener_load,listener_count,listener_game,game_reachable,reachable_now,reachable_before};
// The original's EBX (the cluster buffer, then the reachable set), EBP (the structure BSP), ESI (the game, then the listener offset).
struct Kept {std::uint32_t reachable,structure_address,game_address;};
// Frame: see native_tu15_cluster_visibility_update.hpp.
template<class Memory,class Frame> void update(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    const auto step=frame.step();
    // The sets as they were.
    const auto game_address=memory.load32(game_load,globals::game);
    frame.framing();
    kept.structure_address=memory.load32(structure_load,globals::structure);
    frame.saving();
    auto from=frame.seen_copying(game_address);
    auto to=frame.local(local::seen_before);
    for(std::uint32_t k=0;k<set_words;++k,from+=4u*step,to+=4u*step)memory.store32(seen_copy_store,to,memory.load32(seen_copy,from));
    from=game_address+game::reachable;to=frame.local(local::reachable_before);
    frame.reachable_copying(from,to);
    for(std::uint32_t k=0;k<set_words;++k,from+=4u*step,to+=4u*step)
        memory.store32(reachable_copy_store,to,memory.load32(reachable_copy,from));
    // The seen and local sets gathered again; the reachable set rebuilt.
    frame.seen_gathered(game_address+game::seen,kept);
    const auto game_now=memory.load32(game_again,globals::game);
    frame.local_gathered(game_now,kept);
    frame.count_cleared();
    const auto count_word_value=memory.load16(count_word,at(kept.structure_address,structure::cluster_count));
    const bool found=frame.clusters_found(count_word_value,kept);
    const auto count=memory.load32(count_load,at(kept.structure_address,structure::cluster_count));
    if(found) {
        const auto game_value=memory.load32(game_found,globals::game);
        frame.clusters_added(game_value,kept);
    } else {
        const auto game_value=memory.load32(game_copied,globals::game);
        auto source=game_value+game::seen,target=game_value+game::reachable;
        const auto words=frame.words(count,game_value,kept);
        for(std::uint32_t k=0;k<words;++k,source+=4u*step,target+=4u*step)memory.store32(copy_word_store,target,memory.load32(copy_word,source));
        const auto bytes=frame.bytes();
        for(std::uint32_t k=0;k<bytes;++k,source+=step,target+=step)memory.store8(copy_byte_store,target,memory.load8(copy_byte,source));
        kept.game_address=memory.load32(game_after,globals::game);
    }
    const auto cluster=frame.cluster(kept);
    if((cluster&0xffffu)!=cluster_none) {
        const auto count_now=memory.load32(count_again,at(kept.structure_address,structure::cluster_count));
        const auto row=frame.row(count_now,cluster);
        const auto bits=memory.load32(bits_load,at(kept.structure_address,structure::cluster_bits));
        frame.row_added(bits,row,kept);
    }
    // Each changed set announced to the listeners.
    const auto changed=[&](Access now_role,Access before_role,std::uint32_t now,std::uint32_t before) {
        bool equal=true;
        for(std::uint32_t k=0;k<set_words && equal;++k,now+=4u*step,before+=4u*step) {
            const auto now_value=memory.load32(now_role,now);
            const auto before_value=memory.load32(before_role,before);
            equal=frame.words_compared(before_value,now_value);
        }
        return !equal;
    };
    const auto announce=[&](std::uint32_t slot,std::uint32_t set,std::uint32_t before_offset) {
        for(kept.game_address=frame.listeners();;) {
            const auto callback=memory.load32(listener_load,globals::listeners+kept.game_address+slot);
            frame.callback_tested(callback);
            if(callback) {
                const auto count_value=memory.load32(listener_count,at(kept.structure_address,structure::cluster_count));
                const auto game_value=memory.load32(listener_game,globals::game);
                frame.announced(callback,count_value,game_value,set,before_offset,kept);
            }
            if(!frame.next_listener(kept))break;
        }
    };
    frame.seen_comparing(kept.game_address+game::seen);
    if(changed(seen_now,seen_before,kept.game_address+game::seen,frame.local(local::seen_before)))
        announce(listener::seen,game::seen,local::seen_before);
    const auto game_last=memory.load32(game_reachable,globals::game);
    frame.reachable_comparing(game_last);
    if(changed(reachable_now,reachable_before,game_last+game::reachable,frame.local(local::reachable_before)))
        announce(listener::reachable,game::reachable,local::reachable_before);
    frame.restoring();
}
} // namespace cluster_visibility_update
} // namespace halo2::native_cluster_visibility_update
