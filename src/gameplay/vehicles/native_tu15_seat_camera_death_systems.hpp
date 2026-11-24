#pragma once
// Native TU1.5 seat camera death, entered at 158790 (vehicle director unit):
// a local seat's camera reset, or moved to and from the dead state.
//
//   seat_camera_death::update   158790(ECX = seat, AL = reset)
//
// A reset puts the seat's camera (0x140 bytes at 4F5514 by seat) in the
// default state 245400 with its view (+0x10, +0x14) zero, its fade (+0x10C)
// one and its fade flag (+0x108) clear. Otherwise the seat's player (the
// handles at [4F54F0]+0xC, NONE for none; the 0x21C-byte players of the
// array at 4F54F4) is dead when it has no unit (+0x2C) and, outside game
// mode 2 (the game globals' +8 at [4F3218]), its flags (+2) lack bit 3.
// Without scripted camera control (the byte at [4F5504]) and with the game
// time's float +0xC (at [51DF54]) above zero, 158450 updates the camera's
// mode; then a dead player's camera not yet in the dead state 247140 gets
// its view from 246CF0 (the view, the seat; NONE) and enters it (158900:
// the seat, the state; 1), and a living player's camera in the dead state
// has its mode forced (158450 with 1).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_seat_camera_death.hpp) maps the role to the original
// instruction and owns the ABI of the children (158450, 246CF0, 158900).
// The values the original keeps in callee-saved registers across them are a
// Kept, refreshed from the registers each child returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_seat_camera_death {
using native_object_update::at;
using native_object_update::none;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t cameras=0x4f5514u;        // 0x140-byte camera records, by local seat
inline constexpr std::uint32_t seats=0x4f54f0u;          // +0xC: each local seat's player
inline constexpr std::uint32_t players=0x4f54f4u;        // DataArray* of 0x21C-byte players
inline constexpr std::uint32_t game=0x4f3218u;           // GameGlobals* (+8 the game mode)
inline constexpr std::uint32_t camera_control=0x4f5504u; // the scripted camera control byte's address
inline constexpr std::uint32_t game_time=0x51df54u;      // +0xC: a float that must be above zero
inline constexpr std::uint32_t one=0x46a0d0u,zero=0x46a0e8u;   // floats
}
inline constexpr std::uint32_t camera_size=0x140u,seat_players=0xcu,datum_data=0x44u,player_size=0x21cu,game_mode=8u,
    time_value=0xcu,zero_bits=0u;
namespace player {inline constexpr std::uint32_t flags=2u,unit=0x2cu,kept_alive=1u<<3;}
inline constexpr std::uint32_t lenient_mode=2u;       // the game mode in which only the unit decides
namespace camera {inline constexpr std::uint32_t state=0xcu,view=0x10u,view_second=0x14u,fade_flag=0x108u,fade=0x10cu;}
namespace states {inline constexpr std::uint32_t standard=0x245400u,dead=0x247140u;}

namespace seat_camera_death {
enum class Access {view_clear,view_second_clear,one,state_store,fade_store,fade_flag_store,seats,seat_player,players,player_data,
    player_unit,game,mode,player_flags,control,control_byte,game_time,time,zero,state};
enum class Outcome {reset,controlled,stopped,dead,already_dead,revived,alive};
// The original's EDI, ESI and BL across the children: the seat, its camera
// and whether its player is dead.
struct Kept {std::uint32_t seat,camera,dead;};
// Frame:
//   restoring()                          the saved EDI back before the state's store
//   update_mode(forced, kept)            158450(EDI the seat; forced pushed), refreshes kept
//   view(kept)                           246CF0(EAX the view, EDX the seat; NONE pushed), refreshes kept
//   enter(kept, state)                   158900(EAX the seat, EDX the state; 1 pushed), refreshes kept
template<class Memory,class Frame> Outcome update(Memory &memory,Frame &frame,std::uint32_t seat,bool reset) {
    using enum Access;
    Kept kept{seat,globals::cameras+camera_size*seat,0u};
    if(reset) {
        memory.store32(view_clear,at(kept.camera,camera::view),zero_bits);
        memory.store32(view_second_clear,at(kept.camera,camera::view_second),zero_bits);
        const auto one_bits=memory.load32(one,globals::one);
        frame.restoring();
        memory.store32(state_store,at(kept.camera,camera::state),states::standard);
        memory.store32(fade_store,at(kept.camera,camera::fade),one_bits);
        memory.store8(fade_flag_store,at(kept.camera,camera::fade_flag),0u);
        return Outcome::reset;
    }
    auto player_handle=none;
    if(seat!=none)player_handle=memory.load32(seat_player,memory.load32(Access::seats,globals::seats)+4u*seat+seat_players);
    const auto player_offset=player_size*(player_handle&0xffffu);
    const auto data=memory.load32(player_data,at(memory.load32(Access::players,globals::players),datum_data));
    const auto unit=memory.load32(player_unit,player_offset+data+player::unit);
    const auto player_address=player_offset+data;
    const auto mode_value=memory.load32(mode,at(memory.load32(Access::game,globals::game),game_mode));
    kept.dead=unit==none?1u:0u;
    if(mode_value!=lenient_mode && kept.dead)
        kept.dead=(memory.load8(player_flags,at(player_address,player::flags))&player::kept_alive)?0u:1u;
    if(memory.load8(control_byte,memory.load32(control,globals::camera_control))&0xffu)return Outcome::controlled;
    const auto time_bits=memory.load32(Access::time,at(memory.load32(Access::game_time,globals::game_time),time_value));
    if(sse::not_above(time_bits,memory.load32(zero,globals::zero)))return Outcome::stopped;
    frame.update_mode(0u,kept);
    const auto state_value=memory.load32(state,at(kept.camera,camera::state));
    if(kept.dead&0xffu) {
        if(state_value==states::dead)return Outcome::already_dead;
        frame.view(kept);
        frame.enter(kept,states::dead);
        return Outcome::dead;
    }
    if(state_value!=states::dead)return Outcome::alive;
    frame.update_mode(1u,kept);
    return Outcome::revived;
}
} // namespace seat_camera_death
} // namespace halo2::native_seat_camera_death
