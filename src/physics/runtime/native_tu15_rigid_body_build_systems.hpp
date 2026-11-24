#pragma once
// Native TU1.5 rigid body build, entered at 1DDEA0 (live support unit): a
// physics model's rigid body built for an object.
//
//   rigid_body_build::build   1DDEA0(EAX = types; object, model, scaled, ..., record)
//
// 1DE170 builds the shape (its outputs on the stack: the node index, the
// shape info block and its size, three flags, a value), defaulting the size
// to 0 and the flags to 0, 1, 0; without a shape the build fails with the
// low byte of EBX. Otherwise the node's entry (the model's +0x48, entries
// +0x3C, 0x90 bytes) names a matrix (word +0, -1 for the model's own at +8,
// else the model's matrices +0x50, 0x34 bytes), copied as a 4x4 transform
// (each row's fourth component 0) on the stack; the body's type is 7 for a
// fixed shape, else the type table's byte for the node; 1DEC80 answers a
// value. The entry's flags (word +0x18) or the object's type (the object
// headers' byte +3, bit 1) mark the body; 1DF380 creates it from the
// transform, the type (6 and 7 are fixed), the entry's +0x24 and +0x28 and
// whether the model has one entry (+0x38). The shape loses a reference
// (word +6): at none, 283450 (through the dispatch) keeps it at one or its
// virtual destructor (vtable +0, 1 pushed) frees it. A scaled build takes
// 1DF5B0 (0.85). The record takes the body (+0), the shape info block (+4,
// copied in the string direction), its size (+0x44) and the second flag
// (+0x48); with the first flag the object's flags (+4) take bit 14.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_rigid_body_build.hpp) maps the role to the original
// instruction and owns the ABI of the children (each argument passed with
// frame.pass, in the original's order). The values the original keeps in
// callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_rigid_body_build {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_type=3u,handle_mask=0xffffu,none=0xffffffffu;
inline constexpr std::uint32_t one=0x3f800000u,scaling=0x3f59999au,fixed_type=7u,static_type=6u,marked_types=2u;
namespace object {inline constexpr std::uint32_t flags=4u,handle=8u,built=0x4000u;}
namespace model {inline constexpr std::uint32_t matrix=8u,shapes=0x48u,matrices=0x50u,matrix_size=0x34u;}
namespace shapes {inline constexpr std::uint32_t count=0x38u,entries=0x3cu;}
namespace entry {inline constexpr std::uint32_t size=0x90u,matrix=0u,flags=0x18u,first=0x24u,second=0x28u;inline constexpr std::uint32_t marked=1u;}
namespace shape {inline constexpr std::uint32_t vtable=0u,references=6u;}
namespace record {inline constexpr std::uint32_t body=0u,info=4u,size=0x44u,flag=0x48u;}
// real_matrix4x3: the scale, then forward, left, up and position (three components each).
namespace matrix {inline constexpr std::uint32_t forward=4u,left=0x10u,up=0x1cu,position=0x28u;}
// The frame below the saved registers.
namespace local {inline constexpr std::uint32_t first_flag=0x14u,second_flag=0x15u,fixed_flag=0x16u,marked_flag=0x17u,type=0x18u,
    info_size=0x1cu,value=0x20u,shape_value=0x24u,saved=0x28u,shapes=0x2cu,position=0x30u,transform=0x40u,last_row=0x70u,
    shape_out=0x80u,info=0x90u,body_out=0xd0u,size=0xf4u;}
namespace argument {inline constexpr unsigned object=0u,model=1u,scaled=2u,third=3u,fourth=4u,fifth=5u,sixth=6u,record=7u;}

namespace rigid_body_build {
enum class Access {forwarded,info_size_default,fixed_default,second_default,first_default,node,model_again,shapes_load,entries_load,
    shapes_store,fixed_load,table_type,fourth_again,fourth_value,model_value,object_value,type_store,entry_matrix,value_store,
    model_own,model_indexed,model_matrices,component,component_store,position_row,position_store,entry_flags,object_load,handle_load,type_load,
    headers_load,datums_load,object_type,saved_store,marked_clear,saved_load,marked_set,entry_second,entry_first,value_load,
    shape_value_load,shapes_again,count_load,object_again,references_load,references_store,references_test,references_keep,
    shape_vtable,destructor_slot,scaled_load,record_load,info_size_load,body_store,info_word,info_word_store,info_byte,info_byte_store,second_load,
    flag_store,first_load,size_store,object_last,flags_load,flags_store};
enum class Outcome {failed,built,marked};
// The original's EBX (the entry, then the object), ESI (the shape) and EDI (the types, the type, then the body) across the children.
struct Kept {std::uint32_t entry,object,shape,types,type,body;};
// Frame: see native_tu15_rigid_body_build.hpp; pass(value) pushes the next argument.
template<class Memory,class Frame> Outcome build(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    const auto forward=[&](unsigned n){return memory.load32(forwarded,frame.argument(n));};
    // The shape built, its outputs defaulted.
    frame.framing();
    const auto fourth=forward(argument::fourth);
    frame.saving(kept);
    frame.pass(frame.local(local::first_flag));
    frame.pass(frame.local(local::second_flag));
    frame.pass(fourth);
    frame.pass(frame.local(local::fixed_flag));
    frame.pass(frame.local(local::body_out));
    const auto fifth=forward(argument::fifth);
    frame.pass(frame.local(local::shape_out));
    const auto third=forward(argument::third);
    frame.pass(frame.local(local::shape_value));
    const auto sixth=forward(argument::sixth);
    frame.pass(fifth);
    frame.pass(third);
    frame.pass(sixth);
    const auto model_address=forward(argument::model);
    frame.pass(frame.local(local::info_size));
    const auto object_address=forward(argument::object);
    frame.pass(frame.local(local::info));
    frame.pass(kept.types);
    frame.pass(model_address);
    frame.cleared();
    frame.pass(object_address);
    memory.store32(info_size_default,frame.local(local::info_size),0u);
    memory.store8(fixed_default,frame.local(local::fixed_flag),0u);
    memory.store8(second_default,frame.local(local::second_flag),1u);
    memory.store8(first_default,frame.local(local::first_flag),0u);
    kept.shape=frame.shape_built(kept);
    if(!kept.shape){frame.failed();frame.restoring(false);return Outcome::failed;}
    // The node's entry and the body's type; 1DEC80's value.
    const auto node_index=memory.load8(node,frame.local(local::info));
    const auto model_now=memory.load32(model_again,frame.argument(argument::model));
    const auto shapes_address=memory.load32(shapes_load,at(model_now,model::shapes));
    const auto entries=memory.load32(entries_load,at(shapes_address,shapes::entries));
    const auto node_offset=frame.node_offset(node_index);
    memory.store32(shapes_store,frame.local(local::shapes),shapes_address);
    const auto fixed=memory.load8(fixed_load,frame.local(local::fixed_flag));
    kept.entry=frame.entry(node_offset,entries,fixed);
    kept.type=fixed?frame.typed(fixed_type):frame.typed(memory.load8(table_type,kept.types+frame.node(node_index)));
    const auto fourth_address=memory.load32(fourth_again,frame.argument(argument::fourth));
    const auto fourth_word=memory.load32(fourth_value,fourth_address);
    const auto model_value_now=memory.load32(model_value,frame.argument(argument::model));
    frame.pass(fourth_word);
    frame.pass(model_value_now);
    const auto object_now=memory.load32(object_value,frame.argument(argument::object));
    memory.store32(type_store,frame.local(local::type),kept.type);
    const auto value=frame.valued(object_now,kept);
    // The node's matrix as a 4x4 transform on the stack.
    const auto matrix_index=memory.load16(entry_matrix,at(kept.entry,entry::matrix));
    frame.matrix_compared(matrix_index);
    memory.store32(value_store,frame.local(local::value),value);
    std::uint32_t matrix_address;
    if((matrix_index&0xffffu)==0xffffu)matrix_address=frame.own_matrix(memory.load32(model_own,frame.argument(argument::model)));
    else {
        const auto model_later=memory.load32(model_indexed,frame.argument(argument::model));
        matrix_address=frame.indexed_matrix(matrix_index,memory.load32(model_matrices,at(model_later,model::matrices)));
    }
    const auto component_of=[&](std::uint32_t offset){return memory.load32(component,matrix_address+offset);};
    const auto put=[&](std::uint32_t offset,std::uint32_t v){memory.store32(component_store,frame.local(offset),v);};
    const auto forward_z=component_of(matrix::forward+8u),forward_y=component_of(matrix::forward+4u);
    const auto forward_x=component_of(matrix::forward);
    put(local::transform+8u,forward_z);
    frame.zeroed();
    put(local::transform+0xcu,0u);put(local::transform,forward_x);put(local::transform+4u,forward_y);
    const auto left_z=component_of(matrix::left+8u),left_y=component_of(matrix::left+4u),left_x=component_of(matrix::left);
    put(local::transform+0x1cu,0u);put(local::transform+0x10u,left_x);put(local::transform+0x14u,left_y);put(local::transform+0x18u,left_z);
    const auto up_z=component_of(matrix::up+8u),up_y=component_of(matrix::up+4u),up_x=component_of(matrix::up);
    put(local::transform+0x2cu,0u);put(local::transform+0x20u,up_x);put(local::transform+0x24u,up_y);put(local::transform+0x28u,up_z);
    put(local::position,component_of(matrix::position));
    put(local::position+4u,component_of(matrix::position+4u));
    put(local::position+8u,component_of(matrix::position+8u));
    put(local::position+0xcu,0u);
    const auto position_value=memory.load128(position_row,frame.local(local::position));
    memory.store128(position_store,frame.local(local::last_row),position_value);
    // Marked: the entry's flag, or an object whose type has bit 1.
    const auto flags_word=memory.load16(entry_flags,at(kept.entry,entry::flags));
    if(!(frame.flag_tested(flags_word)&entry::marked)) {
        const auto object_address_now=memory.load32(object_load,frame.argument(argument::object));
        const auto handle=memory.load32(handle_load,at(object_address_now,object::handle));
        kept.type=memory.load32(type_load,frame.local(local::type));
        const auto index=frame.handle_index(handle);
        const auto headers=memory.load32(headers_load,globals::object_headers);
        const auto datums=memory.load32(datums_load,at(headers,datum_data));
        const auto type_value=memory.load8(object_type,datums+header_size*index+header_type);
        memory.store32(saved_store,frame.local(local::saved),frame.saved());
        const auto bit=frame.type_bit(type_value);
        memory.store8(marked_clear,frame.local(local::marked_flag),0u);
        frame.type_tested(bit);
        frame.restored(memory.load32(saved_load,frame.local(local::saved)));
        if(bit&marked_types)memory.store8(marked_set,frame.local(local::marked_flag),1u);
    } else memory.store8(marked_set,frame.local(local::marked_flag),1u);
    // The body created.
    const auto fixed_body=frame.fixed(kept.type);
    const auto second_value=memory.load32(entry_second,at(kept.entry,entry::second));
    frame.pass(second_value);
    const auto first_value=memory.load32(entry_first,at(kept.entry,entry::first));
    frame.pass(first_value);
    frame.pass(frame.halved_flags());
    frame.pass(memory.load32(value_load,frame.local(local::value)));
    const auto shape_value=memory.load32(shape_value_load,frame.local(local::shape_value));
    frame.pass(frame.local(local::transform));
    frame.pass(kept.type);
    frame.pass(shape_value);
    frame.pass(frame.local(local::body_out));
    frame.pass(0u);
    frame.pass(one);
    frame.pass(frame.local(local::shape_out));
    frame.pass(fixed_body);
    const auto shapes_now=memory.load32(shapes_again,frame.local(local::shapes));
    const auto count=memory.load32(count_load,at(shapes_now,shapes::count));
    frame.count_compared(count);
    kept.object=memory.load32(object_again,frame.argument(argument::object));
    frame.pass(frame.single());
    frame.pass(kept.shape);
    kept.body=frame.created(kept);
    // The shape's reference released.
    const auto references=frame.released(memory.load16(references_load,at(kept.shape,shape::references)));
    memory.store16(references_store,at(kept.shape,shape::references),references);
    frame.references_tested(memory.load16(references_test,at(kept.shape,shape::references)));
    if(!(references&0xffffu)) {
        if(frame.kept_static(kept))memory.store16(references_keep,at(kept.shape,shape::references),1u);
        else {
            const auto table=memory.load32(shape_vtable,at(kept.shape,shape::vtable));
            frame.destructor_argument(kept);
            frame.destroyed(memory.load32(destructor_slot,table),kept);
        }
    }
    const auto scaled=memory.load8(scaled_load,frame.argument(argument::scaled));
    frame.byte_tested(scaled);
    if(scaled)kept.body=frame.scaled(kept);
    // The record.
    const auto record_address=memory.load32(record_load,frame.argument(argument::record));
    const auto info_size=memory.load32(info_size_load,frame.local(local::info_size));
    memory.store32(body_store,at(record_address,record::body),kept.body);
    auto from=frame.copying(info_size,frame.local(local::info),at(record_address,record::info));
    auto to=at(record_address,record::info);
    const auto step=frame.step();
    for(std::uint32_t word=0;word<(info_size>>2);++word,from+=4u*step,to+=4u*step)
        memory.store32(info_word_store,to,memory.load32(info_word,from));
    frame.bytes(info_size);
    for(std::uint32_t byte=0;byte<(info_size&3u);++byte,from+=step,to+=step)memory.store8(info_byte_store,to,memory.load8(info_byte,from));
    memory.store8(flag_store,at(record_address,record::flag),memory.load8(second_load,frame.local(local::second_flag)));
    const auto first=memory.load8(first_load,frame.local(local::first_flag));
    frame.byte_tested(first);
    memory.store32(size_store,at(record_address,record::size),info_size);
    frame.succeeded();
    if(!first){frame.restoring(false);return Outcome::built;}
    const auto object_last_value=memory.load32(object_last,frame.argument(argument::object));
    memory.store32(flags_store,at(object_last_value,object::flags),frame.ored(memory.load32(flags_load,at(object_last_value,object::flags)),object::built));
    frame.restoring(true);
    return Outcome::marked;
}
} // namespace rigid_body_build
} // namespace halo2::native_rigid_body_build
