#pragma once
// Native TU1.5 biped action test, entered at 1540D0 (combat action unit):
// whether a biped near a player's unit makes a better action, as the
// player action search (153BA0) asks of each biped it finds.
//
//   biped_action::test   1540D0(EAX = biped; player, context) -> RET 8
//
// The action context (12 bytes) holds an interaction (type and data words,
// then an object, +4) and a melee target (+8). Only a player whose unit
// (+0x2C of the 0x21C-byte players at [4F54F4]) is a biped (its object
// header's type byte +3 is 0) tests, and only a biped whose byte +0x10A
// lacks bit 2. A biped hostile to the player (1E7810: the player's team,
// int8 +0xC0, against the biped's, word +0x138) is offered to E8110 as the
// melee target (the player's unit; the biped, the current target), which
// answers the context's target. Any other biped that E79C0 accepts for the
// player is offered to 153E10 as an interaction of type 2 with the biped
// (the candidate in the frame, the context; the player).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the arguments and the candidate in the
// original's frame; the boundary adapter (native_tu15_biped_action.hpp) maps
// the role to the original instruction and owns the ABI of the children
// (1E7810, E8110, E79C0, 153E10). The values the original keeps in
// callee-saved registers across them are a Kept, refreshed from the
// registers each child returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_biped_action {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t players=0x4f54f4u,objects=0x4ecbd0u;}
inline constexpr std::uint32_t datum_data=0x44u,player_size=0x21cu,header_size=12u,header_type=3u,header_address=8u,handle_mask=0xffffu;
namespace player {inline constexpr std::uint32_t unit=0x2cu,team=0xc0u;}
namespace biped {inline constexpr std::uint32_t flags=0x10au,team=0x138u;inline constexpr std::uint32_t ignored=1u<<2;}
namespace context {inline constexpr std::uint32_t melee_target=8u;}
inline constexpr std::uint32_t biped_type=0u,interaction_type=2u;
// The candidate interaction in the frame: the type and data words, the object.
namespace candidate {inline constexpr std::uint32_t type=0u,object=4u;}

namespace biped_action {
enum class Access {player_argument,players,player_data,objects,object_data,unit,biped_address,unit_type,biped_flags,player_team,biped_team,
    context_argument,melee_target,unit_again,target_store,interaction_argument,candidate_clear,object_clear,object_store,type_store};
enum class Outcome {no_biped_unit,ignored,melee,refused,interaction};
// The original's EDI, EBX, EBP and ESI across the children: the player, the
// biped, the player's handle and (after E8110) the context.
struct Kept {std::uint32_t player,biped,player_handle,context;};
// Frame:
//   argument(k), candidate(offset)       an argument's slot, the frame's candidate, now
//   hostile(player_team, biped_team, kept) -> AL   1E7810(CX, SI), refreshes kept
//   melee(unit, biped, target, kept) -> target     E8110(EDI the unit; biped, target), refreshes kept
//   accepts(player, biped, kept) -> AL   E79C0(EAX the player; biped), refreshes kept
//   offering(kept)                       the player pushed for 153E10
//   offer(candidate, context, player, kept)        153E10(EBX the candidate, EDI the context; player)
template<class Memory,class Frame> Outcome test(Memory &memory,Frame &frame,std::uint32_t biped_handle) {
    using enum Access;
    Kept kept{};
    kept.player_handle=memory.load32(player_argument,frame.argument(0u));
    kept.biped=biped_handle;
    kept.player=player_size*(kept.player_handle&handle_mask)+memory.load32(player_data,at(memory.load32(Access::players,globals::players),datum_data));
    const auto data=memory.load32(object_data,at(memory.load32(Access::objects,globals::objects),datum_data));
    const auto unit_handle=memory.load32(unit,at(kept.player,player::unit));
    const auto biped_object=memory.load32(biped_address,data+header_size*(biped_handle&handle_mask)+header_address);
    const auto type=memory.load8(unit_type,data+header_size*(unit_handle&handle_mask)+header_type)&31u;
    if(!((1u<<type)&(1u<<biped_type)))return Outcome::no_biped_unit;
    if(memory.load8(biped_flags,at(biped_object,biped::flags))&biped::ignored)return Outcome::ignored;
    const auto team=memory.load8(player_team,at(kept.player,player::team));
    if(frame.hostile(team,memory.load16(biped_team,at(biped_object,biped::team)),kept)&0xffu) {
        kept.context=memory.load32(context_argument,frame.argument(1u));
        const auto target=memory.load32(melee_target,at(kept.context,context::melee_target));
        const auto player_unit=memory.load32(unit_again,at(kept.player,player::unit));
        const auto better=frame.melee(player_unit,kept.biped,target,kept);
        memory.store32(target_store,at(kept.context,context::melee_target),better);
        return Outcome::melee;
    }
    if(!(frame.accepts(kept.player_handle,kept.biped,kept)&0xffu))return Outcome::refused;
    const auto context_address=memory.load32(interaction_argument,frame.argument(1u));
    memory.store32(candidate_clear,frame.candidate(candidate::type),0u);
    memory.store32(object_clear,frame.candidate(candidate::object),0u);
    memory.store32(object_store,frame.candidate(candidate::object),kept.biped);
    frame.offering(kept);
    memory.store16(type_store,frame.candidate(candidate::type),interaction_type);
    frame.offer(frame.candidate(0u),context_address,kept.player_handle,kept);
    return Outcome::interaction;
}
} // namespace biped_action
} // namespace halo2::native_biped_action
