#pragma once
// Native TU1.5 node hierarchy transform, entered at 170730 (physics live
// support unit): a parent transform carried down a model's node tree.
//
//   node_hierarchy::propagate   170730(model, parent, node, limit, matrices)
//
// The model definition's nodes (96-byte records at +0x4C, the count at
// +0x48) form a tree: each node names its first child (+6) and its next
// sibling (+8) as signed 16-bit indices, negative for none. For every child
// of the node, in sibling order, until a child index reaches the node count
// or the matrix limit: 145560 combines the parent transform with the
// child's matrix (matrices + child * 52) in place, and when the child has
// children of its own the walk recurses into them, with the transform
// 145560 leaves in ECX.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_node_hierarchy.hpp) maps the role to the original instruction
// and owns the ABI of the children (145560 and 170730 itself). The arguments
// are read as memory through Frame::local, again for every child, in the
// original's order; the values the original keeps in callee-saved
// registers across the children are a Kept, refreshed from the registers
// each child returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_node_hierarchy {
using native_object_update::at;
using native_object_update::signed_value;
using native_object_update::widen16;

inline constexpr std::uint32_t model_node_count=0x48u,model_nodes=0x4cu;
inline constexpr std::uint32_t node_size=0x60u,node_first_child=6u,node_next_sibling=8u;
inline constexpr std::uint32_t matrix_size=0x34u;
inline constexpr std::uint32_t no_child16=0xffffu;

namespace node_hierarchy {
enum class Local {model,parent,node,limit,matrices};
enum class Access {
    node_argument,model_argument,nodes_table,first_child,matrices_argument,
    node_count,limit_argument,nodes_reload,parent_argument,grandchild,limit_for_children,next_sibling
};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t model;      // EBX: the model definition
    std::uint32_t matrices;   // EBP: the node matrices
    std::uint32_t node;       // EDI: the child's node record
    std::uint32_t child;      // ESI: the child's index
};
enum class Exit {leaf,walked};

// Frame:
//   local(Local)       an argument's address now
//   combine(kept)      145560 for the child's matrix (kept.child,
//                      kept.matrices) with the loaded parent
//   descend(kept)      170730 for the child's children
template<class Memory,class Frame> Exit propagate(Memory &memory,Frame &frame) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto node=memory.load32(node_argument,local(Local::node));
    Kept kept{};
    kept.model=memory.load32(model_argument,local(Local::model));
    const auto nodes=memory.load32(nodes_table,at(kept.model,model_nodes));
    kept.child=widen16(memory.load16(first_child,node*node_size+nodes+node_first_child));
    if(signed_value(kept.child)<0)return Exit::leaf;
    kept.matrices=memory.load32(matrices_argument,local(Local::matrices));
    for(;;) {
        if(!(signed_value(kept.child)<signed_value(memory.load32(node_count,at(kept.model,model_node_count)))))return Exit::walked;
        if(!(signed_value(kept.child)<signed_value(memory.load32(limit_argument,local(Local::limit)))))return Exit::walked;
        const auto nodes_now=memory.load32(nodes_reload,at(kept.model,model_nodes));
        (void)memory.load32(parent_argument,local(Local::parent));
        kept.node=kept.child*node_size+nodes_now;
        frame.combine(kept);
        if((memory.load16(grandchild,at(kept.node,node_first_child))&0xffffu)!=no_child16) {
            (void)memory.load32(limit_for_children,local(Local::limit));
            frame.descend(kept);
        }
        kept.child=widen16(memory.load16(next_sibling,at(kept.node,node_next_sibling)));
        if(signed_value(kept.child)<0)return Exit::walked;
    }
}
} // namespace node_hierarchy
} // namespace halo2::native_node_hierarchy
