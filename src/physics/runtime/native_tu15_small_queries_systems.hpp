#pragma once
// Native TU1.5 small queries:
//
//   game_hold::held          13F620() -> AL            (sandbox special unit)
//       the game globals ([51DF50], or none) hold effects (+5)
//   cluster_visible::visible 11E780(EAX = object header) -> EAX   (physics live support)
//       the header's cluster (signed word +4) in the game globals' cluster
//       bits ([4F3218] +0x1178): 1 or 0
//   unit_override::on        E3310(EAX = unit handle) -> AL  (physics live support, AI frame)
//       the unit in state 6 (+0x3DC), active (+0x3F8), bit 13 of +0x348
//   session_variant::match   15DD10() -> AL            (physics live support, death cycle)
//       the session's variant record ([56DFA0] by [4F63B8] +0xC14) set,
//       its +0x6C word 1, and either game mode 4 ([4F3218] +0xC) or +0xC04 1
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapters map the role to the
// original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_small_queries {
using native_object_update::at;
using native_object_update::widen16;
namespace globals {
inline constexpr std::uint32_t game=0x51df50u;             // the game globals (+5: holding effects), or zero
inline constexpr std::uint32_t game_state=0x4f3218u;       // GameGlobals* (+0xC mode, +0x1178 cluster bits)
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t session=0x4f63b8u;          // the session record (+0xC14 its variant index, +0x6C, +0xC04)
inline constexpr std::uint32_t variants=0x56dfa0u;         // the variant records, by the session's index
}

namespace game_hold {
enum class Access {game,hold};
template<class Memory> std::uint32_t held(Memory &memory) {
    using enum Access;
    const auto game_address=memory.load32(game,globals::game);
    return game_address?memory.load8(hold,at(game_address,5u)):0u;
}
} // namespace game_hold

namespace cluster_visible {
enum class Access {cluster,game_state,bits};
inline constexpr std::uint32_t cluster_bits=0x1178u;
template<class Memory> std::uint32_t visible(Memory &memory,std::uint32_t header) {
    using enum Access;
    const auto index=widen16(memory.load16(cluster,at(header,4u)));
    const auto state=memory.load32(game_state,globals::game_state);
    const auto word=memory.load32(bits,state+cluster_bits+4u*std::uint32_t(std::int32_t(index)>>5));
    return (word&(1u<<(index&31u)))?1u:0u;
}
} // namespace cluster_visible

namespace unit_override {
enum class Access {headers,header_datums,unit_address,state,active,bits};
inline constexpr std::uint32_t override_state=6u,override_bit=13u;
template<class Memory> bool on(Memory &memory,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,0x44u));
    const auto unit=memory.load32(unit_address,datums+12u*(handle&0xffffu)+8u);
    if((memory.load8(state,at(unit,0x3dcu))&0xffu)!=override_state)return false;
    if(!(memory.load8(active,at(unit,0x3f8u))&0xffu))return false;
    return (memory.load16(bits,at(unit,0x348u))>>override_bit)&1u;
}
} // namespace unit_override

namespace session_variant {
enum class Access {session,variant_index,variant,session_word,game_state,mode,session_flag};
inline constexpr std::uint32_t wanted_mode=4u;
template<class Memory> bool match(Memory &memory) {
    using enum Access;
    const auto session_address=memory.load32(session,globals::session);
    const auto index=memory.load32(variant_index,at(session_address,0xc14u));
    if(!memory.load32(variant,globals::variants+4u*index))return false;
    if((memory.load16(session_word,at(session_address,0x6cu))&0xffffu)!=1u)return false;
    const auto state=memory.load32(game_state,globals::game_state);
    if((memory.load8(mode,at(state,0xcu))&0xffu)==wanted_mode)return true;
    return memory.load32(session_flag,at(session_address,0xc04u))==1u;
}
} // namespace session_variant
} // namespace halo2::native_small_queries
