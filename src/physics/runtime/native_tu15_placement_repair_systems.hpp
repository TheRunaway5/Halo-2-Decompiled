#pragma once
// Native TU1.5 placement repair, entered at 175380 (physics live support
// unit): a placement record the placement check (175300) refuses, repaired.
//
//   placement_repair::repair   175380(EAX = record)
//
// A sane record is left alone. Otherwise its unit vectors (+0x2C, +0x38)
// that B1720 refuses become the defaults ([474D08] and [474D10] point at
// them); its two points (six floats from +4) are clamped to the world bounds
// ([44EA9C] to [44EAA0]); its vector at +0x44 that B16C0 finds not finite
// becomes zero ([474D04] points at it); its scalar +0x28 is clamped to
// [55D6E8] to [55D6EC] and its scalar +0x24 to 0 to [55D6F0]. Each clamp
// is the original's COMISS pair, in its order (a NaN stays). A record the
// check still refuses goes on to 1722C0 (EDX the record) in a tail jump.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_placement_repair.hpp) maps the role to the original
// instruction and owns the ABI of the children and the tail. The values the
// original keeps in callee-saved registers across them are a Kept, refreshed
// from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_placement_repair {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t first_default=0x474d08u,second_default=0x474d10u,zero_vector=0x474d04u,least=0x44ea9cu,
    most=0x44eaa0u,second_least=0x55d6e8u,second_most=0x55d6ecu,first_most=0x55d6f0u;}   // pointers, then floats
namespace record {inline constexpr std::uint32_t points=4u,first_scalar=0x24u,second_scalar=0x28u,first_vector=0x2cu,second_vector=0x38u,
    finite_vector=0x44u;}
inline constexpr std::uint32_t point_floats=6u,vector_words=3u,zero_float=0u;

namespace placement_repair {
enum class Access {first_pointer,first_read,first_store,second_pointer,second_read,second_store,least,point,most,point_store,point_again,
    point_last,zero_pointer,zero_read,zero_store,second_least,second_scalar,second_again,second_most,second_scalar_store,first_scalar,
    first_again,first_most,first_scalar_store};
enum class Outcome {sane,repaired,reported};
// The original's EBX (the record), ESI and EDI (the parts) across the children.
struct Kept {std::uint32_t record,first,second;};
// Frame:
//   saving(n), restoring()                       PUSH EBX (MOV EBX,EAX), then ESI and EDI; POP EDI, ESI
//   sane(kept) -> AL                             175300(EBX = record), refreshes kept
//   unit_vectors(kept) -> AL                     LEA EDI,[EBX+38]; LEA ESI,[EBX+2C]; B1720, refreshes kept
//   finite(kept) -> AL                           B16C0(EAX = ESI = the vector), refreshes kept
//   xmm(n, value), compared(a, b), zeroed()      the clamps' XMM registers and COMISS flags; XORPS XMM0
//   copied(n, value)                             the default and zero copies' registers
template<class Memory,class Frame> Outcome repair(Memory &memory,Frame &frame,std::uint32_t record_address) {
    using enum Access;
    Kept kept{record_address,0u,0u};
    frame.saving(0u);
    if(frame.sane(kept)&0xffu)return Outcome::sane;
    frame.saving(1u);
    // The unit vectors: the defaults when refused.
    if(!(frame.unit_vectors(kept)&0xffu)) {
        const auto first=memory.load32(first_pointer,globals::first_default);
        std::uint32_t second=0u;
        for(std::uint32_t word=0;word<vector_words;++word) {
            const auto value=memory.load32(first_read,first+4u*word);
            frame.copied(word,value);
            if(word==1u){second=memory.load32(second_pointer,globals::second_default);frame.copied(3u,second);}
            memory.store32(first_store,kept.first+4u*word,value);
        }
        for(std::uint32_t word=0;word<vector_words;++word) {
            const auto value=memory.load32(second_read,second+4u*word);
            frame.copied(4u+word,value);
            memory.store32(second_store,kept.second+4u*word,value);
        }
    }
    // The points, clamped to the world bounds; each clamp's result stored once the next is compared.
    const auto low=memory.load32(least,globals::least);
    frame.xmm(0u,low);
    std::uint32_t high=0u,previous=0u;
    for(std::uint32_t k=0;k<point_floats;++k) {
        const auto address=kept.record+record::points+4u*k;
        const auto value=memory.load32(point,address);
        frame.compared(low,value);
        if(k==0u){high=memory.load32(most,globals::most);frame.xmm(2u,high);}
        else memory.store32(point_store,address-4u,previous);
        const unsigned result_register=k+1u<point_floats?1u:0u;
        if(!sse::not_above(low,value)){previous=low;frame.xmm(result_register,low);continue;}
        const auto again=memory.load32(point_again,address);
        frame.xmm(result_register,again);
        frame.compared(again,high);
        frame.xmm(result_register,high);
        if(!sse::not_above(again,high)){previous=high;continue;}
        previous=memory.load32(point_last,address);
        frame.xmm(result_register,previous);
    }
    frame.finite_vector(kept);
    memory.store32(point_store,kept.record+record::points+4u*(point_floats-1u),previous);
    // The vector at +0x44: zero when not finite.
    if(!(frame.finite(kept)&0xffu)) {
        const auto zero=memory.load32(zero_pointer,globals::zero_vector);
        for(std::uint32_t word=0;word<vector_words;++word) {
            const auto value=memory.load32(zero_read,zero+4u*word);
            frame.copied(8u+word,value);
            memory.store32(zero_store,kept.first+4u*word,value);
        }
    }
    // The scalars: +0x28 to its range, +0x24 to 0 and its most.
    const auto second_low=memory.load32(second_least,globals::second_least);
    frame.xmm(0u,second_low);
    const auto second_value=memory.load32(second_scalar,at(kept.record,record::second_scalar));
    frame.compared(second_low,second_value);
    frame.restoring();
    auto clamped=second_low;
    if(sse::not_above(second_low,second_value)) {
        const auto again=memory.load32(second_again,at(kept.record,record::second_scalar));
        frame.xmm(1u,again);
        const auto most_value=memory.load32(second_most,globals::second_most);
        frame.xmm(0u,most_value);
        frame.compared(again,most_value);
        clamped=most_value;
        if(sse::not_above(again,most_value)){clamped=again;frame.xmm(0u,again);}
    }
    memory.store32(second_scalar_store,at(kept.record,record::second_scalar),clamped);
    frame.zeroed();
    const auto first_value=memory.load32(first_scalar,at(kept.record,record::first_scalar));
    frame.compared(zero_float,first_value);
    clamped=zero_float;
    if(sse::not_above(zero_float,first_value)) {
        const auto again=memory.load32(first_again,at(kept.record,record::first_scalar));
        frame.xmm(1u,again);
        const auto most_value=memory.load32(first_most,globals::first_most);
        frame.xmm(0u,most_value);
        frame.compared(again,most_value);
        clamped=most_value;
        if(sse::not_above(again,most_value)){clamped=again;frame.xmm(0u,again);}
    }
    memory.store32(first_scalar_store,at(kept.record,record::first_scalar),clamped);
    return frame.sane(kept)&0xffu?Outcome::repaired:Outcome::reported;
}
} // namespace placement_repair
} // namespace halo2::native_placement_repair
