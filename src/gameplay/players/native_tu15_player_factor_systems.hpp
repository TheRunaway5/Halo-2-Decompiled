#pragma once
// Native TU1.5 player factor, entered at 15B2D0 (physics live support
// unit): a float factor for a player, for the kind 2 of request.
//
//   player_factor::of   15B2D0(ECX = player; kind) -> XMM0, RET 4
//
// The factor starts at one (46A0D0) and stays one for any kind but 2.
// For kind 2 the game globals ([4F3218]) decide it: in mode 7 (+0x180) a
// player whose bit (by the handle's index) is set in the word at [51DF9C]
// is masked. A masked player's factor is the constant at 46A0E4 when the
// globals' flags (+0x22C) carry bit 1, else zero when the setting (+0x1C4)
// is 1. Any other player's factor is zero for the setting 1 and the
// constant for the setting 2. The player's class (int8 +0xC1 of the
// 0x21C-byte players at [4F54F4]) of 0..3 then selects, through the
// original's jump table (15B39C), a multiplier (46A13C, 46A0CC or 46A130)
// or none; other classes leave the factor.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the jump table's entry; the boundary
// adapter (native_tu15_player_factor.hpp) maps the role to the original
// instruction. The multiplication runs on the live MXCSR, as MULSS does.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_player_factor {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t players=0x4f54f4u;        // DataArray* of 0x21C-byte players
inline constexpr std::uint32_t game=0x4f3218u;           // GameGlobals*
inline constexpr std::uint32_t player_mask=0x51df9cu;    // the address of a word of player bits
inline constexpr std::uint32_t one=0x46a0d0u,special=0x46a0e4u;   // floats
inline constexpr std::uint32_t class_table=0x15b39cu;    // the original's jump table, by class
inline constexpr std::uint32_t first_scale=0x46a13cu,second_scale=0x46a0ccu,third_scale=0x46a130u;
}
inline constexpr std::uint32_t datum_data=0x44u,player_size=0x21cu,class_byte=0xc1u,applying_kind=2u,masked_mode=7u,
    special_flag=2u,zero_setting=1u,special_setting=2u,classes=3u,zero_bits=0u;
namespace game {inline constexpr std::uint32_t mode=0x180u,setting=0x1c4u,flags=0x22cu;}
// The table's targets, as the unit's canonical_branch names them.
namespace cases {inline constexpr std::uint32_t first=0x15b377u,second=0x15b383u,third=0x15b38fu,unscaled=0x15b397u;}

namespace player_factor {
enum class Access {players,one,player_data,kind,game,mode,mask_address,mask,game_flags,special_masked,masked_setting,setting,
    special,player_class,class_case,first_scale,second_scale,third_scale};
enum class Outcome {other_kind,first,second,third,unscaled,unclassed};
struct Result {std::uint32_t factor;Outcome outcome;};
// Frame:
//   argument()         the kind's slot now
//   zeroed()           the factor zero (XORPS)
//   scaled(product)    the factor multiplied
//   unknown_case()     a table entry that is no case (throws)
template<class Memory,class Frame> Result of(Memory &memory,Frame &frame,std::uint32_t player) {
    using enum Access;
    const auto array=memory.load32(players,globals::players);
    auto factor=memory.load32(one,globals::one);
    const auto player_address=player_size*(player&0xffffu)+memory.load32(player_data,at(array,datum_data));
    if(memory.load32(kind,frame.argument())!=applying_kind)return {factor,Outcome::other_kind};
    const auto game_address=memory.load32(Access::game,globals::game);
    bool masked=false;
    if(memory.load32(mode,at(game_address,game::mode))==masked_mode) {
        const auto bit=1u<<(player&31u);
        masked=(memory.load16(mask,memory.load32(mask_address,globals::player_mask))&bit)!=0u;
    }
    if(masked) {
        if(memory.load8(game_flags,at(game_address,game::flags))&special_flag)factor=memory.load32(special_masked,globals::special);
        else if(memory.load32(masked_setting,at(game_address,game::setting))==zero_setting){factor=zero_bits;frame.zeroed();}
    } else {
        const auto value=memory.load32(setting,at(game_address,game::setting));
        if(value==zero_setting){factor=zero_bits;frame.zeroed();}
        else if(value==special_setting)factor=memory.load32(special,globals::special);
    }
    const auto class_value=std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(memory.load8(Access::player_class,at(player_address,class_byte))))));
    if(class_value>classes)return {factor,Outcome::unclassed};
    const auto scale=[&](Access role,std::uint32_t at_address,Outcome outcome) {
        factor=sse::multiply(factor,memory.load32(role,at_address));
        frame.scaled(factor);
        return Result{factor,outcome};
    };
    switch(memory.load32(class_case,globals::class_table+4u*class_value)) {
    case cases::first:return scale(first_scale,globals::first_scale,Outcome::first);
    case cases::second:return scale(second_scale,globals::second_scale,Outcome::second);
    case cases::third:return scale(third_scale,globals::third_scale,Outcome::third);
    case cases::unscaled:return {factor,Outcome::unscaled};
    }
    frame.unknown_case();
    return {factor,Outcome::unscaled};
}
} // namespace player_factor
} // namespace halo2::native_player_factor
