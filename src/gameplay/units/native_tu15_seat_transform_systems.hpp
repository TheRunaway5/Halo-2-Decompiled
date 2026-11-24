#pragma once
// Native TU1.5 seat transform, entered at D35C0 (physics live support unit):
// a unit's transform for its seat, and its seat's settings.
//
//   seat_transform::fill   D35C0(EAX = unit; transform*, settings*) -> AL, RET 8
//
// The transform's scale (+0) is 1 (46A0D0). A unit (through the object
// headers at [4ECBD0]) seated in its parent (+0x14 not NONE, seat word
// +0x1FC not NONE) takes the seat's marker (BAD60: the parent, the seat's
// marker name +8 and 1 pushed, CL 0, EBX a marker frame on the stack; the
// seat is 0xB0 bytes at the parent's definition +0x1CC, through the tag
// instances at 4F0414): with none it answers the frame's byte (0); with one,
// the marker's matrix (13 words from +0x38 of its frame) is the transform,
// BBE00 (EAX the parent's parent, ESI the transform's +0x28) completes it,
// and the seat's 20 bytes from +0x24 are the settings. Any other unit has
// BBE00 (EAX the unit, ESI the transform's +0x28) complete its transform,
// takes its vectors +0x70 and +0x7C as the transform's +4 and +0x1C, and
// its definition's 20 bytes from +0xF0 as the settings; it answers 1.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (the object, the
// answer byte, the marker frame); the boundary adapter
// (native_tu15_seat_transform.hpp) maps the role to the original instruction
// and owns the ABI of the children. The values the original keeps in
// callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_seat_transform {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,one=0x46a0d0u,tag_instances=0x4f0414u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_size=16u,tag_data=8u,seat_size=0xb0u,
    matrix_words=13u,settings_words=5u,marker_mode=1u;
namespace object {inline constexpr std::uint32_t parent=0x14u,forward=0x70u,up=0x7cu,seat=0x1fcu;}
namespace definition {inline constexpr std::uint32_t seats=0x1ccu,settings=0xf0u;}
namespace seat {inline constexpr std::uint32_t marker=8u,settings=0x24u;}
namespace transform {inline constexpr std::uint32_t forward=4u,up=0x1cu,completed=0x28u;}
namespace argument {inline constexpr std::uint32_t transform=0u,settings=1u;}
// The frame's slots, by offset from the return address.
namespace local {inline constexpr std::int32_t answer=-0x75,object=-0x74,marker=-0x70,matrix=-0x38;}

namespace seat_transform {
enum class Access {headers,one,datums,object_address,transform_argument,scale_store,parent,object_store,answer_clear,seat,
    parent_again,datums_again,parent_object,parent_tag,instances,parent_definition,seats,marker_name,answer_load,object_load,
    matrix_read,matrix_write,transform_again,parent_of_parent,settings_argument,settings_read,settings_write,unit_tag,
    unit_instances,unit_definition,vector_read,vector_write};
enum class Outcome {no_marker,seated,unseated};
// The original's EBX (the unit, then its parent, then the marker frame or the up vector), EBP (the seat, then the
// definition), ESI and EDI (the transform) across the children.
struct Kept {std::uint32_t held,record,source,out;};
// Frame:
//   argument(k), local(offset)                 the argument slots and the frame's slots now
//   saving(n), indexing(unit), parent_tested(parent), seat_tested(seat)   the saved registers and tests
//   marker(name, kept) -> AX                   BAD60(kept.held the parent, name, 1; CL 0, EBX the marker frame,
//                                              EBP kept.record the seat), refreshes kept
//   copying(from, to), step()                  the matrix's REP MOVSD registers and direction
//   complete(handle, base, offset, kept)       BBE00(EAX = handle, ESI = base + offset), refreshes kept
//   vector(which, word, value, kept), settings_start(record, offset), settings(word, value)
//                                              the copies' registers, and the pops between their last loads and stores
template<class Memory,class Frame> std::uint32_t fill(Memory &memory,Frame &frame,std::uint32_t unit_handle,Outcome &outcome) {
    using enum Access;
    Kept kept{};
    const auto headers_address=memory.load32(headers,globals::object_headers);
    const auto one_value=memory.load32(one,globals::one);
    frame.saving(0u,unit_handle);
    const auto datums_address=memory.load32(datums,at(headers_address,datum_data));
    kept.held=memory.load32(object_address,datums_address+header_size*(unit_handle&0xffffu)+header_address);
    frame.saving(1u,unit_handle);
    kept.out=memory.load32(transform_argument,frame.argument(argument::transform));
    memory.store32(scale_store,kept.out,one_value);
    const auto parent_handle=memory.load32(parent,at(kept.held,object::parent));
    frame.parent_tested(parent_handle);
    memory.store32(object_store,frame.local(local::object),kept.held);
    memory.store8(answer_clear,frame.local(local::answer),0u);
    // The settings' copy: five words, the original's pops between the last loads and stores.
    const auto settings_copy=[&](std::uint32_t from,std::uint32_t to) {
        for(std::uint32_t word=0;word<settings_words;++word) {
            const auto value=memory.load32(settings_read,from+4u*word);
            frame.settings(word,value);
            memory.store32(settings_write,to+4u*word,value);
        }
    };
    std::uint32_t seat_value=0xffffu;
    if(parent_handle!=none) {
        seat_value=memory.load16(Access::seat,at(kept.held,object::seat));
        frame.seat_tested(seat_value);
    }
    if(seat_value!=0xffffu) {
        // Seated: the seat's marker on the parent.
        kept.held=memory.load32(parent_again,at(kept.held,object::parent));
        const auto datums_now=memory.load32(datums_again,at(headers_address,datum_data));
        const auto parent_address=memory.load32(parent_object,datums_now+header_size*(kept.held&0xffffu)+header_address);
        const auto tag=memory.load32(parent_tag,parent_address);
        const auto instances_address=memory.load32(instances,globals::tag_instances);
        const auto seat_offset=seat_size*widen16(seat_value);
        const auto definition_value=memory.load32(parent_definition,tag_size*(tag&0xffffu)+instances_address+tag_data);
        const auto seats_address=memory.load32(seats,at(definition_value,definition::seats));
        const auto name=memory.load32(marker_name,seat_offset+seats_address+seat::marker);
        kept.record=seat_offset+seats_address;
        if(!(frame.marker(name,kept)&0xffffu)) {
            outcome=Outcome::no_marker;
            return memory.load8(answer_load,frame.local(local::answer));
        }
        const auto unit_address=memory.load32(object_load,frame.local(local::object));
        // The marker's matrix, as REP MOVSD copies it.
        auto from=frame.local(local::matrix),to=kept.out;
        frame.copying(from,to);
        const auto step=frame.step();
        for(std::uint32_t word=0;word<matrix_words;++word,from+=step,to+=step)memory.store32(matrix_write,to,memory.load32(matrix_read,from));
        const auto transform_value=memory.load32(transform_again,frame.argument(argument::transform));
        frame.complete(memory.load32(parent_of_parent,at(unit_address,object::parent)),transform_value,transform::completed,kept);
        const auto settings_address=memory.load32(settings_argument,frame.argument(argument::settings));
        frame.settings_start(kept.record,seat::settings);
        settings_copy(kept.record+seat::settings,settings_address);
        outcome=Outcome::seated;
        return 1u;
    }
    // Unseated: the unit's own transform.
    const auto tag=memory.load32(unit_tag,kept.held);
    const auto instances_address=memory.load32(unit_instances,globals::tag_instances);
    kept.record=memory.load32(unit_definition,tag_size*(tag&0xffffu)+instances_address+tag_data);
    frame.complete(unit_handle,kept.out,transform::completed,kept);
    for(std::uint32_t word=0;word<3u;++word) {
        const auto value=memory.load32(vector_read,kept.held+object::forward+4u*word);
        frame.vector(0u,word,value,kept);
        memory.store32(vector_write,kept.out+transform::forward+4u*word,value);
    }
    std::uint32_t settings_address=0u;
    for(std::uint32_t word=0;word<3u;++word) {
        const auto value=memory.load32(vector_read,kept.held+object::up+4u*word);
        frame.vector(1u,word,value,kept);
        if(word==1u)settings_address=memory.load32(settings_argument,frame.argument(argument::settings));
        memory.store32(vector_write,kept.out+transform::up+4u*word,value);
    }
    frame.settings_start(kept.record,definition::settings);
    settings_copy(kept.record+definition::settings,settings_address);
    outcome=Outcome::unseated;
    return 1u;
}
} // namespace seat_transform
} // namespace halo2::native_seat_transform
