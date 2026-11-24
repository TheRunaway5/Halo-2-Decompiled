#pragma once
// Native TU1.5 point surface, entered at 14D790 (physics live support, map
// world support, model render and combat action units): the surface of a
// BSP leaf a point lies on.
//
//   point_surface::find   14D790(record, result) -> AL found; RET 8
//
// The record names the point (+8), its kind (+0: 1 a structure leaf, 3 an
// instance's leaf), its instance (+0x3C), its leaf (+0x4C) and the plane
// the surface must be on (+0x54, low 15 bits). A structure leaf (8 bytes in
// the structure BSP's +0x30) names its node (0xB0 bytes at +0xA0); an
// instance (0x58 bytes at +0x144) its definition (0xC8 bytes at +0x13C),
// which must have leaves (+0xB8, at +0xBC) and moves the point into its
// space first (145200). When 130980 accepts the node's or the definition's
// geometry (+0x28; EAX 3) its surfaces (+0x50) are searched. The result's
// +0x18 clears, then each of the leaf's surface references (8 bytes from
// its +4, count its +2) whose surface (+4, 8-byte surfaces of the structure
// (+0x18, +4) or the definition (+0x74)) is on the plane takes the point
// projected (171010) and the reference's three vertices (word indices at
// the surfaces' +0x24, 12-byte vertices at +8 plus +0xC of their +0x3C,
// below its count +2; a vertex out of range keeps the last); when 1211C0
// finds the point in that triangle the result takes the leaf's node (+0),
// the instance (+4), the projection (+0x18, +0x20, +0x24) and the
// reference's +2 (+0x28), and the point is found.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_point_surface.hpp) maps the role to the original
// instruction, owns the ABI of the children and leaves through the
// original's epilogue for each exit. The values the original keeps in
// callee-saved registers across the children are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_point_surface {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t structure_bsp=0x4ecc18u;}
inline constexpr std::uint32_t none=0xffffffffu,plane_mask=0x7fffu,geometry_kind=3u,leaf_size=8u,node_size=0xb0u,
    instance_size=0x58u,definition_size=0xc8u,reference_size=8u,surface_size=8u,vertex_size=12u;
namespace kind {inline constexpr std::uint32_t structure=1u,instance=3u;}
namespace record {inline constexpr std::uint32_t kind=0u,point=8u,instance=0x3cu,leaf=0x4cu,plane=0x54u;}
namespace bsp {inline constexpr std::uint32_t surfaces=0x18u,leaves=0x30u,structure_references=0x50u,nodes=0xa0u,definitions=0x13cu,
    instances=0x144u;}
namespace instance {inline constexpr std::uint32_t definition=0x34u;}
namespace geometry {inline constexpr std::uint32_t test=0x28u,surfaces=0x50u,definition_surfaces=0x74u,leaf_count=0xb8u,leaves=0xbcu,
    references=0xc4u;}
namespace leaf {inline constexpr std::uint32_t node=0u,count=2u,first=4u;}
namespace reference {inline constexpr std::uint32_t vertices=0u,extra=2u,surface=4u;}
namespace surfaces {inline constexpr std::uint32_t array=4u,vertex_lists=0x24u,vertices=0x3cu;}
namespace vertices {inline constexpr std::uint32_t count=2u,base=8u,offset=0xcu;}
namespace result {inline constexpr std::uint32_t node=0u,instance=4u,projection_y=0x18u,projection_z=0x20u,projection_x=0x24u,
    extra=0x28u,first=0x30u,second=0x34u;}
inline constexpr std::uint32_t x=0u,y=4u,z=8u;
// The frame below the saved registers (offsets from the ESP after all four
// are pushed): whether a vertex was taken, the answer, the surfaces, the
// leaf, the instance, the reference counter, the plane, the structure BSP,
// the projection, the point, the three vertices, and the two arguments.
namespace local {inline constexpr std::uint32_t taken=0x12u,answer=0x13u,surfaces=0x14u,leaf=0x18u,instance=0x1cu,index=0x20u,
    plane=0x24u,structure=0x28u,projection=0x2cu,point=0x38u,first=0x44u,second=0x50u,third=0x5cu,record=0x6cu,result=0x70u,
    size=0x58u;}
inline constexpr std::uint32_t signed_word(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}

namespace point_surface {
enum class Access {record_load,point_x,point_x_store,point_y,point_z,structure_load,point_z_store,plane_load,point_y_store,
    leaf_index_load,plane_store,instance_load,kind_load,answer_clear,structure_store,surfaces_clear,instance_store,leaves_load,
    node_index,nodes_load,leaf_store,node_surfaces,node_surfaces_store,instances_load,definitions_load,definition_index,leaf_count,
    definition_leaves,instance_leaf_store,definition_surfaces,definition_surfaces_store,leaf_reload,surfaces_check,result_load,
    result_clear,first_reference,reference_count,index_first_store,instance_test,structure_references,definition_references,
    reference_surface,instance_test_again,structure_surfaces,structure_surface_array,definition_surface_array,surface_plane,
    plane_test,reference_vertices,surfaces_again,vertex_list_load,first_index,vertices_load,first_offset,first_base,first_count,
    first_x,first_x_store,first_y,first_z,first_y_store,first_z_store,first_taken,taken_test,second_offset,second_base,
    second_index,second_count,second_x,second_x_store,second_y,second_z,second_y_store,second_z_store,second_taken,third_index,
    third_offset,third_base,third_count,third_x,third_y,third_z,third_x_store,third_y_store,third_z_store,third_taken,result_again,
    structure_reload,index_reload,leaf_again,count_again,first_again,index_store,answer_final,projection_x_load,projection_y_load,
    projection_x_store,reference_extra,projection_z_load,extra_store,leaf_last,projection_z_store,record_again,projection_y_store,
    node_word,node_store,record_instance,instance_store_result,answer_set,answer_load};
enum class Exit {leafless,unsearched,searched};
// The original's EBX (the structure BSP, later the surfaces), EBP (the leaf,
// later the reference), ESI (the node, later the vertex list) and EDI (no
// instance definition, or the instance's).
struct Kept {std::uint32_t structure,leaf_address,node,definition;};

// The answer each searching exit reads back.
template<class Memory,class Frame> Exit answered(Memory &memory,Frame &frame,Exit exit) {
    memory.load8(exit==Exit::searched?Access::answer_final:Access::answer_load,frame.local(local::answer));
    return exit;
}

// Frame: see native_tu15_point_surface.hpp.
template<class Memory,class Frame> Exit find(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The point, the plane, the leaf and the instance.
    frame.framing();
    const auto record_address=memory.load32(record_load,frame.local(local::record));
    const auto point=frame.point_address(record_address);
    memory.store32(point_x_store,frame.local(local::point+x),memory.load32(point_x,point+x));
    const auto py=memory.load32(point_y,point+y);
    const auto pz=memory.load32(point_z,point+z);
    frame.saving(1u);
    kept.structure=memory.load32(structure_load,globals::structure_bsp);
    memory.store32(point_z_store,frame.local(local::point+z),pz);
    const auto plane=frame.plane(memory.load32(plane_load,at(record_address,record::plane)));
    frame.saving(2u);
    frame.saving(3u);
    memory.store32(point_y_store,frame.local(local::point+y),py);
    const auto leaf_index=memory.load32(leaf_index_load,at(record_address,record::leaf));
    memory.store32(plane_store,frame.local(local::plane),plane);
    const auto instance_index=memory.load32(instance_load,at(record_address,record::instance));
    const auto record_kind=memory.load32(kind_load,at(record_address,record::kind));
    frame.saving(4u);
    kept.definition=frame.no_definition();
    const bool structure_leaf=frame.kind_is(record_kind,kind::structure);
    memory.store8(answer_clear,frame.local(local::answer),0u);
    memory.store32(structure_store,frame.local(local::structure),kept.structure);
    memory.store32(surfaces_clear,frame.local(local::surfaces),kept.definition);
    memory.store32(instance_store,frame.local(local::instance),instance_index);
    bool reload_leaf=true;
    if(structure_leaf) {
        // A structure leaf: its node's geometry.
        const auto leaves=memory.load32(leaves_load,at(kept.structure,bsp::leaves));
        const auto node_number=signed_word(memory.load16(node_index,leaves+leaf_size*leaf_index));
        kept.leaf_address=frame.leaf_address(leaves+leaf_size*leaf_index);
        kept.node=frame.node_address(node_number,memory.load32(nodes_load,at(kept.structure,bsp::nodes)));
        frame.test_target(kept.node+geometry::test);
        memory.store32(leaf_store,frame.local(local::leaf),kept.leaf_address);
        if(frame.accepted(kept,false))
            memory.store32(node_surfaces_store,frame.local(local::surfaces),memory.load32(node_surfaces,at(kept.node,geometry::surfaces)));
        reload_leaf=false;
    } else if(frame.kind_is(record_kind,kind::instance)) {
        // An instance's leaf, the point moved into its space.
        const auto instances=memory.load32(instances_load,at(kept.structure,bsp::instances));
        const auto instance_offset=frame.instance_offset(instance_index);
        const auto definitions=memory.load32(definitions_load,at(kept.structure,bsp::definitions));
        const auto number=signed_word(memory.load16(definition_index,instance_offset+instances+instance::definition));
        const auto definition_offset=frame.definition_offset(number);
        const auto count=memory.load32(leaf_count,definition_offset+definitions+geometry::leaf_count);
        kept.definition=frame.definition_address(definition_offset,definitions);
        if(!frame.has_leaves(count))return Exit::leafless;
        const auto leaves=memory.load32(definition_leaves,at(kept.definition,geometry::leaves));
        memory.store32(instance_leaf_store,frame.local(local::leaf),frame.instance_leaf(leaves,leaf_index));
        frame.into_instance(kept);
        frame.test_target(kept.definition+geometry::test);
        if(frame.accepted(kept,true))
            memory.store32(definition_surfaces_store,frame.local(local::surfaces),memory.load32(definition_surfaces,at(kept.definition,geometry::surfaces)));
    }
    if(reload_leaf)kept.leaf_address=memory.load32(leaf_reload,frame.local(local::leaf));
    // The leaf's surface references.
    const bool searched=frame.nonzero(memory.load32(surfaces_check,frame.local(local::surfaces)));
    memory.store32(result_clear,at(memory.load32(result_load,frame.local(local::result)),result::projection_y),0u);
    if(!searched)return answered(memory,frame,Exit::unsearched);
    auto index=memory.load32(first_reference,at(kept.leaf_address,leaf::first));
    auto end=frame.reference_end(signed_word(memory.load16(reference_count,at(kept.leaf_address,leaf::count))),index);
    const bool any=frame.below(index,end);
    memory.store32(index_first_store,frame.local(local::index),index);
    if(!any)return answered(memory,frame,Exit::unsearched);
    for(;;) {
        // The reference, its surface and that surface's plane.
        const bool in_structure=!frame.different(memory.load32(instance_test,frame.local(local::instance)),frame.no_index());
        const auto references=in_structure?memory.load32(structure_references,at(kept.structure,bsp::structure_references)):
            memory.load32(definition_references,at(kept.definition,geometry::references));
        kept.leaf_address=frame.reference_address(references,index);
        const auto surface_index=memory.load32(reference_surface,at(kept.leaf_address,reference::surface));
        bool candidate=frame.different(surface_index,none);
        if(candidate) {
            std::uint32_t array;
            if(!frame.different(memory.load32(instance_test_again,frame.local(local::instance)),none)) {
                const auto block=memory.load32(structure_surfaces,at(kept.structure,bsp::surfaces));
                array=memory.load32(structure_surface_array,at(block,surfaces::array));
            } else array=memory.load32(definition_surface_array,at(kept.definition,geometry::definition_surfaces));
            const auto surface_plane_value=signed_word(memory.load16(surface_plane,array+surface_size*surface_index));
            candidate=!frame.different(surface_plane_value,memory.load32(plane_test,frame.local(local::plane)));
        }
        if(candidate) {
            // The point projected, and the reference's three vertices.
            const auto list_index=memory.load16(reference_vertices,at(kept.leaf_address,reference::vertices));
            kept.structure=memory.load32(surfaces_again,frame.local(local::surfaces));
            const auto lists=memory.load32(vertex_list_load,at(kept.structure,surfaces::vertex_lists));
            kept.node=frame.projecting(kept,lists+2u*list_index);
            // The first vertex: its index, then the vertices.
            const auto i0=memory.load16(first_index,kept.node);
            const auto block=memory.load32(vertices_load,at(frame.surfaces_copy(kept.structure),surfaces::vertices));
            auto base=memory.load32(first_offset,at(block,vertices::offset));
            base=frame.vertex_base(0u,base,memory.load32(first_base,at(block,vertices::base)));
            bool took=false;
            if(base!=0u && frame.in_range(i0) && frame.below(i0,memory.load16(first_count,at(block,vertices::count)))) {
                const auto v=base+vertex_size*frame.vertex_offset(i0,0u);
                memory.store32(first_x_store,frame.local(local::first+x),memory.load32(first_x,v+x));
                const auto vy=memory.load32(first_y,v+y),vz=memory.load32(first_z,v+z);
                memory.store32(first_y_store,frame.local(local::first+y),vy);
                memory.store32(first_z_store,frame.local(local::first+z),vz);
                memory.store8(first_taken,frame.local(local::taken),1u);
                took=true;
            }
            if(took || frame.set(memory.load8(taken_test,frame.local(local::taken)))) {
                // The second vertex: the vertices, then its index.
                auto base1=memory.load32(second_offset,at(block,vertices::offset));
                base1=frame.vertex_base(1u,base1,memory.load32(second_base,at(block,vertices::base)));
                const auto i1=memory.load16(second_index,kept.node+2u);
                if(base1!=0u && frame.in_range(i1) && frame.below(i1,memory.load16(second_count,at(block,vertices::count)))) {
                    const auto v=base1+vertex_size*frame.vertex_offset(i1,1u);
                    memory.store32(second_x_store,frame.local(local::second+x),memory.load32(second_x,v+x));
                    const auto vy=memory.load32(second_y,v+y),vz=memory.load32(second_z,v+z);
                    memory.store32(second_y_store,frame.local(local::second+y),vy);
                    memory.store32(second_z_store,frame.local(local::second+z),vz);
                    memory.store8(second_taken,frame.local(local::taken),1u);
                }
                // The third vertex: its index, then the vertices.
                const auto i2=memory.load16(third_index,kept.node+4u);
                auto base2=memory.load32(third_offset,at(block,vertices::offset));
                base2=frame.vertex_base(2u,base2,memory.load32(third_base,at(block,vertices::base)));
                if(base2!=0u && frame.in_range(i2) && frame.below(i2,memory.load16(third_count,at(block,vertices::count)))) {
                    const auto v=base2+vertex_size*frame.vertex_offset(i2,2u);
                    const auto vx=memory.load32(third_x,v+x),vy=memory.load32(third_y,v+y),vz=memory.load32(third_z,v+z);
                    memory.store32(third_x_store,frame.local(local::third+x),vx);
                    memory.store32(third_y_store,frame.local(local::third+y),vy);
                    memory.store32(third_z_store,frame.local(local::third+z),vz);
                    memory.store8(third_taken,frame.local(local::taken),1u);
                }
                // 1211C0: the point in that triangle.
                const auto result_address=memory.load32(result_again,frame.local(local::result));
                if(frame.inside(kept,result_address)) {
                    // The result as EBX came back from 1211C0.
                    const auto out=kept.structure;
                    const auto px=memory.load32(projection_x_load,frame.local(local::projection+x));
                    const auto pyv=memory.load32(projection_y_load,frame.local(local::projection+y));
                    memory.store32(projection_x_store,at(out,result::projection_x),px);
                    const auto extra=memory.load16(reference_extra,at(kept.leaf_address,reference::extra));
                    const auto pzv=memory.load32(projection_z_load,frame.local(local::projection+z));
                    memory.store32(extra_store,at(out,result::extra),extra);
                    const auto leaf_now=memory.load32(leaf_last,frame.local(local::leaf));
                    memory.store32(projection_z_store,at(out,result::projection_z),pzv);
                    const auto record_now=memory.load32(record_again,frame.local(local::record));
                    memory.store32(projection_y_store,at(out,result::projection_y),pyv);
                    memory.store32(node_store,at(out,result::node),signed_word(memory.load16(node_word,at(leaf_now,leaf::node))));
                    memory.store32(instance_store_result,at(out,result::instance),memory.load32(record_instance,at(record_now,record::instance)));
                    memory.store8(answer_set,frame.local(local::answer),1u);
                    return answered(memory,frame,Exit::unsearched);
                }
            }
            kept.structure=memory.load32(structure_reload,frame.local(local::structure));
            index=memory.load32(index_reload,frame.local(local::index));
        }
        // The next reference.
        const auto leaf_now=memory.load32(leaf_again,frame.local(local::leaf));
        const auto count=signed_word(memory.load16(count_again,at(leaf_now,leaf::count)));
        const auto first=memory.load32(first_again,at(leaf_now,leaf::first));
        index=frame.next_index(index);
        end=frame.reference_end_again(count,first);
        const bool more=frame.below(index,end);
        memory.store32(index_store,frame.local(local::index),index);
        if(!more)break;
    }
    return answered(memory,frame,Exit::searched);
}
} // namespace point_surface
} // namespace halo2::native_point_surface
