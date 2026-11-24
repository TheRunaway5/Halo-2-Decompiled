#pragma once
// Native TU1.5 object point surface, entered at 14D410 (physics live
// support, model render and combat action units): the surface of an
// object's collision model a point lies on.
//
//   object_point_surface::find   14D410(record, model tag, result) -> AL found; RET C
//
// The result's fields clear to NONE. The record names the object (+0x40;
// [4ECBD0], 12-byte entries, the address at +8), its region (+0x44), node
// (+0x46), shape (+0x48), leaf (+0x4C), plane (+0x54, low 15 bits) and the
// point (+8). The object's permutation for the region (the byte at the
// object's +0x11A offset plus the region; 0xFF for none) picks, through the
// model's regions (16 bytes at +0x20, their permutations 16 bytes at +0xC),
// the result's BSP (+0x14: the permutation's word +0xC, else 0xFF). With a
// BSP (0x5C bytes at the model's +0x28) whose geometry (+0x38) 130980
// accepts (EAX 3), the point moves into the node's space (145200 twice: the
// object's node matrix, 0x34 bytes at its +0x116 offset when it has nodes,
// +0x114 bytes of them; then the model's node, 0x60 bytes at +0x4C, its
// +0x28) and 1F7E00 (ESI a buffer, the object pushed) must answer; 1F8040
// (the record's shape and the buffer pushed) gives the shape. Among the
// BSP's parts (its data +0x34: +0x64 node bytes at +0x68) the record's node
// names a part (16 bytes at the model's +0x80 entry +4) whose key (+0) must
// be the shape's (+0x10). Each of the leaf's surface references (8 bytes at
// the part's +0xC, below its +8; the leaf, 8 bytes at the part's +4, names
// the first and +2 the count) whose surface (+4; NONE ends the search),
// below the shape's count, is on the plane (the shape's 8-byte surfaces at
// +4) has its three vertices (243F20: EAX the model's +0x18, EDI the
// material, ECX the vertex; the index pushed; word indices in the BSP
// data's +0x24 list) and 1211C0 tests the point in the triangle (ESI the
// point, EAX, EDX and ECX the vertices; the result's +0x30 and +0x34
// pushed). Found, the result takes the object (+8), the section (+0x20,
// 0x48 bytes at the BSP data's +4, whose words +6 and +8 hold the result's
// first word), the reference's two words (+0x24, +0x28), the tag (+0xC),
// the region (+0x1C) and the material (+0x2C), its +0x18 NONE.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_object_point_surface.hpp) maps the role to the original
// instruction, owns the ABI of the children and leaves through the
// original's epilogue for each exit. The values the original keeps in
// callee-saved registers across the children are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_point_surface {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t definitions=0x4f0414u,objects=0x4ecbd0u;}
inline constexpr std::uint32_t datum_data=0x44u,none=0xffffffffu,handle_mask=0xffffu,no_permutation=0xffu,no_bsp=0xffu,
    plane_mask=0x7fffu,geometry_kind=3u,node_matrix_size=0x34u,triangle=3u;
namespace definition_entry {inline constexpr std::uint32_t size=16u,address=8u;}
namespace entry {inline constexpr std::uint32_t size=12u,address=8u;}
namespace record {inline constexpr std::uint32_t point=8u,object=0x40u,region=0x44u,node=0x46u,shape=0x48u,leaf=0x4cu,plane=0x54u;}
namespace object {inline constexpr std::uint32_t node_bytes=0x114u,nodes=0x116u,region_permutations=0x11au;}
namespace model {inline constexpr std::uint32_t vertices=0x18u,regions=0x20u,bsps=0x28u,nodes=0x4cu,shapes=0x80u;}
namespace model_region {inline constexpr std::uint32_t size=16u,permutations=0xcu;}
namespace model_permutation {inline constexpr std::uint32_t size=16u,bsp=0xcu;}
namespace model_node {inline constexpr std::uint32_t size=0x60u,matrix=0x28u;}
namespace bsp {inline constexpr std::uint32_t size=0x5cu,data=0x34u,geometry=0x38u;}
namespace bsp_data {inline constexpr std::uint32_t section_count=0u,sections=4u,vertex_lists=0x24u,part_count=0x64u,part_nodes=0x68u;}
namespace section {inline constexpr std::uint32_t size=0x48u,first=6u,count=8u;}
namespace shape_reference {inline constexpr std::uint32_t size=8u,parts=4u;}
namespace part {inline constexpr std::uint32_t size=16u,key=0u,leaves=4u,reference_count=8u,references=0xcu;}
namespace leaf {inline constexpr std::uint32_t size=8u,count=2u,first=4u;}
namespace reference {inline constexpr std::uint32_t size=8u,vertices=0u,second=2u,surface=4u;}
namespace shape {inline constexpr std::uint32_t surface_count=0u,surfaces=4u,key=0x10u;inline constexpr std::uint32_t surface_size=8u;}
namespace result {inline constexpr std::uint32_t object=8u,tag=0xcu,spare=0x10u,bsp=0x14u,hit=0x18u,region=0x1cu,section=0x20u,
    first_word=0x24u,second_word=0x28u,material=0x2cu,first=0x30u,second=0x34u;}
inline constexpr std::uint32_t x=0u,y=4u,z=8u,vertex_size=12u;
// The frame (offsets from the ESP after EBX, EBP, ESI and EDI are pushed):
// the shape, the BSP, the part, the reference index, the shape reference
// (later the BSP data), the vertex list, the model's vertices, the leaf,
// the reference, the material, the point, the buffer (later the triangle),
// and the three arguments (the result's slot later the answer byte).
namespace local {inline constexpr std::uint32_t shape=0x10u,bsp=0x14u,part=0x18u,index=0x1cu,shape_reference=0x20u,
    vertex_list=0x24u,vertices=0x28u,leaf=0x2cu,reference=0x30u,material=0x34u,point=0x38u,buffer=0x44u,triangle=0x44u,
    record=0x6cu,tag=0x70u,result=0x74u,answer=0x74u;}
inline constexpr std::uint32_t signed_byte(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(v))));}
inline constexpr std::uint32_t signed_word(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}

namespace object_point_surface {
enum class Access {
    definitions_load,result_argument,clear_hit,clear_object,clear_bsp,clear_section,clear_first_word,clear_second_word,clear_tag,
    clear_spare,tag_argument,model_load,record_argument,object_handle,objects_load,objects_data,object_address,permutations_offset,
    record_region,region_permutation_test,found_clear,record_region_again,region_permutation,model_regions,region_permutations,
    permutation_bsp,bsp_store,bsp_none_store,bsp_load,model_bsps,model_vertices,vertices_store,bsp_address_store,
    object_handle_again,objects_again,objects_data_again,object_again,point_x,point_y,local_x_store,point_z,local_y_store,
    node_bytes,local_z_store,record_node,nodes_offset,record_node_again,model_nodes,object_handle_pushed,record_shape,result_bsp,
    model_shapes,shape_store,bsp_address_load,bsp_data,part_count,shape_reference_store,part_nodes,record_node_search,part_node,
    unmatched_answer,shape_reference_load,parts,shape_load,shape_key,part_key,part_store,record_leaf,part_leaves,leaf_first,
    leaf_store,leaf_count,reference_index_store,shape_reload,reference_count_test,references,reference_surface,reference_store,
    surface_count_test,record_plane,surfaces,surface_plane,bsp_address_again,bsp_data_again,reference_vertices,vertex_lists,
    bsp_data_store,vertex_list_store,bsp_data_reload,vertex_list_load,vertex_index,vertices_load,reference_index_reload,
    record_reload,part_reload,leaf_reload,leaf_count_next,leaf_first_next,reference_index_next,exhausted_answer,record_hit,
    hit_clear,hit_object,hit_data,hit_object_store,hit_section_store,section_count,found_store,sections,result_first_word,
    section_first,section_size,section_store,reference_load,reference_first_word,tag_reload,result_first_store,
    reference_second_word,record_final,result_tag_store,material_load,result_second_store,record_region_final,result_region_store,
    result_material_store,answer};
// The original's exits, each with its own epilogue: no part names the
// node, the references run out, and every other.
enum class Exit {unmatched,exhausted,done};
// The original's EBX (the model, later the part's node, the parts, each
// reference and each vertex), EBP (the result), ESI (the object, later the
// buffer and the vertex counter) and EDI (the record).
struct Kept {std::uint32_t cursor,result,item,record;};

// Frame: see native_tu15_object_point_surface.hpp.
template<class Memory,class Frame> Exit find(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The result cleared and the model.
    frame.framing();
    const auto definitions=memory.load32(definitions_load,globals::definitions);
    const auto none_value=frame.none_value();
    frame.saving(1u);
    frame.saving(2u);
    kept.result=memory.load32(result_argument,frame.local(local::result));
    memory.store32(clear_hit,at(kept.result,result::hit),none_value);
    memory.store32(clear_object,at(kept.result,result::object),none_value);
    memory.store32(clear_bsp,at(kept.result,result::bsp),none_value);
    memory.store32(clear_section,at(kept.result,result::section),none_value);
    memory.store32(clear_first_word,at(kept.result,result::first_word),none_value);
    memory.store32(clear_second_word,at(kept.result,result::second_word),none_value);
    memory.store32(clear_tag,at(kept.result,result::tag),none_value);
    memory.store32(clear_spare,at(kept.result,result::spare),none_value);
    const auto tag_index=frame.tag_index(memory.load32(tag_argument,frame.local(local::tag)));
    kept.cursor=memory.load32(model_load,definitions+definition_entry::size*tag_index+definition_entry::address);
    frame.saving(3u);
    frame.saving(4u);
    kept.record=memory.load32(record_argument,frame.local(local::record));

    // The region's permutation and its BSP.
    const auto object_index=frame.object_index(memory.load32(object_handle,at(kept.record,record::object)));
    const auto objects_header=memory.load32(objects_load,globals::objects);
    const auto objects=memory.load32(objects_data,at(objects_header,datum_data));
    const auto object_base=memory.load32(object_address,objects+entry::size*object_index+entry::address);
    const auto offset=signed_word(memory.load16(permutations_offset,at(object_base,object::region_permutations)));
    const auto permutations=frame.permutations_address(offset,object_base);
    const auto region=signed_word(memory.load16(record_region,at(kept.record,record::region)));
    const bool has_permutation=frame.permutation_named(memory.load8(region_permutation_test,region+permutations));
    memory.store8(found_clear,frame.local(local::answer),0u);
    if(has_permutation) {
        const auto region_again=signed_word(memory.load16(record_region_again,at(kept.record,record::region)));
        const auto permutation=signed_byte(memory.load8(Access::region_permutation,region_again+permutations));
        const auto regions=memory.load32(model_regions,at(kept.cursor,model::regions));
        const auto region_offset=frame.region_offset(region);
        const auto region_list=memory.load32(region_permutations,regions+region_offset+model_region::permutations);
        const auto permutation_offset=frame.permutation_offset(permutation);
        const auto bsp_index=signed_word(memory.load16(permutation_bsp,region_list+permutation_offset+model_permutation::bsp));
        memory.store32(bsp_store,at(kept.result,result::bsp),bsp_index);
    } else memory.store32(bsp_none_store,at(kept.result,result::bsp),no_bsp);
    const auto bsp_index=memory.load32(bsp_load,at(kept.result,result::bsp));
    if(!frame.has_bsp(bsp_index)) {
        memory.load8(answer,frame.local(local::answer));
        return Exit::done;
    }
    const auto bsps=memory.load32(model_bsps,at(kept.cursor,model::bsps));
    const auto bsp_offset=frame.bsp_offset(bsp_index);
    const auto vertices=memory.load32(model_vertices,at(kept.cursor,model::vertices));
    const auto bsp_address=frame.bsp_address(bsp_offset,bsps);
    memory.store32(vertices_store,frame.local(local::vertices),vertices);
    memory.store32(bsp_address_store,frame.local(local::bsp),bsp_address);
    if(!frame.geometry_accepted(kept,bsp_address+bsp::geometry)) {
        memory.load8(answer,frame.local(local::answer));
        return Exit::done;
    }

    // The point in the node's space, and the object's shape.
    const auto object_index_again=frame.object_index(memory.load32(object_handle_again,at(kept.record,record::object)));
    const auto objects_header_again=memory.load32(objects_again,globals::objects);
    const auto objects_now=memory.load32(objects_data_again,at(objects_header_again,datum_data));
    kept.item=memory.load32(object_again,objects_now+entry::size*object_index_again+entry::address);
    const auto point=frame.point_address(kept.record);
    const auto px=memory.load32(point_x,point+x);
    const auto py=memory.load32(point_y,point+y);
    memory.store32(local_x_store,frame.local(local::point+x),px);
    const auto pz=memory.load32(point_z,point+z);
    memory.store32(local_y_store,frame.local(local::point+y),py);
    const auto node_bytes_value=signed_word(memory.load16(node_bytes,at(kept.item,object::node_bytes)));
    memory.store32(local_z_store,frame.local(local::point+z),pz);
    if(frame.has_nodes(node_bytes_value)) {
        const auto node=signed_word(memory.load16(record_node,at(kept.record,record::node)));
        const auto nodes=signed_word(memory.load16(nodes_offset,at(kept.item,object::nodes)));
        const auto moved=frame.into_node(kept,frame.node_matrix(node,nodes,kept.item));
        const auto node_again=signed_word(memory.load16(record_node_again,at(kept.record,record::node)));
        const auto model_node_list=memory.load32(model_nodes,at(kept.cursor,model::nodes));
        frame.into_model(kept,frame.model_matrix(node_again,model_node_list),moved);
    }
    frame.object_pushed(memory.load32(object_handle_pushed,at(kept.record,record::object)));
    if(!frame.buffered(kept)) {
        memory.load8(answer,frame.local(local::answer));
        return Exit::done;
    }
    const auto shape_address=frame.shaped(kept,memory.load32(record_shape,at(kept.record,record::shape)));

    // The part the node names.
    const auto bsp_now=memory.load32(result_bsp,at(kept.result,result::bsp));
    const auto shapes=memory.load32(model_shapes,at(kept.cursor,model::shapes));
    memory.store32(shape_store,frame.local(local::shape),shape_address);
    const auto shape_reference_address=frame.shape_reference(shapes,bsp_now);
    const auto bsp_saved=memory.load32(bsp_address_load,frame.local(local::bsp));
    const auto data=memory.load32(Access::bsp_data,at(bsp_saved,bsp::data));
    const auto parts_total=memory.load32(part_count,at(data,bsp_data::part_count));
    const bool parted=frame.positive(parts_total);
    memory.store32(shape_reference_store,frame.local(local::shape_reference),shape_reference_address);
    if(!parted) {
        memory.load8(answer,frame.local(local::answer));
        return Exit::done;
    }
    const auto node_list=memory.load32(part_nodes,at(data,bsp_data::part_nodes));
    auto part_index=frame.first_part();
    if(!frame.positive(parts_total)) {
        memory.load8(answer,frame.local(local::answer));
        return Exit::done;
    }
    const auto node=signed_word(memory.load16(record_node_search,at(kept.record,record::node)));
    for(;;) {
        kept.cursor=memory.load8(part_node,part_index+node_list);
        if(frame.node_matches(kept.cursor,node))break;
        part_index=frame.next_part(part_index);
        if(!frame.below(part_index,parts_total)) {
            memory.load8(unmatched_answer,frame.local(local::answer));
            return Exit::unmatched;
        }
    }
    if(!frame.named(part_index)) {
        memory.load8(answer,frame.local(local::answer));
        return Exit::done;
    }
    const auto shape_reference_now=memory.load32(shape_reference_load,frame.local(local::shape_reference));
    kept.cursor=memory.load32(parts,at(shape_reference_now,shape_reference::parts));
    auto shape_now=memory.load32(shape_load,frame.local(local::shape));
    kept.item=memory.load32(shape_key,at(shape_now,shape::key));
    const auto part_offset=frame.part_offset(part_index);
    const auto key=memory.load32(part_key,part_offset+kept.cursor+part::key);
    auto part_address=frame.part_address(part_offset,kept.cursor);
    const bool keyed=!frame.different(key,kept.item);
    memory.store32(part_store,frame.local(local::part),part_address);
    if(!keyed) {
        memory.load8(answer,frame.local(local::answer));
        return Exit::done;
    }

    // The leaf's references on the plane.
    const auto leaf_index=memory.load32(record_leaf,at(kept.record,record::leaf));
    const auto leaves=memory.load32(part_leaves,at(part_address,part::leaves));
    kept.item=frame.leaf_address(leaves+leaf::size*leaf_index);
    auto index=memory.load32(leaf_first,at(kept.item,leaf::first));
    memory.store32(leaf_store,frame.local(local::leaf),kept.item);
    const auto first_count=signed_word(memory.load16(leaf_count,at(kept.item,leaf::count)));
    const auto end=frame.leaf_end(first_count,index);
    const bool any=frame.below(index,end);
    memory.store32(reference_index_store,frame.local(local::index),index);
    if(!any) {
        memory.load8(answer,frame.local(local::answer));
        return Exit::done;
    }
    for(bool first=true;;first=false) {
        if(!first)shape_now=memory.load32(shape_reload,frame.local(local::shape));
        if(frame.non_negative(index) &&
           frame.below(index,memory.load32(reference_count_test,at(part_address,part::reference_count)))) {
            const auto reference_list=memory.load32(references,at(part_address,part::references));
            kept.cursor=frame.reference_address(reference_list+reference::size*index);
            kept.item=memory.load32(reference_surface,at(kept.cursor,reference::surface));
            const bool surfaced=frame.named(kept.item);
            memory.store32(reference_store,frame.local(local::reference),kept.cursor);
            if(!surfaced) {
                memory.load8(answer,frame.local(local::answer));
                return Exit::done;
            }
            if(frame.below(kept.item,memory.load32(surface_count_test,at(shape_now,shape::surface_count)))) {
                const auto plane_value=memory.load32(record_plane,at(kept.record,record::plane));
                const auto surface_list=memory.load32(surfaces,at(shape_now,shape::surfaces));
                const auto plane=frame.plane(plane_value);
                if(frame.on_plane(memory.load16(surface_plane,surface_list+shape::surface_size*kept.item),plane)) {
                    // The triangle.
                    const auto bsp_again=memory.load32(bsp_address_again,frame.local(local::bsp));
                    auto data_again=memory.load32(bsp_data_again,at(bsp_again,bsp::data));
                    const auto vertex_index_list=memory.load16(reference_vertices,at(kept.cursor,reference::vertices));
                    const auto lists=memory.load32(vertex_lists,at(data_again,bsp_data::vertex_lists));
                    const auto list=frame.vertex_list(lists+2u*vertex_index_list);
                    memory.store32(bsp_data_store,frame.local(local::shape_reference),data_again);
                    memory.store32(vertex_list_store,frame.local(local::vertex_list),list);
                    kept.item=frame.first_vertex();
                    kept.cursor=frame.triangle_address();
                    for(bool vertex_first=true;;vertex_first=false) {
                        if(!vertex_first)data_again=memory.load32(bsp_data_reload,frame.local(local::shape_reference));
                        const auto list_now=memory.load32(vertex_list_load,frame.local(local::vertex_list));
                        frame.vertex_pushed(memory.load16(vertex_index,list_now+2u*kept.item));
                        frame.vertex(kept,memory.load32(vertices_load,frame.local(local::vertices)));
                        kept.item=frame.next_vertex(kept.item);
                        kept.cursor=frame.next_vertex_address(kept.cursor);
                        if(!frame.vertices_left(kept.item))break;
                    }
                    if(frame.inside(kept)) {
                        // Found: the result.
                        const auto record_now=memory.load32(record_hit,frame.local(local::record));
                        memory.store32(hit_clear,at(kept.result,result::hit),none);
                        const auto hit_object_value=memory.load32(hit_object,at(record_now,record::object));
                        const auto hit_data_value=memory.load32(hit_data,frame.local(local::shape_reference));
                        auto section_index=frame.first_section();
                        memory.store32(hit_object_store,at(kept.result,result::object),hit_object_value);
                        memory.store32(hit_section_store,at(kept.result,result::section),section_index);
                        const auto sections_total=memory.load32(section_count,at(hit_data_value,bsp_data::section_count));
                        const bool sectioned=frame.above(sections_total,section_index);
                        memory.store8(found_store,frame.local(local::answer),1u);
                        if(sectioned) {
                            const auto section_list=memory.load32(Access::sections,at(hit_data_value,bsp_data::sections));
                            const auto first_word=memory.load32(result_first_word,at(kept.result,result::first_word));
                            // The cursor on each section's first word.
                            auto cursor=frame.section_cursor(section_list);
                            for(;;) {
                                const auto start=memory.load16(section_first,cursor);
                                if(!frame.below(first_word,start)) {
                                    const auto size=memory.load16(section_size,cursor+section::count-section::first);
                                    if(frame.below(first_word,frame.section_end(size,start))) {
                                        memory.store32(section_store,at(kept.result,result::section),section_index);
                                        break;
                                    }
                                }
                                section_index=frame.next_section(section_index);
                                cursor=frame.next_section_cursor(cursor);
                                if(!frame.below(section_index,sections_total))break;
                            }
                        }
                        const auto reference_now=memory.load32(reference_load,frame.local(local::reference));
                        const auto first_value=memory.load16(reference_first_word,at(reference_now,reference::vertices));
                        const auto tag_now=memory.load32(tag_reload,frame.local(local::tag));
                        memory.store32(result_first_store,at(kept.result,result::first_word),first_value);
                        const auto second_value=memory.load16(reference_second_word,at(reference_now,reference::second));
                        const auto record_last=memory.load32(record_final,frame.local(local::record));
                        memory.store32(result_tag_store,at(kept.result,result::tag),tag_now);
                        const auto material=memory.load32(material_load,frame.local(local::material));
                        memory.store32(result_second_store,at(kept.result,result::second_word),second_value);
                        const auto region_now=signed_word(memory.load16(record_region_final,at(record_last,record::region)));
                        memory.store32(result_region_store,at(kept.result,result::region),region_now);
                        memory.store32(result_material_store,at(kept.result,result::material),material);
                        memory.load8(answer,frame.local(local::answer));
                        return Exit::done;
                    }
                    index=memory.load32(reference_index_reload,frame.local(local::index));
                    kept.record=memory.load32(record_reload,frame.local(local::record));
                }
                part_address=memory.load32(part_reload,frame.local(local::part));
            }
        }
        // The next reference.
        const auto leaf_now=memory.load32(leaf_reload,frame.local(local::leaf));
        const auto count_now=signed_word(memory.load16(leaf_count_next,at(leaf_now,leaf::count)));
        kept.cursor=memory.load32(leaf_first_next,at(leaf_now,leaf::first));
        index=frame.next_reference(index);
        const auto end_now=frame.leaf_end(count_now,kept.cursor);
        const bool more=frame.below(index,end_now);
        memory.store32(reference_index_next,frame.local(local::index),index);
        if(!more) {
            memory.load8(exhausted_answer,frame.local(local::answer));
            return Exit::exhausted;
        }
    }
}
} // namespace object_point_surface
} // namespace halo2::native_object_point_surface
