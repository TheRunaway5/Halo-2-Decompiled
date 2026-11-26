#pragma once
// Native TU1.5 physics groups, entered at 1DDBF0 (live support unit): build
// an object's rigid-body physics groups from its physics model, reserve the
// object's group array, instantiate each group, and mark the object built.
//
//   physics_groups::build   1DDBF0(model_state, force_keyframed, use_fixed, allow_dynamic) -> EAX; RET 10
//
// The object handle at model_state+8 resolves into the object header array
// ([4ECBD0]+0x44, 12-byte entries). When header flag byte +2 lacks bit 0 and
// bit (header+3) is not bit 7 (0x80), force_keyframed is forced to 1.
// 212C70 populates the model-view block on the 0x4E84-byte frame; if absent,
// build returns 0. Otherwise 184920 resolves the physics model's node matrix
// table, the 256-bit visited bitset is cleared, and every rigid-body record
// (0x90 bytes at model+0x3C, model+0x38 of them) is classified through the
// six-entry kind jump table at 1DDE84 (values 2..7), overridden to 7 when
// force_keyframed is set, or to 6 when use_fixed is set and the record's
// motion type is neither 7 nor a parented non-fixed node (node+4 bit 0 in
// model+0xCC, 12-byte entries). 1DDEA0 then builds the ungrouped root (-1)
// and each unvisited node group (0..model+0xC8-1), tracking the maximum
// status returned in frame+0x10. If any group was created, 2E31E0 ensures
// the object's group array (model_state+0x70, capacity masked by 0x7FFFFFFF
// at +0x78, 0x60-byte entries) holds them, 1D8330 creates each group, 1DED80
// finalizes the model view, and bit 0x800 is set on model_state+4.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_physics_groups.hpp) maps the role to the original instruction,
// owns the ABI of the children (the stack probe 32A660 among them) and
// leaves through the original's epilogue. The values the original keeps in
// callee-saved registers across the children are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_physics_groups {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t objects=0x4ecbd0u,jump_table=0x1dde84u;}
inline constexpr std::uint32_t datum_data=0x44u,handle_mask=0xffffu,header_size=12u,frame_size=0x4e84u,
    body_size=0x90u,node_size=12u,group_size=0x4cu,array_element_size=0x60u,capacity_mask=0x7fffffffu,
    none=0xffffffffu,no_node=0xffffu,built_flag=0x800u,bitset_words=8u;
namespace header {inline constexpr std::uint32_t flags=2u,kind=3u;}
namespace state_field {inline constexpr std::uint32_t flags=4u,object_handle=8u,groups_array=0x70u,groups_capacity=0x78u;}
namespace model_field {inline constexpr std::uint32_t body_count=0x38u,bodies=0x3cu,node_count=0xc8u,nodes=0xccu;}
namespace rigid_body {inline constexpr std::uint32_t node_index=0u,motion_type=0x1au;}
namespace model_node {inline constexpr std::uint32_t flags=4u;}
namespace group_entry {inline constexpr std::uint32_t head=0u,payload=4u,count=0x44u,kind=0x48u;}
// The frame (offsets from the ESP after EBX, EBP, ESI and EDI are pushed):
// status scratch, max result, 256-bit visited bitset, model view (+0x38;
// physics model pointer at +0x80), per-body motion bytes (+0x8C), built
// groups (+0xCC, 0x4C bytes each), matrix workspace (+0xA4C), and the four
// stack arguments (+0x4E98..+0x4EA4).
namespace local {inline constexpr std::uint32_t step_result=0x10u,max_result=0x14u,visited=0x18u,model_view=0x38u,
    physics_model=0x80u,motion_bytes=0x8cu,groups=0xccu,matrices=0xa4cu,
    arg_state=0x4e98u,arg_keyframed=0x4e9cu,arg_fixed=0x4ea0u,arg_allow=0x4ea4u;}
inline constexpr std::uint32_t signed_word(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}

namespace physics_groups {
enum class Access {
    objects_load,objects_data,state_argument,object_handle,header_flags,result_clear,header_kind,keyframed_force,
    object_handle_matrices,model_load,
    visited_clear_0,visited_clear_1,visited_clear_2,visited_clear_3,visited_clear_4,visited_clear_5,visited_clear_6,visited_clear_7,
    body_count_first,bodies_load,body_motion_type,kind_case,keyframed_load,fixed_load,body_node,nodes_load,node_flags,
    motion_store,body_count_next,
    allow_root,root_step_clear,root_step_result,root_max_store,
    model_before_nodes,node_count_first,visited_word,allow_node,node_step_clear,node_step_result,max_result_compare,node_max_store,
    model_next_node,node_count_next,
    array_capacity,group_count_load,group_kind_load,group_head_load,
    state_flags_load,state_flags_store,result_load};
// The original's EBX (model_state), EBP (body byte offset, then group cursor),
// ESI (physics model, then node index, then group tail pointer) and EDI (body
// index, then built group count).
struct Kept {std::uint32_t state_address,cursor,index,count;};

// Frame: see native_tu15_physics_groups.hpp.
template<class Memory,class Frame> void build(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    frame.probed();
    const auto objects=memory.load32(objects_load,globals::objects);
    const auto data=memory.load32(objects_data,at(objects,datum_data));
    frame.saving(1u);
    kept.state_address=memory.load32(state_argument,frame.local(local::arg_state));
    const auto handle=memory.load32(object_handle,at(kept.state_address,state_field::object_handle));
    const auto header_address=frame.header_address(data,handle);
    const bool has_flag=frame.header_flag(memory.load8(header_flags,at(header_address,header::flags)));
    frame.saving(3u);
    frame.saving(4u);
    memory.store32(result_clear,frame.local(local::max_result),0u);
    if(!has_flag) {
        const auto kind_byte=memory.load8(header_kind,at(header_address,header::kind));
        if(frame.kind_forces_keyframed(kind_byte))
            memory.store8(keyframed_force,frame.local(local::arg_keyframed),1u);
    }

    if(!frame.load_model_view(kept)) {
        memory.load32(result_load,frame.local(local::max_result));
        return;
    }

    const auto handle_again=memory.load32(object_handle_matrices,at(kept.state_address,state_field::object_handle));
    frame.resolve_matrices(kept,handle_again);
    kept.index=memory.load32(model_load,frame.local(local::physics_model));
    const auto zero=frame.zero_value();
    for(std::uint32_t word=0;word<bitset_words;++word)
        memory.store32(Access(unsigned(visited_clear_0)+word),frame.local(local::visited+4u*word),zero);

    // Classify every rigid body's effective motion mode into the frame table.
    const auto initial_bodies=memory.load32(body_count_first,at(kept.index,model_field::body_count));
    kept.count=frame.zero_body_index();
    if(frame.has_bodies(initial_bodies)) {
        kept.cursor=frame.zero_body_offset();
        for(;;) {
            const auto bodies_base=memory.load32(bodies_load,at(kept.index,model_field::bodies));
            const auto motion_raw=memory.load16(body_motion_type,at(bodies_base+kept.cursor,rigid_body::motion_type));
            const auto body_address=frame.advance_body(bodies_base,kept.cursor);
            const auto motion_type=signed_word(motion_raw);
            const auto target=memory.load32(kind_case,globals::jump_table+4u*motion_type);
            std::uint32_t mode=frame.mapped_mode(target);
            if(frame.flag_nonzero(memory.load8(keyframed_load,frame.local(local::arg_keyframed))))
                mode=frame.override_mode(7u);
            if(frame.flag_nonzero(memory.load8(fixed_load,frame.local(local::arg_fixed)))) {
                if(!frame.equals_seven(mode)) {
                    const auto node_word=memory.load16(body_node,at(body_address,rigid_body::node_index));
                    if(frame.node_is_none(node_word)) {
                        mode=frame.override_mode(6u);
                    } else {
                        const auto nodes=memory.load32(nodes_load,at(kept.index,model_field::nodes));
                        const auto node_address=frame.node_address(nodes,node_word);
                        if(!frame.node_fixed(memory.load8(node_flags,at(node_address,model_node::flags))))
                            mode=frame.override_mode(6u);
                    }
                }
            }
            memory.store8(motion_store,frame.local(local::motion_bytes)+kept.count,mode);
            const auto body_count=memory.load32(body_count_next,at(kept.index,model_field::body_count));
            kept.count=frame.next_body_index(kept.count);
            kept.cursor=frame.next_body_offset(kept.cursor);
            if(!frame.below(kept.count,body_count))break;
        }
    }

    // Build the ungrouped root (-1), then every unvisited node group.
    frame.push_root_head();
    const auto allow_for_root=memory.load32(allow_root,frame.local(local::arg_allow));
    kept.index=frame.push_root_tail(kept.state_address,allow_for_root);
    memory.store32(root_step_clear,frame.local(local::step_result),kept.index);
    kept.count=frame.zero_group_count();
    if(frame.build_group(kept,true))kept.count=frame.one_group();
    const auto root_result=memory.load32(root_step_result,frame.local(local::step_result));
    if(frame.at_least(root_result,kept.index))
        memory.store32(root_max_store,frame.local(local::max_result),root_result);

    const auto model_for_nodes=memory.load32(model_before_nodes,frame.local(local::physics_model));
    const auto initial_nodes=memory.load32(node_count_first,at(model_for_nodes,model_field::node_count));
    if(frame.has_nodes(initial_nodes,kept.index)) {
        kept.cursor=frame.group_slot(kept.count);
        for(;;) {
            const auto mask=frame.visited_bit_mask(kept.index);
            const auto word_index=frame.visited_word_index(kept.index);
            const auto word=memory.load32(visited_word,frame.local(local::visited)+4u*word_index);
            if(!frame.bit_set(word,mask)) {
                frame.push_node_head(kept.cursor);
                const auto allow_for_node=memory.load32(allow_node,frame.local(local::arg_allow));
                frame.push_node_tail(kept.state_address,allow_for_node,kept.index);
                memory.store32(node_step_clear,frame.local(local::step_result),0u);
                if(frame.build_group(kept,false)) {
                    kept.count=frame.next_group_count(kept.count);
                    kept.cursor=frame.next_group_slot(kept.cursor);
                }
                const auto step_value=memory.load32(node_step_result,frame.local(local::step_result));
                const auto current_max=memory.load32(max_result_compare,frame.local(local::max_result));
                if(!frame.max_exceeds(current_max,step_value))
                    memory.store32(node_max_store,frame.local(local::max_result),step_value);
            }
            const auto model_next=memory.load32(model_next_node,frame.local(local::physics_model));
            const auto node_count=memory.load32(node_count_next,at(model_next,model_field::node_count));
            kept.index=frame.next_node(kept.index);
            if(!frame.below(kept.index,node_count))break;
        }
    }

    // Reserve the object's group array and instantiate each built group.
    if(frame.has_groups(kept.count)) {
        const auto capacity_raw=memory.load32(array_capacity,at(kept.state_address,state_field::groups_capacity));
        if(frame.needs_reserve(kept.state_address,capacity_raw,kept.count))
            frame.reserve_groups(kept,kept.count);
        if(frame.positive(kept.count)) {
            kept.index=frame.group_tail_start();
            for(;;) {
                const auto count_word=memory.load32(group_count_load,kept.index);
                frame.clear_group_kind();
                const auto kind_byte=memory.load8(group_kind_load,at(kept.index,4u));
                const auto payload=frame.push_group_tail(kept.index,count_word,kind_byte);
                const auto head_word=memory.load32(group_head_load,kept.index-group_entry::count);
                frame.instantiate_group(kept,payload,head_word);
                kept.index=frame.next_group_tail(kept.index);
                kept.count=frame.decrement_groups(kept.count);
                if(kept.count==0u)break;
            }
        }
    }

    frame.finalize_view(kept);
    const auto old_flags=memory.load32(state_flags_load,at(kept.state_address,state_field::flags));
    const auto new_flags=frame.set_built_flag(old_flags);
    memory.store32(state_flags_store,at(kept.state_address,state_field::flags),new_flags);
    memory.load32(result_load,frame.local(local::max_result));
}
} // namespace physics_groups
} // namespace halo2::native_physics_groups
