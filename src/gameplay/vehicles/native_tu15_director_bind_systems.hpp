#pragma once
// Native TU1.5 director bind, entered at 171FF0 (vehicle director unit): an
// object bound to a director record.
//
//   director_bind::bind   171FF0(EAX = object*, ECX = record index)
//
// After 175380, the record (0x358 bytes at 4F64A4) takes the object (+4)
// and clears its pending byte (+0xB4). A record not yet started (byte
// +0xB5) starts: the object's value +0x88 becomes zero, its flags (+0)
// gain bit 3 and its six words from +0x94 are cleared, each through the
// record's object pointer as the original reads it back.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_director_bind.hpp) maps the role to the original instruction
// and owns the ABI of the child; the record and the object, which the
// original keeps in ESI and EDI across it, are a Kept, refreshed from the
// registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_director_bind {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t records=0x4f64a4u;}   // 0x358-byte director records
inline constexpr std::uint32_t record_size=0x358u;
namespace record {inline constexpr std::uint32_t object=4u,pending=0xb4u,started=0xb5u;}
namespace object {inline constexpr std::uint32_t flags=0u,value=0x88u,words=0x94u;}
inline constexpr std::uint32_t bound=8u,word_count=6u,zero_bits=0u;

namespace director_bind {
enum class Access {started,object_store,pending_store,started_store,value_store,object,flags,flags_store,object_again,word_store};
enum class Outcome {bound,started};
struct Kept {std::uint32_t record,object;};   // ESI, EDI
// Frame: prepared(kept) 175380, refreshes kept.
template<class Memory,class Frame> Outcome bind(Memory &memory,Frame &frame,std::uint32_t object_address,std::uint32_t index) {
    using enum Access;
    Kept kept{globals::records+record_size*index,object_address};
    frame.prepared(kept);
    const bool already=memory.load8(Access::started,at(kept.record,record::started))&0xffu;
    memory.store32(object_store,at(kept.record,record::object),kept.object);
    memory.store8(pending_store,at(kept.record,record::pending),0u);
    if(already)return Outcome::bound;
    memory.store8(started_store,at(kept.record,record::started),1u);
    memory.store32(value_store,at(kept.object,object::value),zero_bits);
    const auto bound_object=memory.load32(Access::object,at(kept.record,record::object));
    memory.store32(flags_store,at(bound_object,object::flags),memory.load32(flags,at(bound_object,object::flags))|bound);
    const auto words=at(memory.load32(object_again,at(kept.record,record::object)),object::words);
    for(std::uint32_t k=0;k<word_count;++k)memory.store32(word_store,words+4u*k,0u);
    return Outcome::started;
}
} // namespace director_bind
} // namespace halo2::native_director_bind
