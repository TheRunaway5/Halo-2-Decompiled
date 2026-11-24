#pragma once
// Native TU1.5 engine queries: two leaves of the multiplayer engines unit.
//
//   option_flag::set          25BCC0(?, kind) -> AL, RET 8
//     The game globals' options ([4F3218] +0x184) for a kind: bit 2 for
//     kind 1, bit 12 for kind 2, bit 13 for kind 3; zero for others.
//   director_player::of       18A240(EAX = record index) -> EAX
//     A director record's player (0x94 bytes at [4F9B54], +0x60), when it
//     names a live player (an index below the players' limit, +0x38, whose
//     datum's salt, the word at its start, is nonzero and the handle's high
//     word) that has a unit (+0x2C); NONE otherwise.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapters
// (native_tu15_option_flag.hpp, native_tu15_director_player.hpp) map the
// role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_engine_queries {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;

namespace option_flag {
namespace globals {inline constexpr std::uint32_t game_state=0x4f3218u;}
inline constexpr std::uint32_t options=0x184u;
struct Kind {std::uint32_t kind,bit;};
inline constexpr Kind kinds[]{{2u,12u},{3u,13u},{1u,2u}};   // in the original's order of tests
enum class Access {kind,game_state,options};
// Frame: argument(k) the k-th argument's slot; tested(k) before each kind's test.
template<class Memory,class Frame> std::uint32_t set(Memory &memory,Frame &frame) {
    const auto kind_value=memory.load32(Access::kind,frame.argument(2u));
    for(const auto &k:kinds) {
        frame.tested(k.kind);
        if(kind_value==k.kind)
            return (memory.load32(Access::options,at(memory.load32(Access::game_state,globals::game_state),options))>>k.bit)&1u;
    }
    return 0u;
}
} // namespace option_flag

namespace director_player {
namespace globals {
inline constexpr std::uint32_t records=0x4f9b54u;   // 0x94-byte director records
inline constexpr std::uint32_t players=0x4f54f4u;   // DataArray* of the players
}
inline constexpr std::uint32_t record_size=0x94u,record_player=0x60u;
namespace data_array {inline constexpr std::uint32_t datum_size=0x24u,limit=0x38u,data=0x44u;}
inline constexpr std::uint32_t player_unit=0x2cu;
enum class Access {records,player,players,limit,datum_size,data,salt,unit};
template<class Memory,class Frame> std::uint32_t of(Memory &memory,Frame &,std::uint32_t index) {
    using enum Access;
    const auto record_address=memory.load32(Access::records,globals::records)+record_size*index;
    const auto handle=memory.load32(player,at(record_address,record_player));
    if(handle==none)return none;
    const auto array=memory.load32(Access::players,globals::players);
    const auto slot=handle&0xffffu;
    if(signed_value(slot)>=signed_value(memory.load32(limit,at(array,data_array::limit))))return none;
    const auto size=memory.load32(Access::datum_size,at(array,data_array::datum_size));
    const auto datum=size*slot+memory.load32(data,at(array,data_array::data));
    const auto salt_value=memory.load16(Access::salt,datum);
    if(!salt_value || widen16(salt_value)!=std::uint32_t(std::int32_t(handle)>>16))return none;
    if(memory.load32(unit,at(datum,player_unit))==none)return none;
    return handle;
}
} // namespace director_player
} // namespace halo2::native_engine_queries
