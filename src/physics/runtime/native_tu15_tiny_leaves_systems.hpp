#pragma once
// Native TU1.5 tiny leaves (physics live support unit, some in more units):
//
//   six::value                 31EDF0() -> EAX 6
//   double_exponent::bits      33210C(double) -> EAX (also AI frame)
//       the high word's exponent bits; for an infinity or a NaN (all set)
//       the whole high word
//   variant_option::flag       161530() -> AL (also sandbox)
//       bit 0 of the game globals' byte +0x184 ([4F3218]) when the session
//       has a variant record ([56DFA0] by [4F63B8] +0xC14), else 0
//   definition_word::read      1D4140(ECX = record*) -> EAX
//       the signed word +0xC of the definition of the record's tag (+0x68),
//       or 0 when the tag has none
//   block_ready::test          116480(EAX = object handle) -> EAX (also AI frame)
//       of the object's block (its signed offset +0x33E): 1 when the byte
//       +0x89 is clear; otherwise 1 only when the byte +0x8B has bit 1 and
//       not bit 0
//   tick_seconds::store        E32C0(EAX = object handle; out*) -> AL, RET 4
//       with the object's tick count (signed byte +0x3AA) set, the count in
//       seconds (seconds per tick, [51DF54] +4) times [44A8A0] is stored and
//       the result is 1; otherwise 0
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapters
// (native_tu15_{six,double_exponent,variant_option,definition_word,
// block_ready,tick_seconds}.hpp) map the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include <cstdint>

namespace halo2::native_tiny_leaves {
using native_object_update::at;
using native_object_update::widen16;
using native_object_update::widen8;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // 16-byte tag instances (+8 the tag's address)
inline constexpr std::uint32_t session=0x4f63b8u;          // the session record (+0xC14 its variant index)
inline constexpr std::uint32_t variants=0x56dfa0u;         // the variant records, by the session's index
inline constexpr std::uint32_t game_state=0x4f3218u;       // GameGlobals*
inline constexpr std::uint32_t game_time=0x51df54u;        // +4 seconds per tick
inline constexpr std::uint32_t tick_scale=0x44a8a0u;       // float
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
// The object's data address from its handle, through the object headers.
template<class Memory,class Access> std::uint32_t object_of(Memory &memory,Access headers,Access header_datums,Access address,
                                                            std::uint32_t handle) {
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    return memory.load32(address,datums+header_size*(handle&0xffffu)+header_address);
}

namespace six {inline constexpr std::uint32_t value=6u;}

namespace double_exponent {
inline constexpr std::uint32_t exponent=0x7ff00000u;
enum class Access {high,high_again};
struct Result {bool special;std::uint32_t value;};
// Frame: high_slot() the double's high word.
template<class Memory,class Frame> Result bits(Memory &memory,Frame &frame) {
    const auto masked=memory.load32(Access::high,frame.high_slot())&exponent;
    if(masked!=exponent)return {false,masked};
    return {true,memory.load32(Access::high_again,frame.high_slot())};
}
} // namespace double_exponent

namespace variant_option {
inline constexpr std::uint32_t variant_index=0xc14u,option=0x184u,option_bit=1u<<0;
enum class Access {session,variant_index,variant,game_state,option};
struct Result {bool varied;std::uint32_t value;};
template<class Memory> Result flag(Memory &memory) {
    using enum Access;
    const auto session_address=memory.load32(session,globals::session);
    const auto index=memory.load32(Access::variant_index,at(session_address,native_tiny_leaves::variant_option::variant_index));
    if(!memory.load32(variant,globals::variants+4u*index))return {false,0u};
    const auto state=memory.load32(game_state,globals::game_state);
    return {true,memory.load8(Access::option,at(state,native_tiny_leaves::variant_option::option))&option_bit};
}
} // namespace variant_option

namespace definition_word {
inline constexpr std::uint32_t record_tag=0x68u,word=0xcu;
enum class Access {tag,tags,definition,word};
struct Result {bool defined;std::uint32_t value;};
template<class Memory> Result read(Memory &memory,std::uint32_t record) {
    using enum Access;
    const auto tag_handle=memory.load32(tag,at(record,record_tag));
    const auto instances=memory.load32(tags,globals::tag_instances);
    const auto owner=memory.load32(definition,16u*(tag_handle&0xffffu)+instances+8u);
    if(!owner)return {false,0u};
    return {true,widen16(memory.load16(Access::word,at(owner,native_tiny_leaves::definition_word::word)))};
}
} // namespace definition_word

namespace block_ready {
inline constexpr std::uint32_t block_offset=0x33eu,gate=0x89u,state=0x8bu;
inline constexpr std::uint32_t blocked_bit=1u<<0,ready_bit=1u<<1;
enum class Access {headers,header_datums,object_address,block,gate,state};
enum class Outcome {open,blocked,ready,waiting};
template<class Memory> Outcome test(Memory &memory,std::uint32_t handle) {
    using enum Access;
    const auto object=object_of(memory,headers,header_datums,object_address,handle);
    const auto base=widen16(memory.load16(Access::block,at(object,block_offset)))+object;
    if(!(memory.load8(Access::gate,at(base,native_tiny_leaves::block_ready::gate))&0xffu))return Outcome::open;
    const auto bits=memory.load8(Access::state,at(base,native_tiny_leaves::block_ready::state))&0xffu;
    if(bits&blocked_bit)return Outcome::blocked;
    return (bits&ready_bit)?Outcome::ready:Outcome::waiting;
}
} // namespace block_ready

namespace tick_seconds {
inline constexpr std::uint32_t ticks=0x3aau,seconds_per_tick=4u;
enum class Access {headers,header_datums,object_address,ticks,game_time,seconds_per_tick,scale,out,store};
struct Result {bool stored;std::uint32_t value,count;};
// Frame: out_slot() the out pointer's slot.
template<class Memory,class Frame> Result store(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto object=object_of(memory,headers,header_datums,object_address,handle);
    const auto count=memory.load8(Access::ticks,at(object,native_tiny_leaves::tick_seconds::ticks))&0xffu;
    if(!count)return {false,0u,0u};
    const auto time=memory.load32(game_time,globals::game_time);
    const auto tick=memory.load32(Access::seconds_per_tick,at(time,native_tiny_leaves::tick_seconds::seconds_per_tick));
    const auto scaled=sse::multiply(tick,memory.load32(scale,globals::tick_scale));
    const auto target=memory.load32(out,frame.out_slot());
    const auto counted=sse::bits(_mm_cvtsi32_ss(_mm_setzero_ps(),int(widen8(count))));
    const auto value=sse::multiply(scaled,counted);
    memory.store32(Access::store,target,value);
    return {true,value,widen8(count)};
}
} // namespace tick_seconds
} // namespace halo2::native_tiny_leaves
