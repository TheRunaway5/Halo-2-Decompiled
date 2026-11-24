#pragma once
// Native TU1.5 object summary, entered at BA910 (physics live support unit):
// a short record of an object written out.
//
//   object_summary::write   BA910(EDX = object handle, EDI = summary*)
//
// The summary holds the object's type (+0, the byte +0xAA widened to a
// word), its state bits (+2, from 16E290 when bit 9 of the object's +4 is
// set, otherwise zero), the vector at the object's +0x40 (+4) and its word
// +0x4C (+0x10).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_object_summary.hpp) maps the role to the original instruction
// and owns the ABI of 16E290; the object and the summary the original keeps
// in ESI and EDI across it are a Kept, refreshed from the registers it
// returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_summary {
using native_object_update::at;
using native_object_update::widen8;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
namespace object {inline constexpr std::uint32_t flags=4u,type=0xaau,vector=0x40u,word=0x4cu;}
inline constexpr std::uint32_t has_state_bits=1u<<9;
namespace summary {inline constexpr std::uint32_t type=0u,bits=2u,vector=4u,word=0x10u;}

namespace object_summary {
enum class Access {headers,header_datums,object_address,flags,type,bits_store,type_store,vector,vector_store,word,word_store};
struct Kept {std::uint32_t object,summary;};   // ESI, EDI
// Frame: state_bits(kept, handle) 16E290, refreshes kept, returns AX;
// component(k) before each part of the vector.
template<class Memory,class Frame> void write(Memory &memory,Frame &frame,std::uint32_t handle,std::uint32_t out) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    Kept kept{memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address),out};
    std::uint32_t bits=0;
    if(memory.load32(Access::flags,at(kept.object,object::flags))&has_state_bits)bits=frame.state_bits(kept,handle);
    const auto kind=widen8(memory.load8(Access::type,at(kept.object,object::type)));
    memory.store16(bits_store,at(kept.summary,summary::bits),bits);
    memory.store16(type_store,at(kept.summary,summary::type),kind);
    for(std::uint32_t k=0;k<3u;++k) {
        frame.component(k);
        memory.store32(vector_store,at(kept.summary,summary::vector+4u*k),memory.load32(Access::vector,at(kept.object,object::vector+4u*k)));
    }
    memory.store32(word_store,at(kept.summary,summary::word),memory.load32(Access::word,at(kept.object,object::word)));
}
} // namespace object_summary
} // namespace halo2::native_object_summary
