#pragma once
// Native TU1.5 object leaves:
//
//   indexed_record::address   172200(ECX = index) -> EAX
//       (physics live support, combat action) the 0x358-byte record at
//       4F655C for an index, when one is given (not NONE) and a structure
//       BSP is loaded ([474C24] not NONE); otherwise 0
//   attachment_root::find     BCF70(EAX = object handle) -> EAX
//       (physics live support, AI frame, sandbox) from the object up its
//       parents (+0x14) while bit 26 of its attachment flags (+4) is set:
//       the first object without it, or NONE when the chain ends first
//   ready_slot::any           111F00(EAX = object handle) -> AL
//       (physics live support) whether either of two 32-byte slots of the
//       object's block (at its signed offset +0x33E; slots at +0x9C and
//       +0xBC) is ready: a handle (+0) and a word (+6) set, bit 0 of byte
//       +0x12 set and neither bit 0 nor bit 3 of byte +0x11
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapters
// (native_tu15_indexed_record.hpp, native_tu15_attachment_root.hpp,
// native_tu15_ready_slot.hpp) map the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_object_leaves {
using native_object_update::ObjectData;
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;        // DataArray* of 12-byte object headers
inline constexpr std::uint32_t structure_bsp_index=0x474c24u;   // word: the loaded structure BSP, NONE for none
inline constexpr std::uint32_t records=0x4f655cu;               // 0x358-byte records by index
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;

namespace indexed_record {
inline constexpr std::uint32_t record_size=0x358u;
enum class Access {bsp_index};
enum class Outcome {no_index,no_bsp,record};
struct Result {Outcome outcome;std::uint32_t address;};
template<class Memory> Result address(Memory &memory,std::uint32_t index) {
    if(index==none)return {Outcome::no_index,0u};
    if((memory.load16(Access::bsp_index,globals::structure_bsp_index)&0xffffu)==0xffffu)return {Outcome::no_bsp,0u};
    return {Outcome::record,record_size*index+globals::records};
}
} // namespace indexed_record

namespace attachment_root {
inline constexpr std::uint32_t attached_bit=26u;
enum class Access {headers,header_datums,object_address,flags,parent};
enum class Outcome {none_given,settled,top};
struct Result {Outcome outcome;std::uint32_t handle;};
template<class Memory> Result find(Memory &memory,std::uint32_t handle) {
    using enum Access;
    if(handle==none)return {Outcome::none_given,handle};
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    for(;;) {
        const auto object=memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address);
        if(!((memory.load32(Access::flags,at(object,offsetof(ObjectData,attachment_flags)))>>attached_bit)&1u))
            return {Outcome::settled,handle};
        handle=memory.load32(Access::parent,at(object,offsetof(ObjectData,parent)));
        if(handle==none)return {Outcome::top,handle};
    }
}
} // namespace attachment_root

namespace ready_slot {
inline constexpr std::uint32_t block_offset=0x33eu,first_slot=0x9cu,slot_size=0x20u,slot_count=2u;
namespace slot {inline constexpr std::uint32_t handle=0u,word=6u,blocked=0x11u,armed=0x12u;}
inline constexpr std::uint32_t armed_bit=1u<<0,blocking_bits=(1u<<0)|(1u<<3);
enum class Access {headers,header_datums,object_address,block,handle,word,armed,blocked};
enum class Outcome {first_ready,second_ready,none_ready};
// Frame: slot(k) before the k-th slot.
template<class Memory,class Frame> Outcome any(Memory &memory,Frame &frame,std::uint32_t object_handle) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto object=memory.load32(object_address,datums+header_size*(object_handle&0xffffu)+header_address);
    const auto base=widen16(memory.load16(Access::block,at(object,block_offset)))+object;
    for(std::uint32_t k=0;k<slot_count;++k) {
        frame.slot(k);
        const auto entry=base+first_slot+slot_size*k;
        if(memory.load32(Access::handle,at(entry,slot::handle))==none)continue;
        if((memory.load16(Access::word,at(entry,slot::word))&0xffffu)==0xffffu)continue;
        if(!(memory.load8(Access::armed,at(entry,slot::armed))&armed_bit))continue;
        if(memory.load8(Access::blocked,at(entry,slot::blocked))&blocking_bits)continue;
        return k==0?Outcome::first_ready:Outcome::second_ready;
    }
    return Outcome::none_ready;
}
} // namespace ready_slot
} // namespace halo2::native_object_leaves
