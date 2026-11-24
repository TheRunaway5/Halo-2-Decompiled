#pragma once
// Native TU1.5 record sanity check, entered at 14FBC0 (physics live support
// unit): whether a record's fields are all within their ranges.
//
//   record_sanity::sane   14FBC0(ECX = record) -> AL
//
// No record is not sane. Otherwise, in the original's order:
//   +8, +4, +0x14, +0x18 finite; +0x14 within 0..1 (an unordered value fails);
//   +0xC, +0x10 finite and within [46A0DC]..1;
//   the bytes +0x20, +0x21 NONE or 0..3, and +0x20 (when not NONE) not +0x21;
//   the words +0x22 NONE or 0..1, +0x24 NONE or 0..3; the byte +0x1D at most 1;
//   the pair +0x50, +0x54 within 0..1 decides with the rest: when either is
//   nonzero (or unordered), or bit 0 of +0x4C is set, the pair must be in
//   range and +0x34 not NONE; otherwise the pair must be in range;
//   the word +0x28 within 0..8, and when nonzero +0x2C not NONE.
// Each finite test passes the value through the slot the entry PUSH ECX
// makes, as the original reads its bits.
//
// The comparisons are the original's scalar SSE (COMISS, and UCOMISS for
// the pair's zero tests) on the live MXCSR. This is the game logic only. The
// game's memory is the retained TU1.5 bank, reached through a Memory object
// with typed loads and stores; every access names its role (Access), and
// the boundary adapter (native_tu15_record_sanity.hpp) maps the role to the
// original instruction.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_perpendicular_systems.hpp"
#include <cstdint>

namespace halo2::native_record_sanity {
using native_object_update::at;
using native_object_update::none;
namespace sse=native_aim_clusters::sse;
using native_perpendicular::unequal;
namespace globals {inline constexpr std::uint32_t one=0x46a0d0u,low=0x46a0dcu;}   // floats
inline constexpr std::uint32_t exponent_bits=0x7f800000u,zero_bits=0u;
constexpr bool finite(std::uint32_t bits){return (bits&exponent_bits)!=exponent_bits;}
// A byte (width 1) or word (width 2) kind: NONE, or 0 to limit - 1 (signed).
constexpr std::int32_t signed_of(std::uint32_t value,unsigned width) {
    return width==1?std::int32_t(std::int8_t(value)):std::int32_t(std::int16_t(value));
}
constexpr bool kind_ok(std::uint32_t value,std::uint32_t none_value,std::int32_t limit,unsigned width) {
    return value==none_value || (signed_of(value,width)>=0 && signed_of(value,width)<limit);
}
namespace record {
inline constexpr std::uint32_t first_finite[]{8u,4u,0x14u,0x18u},weight=0x14u,bounded[]{0xcu,0x10u};
inline constexpr std::uint32_t first_kind=0x20u,second_kind=0x21u,third_kind=0x22u,fourth_kind=0x24u,flags=0x1du,
    pair=0x50u,pair_second=0x54u,marked=0x4cu,link=0x34u,mode=0x28u,mode_link=0x2cu;
}

namespace record_sanity {
enum class Access {
    finite_read,finite_store,finite_reload,weight,one,weight_again,bounded_read,bounded_store,bounded_reload,
    bounded_value,low,first_kind,second_kind,third_kind,fourth_kind,flags,pair_range,pair_zero,marked,link,mode,
    mode_link
};
// Frame:
//   present(record)              TEST ECX,ECX
//   slot()                       the stack slot
//   checking(step)               before each finite or bounded test (its registers)
//   compared(a,b,unordered)      COMISS / UCOMISS a,b (the flags; LAHF and
//                                TEST AH,44 after a UCOMISS)
//   kind_compared(v,none,limit,width)  CMP NONE; TEST; CMP limit (as far as they run)
//   kinds_compared(first,second) CMP AL,FF; CMP AL,DL
//   pair_in_range(in)            MOV DL,1 / XOR DL,DL
//   pair_tested()                TEST DL,DL
//   mode_address(a)              LEA EAX,[ECX+28]; TEST EAX,EAX
//   mode_compared(v)             TEST CX,CX; CMP CX,9; TEST CX,CX (as far as they run)
template<class Memory,class Frame> bool sane(Memory &memory,Frame &frame,std::uint32_t address) {
    using enum Access;
    frame.present(address);
    if(!address)return false;
    // A value passes through the slot; its bits must not be all-exponent.
    const auto finite_at=[&](std::uint32_t offset,Access read,Access store,Access reload) {
        memory.store32(store,frame.slot(),memory.load32(read,at(address,offset)));
        return finite(memory.load32(reload,frame.slot()));
    };
    unsigned step=0;
    for(const auto offset:record::first_finite) {
        frame.checking(step++);
        if(!finite_at(offset,finite_read,finite_store,finite_reload))return false;
    }
    const auto weight_value=memory.load32(weight,at(address,record::weight));
    frame.compared(weight_value,zero_bits,false);
    if(sse::below(weight_value,zero_bits))return false;
    const auto one_value=memory.load32(one,globals::one);
    const auto weight_reread=memory.load32(weight_again,at(address,record::weight));
    frame.compared(one_value,weight_reread,false);
    if(sse::below(one_value,weight_reread))return false;
    for(const auto offset:record::bounded) {
        frame.checking(step++);
        if(!finite_at(offset,bounded_read,bounded_store,bounded_reload))return false;
    }
    // +0xC and +0x10 within [46A0DC]..1 (the first read before the bound).
    const auto first_bounded=memory.load32(bounded_value,at(address,record::bounded[0]));
    const auto low_value=memory.load32(low,globals::low);
    for(unsigned k=0;k<2u;++k) {
        const auto value=k?memory.load32(bounded_value,at(address,record::bounded[1])):first_bounded;
        frame.compared(value,low_value,false);
        if(sse::below(value,low_value))return false;
        frame.compared(one_value,value,false);
        if(sse::below(one_value,value))return false;
    }
    // Kinds: NONE or within range, and the first two apart.
    const auto first=memory.load8(first_kind,at(address,record::first_kind));
    frame.kind_compared(first,0xffu,4,1);
    if(!kind_ok(first,0xffu,4,1))return false;
    const auto second=memory.load8(second_kind,at(address,record::second_kind));
    frame.kind_compared(second,0xffu,4,1);
    if(!kind_ok(second,0xffu,4,1))return false;
    frame.kinds_compared(first,second);
    if(first!=0xffu && first==second)return false;
    const auto third=memory.load16(third_kind,at(address,record::third_kind));
    frame.kind_compared(third,0xffffu,2,2);
    if(!kind_ok(third,0xffffu,2,2))return false;
    const auto fourth=memory.load16(fourth_kind,at(address,record::fourth_kind));
    frame.kind_compared(fourth,0xffffu,4,2);
    if(!kind_ok(fourth,0xffffu,4,2))return false;
    if(memory.load8(flags,at(address,record::flags))&0xfeu)return false;
    // The pair within 0..1.
    bool pair_ok=true;
    for(unsigned k=0;k<2u&&pair_ok;++k) {
        const auto value=memory.load32(pair_range,at(address,k?record::pair_second:record::pair));
        frame.compared(value,zero_bits,false);
        if(sse::below(value,zero_bits)){pair_ok=false;break;}
        frame.compared(one_value,value,false);
        if(sse::below(one_value,value))pair_ok=false;
    }
    frame.pair_in_range(pair_ok);
    // Either of the pair nonzero, or the mark, asks for the link as well.
    bool linked=false;
    for(unsigned k=0;k<2u&&!linked;++k) {
        const auto value=memory.load32(pair_zero,at(address,k?record::pair_second:record::pair));
        frame.compared(value,zero_bits,true);
        if(unequal(value,zero_bits))linked=true;
    }
    if(!linked)linked=memory.load8(marked,at(address,record::marked))&1u;
    frame.pair_tested();
    if(!pair_ok)return false;
    if(linked && memory.load32(link,at(address,record::link))==none)return false;
    // The mode word within 0..8; a nonzero mode needs its link.
    frame.mode_address(at(address,record::mode));
    if(!at(address,record::mode))return false;
    const auto mode_value=memory.load16(mode,at(address,record::mode));
    frame.mode_compared(mode_value);
    if(signed_of(mode_value,2)<0 || signed_of(mode_value,2)>=9)return false;
    if(mode_value && memory.load32(mode_link,at(address,record::mode_link))==none)return false;
    return true;
}
} // namespace record_sanity
} // namespace halo2::native_record_sanity
