#pragma once
// Native TU1.5 record placement, entered at 1DC680 (physics live support
// unit): the frame a record was taken at (1DC610), placed.
//
//   record_place::place   1DC680(EAX = record, ECX = frame; placement*) -> RET 4
//
// The frame (a 52-byte matrix, 13 dwords) is copied to the placement as REP
// MOVSD copies it, in the direction the string direction flag gives. A
// record flagged bit 9 (+4) whose object (+8) is a biped (its header's type
// byte +3 is 0) and that E32C0 answers an offset for (into the argument's
// slot) has the placement's height (+0x30) raised by it, in single precision
// on the live MXCSR. Otherwise a marked record (its marker, int8 +0x18, one
// of its markers +0x74) with a node (byte +0x19 not 0xFF) is transformed by
// that node (145560: ECX the placement, EDX the node's matrix, +0x28 of the
// 0x60-byte nodes +0x4C of the render model +4 of the model +0x38 of the
// object's definition, each through the tag instances at 4F0414).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter (native_tu15_record_place.hpp)
// maps the role to the original instruction and owns the ABI of the children
// (E32C0, 145560). The placement the original keeps in EBX across E32C0 is a
// Kept, refreshed from the register it returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_record_place {
using native_object_update::at;
using native_object_update::signed_value;
using native_object_update::widen8;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,tag_instances=0x4f0414u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_type=3u,header_address=8u,tag_size=16u,tag_data=8u;
namespace record {inline constexpr std::uint32_t flags=4u,object=8u,marker=0x18u,node=0x19u,markers=0x74u;}
inline constexpr std::uint32_t raised_bit=9u,biped_type=0u,no_node=0xffu,matrix_words=13u,height_offset=0x30u;
namespace definition {inline constexpr std::uint32_t model=0x38u;}
namespace model {inline constexpr std::uint32_t render_model=4u;}
namespace render_model {inline constexpr std::uint32_t nodes=0x4cu;}
inline constexpr std::uint32_t node_size=0x60u,node_matrix=0x28u;

namespace record_place {
enum class Access {placement,flags,copy_read,copy_write,object,headers,datums,type,height,offset,height_store,marker,markers,node,
    object_again,headers_again,datums_again,object_address,object_tag,instances,object_definition,model_tag,model_definition,
    render_tag,render_definition,nodes};
enum class Outcome {raised,not_biped,no_offset,unmarked,no_node,transformed};
// The original's EBX across E32C0: the placement.
struct Kept {std::uint32_t placement;};
// Frame:
//   argument()                        the argument's slot now (E32C0's answer lands there)
//   copy_started(), copy_step()       the REP MOVSD's registers and direction
//   offset(object, kept) -> AL        E32C0(EAX the object; the slot pushed), refreshes kept
//   restoring()                       the saved EDI and ESI back before the height's store
//   compared(marker)                  the marker's checks' flags
//   transform(placement, matrix)      145560(ECX the placement, EDX the matrix; the placement pushed)
template<class Memory,class Frame> void copy(Memory &memory,Frame &frame,std::uint32_t frame_address,std::uint32_t placement_address) {
    using enum Access;
    frame.copy_started();
    const auto step=frame.copy_step();
    for(std::uint32_t word=0;word<matrix_words;++word)
        memory.store32(copy_write,placement_address+word*step,memory.load32(copy_read,frame_address+word*step));
}
template<class Memory> std::uint32_t tag_data_of(Memory &memory,Access role,std::uint32_t tag,std::uint32_t instances) {
    return memory.load32(role,tag_size*(tag&0xffffu)+instances+tag_data);
}
template<class Memory,class Frame> Outcome place(Memory &memory,Frame &frame,std::uint32_t record_address,std::uint32_t frame_address) {
    using enum Access;
    Kept kept{memory.load32(placement,frame.argument())};
    if((memory.load32(flags,at(record_address,record::flags))>>raised_bit)&1u) {
        copy(memory,frame,frame_address,kept.placement);
        const auto object_handle=memory.load32(object,at(record_address,record::object));
        const auto header=memory.load32(datums,at(memory.load32(headers,globals::object_headers),datum_data))+header_size*(object_handle&0xffffu);
        if(!((1u<<(memory.load8(type,at(header,header_type))&31u))&(1u<<biped_type)))return Outcome::not_biped;
        if(!(frame.offset(object_handle,kept)&0xffu))return Outcome::no_offset;
        const auto height_value=memory.load32(Access::height,at(kept.placement,height_offset));
        const auto raised=sse::add(height_value,memory.load32(offset,frame.argument()));
        frame.restoring();
        memory.store32(height_store,at(kept.placement,height_offset),raised);
        return Outcome::raised;
    }
    const auto marker_value=widen8(memory.load8(marker,at(record_address,record::marker)));
    if(signed_value(marker_value)<0 || !(signed_value(marker_value)<signed_value(memory.load32(markers,at(record_address,record::markers)))) ||
       (marker_value&0xffffu)==0xffffu) {
        frame.compared(marker_value);
        copy(memory,frame,frame_address,kept.placement);
        return Outcome::unmarked;
    }
    frame.compared(marker_value);
    copy(memory,frame,frame_address,kept.placement);
    const auto node_index=memory.load8(node,at(record_address,record::node));
    if(node_index==no_node)return Outcome::no_node;
    // The node's matrix: the object's definition, its model, the model's render model, its nodes.
    const auto object_handle=memory.load32(object_again,at(record_address,record::object));
    const auto object_datums=memory.load32(datums_again,at(memory.load32(headers_again,globals::object_headers),datum_data));
    const auto object_address_value=memory.load32(object_address,object_datums+header_size*(object_handle&0xffffu)+header_address);
    const auto tag=memory.load32(object_tag,object_address_value);
    const auto instances_address=memory.load32(instances,globals::tag_instances);
    const auto object_definition_address=tag_data_of(memory,object_definition,tag,instances_address);
    const auto model_definition_address=tag_data_of(memory,model_definition,
        memory.load32(model_tag,at(object_definition_address,definition::model)),instances_address);
    const auto render_definition_address=tag_data_of(memory,render_definition,
        memory.load32(render_tag,at(model_definition_address,model::render_model)),instances_address);
    const auto nodes_address=memory.load32(nodes,at(render_definition_address,render_model::nodes));
    frame.transform(kept.placement,nodes_address+node_size*widen8(node_index)+node_matrix);
    return Outcome::transformed;
}
} // namespace record_place
} // namespace halo2::native_record_place
