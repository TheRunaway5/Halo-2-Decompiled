#pragma once
// Native TU1.5 object lights, entered at 1181F0 (scene device constructor
// unit): the lights an object carries, placed on its markers.
//
//   object_lights::update   1181F0(EAX = object; active) -> RET 4
//
// An object (through the object headers at [4ECBD0]) with a light block
// (+0xB4 not NONE) whose model state 212C70 fills (ESI a state on the stack:
// its default transform +8, its model +0x48, its node matrices +0x50) walks
// the block's lights (the 0xA0-byte blocks at [52BE5C]: a count +0x74,
// 0x60-byte entries at +0x70). A light whose owner's type (its virtual
// method +0x18, ECX the owner +0x3C of the entry's light +0x40) is 6 and
// whose entry names markers (a count +0x4C, the first marker's byte at
// [+0x48]) takes that marker (0x90-byte markers at the model's +0x3C): when
// the object is active, 1D9080 and 1D9100 (the light's index, the block
// pushed) place and aim it and 1D8ED0 attaches it, else 1D9190 holds it;
// either at the marker's node matrix (0x34 bytes, by its int16 +0) or, with
// none, at the default transform. Finally the object's bit 6 of +0x1CC
// records that it is not active.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (the object, the
// model state) and its argument; the boundary adapter
// (native_tu15_object_lights.hpp) maps the role to the original instruction
// and owns the ABI of the children. The values the original keeps in
// callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_lights {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;
using native_object_update::widen8;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,light_blocks=0x52be5cu,light_scale=0x474d04u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,block_size=0xa0u,entry_size=0x60u,
    marker_size=0x90u,matrix_size=0x34u,light_type=6u,type_method=0x18u;
namespace object {inline constexpr std::uint32_t light_block=0xb4u,flags=0x1ccu;inline constexpr std::uint32_t inactive=0x40u;}
namespace block {inline constexpr std::uint32_t entries=0x70u,count=0x74u;}
namespace entry {inline constexpr std::uint32_t light=0x40u,markers=0x48u,marker_count=0x4cu;}
namespace light {inline constexpr std::uint32_t owner=0x3cu;}
namespace model {inline constexpr std::uint32_t markers=0x3cu;}
// The frame's slots, by offset from the return address.
namespace local {inline constexpr std::int32_t object=-0x58,state=-0x54,default_transform=-0x4c,model=-0xc,matrices=-4;}

namespace object_lights {
enum class Access {headers,datums,object_address,light_block,object_store,light_block_again,light_blocks,blocks_data,light_count,
    entries,entry_light,light_owner,owner_vtable,type_method_load,entries_again,marker_count,marker_list,marker_byte,model_load,
    model_markers,active_argument,scale_place,scale_aim,marker_node,matrices_load,object_again,count_again,active_flag,object_flags,
    object_flags_store};
enum class Outcome {no_block,no_model,updated};
// The original's EBX (the entry's offset), EBP (the object, then the marker), ESI (the light block) and EDI (the light's
// index) across the children.
struct Kept {std::uint32_t offset,held,block,index;};
// Frame:
//   argument(), local(offset)                   the argument's slot and the frame's slots now
//   indexing(object), saving(n), restoring(n)   MOV ECX,EAX; AND; LEA; the saved registers
//   block_tested(block), count_tested(count), type_tested(type), markers_tested(count), node_tested(node)
//   model_state(kept) -> AL                     212C70(EAX = object, ESI = the state), refreshes kept
//   type(owner, method, kept) -> EAX            the owner's virtual +0x18 (ECX the owner) through the dispatch
//   place(scale, kept), aim(scale, kept)        1D9080(EAX = scale, ECX = index; block) and 1D9100(ECX = scale, EAX = index; block)
//   attach(transform, kept), hold(transform, kept)   1D8ED0(EAX = transform, ECX = index; block) and 1D9190(EAX = transform)
//   advanced(index, count, offset), finished(active, flags)
template<class Memory,class Frame> Outcome update(Memory &memory,Frame &frame,std::uint32_t object_handle,Kept kept) {
    using enum Access;
    const auto datums_address=memory.load32(datums,at(memory.load32(headers,globals::object_headers),datum_data));
    frame.indexing(object_handle);
    kept.held=memory.load32(object_address,datums_address+header_size*(object_handle&0xffffu)+header_address);
    const auto block_handle=memory.load32(Access::light_block,at(kept.held,object::light_block));
    frame.block_tested(block_handle);
    memory.store32(object_store,frame.local(local::object),kept.held);
    if(block_handle==none)return Outcome::no_block;
    frame.saving(0u);
    if(!(frame.model_state(object_handle,kept)&0xffu))return Outcome::no_model;
    const auto block_offset=block_size*(memory.load32(light_block_again,at(kept.held,object::light_block))&0xffffu);
    const auto data=memory.load32(blocks_data,at(memory.load32(light_blocks,globals::light_blocks),datum_data));
    const auto count=memory.load32(light_count,data+block_offset+block::count);
    kept.block=block_offset+data;
    frame.saving(1u);
    kept.index=0u;
    frame.count_tested(count,kept);
    if(signed_value(count)>0) {
        frame.saving(2u);
        kept.offset=0u;
        std::uint32_t count_now;
        do {
            const auto light_address=memory.load32(entry_light,memory.load32(entries,at(kept.block,block::entries))+kept.offset+entry::light);
            const auto owner=memory.load32(light_owner,at(light_address,light::owner));
            const auto method=memory.load32(type_method_load,at(memory.load32(owner_vtable,owner),type_method));
            const auto type_value=frame.type(owner,method,kept);
            frame.type_tested(type_value);
            if(type_value==light_type) {
                const auto entry_address=memory.load32(entries_again,at(kept.block,block::entries))+kept.offset;
                const auto markers=memory.load32(marker_count,at(entry_address,entry::marker_count));
                frame.markers_tested(entry_address,markers);
                if(signed_value(markers)>0) {
                    const auto marker=widen8(memory.load8(marker_byte,memory.load32(marker_list,at(entry_address,entry::markers))));
                    const auto model_value=memory.load32(model_load,frame.local(local::model));
                    const auto markers_base=memory.load32(model_markers,at(model_value,model::markers));
                    const auto active=memory.load8(active_argument,frame.argument());
                    kept.held=marker_size*marker+markers_base;
                    frame.marker_chosen(kept,active);
                    // The marker's node matrix, or the default transform.
                    const auto transform_of=[&]() {
                        const auto node=widen16(memory.load16(marker_node,kept.held));
                        frame.node_tested(node);
                        if(node==none)return frame.local(local::default_transform);
                        return memory.load32(matrices_load,frame.local(local::matrices))+matrix_size*node;
                    };
                    if(active) {
                        frame.place(memory.load32(scale_place,globals::light_scale),kept);
                        frame.aim(memory.load32(scale_aim,globals::light_scale),kept);
                        frame.attach(transform_of(),kept);
                    } else frame.hold(transform_of(),kept);
                    kept.held=memory.load32(object_again,frame.local(local::object));
                }
            }
            count_now=memory.load32(count_again,at(kept.block,block::count));
            ++kept.index;kept.offset+=entry_size;
            frame.advanced(kept,count_now);
        } while(signed_value(kept.index)<signed_value(count_now));
        frame.restoring(2u);
    }
    const auto active=memory.load8(active_flag,frame.argument());
    const auto flags_value=memory.load32(object_flags,at(kept.held,object::flags));
    frame.finished(active);
    const auto now=active?flags_value&~object::inactive:flags_value|object::inactive;
    frame.flagged(now,active);
    memory.store32(object_flags_store,at(kept.held,object::flags),now);
    return Outcome::updated;
}
} // namespace object_lights
} // namespace halo2::native_object_lights
