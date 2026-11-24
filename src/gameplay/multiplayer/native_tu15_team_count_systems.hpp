#pragma once
// Native TU1.5 team count, entered at 15B620 (multiplayer engines unit): how
// many teams have players, and how many have living ones.
//
//   team_count::count   15B620(last*, living*, teams*) -> RET C
//
// Each active player (the players DataArray at [4F54F4]: its bitmap +0x48
// over the count +0x38; datums of +0x24 bytes at +0x44), in index order,
// with its flags (+2) lacking bit 1 and a team (int8 +0xC0) of 0..15, marks
// its team as having players. A player counts as living when 15B470
// answers so, or when its flags have neither bit 0 nor bit 1; its handle
// (the salt word +0 over the index) is then the last living player, and its
// team is marked living. The counts go to the outputs given (not NULL); the
// last living player's to its output each time one is found.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (the two team
// masks, their counts and the scan's index) and its arguments; the boundary
// adapter (native_tu15_team_count.hpp) maps the role to the original
// instruction and owns the ABI of the child (15B470). The values the
// original keeps in callee-saved registers across it are a Kept, refreshed
// from the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_team_count {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t players=0x4f54f4u;}
namespace array {inline constexpr std::uint32_t datum_size=0x24u,count=0x38u,data=0x44u,bitmap=0x48u;}
namespace player {inline constexpr std::uint32_t flags=2u,team=0xc0u;inline constexpr std::uint32_t inactive=1u<<1,unliving=3u;}
inline constexpr std::uint32_t teams_limit=16u;
// The frame's locals, by offset.
namespace local {inline constexpr std::uint32_t teams=0u,living_teams=4u,teams_counted=8u,living_counted=0xcu,index=0x1cu;}
inline std::uint32_t widen8(std::uint32_t byte){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(byte))));}

namespace team_count {
enum class Access {players,living_clear,teams_clear,counted_clear,index_init,living_counted_store,index_load,array_count,bitmap,bit,
    datum_size,array_data,team,salt,index_store,flags,teams_load,counted_load,teams_store,counted_store,flags_again,living_load,
    last_argument,last_store,living_counted_load,living_store,teams_argument,counted_final,teams_output,living_argument,
    living_counted_final,living_output};
enum class Outcome {counted};
// The original's EBX, EDI, ESI and EBP across 15B470: the players array,
// the player, its handle and its team's bit.
struct Kept {std::uint32_t array,player,handle,team_bit;};
// Frame:
//   local(offset), argument(k)    the frame's local, an argument's slot, now
//   saving(), restoring()         the registers saved after the array's read, and back
//   living(kept) -> AL            15B470, refreshes kept
template<class Memory,class Frame> Outcome finish(Memory &memory,Frame &frame) {
    using enum Access;
    const auto teams_address=memory.load32(teams_argument,frame.argument(2u));
    frame.restoring();
    if(teams_address)memory.store32(teams_output,teams_address,memory.load32(counted_final,frame.local(local::teams_counted)));
    const auto living_address=memory.load32(living_argument,frame.argument(1u));
    if(living_address)memory.store32(living_output,living_address,memory.load32(living_counted_final,frame.local(local::living_counted)));
    return Outcome::counted;
}
template<class Memory,class Frame> Outcome count(Memory &memory,Frame &frame) {
    using enum Access;
    Kept kept{};
    kept.array=memory.load32(players,globals::players);
    frame.saving();
    memory.store32(living_clear,frame.local(local::living_teams),0u);
    memory.store32(teams_clear,frame.local(local::teams),0u);
    memory.store32(counted_clear,frame.local(local::teams_counted),0u);
    memory.store32(index_init,frame.local(local::index),none);
    std::uint32_t living_counted=0u;
    for(;;) {
        memory.store32(living_counted_store,frame.local(local::living_counted),living_counted);
        for(;;) {
            // The next active player.
            auto index=memory.load32(index_load,frame.local(local::index))+1u;
            if(signed_value(index)<0)return finish(memory,frame);
            const auto count_value=memory.load32(array_count,at(kept.array,array::count));
            if(!(signed_value(index)<signed_value(count_value)))return finish(memory,frame);
            const auto bits=memory.load32(bitmap,at(kept.array,array::bitmap));
            while(!(memory.load32(bit,bits+4u*std::uint32_t(signed_value(index)>>5))&(1u<<(index&31u))))
                if(!(signed_value(++index)<signed_value(count_value)))return finish(memory,frame);
            if(index==none)return finish(memory,frame);
            const auto size=memory.load32(datum_size,at(kept.array,array::datum_size));
            const auto data=memory.load32(array_data,at(kept.array,array::data));
            const auto team_value=widen8(memory.load8(team,size*index+data+player::team));
            kept.player=size*index+data;
            kept.handle=(widen16(memory.load16(salt,kept.player))<<16)|index;
            memory.store32(index_store,frame.local(local::index),index);
            if(memory.load8(flags,at(kept.player,player::flags))&player::inactive)continue;
            if(signed_value(team_value)<0 || !(signed_value(team_value)<signed_value(teams_limit)) || team_value==none)continue;
            // The team has players.
            kept.team_bit=1u<<(team_value&31u);
            const auto teams=memory.load32(teams_load,frame.local(local::teams));
            if(!(teams&kept.team_bit)) {
                const auto counted=memory.load32(counted_load,frame.local(local::teams_counted));
                memory.store32(teams_store,frame.local(local::teams),teams|kept.team_bit);
                memory.store32(counted_store,frame.local(local::teams_counted),counted+1u);
            }
            // A living player.
            if(!(frame.living(kept)&0xffu) && (memory.load8(flags_again,at(kept.player,player::flags))&player::unliving))continue;
            const auto living=memory.load32(living_load,frame.local(local::living_teams));
            memory.store32(last_store,memory.load32(last_argument,frame.argument(0u)),kept.handle);
            if(living&kept.team_bit)continue;
            living_counted=memory.load32(living_counted_load,frame.local(local::living_counted))+1u;
            memory.store32(living_store,frame.local(local::living_teams),living|kept.team_bit);
            break;
        }
    }
}
} // namespace team_count
} // namespace halo2::native_team_count
