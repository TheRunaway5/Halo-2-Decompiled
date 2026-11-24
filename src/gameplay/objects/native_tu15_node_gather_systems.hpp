#pragma once
// Native TU1.5 node matrix gather, entered at 16B240 (sandbox special
// unit): a model's node matrices, gathered from another node order.
//
//   node_gather::gather   16B240(EAX = model tag; transform, destination,
//                                capacity, sources, node map)
//
// For each of the model definition's nodes (the count at +0x48, capped by
// the capacity), the node map names a source matrix (NONE skips the node,
// leaving its destination as it was). With a transform, 145560 combines the
// transform with the source matrix into the destination; without one the
// 52-byte matrix (13 dwords) is copied as REP MOVSD copies it, in the
// direction the string direction flag gives. The destination advances by a
// matrix for every node.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_node_gather.hpp) maps the role to the original instruction
// and owns the ABI of the child 145560. The arguments and the one local (the
// node position) live in the original's EBP frame and are read and written
// as memory through Frame::local, in the original's order: the position and
// the destination (kept in the capacity argument's slot) are updated in
// memory for every node, and the arguments are read again for every node.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_node_gather {
using native_object_update::at;
using native_object_update::element;
using native_object_update::none;
using native_object_update::signed_value;

namespace globals {
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // 16-byte tag instances; +8 a tag's data
}
inline constexpr std::uint32_t tag_size=16u,tag_data=8u;
inline constexpr std::uint32_t definition_node_count=0x48u;
inline constexpr std::uint32_t matrix_size=0x34u,matrix_words=13u;   // scale, three axes, position

namespace node_gather {
enum class Local {
    position,      // the node position (EBP-4)
    transform,     // the transform to combine with, or zero
    destination,   // the first destination matrix
    capacity,      // the destination's room; then the current destination
    sources,       // the source matrices
    node_map       // int32[node]: the source matrix of each node, NONE for none
};
enum class Access {
    tag_instances,definition,node_count,capacity_compare,capacity_move,position_clear_read,position_clear,
    destination_argument,destination_store,
    node_map_argument,position_read,map_entry,transform_argument,destination_push,sources_for_transform,
    destination_load,sources_for_copy,copy_read,copy_write,
    position_increment_read,position_increment,destination_advance_read,destination_advance,position_bound
};
enum class Exit {empty,gathered};

// Frame:
//   local(Local)        an argument's or the local's address now
//   transform(count)    145560 with the loaded values; returns the node
//                       count the original keeps in EBX afterwards
//   copy_step()         the direction flag's step: +4 or -4
//   copy_started()      PUSH D; POP ECX before the copy
template<class Memory,class Frame> Exit gather(Memory &memory,Frame &frame,std::uint32_t model_tag) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto instances=memory.load32(tag_instances,globals::tag_instances);
    const auto model=memory.load32(definition,(model_tag&0xffffu)*tag_size+instances+tag_data);
    auto count=memory.load32(node_count,at(model,definition_node_count));
    // CMP EBX,[capacity]; CMOVG EBX,[capacity] (the move reads it again).
    const auto capacity=memory.load32(capacity_compare,local(Local::capacity));
    const auto capacity_again=memory.load32(capacity_move,local(Local::capacity));
    if(signed_value(count)>signed_value(capacity))count=capacity_again;
    memory.store32(position_clear,local(Local::position),memory.load32(position_clear_read,local(Local::position))&0u);
    if(signed_value(count)<=0)return Exit::empty;
    memory.store32(destination_store,local(Local::capacity),memory.load32(destination_argument,local(Local::destination)));
    for(;;) {
        const auto map=memory.load32(node_map_argument,local(Local::node_map));
        const auto position=memory.load32(position_read,local(Local::position));
        const auto index=memory.load32(map_entry,element(map,position,4));
        if(index!=none) {
            if(memory.load32(transform_argument,local(Local::transform))) {
                (void)memory.load32(destination_push,local(Local::capacity));
                (void)memory.load32(sources_for_transform,local(Local::sources));
                count=frame.transform(count);
            } else {
                const auto destination=memory.load32(destination_load,local(Local::capacity));
                const auto source=index*matrix_size+memory.load32(sources_for_copy,local(Local::sources));
                frame.copy_started();
                const auto step=frame.copy_step();
                for(std::uint32_t word=0;word<matrix_words;++word)
                    memory.store32(copy_write,destination+word*step,memory.load32(copy_read,source+word*step));
            }
        }
        memory.store32(position_increment,local(Local::position),memory.load32(position_increment_read,local(Local::position))+1u);
        memory.store32(destination_advance,local(Local::capacity),
                       memory.load32(destination_advance_read,local(Local::capacity))+matrix_size);
        if(!(signed_value(memory.load32(position_bound,local(Local::position)))<signed_value(count)))return Exit::gathered;
    }
}
} // namespace node_gather
} // namespace halo2::native_node_gather
