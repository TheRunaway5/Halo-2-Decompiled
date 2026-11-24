#pragma once
// Native TU1.5 seat camera mode, entered at 158450 (vehicle director unit):
// a local seat's camera following its unit's mode.
//
//   seat_camera_mode::update   158450(EDI = seat; forced byte) -> RET 4
//
// The seat's unit (its player, [4F54F0] +0xC by seat, NONE for none; the
// player's unit, +0x2C of the 0x21C-byte players at [4F54F4]) goes to
// 158340, which answers its mode (AX) and a word in the frame. Unless
// forced, a camera (0x140 bytes at 4F5514 by seat) that already remembers
// the word (+0x58) is left alone. Mode 1 moves a camera in state 245400 (or
// any, forced) to state 245EA0, resetting its view: the bytes +0x10..+0x14
// and the word +0x16 cleared, the floats +0x20 and +0x24 zero, the target
// (+0x18) and its index (+0x1C) NONE, the blend (+0x28) one. Any other mode
// moves a camera in state 245EA0 (or any, forced) to state 245400, clearing
// the floats +0x10 and +0x14. 158900 enters the state (EAX the seat, EDX
// the state, the flag "not forced" pushed). The camera then remembers the
// word.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_seat_camera_mode.hpp) maps the role to the original
// instruction and owns the ABI of the children; the seat, the camera and the
// word, which the original keeps in EDI, ESI and BX across them, are a
// Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_seat_camera_mode {
using native_object_update::at;
using native_object_update::none;
namespace globals {
inline constexpr std::uint32_t cameras=0x4f5514u;   // 0x140-byte camera records, by local seat
inline constexpr std::uint32_t seats=0x4f54f0u;     // +0xC: each local seat's player
inline constexpr std::uint32_t players=0x4f54f4u;   // DataArray* of 0x21C-byte players (+0x2C the unit)
inline constexpr std::uint32_t one=0x46a0d0u;       // float 1
}
inline constexpr std::uint32_t camera_size=0x140u,seat_players=0xcu,datum_data=0x44u,player_size=0x21cu,player_unit=0x2cu;
inline constexpr std::uint32_t mode_one=1u,mode_one_state=0x245ea0u,other_state=0x245400u,no_index=0xffffu,zero_bits=0u;
namespace camera {inline constexpr std::uint32_t state=0xcu,view=0x10u,view_word=0x16u,target=0x18u,target_index=0x1cu,
    turn=0x20u,pitch=0x24u,blend=0x28u,remembered=0x58u;}

namespace seat_camera_mode {
enum class Access {seats,seat_player,players,player_data,unit,forced,word,remembered,state,clear_store,one,blend_store,remember_store};
enum class Outcome {unchanged,kept,mode_one,other_mode};
struct Kept {std::uint32_t seat,camera,word;};   // EDI, ESI, BX
// Frame:
//   argument(), local()                    the forced byte's slot and the frame's word now
//   mode(kept, unit) -> AX                 158340(EAX the unit; the word's address), refreshes kept
//   enter(kept, state, not_forced)         158900, refreshes kept
template<class Memory,class Frame> void remember(Memory &memory,const Kept &kept) {
    memory.store16(Access::remember_store,at(kept.camera,camera::remembered),kept.word);
}
template<class Memory,class Frame> Outcome update(Memory &memory,Frame &frame,std::uint32_t seat) {
    using enum Access;
    Kept kept{seat,globals::cameras+camera_size*seat,0u};
    auto unit_handle=none;
    if(seat!=none) {
        const auto player=memory.load32(seat_player,memory.load32(Access::seats,globals::seats)+4u*seat+seat_players);
        if(player!=none) {
            const auto data=memory.load32(player_data,at(memory.load32(Access::players,globals::players),datum_data));
            unit_handle=memory.load32(unit,data+player_size*(player&0xffffu)+player_unit);
        }
    }
    const auto mode=frame.mode(kept,unit_handle)&0xffffu;
    const bool is_forced=memory.load8(forced,frame.argument())&0xffu;
    kept.word=memory.load16(word,frame.local());
    if(!is_forced && memory.load16(remembered,at(kept.camera,camera::remembered))==kept.word)return Outcome::unchanged;
    const bool to_mode_one=mode==mode_one;
    if(!is_forced && memory.load32(state,at(kept.camera,camera::state))!=(to_mode_one?other_state:mode_one_state)) {
        remember<Memory,Frame>(memory,kept);
        return Outcome::kept;
    }
    if(to_mode_one) {
        // The view reset: its bytes and word cleared, the turn and pitch zero, no target, the blend one.
        for(std::uint32_t k=0;k<4u;++k)memory.store8(clear_store,at(kept.camera,camera::view+k),0u);
        memory.store16(clear_store,at(kept.camera,camera::view_word),0u);
        memory.store8(clear_store,at(kept.camera,camera::view+4u),0u);
        memory.store32(clear_store,at(kept.camera,camera::pitch),zero_bits);
        memory.store32(clear_store,at(kept.camera,camera::turn),zero_bits);
        const auto one_value=memory.load32(one,globals::one);
        memory.store32(clear_store,at(kept.camera,camera::target),none);
        memory.store16(clear_store,at(kept.camera,camera::target_index),no_index);
        memory.store32(blend_store,at(kept.camera,camera::blend),one_value);
        frame.enter(kept,mode_one_state,!is_forced);
    } else {
        memory.store32(clear_store,at(kept.camera,camera::view),zero_bits);
        memory.store32(clear_store,at(kept.camera,camera::view+4u),zero_bits);
        frame.enter(kept,other_state,!is_forced);
    }
    remember<Memory,Frame>(memory,kept);
    return to_mode_one?Outcome::mode_one:Outcome::other_mode;
}
} // namespace seat_camera_mode
} // namespace halo2::native_seat_camera_mode
