#pragma once
#include "halo2/native_tu15_bsp_surface_2d_kernel.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_bsp_surface_polygon_detail {
using Scalar=native_bsp_surface_2d_detail::Scalar;
using native_bsp_surface_2d_detail::scalar;
using native_bsp_surface_2d_detail::multiply;
using native_bsp_surface_2d_detail::subtract;
inline std::uint32_t signed_half(std::uint32_t value){return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));}
struct Comparison {bool above{};std::uint32_t flags{};};
inline Comparison compare_zero(Scalar value){
    unsigned char below{},equal{},unordered{};const Scalar zero{};
    asm volatile("comiss %4,%3;setb %0;sete %1;setp %2":"=qm"(below),"=qm"(equal),"=qm"(unordered):"x"(value),"x"(zero):"cc","memory");
    return {!below&&!equal,2u|std::uint32_t(below)|(std::uint32_t(unordered)<<2u)|(std::uint32_t(equal)<<6u)};
}
enum class Phase {surfaces,surface_flags,material,material_limit,material_reload,material_word,
    edges,vertices,axis,first_edge,cache_edges,winding,first_component,save_bp,second_component,
    save_first_edge,save_vertices,edge_cursor,edge_surface,surface_compare,first_vertex,
    first_x,first_y,point_source,point_x,point_y,second_vertex,second_vertices,second_x,second_y,
    next_edge,loop_limit,finished};
enum class Exit {bitmap,outside,inside};
struct Progress {
    std::uint32_t descriptor{},bitmap{},surface{},surfaces{},record{},flags{},material{},limit{},current_material{},mask{},word_index{},word{};
    std::uint32_t edges{},vertices{},axis{},first_edge{},winding{},table_index{},first_component{},second_component{},first_offset{},second_offset{};
    std::uint32_t index{},edge_offset{},current_edges{},edge_surface{},edge{},current_surface{},selected{},first_vertex{},first_position{},point{};
    std::uint32_t second_vertex{},second_position{},next{},loop_limit{};
    Scalar first_x{},first_y{},point_x{},point_y{},dx{},dy{},second_x{},second_y{},edge_x{},edge_y{},product_y{},product_x{},cross{};
    Comparison comparison;Exit exit{Exit::bitmap};bool accepted{};
};
// Whole1E6E80 DATA algorithm. Inputs retain the public value contract or the
// caller's authored scratch/argument rereads; Notify owns the separate ABI.
// Every edge is tested in source order, including repeated vertex/point reads.
template<class Memory,class Inputs,class Point,class Notify>
bool contains(Memory& memory,std::uint32_t descriptor,std::uint32_t bitmap,std::uint32_t surface,
    Inputs& inputs,Point& point,Notify notify){
    Progress d{};d.descriptor=descriptor;d.bitmap=bitmap;d.surface=surface;
    notify(Phase::surfaces,d);d.surfaces=memory.word(descriptor+0x2cu);d.record=d.surfaces+surface*8u;
    if(bitmap){
        notify(Phase::surface_flags,d);d.flags=memory.word(d.record+4u,1)&8u;
        if(d.flags){
            notify(Phase::material,d);d.material=memory.word(d.record+5u,1);
            notify(Phase::material_limit,d);d.limit=inputs.limit();
            if(std::bit_cast<std::int32_t>(signed_half(d.material))<std::bit_cast<std::int32_t>(signed_half(d.limit))){
                notify(Phase::material_reload,d);d.current_material=memory.word(d.record+5u,1);
                d.mask=1u<<(d.current_material&31u);d.word_index=d.current_material>>5u;
                notify(Phase::material_word,d);d.word=memory.word(bitmap+d.word_index*4u);
                if(!(d.word&d.mask)){notify(Phase::finished,d);return false;}
            }
        }
    }
    notify(Phase::edges,d);d.edges=memory.word(descriptor+0x34u);
    notify(Phase::vertices,d);d.vertices=memory.word(descriptor+0x3cu);
    notify(Phase::axis,d);d.axis=signed_half(inputs.axis());
    notify(Phase::first_edge,d);d.first_edge=memory.word(d.record+2u,2);
    notify(Phase::cache_edges,d);
    notify(Phase::winding,d);d.winding=inputs.winding();d.table_index=(d.winding+d.axis*2u)*6u;
    notify(Phase::first_component,d);d.first_component=signed_half(memory.word(0x44ac8cu+d.table_index,2));
    notify(Phase::save_bp,d);
    notify(Phase::second_component,d);d.second_component=signed_half(memory.word(0x44ac8eu+d.table_index,2));
    d.first_offset=d.first_component*4u;notify(Phase::save_first_edge,d);notify(Phase::save_vertices,d);
    d.second_offset=d.second_component*4u;d.index=d.first_edge;
    for(;;){
        d.edge_offset=d.index*12u;notify(Phase::edge_cursor,d);d.current_edges=inputs.edges(d.edges);
        notify(Phase::edge_surface,d);d.edge_surface=signed_half(memory.word(d.current_edges+d.edge_offset+10u,2));d.edge=d.current_edges+d.edge_offset;
        notify(Phase::surface_compare,d);d.current_surface=inputs.surface(surface);d.selected=d.edge_surface==d.current_surface?1u:0u;
        notify(Phase::first_vertex,d);d.first_vertex=memory.word(d.edge+d.selected*2u,2);d.first_position=d.vertices+d.first_vertex*16u;
        notify(Phase::first_x,d);d.first_x=scalar(memory.word(d.first_position+d.first_offset));
        notify(Phase::first_y,d);d.first_y=scalar(memory.word(d.first_position+d.second_offset));
        notify(Phase::point_source,d);d.point=point.address();
        notify(Phase::point_x,d);d.point_x=scalar(point.word(d.point,0));
        notify(Phase::point_y,d);d.point_y=scalar(point.word(d.point,1));
        d.dx=subtract(d.point_x,d.first_x);d.dy=subtract(d.point_y,d.first_y);
        notify(Phase::second_vertex,d);d.second_vertex=memory.word(d.edge+(d.selected?0u:1u)*2u,2);
        notify(Phase::second_vertices,d);d.vertices=inputs.vertices(d.vertices);d.second_position=d.vertices+d.second_vertex*16u;
        notify(Phase::second_x,d);d.second_x=scalar(memory.word(d.second_position+d.first_offset));
        notify(Phase::second_y,d);d.second_y=scalar(memory.word(d.second_position+d.second_offset));
        d.edge_y=subtract(d.second_y,d.first_y);d.edge_x=subtract(d.second_x,d.first_x);
        d.product_y=multiply(d.edge_y,d.dx);d.product_x=multiply(d.dy,d.edge_x);d.cross=subtract(d.product_y,d.product_x);
        d.comparison=compare_zero(d.cross);
        if(d.comparison.above){d.exit=Exit::outside;notify(Phase::finished,d);return false;}
        notify(Phase::next_edge,d);d.next=memory.word(d.edge+d.selected*2u+4u,2);
        notify(Phase::loop_limit,d);d.loop_limit=inputs.first_edge(d.first_edge);d.index=d.next;
        if(d.next==d.loop_limit){d.exit=Exit::inside;d.accepted=true;notify(Phase::finished,d);return true;}
    }
}
} // namespace halo2::native_bsp_surface_polygon_detail
#endif
