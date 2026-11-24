#pragma once
// Native TU1.5 marker chains, entered at 116060 (physics live support
// unit): the node chains a definition's marker pairs name are posed from
// one object toward another.
//
//   marker_chains::pose   116060(object, other, keyed, weight, definition,
//                         records*, poses*, matrices*)
//
// Nothing is posed unless both objects are named and the weight is above
// 46A0E8 (COMISS), and the definition's tag (+0x68, through the tag
// instances at 4F0414) exists with nodes (int16 +0xC). The object's model
// (its tag's +0x38) is kept; a bit per node (the count 1D4140 gives for the
// definition) marks the nodes already posed, all clear at first. The
// definition's marker records are walked (the record iterator takes the
// definition's keys +0x70 and +0x74 and its tag: 1E5250 when keyed, else
// 1E51D0); a record names a marker of the object (+0) and one of the other
// (+4), NONE or zero for none. The object's marker (BAD60) gives a node and
// a pose; for a node not yet posed, the other's marker (BAD60) gives the
// goal, and 1E1390 poses the node's chain from the pose toward the goal's
// offset (+0x38) by the weight; the node is then marked.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_marker_chains.hpp) maps the role to the original instruction
// and owns the ABI of the children (1D4140, 1E5250, 1E51D0, BAD60, 1E1390).
// The values the original keeps in callee-saved registers across them are
// a Kept, refreshed from the registers each child returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_marker_chains {
using native_object_update::at;
using native_object_update::none;
using native_object_update::object_of;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t tag_instances=0x4f0414u;   // 16-byte instances (+8 the tag's address)
inline constexpr std::uint32_t least_weight=0x46a0e8u;    // float
}
inline constexpr std::uint32_t tag_address=8u,instance_shift=4u;
namespace definition {inline constexpr std::uint32_t tag=0x68u,first_key=0x70u,second_key=0x74u;}
inline constexpr std::uint32_t tag_nodes=0xcu,object_tag_field=0u,object_model_field=0x38u,model_handle_field=4u;
namespace record {inline constexpr std::uint32_t object_marker=0u,other_marker=4u,first_key=0xcu,second_key=0x10u;}
inline constexpr std::uint32_t goal_offset=0x38u,pose_matrix=4u;

namespace marker_chains {
// The arguments, and the locals of the 0x104-byte frame: the model, the
// posed nodes' bits, the pose (its node first) and the goal.
enum class Local {object,other,keyed,weight,definition,records,poses,matrices,model,posed,pose,goal};
enum class Access {
    object_argument,records_argument,other_argument,weight_argument,least_weight,instances,definition_argument,
    definition_tag,definition_address,definition_nodes,object_table,object_datums,object_address,object_tag,
    object_definition,object_model,model_address,model_store,posed_clear,weight_again,definition_again,
    instances_again,keyed,first_key,record_tag,record_tag_address,first_key_store,second_key,second_key_store,
    other_marker,object_marker,object_again,pose_node,posed_word,other_marker_again,other_again,poses_argument,
    chain_node,matrices_argument,model_local,model_handle,posed_node,posed_read,posed_store
};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t instances;    // EBX: the tag instances (then the record tag, the matrices)
    std::uint32_t definition;   // ESI: the definition (then the records, the node)
    std::uint32_t weight;       // EDI: the weight's bits
    std::uint32_t records;      // EBP: the record iterator
};
enum class Exit {not_posed,no_nodes,walked};

// Frame:
//   local(Local)              an argument's or local's address now
//   node_count(kept)          1D4140(ECX the definition); returns EAX
//   clearing(), cleared()     the clear's registers (REP STOSD, then the
//                             REP STOSB of no bytes)
//   next_record(keyed, kept)  1E5250 or 1E51D0 (ESI the records, EBX the
//                             tag); returns AL
//   marker(other, kept)       BAD60(EBX the pose or the goal, CL 0;
//                             object, marker, 1); returns AX
//   pose_chain(kept)          1E1390(EAX the model, EBX the matrices, ESI
//                             the node; pose, goal offset, weight, poses)
// Every child refreshes kept.
template<class Memory,class Frame> Exit pose(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto object=memory.load32(object_argument,local(Local::object));
    kept.records=memory.load32(records_argument,local(Local::records));
    if(object==none || memory.load32(other_argument,local(Local::other))==none)return Exit::not_posed;
    const auto weight_value=memory.load32(weight_argument,local(Local::weight));
    if(sse::not_above(weight_value,memory.load32(least_weight,globals::least_weight)))return Exit::not_posed;
    // The definition's tag, with nodes.
    kept.instances=memory.load32(instances,globals::tag_instances);
    kept.definition=memory.load32(definition_argument,local(Local::definition));
    const auto definition_tag_handle=memory.load32(definition_tag,at(kept.definition,definition::tag));
    const auto definition_tag_address=memory.load32(definition_address,
        at(((definition_tag_handle&0xffffu)<<instance_shift)+kept.instances,tag_address));
    if(definition_tag_address==0 || widen16(memory.load16(definition_nodes,at(definition_tag_address,tag_nodes)))==0)
        return Exit::no_nodes;
    // The object's model.
    const auto object_data=object_of(memory,object,object_table,object_datums,object_address);
    const auto object_tag_handle=memory.load32(object_tag,at(object_data,object_tag_field));
    const auto object_tag_address=memory.load32(object_definition,
        at(((object_tag_handle&0xffffu)<<instance_shift)+kept.instances,tag_address));
    const auto model_tag_handle=memory.load32(object_model,at(object_tag_address,object_model_field));
    const auto model=memory.load32(model_address,at(((model_tag_handle&0xffffu)<<instance_shift)+kept.instances,tag_address));
    memory.store32(model_store,local(Local::model),model);
    // The posed nodes start clear.
    const auto count=frame.node_count(kept);
    const auto bytes=std::uint32_t(std::int32_t(count+31u)>>5)<<2;
    frame.clearing();
    for(std::uint32_t word=0;word<(bytes>>2);++word)memory.store32(posed_clear,local(Local::posed)+4u*word,0u);
    frame.cleared();
    kept.weight=memory.load32(weight_again,local(Local::weight));
    for(bool first=true;;first=false) {
        if(!first) {
            kept.definition=memory.load32(definition_again,local(Local::definition));
            kept.instances=memory.load32(instances_again,globals::tag_instances);
        }
        // The next marker record.
        const bool keyed_walk=memory.load8(keyed,local(Local::keyed))!=0;
        const auto first_key_value=memory.load32(first_key,at(kept.definition,definition::first_key));
        const auto record_tag_handle=memory.load32(record_tag,at(kept.definition,definition::tag));
        kept.instances=memory.load32(record_tag_address,at(((record_tag_handle&0xffffu)<<instance_shift)+kept.instances,tag_address));
        memory.store32(first_key_store,at(kept.records,record::first_key),first_key_value);
        memory.store32(second_key_store,at(kept.records,record::second_key),
                       memory.load32(second_key,at(kept.definition,definition::second_key)));
        if(!(frame.next_record(keyed_walk,kept)&0xffu))return Exit::walked;
        const auto other_marker_value=memory.load32(other_marker,at(kept.records,record::other_marker));
        if(other_marker_value==none || other_marker_value==0)continue;
        const auto object_marker_value=memory.load32(object_marker,at(kept.records,record::object_marker));
        if(object_marker_value==none || object_marker_value==0)continue;
        // The object's marker: the node and its pose.
        (void)memory.load32(object_again,local(Local::object));
        if(!(frame.marker(false,kept)&0xffffu))continue;
        const auto node=widen16(memory.load16(pose_node,local(Local::pose)));
        const auto bit=1u<<(node&31u);
        if(memory.load32(posed_word,local(Local::posed)+4u*std::uint32_t(std::int32_t(node)>>5))&bit)continue;
        // The other's marker: the goal.
        (void)memory.load32(other_marker_again,at(kept.records,record::other_marker));
        (void)memory.load32(other_again,local(Local::other));
        if(!(frame.marker(true,kept)&0xffffu))continue;
        (void)memory.load32(poses_argument,local(Local::poses));
        kept.definition=widen16(memory.load16(chain_node,local(Local::pose)));
        kept.instances=memory.load32(matrices_argument,local(Local::matrices));
        const auto model_now=memory.load32(model_local,local(Local::model));
        (void)memory.load32(model_handle,at(model_now,model_handle_field));
        frame.pose_chain(kept);
        // The node is posed.
        const auto posed_node_index=widen16(memory.load16(posed_node,local(Local::pose)));
        const auto word_address=local(Local::posed)+4u*std::uint32_t(std::int32_t(posed_node_index)>>5);
        memory.store32(posed_store,word_address,memory.load32(posed_read,word_address)|(1u<<(posed_node_index&31u)));
    }
}
} // namespace marker_chains
} // namespace halo2::native_marker_chains
