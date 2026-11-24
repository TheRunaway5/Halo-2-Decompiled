#pragma once
// Native TU1.5 group flood, entered at 1DE8E0 (physics live support unit):
// a group of a physics model's nodes collected, and the groups joined to it
// by enabled edges flooded in turn.
//
//   group_flood::flood   1DE8E0(ECX = object; model, kinds, enabled, groups,
//                        a5, a6, group, list, count, a10, visited, failed)
//                        -> RET 30
//
// The group (or the root group for NONE) lists member nodes (the groups'
// 0x44-byte entries: a count at +0x44 (+0x4444 for the root), byte members
// from +4 (+0x4404)). Each member goes to the list (the byte list at the
// count's position, which then counts up): a member whose node (0x90 bytes
// at the model data's +0x3C) has +0x18 bit 5 and an index (+0x1C, clamped
// into the model data's node count +0x38) that is itself marks the object
// (+4 bit 19) and, when the object is not attached (+4 bit 13 clear), goes
// in as that index and marks it (+4 bit 20). The group is then visited (a
// bit of the visited set). Each enabled edge (the model data's +0x34, 0x18
// bytes, count +0x30; a bit of the enabled set) touching the group (+4 or
// +6) leads to the other end: with no other end, its pairs (12 bytes at
// +0xC, count +8) that neither DDA90 nor DDA10 accept (EAX the object's
// +8; the pair's two words pushed, RET 8) fail the flood; an other group
// with members whose first member's kind is not 6 and that is not visited
// is flooded too (itself, the group as the parent), unless its pairs, or
// with none the two groups' first kinds (7), say otherwise.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter (native_tu15_group_flood.hpp)
// maps the role to the original instruction and owns the ABI of the
// children, this routine among them. The values the original keeps in
// callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_group_flood {
using native_object_update::at;
inline constexpr std::uint32_t none=0xffffffffu,group_size=0x44u,node_size=0x90u,edge_size=0x18u,pair_size=12u;
inline constexpr std::uint32_t no_kind=6u,closed_kind=7u;
namespace object_layout {inline constexpr std::uint32_t flags=4u,inner=8u;inline constexpr unsigned attached_bit=13u;
    inline constexpr std::uint32_t indexed_mark=0x100000u,single_mark=0x80000u;}
namespace groups {inline constexpr std::uint32_t members=4u,count=0x44u,root_members=0x4404u,root_count=0x4444u;}
namespace model {inline constexpr std::uint32_t data=0x48u;}
namespace data {inline constexpr std::uint32_t edge_count=0x30u,edges=0x34u,node_count=0x38u,nodes=0x3cu;}
namespace node {inline constexpr std::uint32_t flags=0x18u,index=0x1cu;inline constexpr std::uint32_t indexed_bit=0x20u;}
namespace edge {inline constexpr std::uint32_t first=4u,second=6u,pair_count=8u,pairs=0xcu;}
namespace pair {inline constexpr std::uint32_t first=0u,second=2u;}
// The frame (offsets from the ESP after all four registers are pushed):
// the detached byte, the single byte, the object, the edge offset, the
// other group's first member, the edge index, the other group, and the
// twelve arguments.
namespace local {inline constexpr std::uint32_t detached=0x12u,single=0x13u,object=0x14u,edge_offset=0x18u,first=0x1cu,
    edge_index=0x20u,other=0x24u,model=0x2cu,kinds=0x30u,enabled=0x34u,groups=0x38u,a5=0x3cu,group=0x44u,list=0x48u,
    count=0x4cu,a10=0x50u,visited=0x54u,failed=0x58u,size=0x18u;}
inline constexpr std::uint32_t signed_byte(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(v))));}
inline constexpr std::uint32_t signed_word(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}

namespace group_flood {
enum class Access {
    flags_load,groups_load,node_load,object_store,detached_store,root_count,groups_again,group_count,root_count_again,
    group_count_again,root_member,group_member,model_load,data_load,nodes_load,node_flags_test,single_clear,node_index,node_count,
    detached_load,single_load,list_count_pointer,list_count,list_load,list_store,list_count_again,list_count_store,object_load,
    object_flag_load,object_flag_store,object_flag2_load,object_flag2_store,groups_reload,node_reload,visited_load,
    visited_mark_load,visited_mark_store,model_again,data_again,edge_count,edge_index_store,edge_offset_store,edges_load,
    edge_offset_add,enabled_load,enabled_test,edge_a,edge_b_test,edge_b,other_store,group_size,group_first,first_store,kinds_load,
    kind_test,visited_again,visited_test,pair_count,pairs_load,object_pair,pair_y,object_inner,pair_x,pair_y_again,pair_x_again,
    object_pair_again,object_inner_again,pair_count_again,groups_reload_pairs,node_reload_pairs,model_next,edge_index_load,
    edge_offset_load,data_next,edge_count_next,edge_index_store_next,edge_offset_store_next,first_none_store,group_size_other,
    pair_count_other,root_first,group_first_member,kinds_again,other_first,kind_seven,other_kind_seven,pairs_load_b,object_pair_b,
    pair_y_b,object_inner_b,pair_x_b,pair_y_b_again,pair_x_b_again,object_pair_b_again,object_inner_b_again,pair_count_b,
    failed_pointer,failed_store,other_reload,groups_reload_b,node_reload_b,first_test,argument12,argument11,argument10,argument9,
    argument8,argument3,argument2,argument5,argument1,object_argument};
enum class Exit {empty,flooded};
// The original's EBX (the groups; the member listed, in BL; the pair
// offset), EBP (the list, the pair counter), ESI (the group, the pairs) and
// EDI (the member counter, the edge).
struct Kept {std::uint32_t groups_address,base,group,counter;};

// An edge's pairs: none to check (a count not positive), all accepted by
// DDA90 or DDA10, or one accepted by neither.
enum class Pairs {none,passed,failed};
// The edge is EDI, as each child returned it.
template<class Memory,class Frame> Pairs check_pairs(Memory &memory,Frame &frame,Kept &kept,std::uint32_t count,bool second_loop) {
    using enum Access;
    kept.base=frame.first_pair();
    if(!frame.positive(count))return Pairs::none;
    kept.groups_address=frame.pair_offset_start();
    for(;;) {
        const auto pairs=memory.load32(second_loop?pairs_load_b:pairs_load,at(kept.counter,edge::pairs));
        const auto object=memory.load32(second_loop?object_pair_b:object_pair,frame.local(local::object));
        const auto py=signed_word(memory.load16(second_loop?pair_y_b:pair_y,pairs+kept.groups_address+pair::second));
        memory.load32(second_loop?object_inner_b:object_inner,at(object,object_layout::inner));
        const auto px=signed_word(memory.load16(second_loop?pair_x_b:pair_x,pairs+kept.groups_address+pair::first));
        kept.group=frame.pair_address(pairs,kept.groups_address);
        if(!frame.pair_accepted(kept,second_loop,false,py,px)) {
            // The pair as ESI came back from DDA90.
            const auto py2=signed_word(memory.load16(second_loop?pair_y_b_again:pair_y_again,at(kept.group,pair::second)));
            const auto px2=signed_word(memory.load16(second_loop?pair_x_b_again:pair_x_again,at(kept.group,pair::first)));
            const auto object2=memory.load32(second_loop?object_pair_b_again:object_pair_again,frame.local(local::object));
            memory.load32(second_loop?object_inner_b_again:object_inner_again,at(object2,object_layout::inner));
            if(!frame.pair_accepted(kept,second_loop,true,py2,px2))return Pairs::failed;
        }
        const auto count_now=memory.load32(second_loop?pair_count_b:pair_count_again,at(kept.counter,edge::pair_count));
        kept.base=frame.next_pair(kept.base);
        kept.groups_address=frame.pair_offset_next(kept.groups_address);
        if(!frame.below(kept.base,count_now))return Pairs::passed;
    }
}

// Frame: see native_tu15_group_flood.hpp.
template<class Memory,class Frame> Exit flood(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The group's member count.
    frame.framing();
    const auto object=frame.object();
    const auto flags=frame.attachment(memory.load32(flags_load,at(object,object_layout::flags)));
    frame.saving(1u);
    kept.groups_address=memory.load32(groups_load,frame.local(local::groups));
    frame.saving(2u);
    kept.group=memory.load32(node_load,frame.local(local::group));
    const auto detached=frame.detached(flags);
    const bool root=!frame.different(kept.group,none);
    memory.store32(object_store,frame.local(local::object),object);
    memory.store8(detached_store,frame.local(local::detached),detached);
    std::uint32_t count;
    if(root)count=memory.load32(root_count,at(kept.groups_address,groups::root_count));
    else {
        const auto groups_now=memory.load32(groups_again,frame.local(local::groups));
        count=memory.load32(group_count,frame.group_offset(kept.group)+groups_now);
    }
    if(!frame.positive(count))return Exit::empty;
    frame.saving(3u);
    kept.counter=frame.first_member();
    frame.saving(4u);
    // Each member into the list, marking the object.
    for(;;) {
        const bool at_root=!frame.different(kept.group,none);
        const auto members=at_root?memory.load32(root_count_again,at(kept.groups_address,groups::root_count)):
            memory.load32(group_count_again,frame.group_offset_edx(kept.group)+kept.groups_address);
        if(!frame.below(kept.counter,members))break;
        const auto member=frame.member(!frame.different(kept.group,none)?
            memory.load8(root_member,kept.counter+kept.groups_address+groups::root_members):
            memory.load8(group_member,frame.member_slot(kept.group,kept.counter)+kept.groups_address+groups::members));
        const auto model_address=memory.load32(model_load,frame.local(local::model));
        const auto data_address=memory.load32(data_load,at(model_address,model::data));
        const auto nodes=memory.load32(nodes_load,at(data_address,data::nodes));
        const auto node_address=frame.node_address(member,nodes);
        const bool indexed=frame.indexed(memory.load8(node_flags_test,at(node_address,node::flags)));
        memory.store8(single_clear,frame.local(local::single),0u);
        bool single=false,listed_index=false;
        if(indexed) {
            const auto index=signed_word(memory.load16(node_index,at(node_address,node::index)));
            const auto clamped=frame.non_negative(index)?frame.clamped(index,memory.load32(node_count,at(data_address,data::node_count))):frame.zero();
            if(frame.index_kept(clamped,index)) {
                single=frame.single_set();
                if(frame.set(memory.load8(detached_load,frame.local(local::detached)))){listed_index=true;frame.listed_as_index(index);}
            } else single=frame.single_from(memory.load8(single_load,frame.local(local::single)));
        } else single=frame.single_from(memory.load8(single_load,frame.local(local::single)));
        const auto count_pointer=memory.load32(list_count_pointer,frame.local(local::count));
        const auto position=memory.load32(list_count,count_pointer);
        kept.base=memory.load32(list_load,frame.local(local::list));
        memory.store8(list_store,position+kept.base,frame.listed());
        const auto next=frame.counted(memory.load32(list_count_again,count_pointer),listed_index);
        memory.store32(list_count_store,count_pointer,next);
        const auto object_now=memory.load32(object_load,frame.local(local::object));
        if(listed_index)memory.store32(object_flag_store,at(object_now,object_layout::flags),
            frame.marked(memory.load32(object_flag_load,at(object_now,object_layout::flags)),object_layout::indexed_mark));
        if(frame.single_tested(single))memory.store32(object_flag2_store,at(object_now,object_layout::flags),
            frame.marked(memory.load32(object_flag2_load,at(object_now,object_layout::flags)),object_layout::single_mark));
        kept.groups_address=memory.load32(groups_reload,frame.local(local::groups));
        kept.group=memory.load32(node_reload,frame.local(local::group));
        kept.counter=frame.next_member(kept.counter);
    }
    // The group is visited.
    if(frame.different(kept.group,none)) {
        const auto visited=memory.load32(visited_load,frame.local(local::visited));
        const auto word=visited+4u*frame.group_word(kept.group);
        const auto bit=frame.group_bit(kept.group);
        memory.store32(visited_mark_store,word,frame.marked(memory.load32(visited_mark_load,word),bit));
    }
    // Each enabled edge touching it.
    const auto model_address=memory.load32(model_again,frame.local(local::model));
    auto data_address=memory.load32(data_again,at(model_address,model::data));
    const auto edges_total=memory.load32(edge_count,at(data_address,data::edge_count));
    auto index=frame.first_edge();
    const bool any=frame.edges_present(edges_total);
    memory.store32(edge_index_store,frame.local(local::edge_index),index);
    if(!any)return Exit::flooded;
    memory.store32(edge_offset_store,frame.local(local::edge_offset),index);
    for(;;) {
        const auto edges_address=memory.load32(edges_load,at(data_address,data::edges));
        kept.counter=frame.edge_address(edges_address,memory.load32(edge_offset_add,frame.local(local::edge_offset)));
        const auto bit=frame.edge_bit(index);
        const auto enabled=memory.load32(enabled_load,frame.local(local::enabled));
        if(frame.active(memory.load32(enabled_test,enabled+4u*frame.edge_word(index)),bit)) {
            // The other end.
            const auto a=memory.load16(edge_a,at(kept.counter,edge::first));
            std::uint32_t other=none;
            bool touches=true;
            if(!frame.first_touches(a,kept.group)) {
                if(frame.second_touches(signed_word(memory.load16(edge_b_test,at(kept.counter,edge::second))),kept.group))other=frame.other_is_first(a);
                else touches=false;
            } else other=frame.other_is_second(memory.load16(edge_b,at(kept.counter,edge::second)));
            if(touches) {
                // The other group, unless its first member's kind or the visited set rule it out.
                const bool named=frame.named(other);
                memory.store32(other_store,frame.local(local::other),other);
                bool considered=true,first_stored=false;
                if(named) {
                    const auto entry=frame.group_entry(other,kept.groups_address);
                    if(frame.positive(memory.load32(group_size,at(entry,groups::count)))) {
                        const auto first=signed_byte(memory.load8(group_first,at(entry,groups::members)));
                        const bool has_first=frame.named(first);
                        memory.store32(first_store,frame.local(local::first),first);
                        first_stored=true;
                        if(has_first) {
                            const auto kinds=memory.load32(kinds_load,frame.local(local::kinds));
                            if(frame.kind_is(memory.load8(kind_test,first+kinds),no_kind))considered=false;
                            else {
                                const auto visited=memory.load32(visited_again,frame.local(local::visited));
                                const auto other_bit=frame.other_bit(other);
                                if(frame.active(memory.load32(visited_test,visited+4u*frame.other_word(other)),other_bit))considered=false;
                            }
                        }
                    }
                }
                if(considered) {
                    if(!first_stored)memory.store32(first_none_store,frame.local(local::first),none);
                    bool flood_it=false;
                    if(!frame.named(other)) {
                        // No other group: its pairs must pass, or the flood fails.
                        const auto result=check_pairs(memory,frame,kept,memory.load32(pair_count,at(kept.counter,edge::pair_count)),false);
                        if(result!=Pairs::none) {
                            if(result==Pairs::failed)memory.store8(failed_store,memory.load32(failed_pointer,frame.local(local::failed)),1u);
                            kept.groups_address=memory.load32(groups_reload_pairs,frame.local(local::groups));
                            kept.group=memory.load32(node_reload_pairs,frame.local(local::group));
                        }
                    } else {
                        const auto entry=frame.group_entry_again(other,kept.groups_address);
                        if(!frame.positive(memory.load32(group_size_other,at(entry,groups::count))))flood_it=true;
                        else {
                            const auto count=memory.load32(pair_count_other,at(kept.counter,edge::pair_count));
                            if(!frame.nonzero(count)) {
                                // No pairs: neither group's first member may be closed.
                                const auto own_first=!frame.different(kept.group,none)?
                                    memory.load8(root_first,at(kept.groups_address,groups::root_members)):
                                    memory.load8(group_first_member,frame.member_slot_eax(kept.group)+kept.groups_address+groups::members);
                                const auto kinds=memory.load32(kinds_again,frame.local(local::kinds));
                                const auto other_first_member=memory.load8(other_first,at(entry,groups::members));
                                if(!frame.kind_is(memory.load8(kind_seven,frame.widened_own(own_first)+kinds),closed_kind) &&
                                   !frame.kind_is(memory.load8(other_kind_seven,frame.widened_other(other_first_member)+kinds),closed_kind))flood_it=true;
                            } else if(check_pairs(memory,frame,kept,count,true)==Pairs::failed) {
                                other=memory.load32(other_reload,frame.local(local::other));
                                kept.groups_address=memory.load32(groups_reload_b,frame.local(local::groups));
                                kept.group=memory.load32(node_reload_b,frame.local(local::group));
                                flood_it=true;
                            } else {
                                kept.groups_address=memory.load32(groups_reload_pairs,frame.local(local::groups));
                                kept.group=memory.load32(node_reload_pairs,frame.local(local::group));
                            }
                        }
                    }
                    // The other group flooded in turn, this group its parent.
                    if(flood_it && frame.different(memory.load32(first_test,frame.local(local::first)),none)) {
                        memory.load32(argument12,frame.local(local::failed));
                        memory.load32(argument11,frame.local(local::visited));
                        frame.argument_pushed(0u);
                        memory.load32(argument10,frame.local(local::a10));
                        frame.argument_pushed(1u);
                        memory.load32(argument9,frame.local(local::count));
                        frame.argument_pushed(2u);
                        memory.load32(argument8,frame.local(local::list));
                        frame.argument_pushed(3u);
                        memory.load32(argument3,frame.local(local::enabled));
                        frame.argument_pushed(4u);
                        memory.load32(argument2,frame.local(local::kinds));
                        frame.argument_pushed(5u);
                        memory.load32(argument5,frame.local(local::a5));
                        frame.argument_pushed(6u);
                        frame.argument_pushed(7u);
                        memory.load32(argument1,frame.local(local::model));
                        frame.argument_pushed(8u);
                        frame.argument_pushed(9u);
                        frame.argument_pushed(10u);
                        memory.load32(object_argument,frame.local(local::object));
                        frame.flood_other(kept);
                    }
                }
            }
        }
        // The next edge.
        const auto model_now=memory.load32(model_next,frame.local(local::model));
        index=memory.load32(edge_index_load,frame.local(local::edge_index));
        const auto offset=memory.load32(edge_offset_load,frame.local(local::edge_offset));
        data_address=memory.load32(data_next,at(model_now,model::data));
        const auto edges_count=memory.load32(edge_count_next,at(data_address,data::edge_count));
        index=frame.next_edge(index);
        const bool more=frame.below(index,edges_count);
        memory.store32(edge_index_store_next,frame.local(local::edge_index),index);
        memory.store32(edge_offset_store_next,frame.local(local::edge_offset),frame.edge_offset_next(offset));
        if(!more)break;
    }
    return Exit::flooded;
}
} // namespace group_flood
} // namespace halo2::native_group_flood
