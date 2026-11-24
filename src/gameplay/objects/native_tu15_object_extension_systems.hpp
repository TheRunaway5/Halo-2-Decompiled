#pragma once
// Native TU1.5 object extension, entered at BE3B0 (physics live support
// unit): a block appended to an object's memory, its place recorded.
//
//   object_extension::append   BE3B0(EAX = object; field offset, size, alignment bits) -> AL, RET 0xC
//
// The field (at the int16 offset, in the object's data) records a block's
// size (word +0) and offset (word +2). A size of 0 records none (size 0,
// offset NONE). Otherwise the object's memory (its header's data pointer,
// +8, through the object headers at [4ECBD0]) grows by the size plus the
// alignment's mask (1 << bits, less one) in the object memory pool
// ([4EABBC]) through 191130 (the new size in EAX, the data pointer's
// address in EBX; the pool and two zeros pushed). When it does, the header's
// size (word +6) takes the growth, the field the size and the old size
// rounded up to the alignment, and the grown bytes are zeroed (REP STOSD,
// then REP STOSB, in the string direction flag's direction). The answer is
// whether the block is recorded, else the frame's answer byte (0).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (the header's
// offset and the answer byte); the boundary adapter
// (native_tu15_object_extension.hpp) maps the role to the original
// instruction and owns the ABI of 191130. The values the original keeps in
// callee-saved registers across it are a Kept, refreshed from the registers
// it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_extension {
using native_object_update::at;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,object_pool=0x4eabbcu;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u;
namespace header {inline constexpr std::uint32_t size=6u,data=8u;}
namespace field {inline constexpr std::uint32_t size=0u,offset=2u;}
namespace argument {inline constexpr std::uint32_t field=0u,size=1u,alignment=2u;}
// The frame's slots, by offset from the return address: the answer byte, and the header's offset.
namespace local {inline constexpr std::int32_t answer=-5,header=-4;}

namespace object_extension {
enum class Access {headers,datums,header_store,size_argument,answer_clear,object_address,empty_field,empty_size,empty_offset,
    alignment_argument,pool,header_size,grown_headers,old_size,size_store,grown_datums,field_argument,header_again,grown_object,
    field_offset,size_again,field_size,data_address,zero_word,zero_byte,answer_load};
enum class Outcome {empty,appended,failed};
// The original's EBX (the data pointer's address), EBP (the header), ESI (the mask) and EDI (the growth) across 191130.
struct Kept {std::uint32_t data_pointer,header,mask,growth;};
// Frame:
//   argument(k), local(offset), saving(n)        the slots now; PUSH EBP, then EBX, ESI, EDI
//   indexing(object), size_tested(size)          AND EAX,FFFF; LEA; SHL EAX,2; TEST AX,AX
//   masked(bits) -> 1 << bits, grown(size, mask, header size) -> growth   MOV ESI,1; SHL ESI,CL; PUSH 0; DEC ESI; PUSH 0;
//                                                ADD EDI,ESI; ADD EAX,EDI
//   field_found(field, object) -> address, summed(old, growth) -> size   ADD EAX,...; LEA EAX,[EDX+EDI]
//   resize(new size, pool, kept) -> AL           PUSH 0, 0, pool; LEA EBX; 191130 (RET C), refreshes kept
//   placed(field, old size, mask) -> offset      LEA ECX,[ESI+EDX]; NOT ESI; AND ECX,ESI
//   zeroing(growth, old, object) -> destination, step(), bytes(growth)   the REP STOSD/STOSB registers
//   restoring(n)                                 POP EDI, ESI, EBX; (MOV AL,1); POP EBP
template<class Memory,class Frame> std::uint32_t append(Memory &memory,Frame &frame,std::uint32_t object_handle,Outcome &outcome) {
    using enum Access;
    Kept kept{};
    const auto datums_address=memory.load32(datums,at(memory.load32(headers,globals::object_headers),datum_data));
    const auto offset=frame.indexing(object_handle);
    frame.saving(0u);
    memory.store32(header_store,frame.local(local::header),offset);
    kept.header=datums_address+offset;
    const auto size=memory.load16(size_argument,frame.argument(argument::size));
    frame.size_tested(size);
    memory.store8(answer_clear,frame.local(local::answer),0u);
    if(!size) {
        // No block: size 0, offset NONE.
        const auto object_data=memory.load32(Access::object_address,at(kept.header,header::data));
        const auto field_value=widen16(memory.load16(empty_field,frame.argument(argument::field)));
        const auto field_address=frame.field_found(field_value,object_data);
        memory.store16(empty_offset,at(field_address,field::offset),0xffffu);
        memory.store16(empty_size,at(field_address,field::size),0u);
        outcome=Outcome::empty;
        return 1u;
    }
    const auto bits=memory.load8(alignment_argument,frame.argument(argument::alignment));
    frame.saving(1u);
    kept.mask=frame.masked(bits)-1u;
    const auto pool_address=memory.load32(pool,globals::object_pool);
    const auto current=widen16(memory.load16(Access::header_size,at(kept.header,header::size)));
    kept.growth=frame.grown(widen16(size),kept.mask,current);
    kept.data_pointer=kept.header+header::data;
    if(!(frame.resize(current+kept.growth,pool_address,kept)&0xffu)) {
        outcome=Outcome::failed;
        const auto answer=memory.load8(answer_load,frame.local(local::answer));
        frame.restoring(0u);
        return answer;
    }
    // Grown: the header's size, the field's size and aligned offset, the grown bytes zeroed.
    const auto headers_now=memory.load32(grown_headers,globals::object_headers);
    const auto old=memory.load16(old_size,at(kept.header,header::size));
    memory.store16(size_store,at(kept.header,header::size),frame.summed(old,kept.growth));
    const auto datums_now=memory.load32(grown_datums,at(headers_now,datum_data));
    const auto field_value=widen16(memory.load16(field_argument,frame.argument(argument::field)));
    const auto header_offset=memory.load32(header_again,frame.local(local::header));
    const auto field_address=frame.field_found(field_value,memory.load32(grown_object,datums_now+header_offset+header::data));
    memory.store16(field_offset,at(field_address,field::offset),frame.placed(old,kept.mask));
    memory.store16(field_size,at(field_address,field::size),memory.load16(size_again,frame.argument(argument::size)));
    const auto object_data=memory.load32(data_address,kept.data_pointer);
    auto to=frame.zeroing(kept.growth,widen16(old),object_data);
    const auto step=frame.step();
    for(std::uint32_t word=0;word<(kept.growth>>2);++word,to+=4u*step)memory.store32(zero_word,to,0u);
    frame.bytes(kept.growth);
    for(std::uint32_t byte=0;byte<(kept.growth&3u);++byte,to+=step)memory.store8(zero_byte,to,0u);
    frame.restoring(1u);
    outcome=Outcome::appended;
    return 1u;
}
} // namespace object_extension
} // namespace halo2::native_object_extension
