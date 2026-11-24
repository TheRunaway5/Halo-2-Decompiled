#pragma once
// Native TU1.5 chain byte propagation, entered at 17FAF0 (combat action
// unit): a chain head's byte is copied along its chain.
//
//   chain_byte::propagate   17FAF0(EDX = record handle)
//
// The records live in the data array at 4F7220 (64-byte records). A record
// that heads its chain (its +0x38 names itself) has its byte at +0x27 and the
// next record at +0x3C. Unless 17FB60 accepts the head (AL nonzero), every
// record after it along the chain takes the head's byte as the top byte of
// its +0x24 word (the record's own +0x27), until the chain ends (NONE).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_chain_byte.hpp) maps the role to the original instruction and
// owns the ABI of the child 17FB60; the byte and the next record the
// original keeps in callee-saved registers across it are a Kept, refreshed
// from the registers it returned. The array's datums are read again for
// every step.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_chain_byte {
using native_object_update::at;
using native_object_update::none;

namespace globals {inline constexpr std::uint32_t records=0x4f7220u;}   // DataArray* of 64-byte records
inline constexpr std::uint32_t datum_data=0x44u,record_shift=6u;
namespace record {inline constexpr std::uint32_t word=0x24u,byte=0x27u,head=0x38u,next=0x3cu;}

namespace chain_byte {
enum class Access {array,data,head,group,next,array_again,data_reload,element_word,element_next,element_store};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t next;    // ESI: the next record's handle
    std::uint32_t group;   // EDI: the head's byte
};
enum class Exit {not_head,stopped,walked};
// Frame:
//   accept(kept)   17FB60 for the head; returns AL; kept as it returned
template<class Memory,class Frame> Exit propagate(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(array,globals::records);
    const auto head_record=((handle&0xffffu)<<record_shift)+memory.load32(data,at(table,datum_data));
    if(memory.load32(head,at(head_record,record::head))!=handle)return Exit::not_head;
    Kept kept{};
    kept.group=memory.load8(group,at(head_record,record::byte));
    kept.next=memory.load32(next,at(head_record,record::next));
    if(frame.accept(kept)&0xffu)return Exit::stopped;
    if(kept.next==none)return Exit::stopped;
    const auto table_again=memory.load32(array_again,globals::records);
    for(;;) {
        const auto element=((kept.next&0xffffu)<<record_shift)+memory.load32(data_reload,at(table_again,datum_data));
        const auto value=memory.load32(element_word,at(element,record::word));
        kept.next=memory.load32(element_next,at(element,record::next));
        memory.store32(element_store,at(element,record::word),(value&0xffffffu)|(kept.group<<24));
        if(kept.next==none)return Exit::walked;
    }
}
} // namespace chain_byte
} // namespace halo2::native_chain_byte
