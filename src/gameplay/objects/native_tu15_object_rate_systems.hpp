#pragma once
// Native TU1.5 object rate, entered at DFE30 (physics live support unit): an
// object's rate per second, where it has one.
//
//   object_rate::of   DFE30(EDI = object handle; rate*) -> AL, RET 4
//
// DFD20 answers the object's value (XMM0). An object whose definition (the
// tag instances at [4F0414], 16 bytes, +8; by the object's tag, +0) does
// not carry bit 1 of its flags (+0x264), that E65A0 or else E3310 accepts,
// and whose value is not zero, has the rate: the definition's span (+0x268
// less +0x26C) times the ticks per second (int16 [51DF54] +2) times the
// value. The answer is 1 with the rate written, 0 otherwise.
//
// The single-precision arithmetic is the original's scalar SSE on the live
// MXCSR. This is the game logic only. The game's memory is the retained
// TU1.5 bank, reached through a Memory object with typed loads and stores;
// every access names its role (Access), and the boundary adapter
// (native_tu15_object_rate.hpp) maps the role to the original instruction
// and owns the ABI of the children; the definition, the handle and the value
// the original keeps in ESI, EDI and XMM1 across them are a Kept, refreshed
// from the registers each returned.
#include "physics/runtime/native_tu15_perpendicular_systems.hpp"
#include <cstdint>

namespace halo2::native_object_rate {
using native_object_update::at;
using native_object_update::widen16;
using native_perpendicular::unequal;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t objects=0x4ecbd0u;   // DataArray* of 12-byte rows (+8 the object)
inline constexpr std::uint32_t tags=0x4f0414u;      // 16-byte tag instances (+8 the definition)
inline constexpr std::uint32_t zero=0x46a0e8u;      // float 0
inline constexpr std::uint32_t time=0x51df54u;      // int16 +2: the ticks per second
}
inline constexpr std::uint32_t datum_data=0x44u,row_size=12u,row_object=8u,tag_size=16u,tag_definition=8u,ticks_per_second=2u;
inline constexpr std::uint32_t unrated=2u;          // definition flag bit 1
namespace definition {inline constexpr std::uint32_t flags=0x264u,span_end=0x268u,span_start=0x26cu;}
// The CVTSI2SS of a 32-bit integer on the live MXCSR.
inline std::uint32_t converted(std::int32_t value){return sse::bits(_mm_cvtsi32_ss(_mm_setzero_ps(),value));}

namespace object_rate {
enum class Access {objects,object_data,object,tag,tags,definition,flags,zero,time,ticks,span_end,span_start,rate_slot,rate_store};
enum class Outcome {unrated,refused,still,rated};
struct Kept {std::uint32_t definition,handle,value;};   // ESI, EDI, XMM1
// Frame:
//   value(kept) -> XMM0             DFD20
//   accepted(kept, k) -> AL         E65A0 (k 0), E3310 (k 1)
//   argument()                      the rate's slot
//   rated(rate)                     the rate's registers
template<class Memory,class Frame> Outcome of(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto data=memory.load32(object_data,at(memory.load32(objects,globals::objects),datum_data));
    const auto object_address=memory.load32(object,data+row_size*(handle&0xffffu)+row_object);
    const auto tag_handle=memory.load32(tag,at(object_address,0u));
    const auto tag_array=memory.load32(tags,globals::tags);
    Kept kept{memory.load32(Access::definition,tag_array+tag_size*(tag_handle&0xffffu)+tag_definition),handle,0u};
    kept.value=frame.value(kept);
    if(memory.load32(flags,at(kept.definition,definition::flags))&unrated)return Outcome::unrated;
    if(!(frame.accepted(kept,0u)&0xffu) && !(frame.accepted(kept,1u)&0xffu))return Outcome::refused;
    if(!unequal(kept.value,memory.load32(Access::zero,globals::zero)))return Outcome::still;
    const auto ticks_value=widen16(memory.load16(Access::ticks,at(memory.load32(Access::time,globals::time),ticks_per_second)));
    const auto end=memory.load32(span_end,at(kept.definition,definition::span_end));
    const auto span=sse::subtract(end,memory.load32(span_start,at(kept.definition,definition::span_start)));
    const auto rate_address=memory.load32(rate_slot,frame.argument());
    const auto rate=sse::multiply(sse::multiply(span,converted(std::int32_t(ticks_value))),kept.value);
    frame.rated(rate);
    memory.store32(rate_store,rate_address,rate);
    return Outcome::rated;
}
} // namespace object_rate
} // namespace halo2::native_object_rate
