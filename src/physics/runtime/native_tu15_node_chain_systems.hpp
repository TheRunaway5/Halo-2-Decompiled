#pragma once
// Native TU1.5 node chain, entered at 1E1390 (physics live support unit): a
// node, its parent and its grandparent are posed toward a target.
//
//   node_chain::pose   1E1390(EAX = model tag, EBX = node matrices,
//                      ESI = node; target*, offset*, weight, poses*)
//
// The model tag's nodes (+0x4C, 0x60 bytes each) name their parents (int16
// +4, NONE for none). A node with a parent and a grandparent is posed: the
// target (144090) transformed by the offset (145560) is the goal; unless
// the weight is within 46A0EC of one (the original's x87 test), the goal's
// position (+0x28) is blended toward the node's own by that weight, as
// node * (1 - weight) + goal * weight; the chain is then solved (122BE0 for
// the parent's and the grandparent's matrices toward the goal and the
// node's matrix), the node's matrix composed (145560) and the poses stored
// (170730). The matrices are 0x34 bytes.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_node_chain.hpp) maps the role to the original instruction
// and owns the ABI and the x87 of the children (144090, 145560, 122BE0,
// 170730). The values the original keeps in callee-saved registers across
// them are a Kept, refreshed from the registers each child returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_node_chain {
using native_object_update::at;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t tag_instances=0x4f0414u;   // 16-byte instances (+8 the tag's address)
inline constexpr std::uint32_t one=0x46a0d0u,full_weight=0x46a0ecu;   // floats
}
inline constexpr std::uint32_t tag_address=8u,instance_shift=4u,model_nodes=0x4cu,node_size=0x60u,node_parent=4u,
    matrix_size=0x34u,matrix_position=0x28u,no_node=0xffffu;

namespace node_chain {
// The arguments, and the locals of the 0xA4-byte frame: the parent, the
// grandparent, and three matrices (the goal, the node's, the result).
enum class Local {target,offset,weight,poses,parent,grandparent,goal,node_matrix,result};
enum class Access {
    instances,tag,nodes,parent,parent_store,grandparent,grandparent_store,target_argument,offset_argument,
    weight_x87,one_x87,full_x87,weight,one,goal_x,node_x,goal_y,goal_x_store,node_y,goal_y_store,node_z,goal_z,
    goal_z_store,grandparent_again,parent_again,poses_argument
};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t matrices;   // EBX: the node matrices
    std::uint32_t node;       // ESI: the node index
    std::uint32_t tag;        // EBP: the model tag
    std::uint32_t matrix;     // EDI: the node's matrix
};
// The SSE registers the blend leaves.
struct Blend {std::uint32_t xmm0,xmm1,xmm2,xmm3;};
enum class Exit {unchained,posed};

// Frame:
//   local(Local)              an argument's or local's address now
//   copy_target(kept)         144090(ECX the target, EAX the goal)
//   transform_goal(kept)      145560(ECX the offset, EDX the goal; the goal)
//   copy_node(kept)           144090(ECX the node's matrix, EAX the local)
//   weight_full()             the x87 test (the loads ran it): whether the
//                             weight is within the full weight of one
//   blended(Blend)            the blend's registers
//   solve(kept)               122BE0(EAX the parent's matrix, ECX the
//                             grandparent's; the goal, the node's matrix)
//   compose(kept)             145560(ECX the node's matrix, EDX the local;
//                             the result)
//   store_poses(kept)         170730(tag, result, node, poses, matrices)
// Every child refreshes kept.
template<class Memory,class Frame> Exit pose(Memory &memory,Frame &frame,std::uint32_t model,Kept kept) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto instances_address=memory.load32(instances,globals::tag_instances);
    kept.tag=memory.load32(tag,at(((model&0xffffu)<<instance_shift)+instances_address,tag_address));
    const auto nodes_address=memory.load32(nodes,at(kept.tag,model_nodes));
    const auto parent_word=memory.load16(parent,at(kept.node*node_size+nodes_address,node_parent));
    if(parent_word==no_node)return Exit::unchained;
    const auto parent_index=widen16(parent_word);
    memory.store32(parent_store,local(Local::parent),parent_index);
    const auto grandparent_index=widen16(memory.load16(grandparent,at(parent_index*node_size+nodes_address,node_parent)));
    memory.store32(grandparent_store,local(Local::grandparent),grandparent_index);
    if((grandparent_index&0xffffu)==no_node)return Exit::unchained;
    // The goal: the target transformed by the offset.
    (void)memory.load32(target_argument,local(Local::target));
    frame.copy_target(kept);
    (void)memory.load32(offset_argument,local(Local::offset));
    frame.transform_goal(kept);
    kept.matrix=kept.node*matrix_size+kept.matrices;
    frame.copy_node(kept);
    // A partial weight blends the goal's position toward the node's.
    (void)memory.load32(weight_x87,local(Local::weight));
    (void)memory.load32(one_x87,globals::one);
    (void)memory.load32(full_x87,globals::full_weight);
    if(!frame.weight_full()) {
        const auto w=memory.load32(weight,local(Local::weight));
        const auto one_value=memory.load32(one,globals::one);
        const auto goal_position=local(Local::goal)+matrix_position;
        const auto node_position=at(kept.matrix,matrix_position);
        const auto gx=memory.load32(goal_x,goal_position);
        const auto nx=memory.load32(node_x,node_position);
        const auto rest=sse::subtract(one_value,w);
        const auto x=sse::add(sse::multiply(nx,rest),sse::multiply(gx,w));
        const auto gy=memory.load32(goal_y,goal_position+4u);
        memory.store32(goal_x_store,goal_position,x);
        const auto ny=memory.load32(node_y,node_position+4u);
        const auto gy_weighted=sse::multiply(gy,w);
        const auto y=sse::add(sse::multiply(ny,rest),gy_weighted);
        memory.store32(goal_y_store,goal_position+4u,y);
        const auto nz=sse::multiply(memory.load32(node_z,node_position+8u),rest);
        const auto gz_weighted=sse::multiply(memory.load32(goal_z,goal_position+8u),w);
        const auto z=sse::add(nz,gz_weighted);
        memory.store32(goal_z_store,goal_position+8u,z);
        frame.blended({gz_weighted,w,z,gy_weighted});
    }
    // The chain, the node's matrix and the poses.
    (void)memory.load16(grandparent_again,local(Local::grandparent));
    (void)memory.load32(parent_again,local(Local::parent));
    frame.solve(kept);
    frame.compose(kept);
    (void)memory.load32(poses_argument,local(Local::poses));
    frame.store_poses(kept);
    return Exit::posed;
}
} // namespace node_chain
} // namespace halo2::native_node_chain
