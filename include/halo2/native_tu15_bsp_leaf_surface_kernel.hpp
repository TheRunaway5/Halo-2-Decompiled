#pragma once
#include "halo2/native_tu15_bsp_surface_query.hpp"
#include "halo2/native_tu15_bsp_surface_polygon_kernel.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_bsp_leaf_surface_detail {
using Scalar=native_bsp_surface_2d_detail::Scalar;
using native_bsp_surface_2d_detail::scalar;
using native_bsp_surface_2d_detail::multiply;
using native_bsp_surface_2d_detail::add;
using native_bsp_surface_polygon_detail::signed_half;
inline std::uint32_t bits(Scalar value){std::uint32_t result;std::memcpy(&result,&value,4);return result;}
struct Comparison {bool below{},equal{};std::uint32_t flags{};bool above()const{return!below&&!equal;}};
inline Comparison compare(Scalar left,Scalar right){
    unsigned char below{},equal{},unordered{};
    asm volatile("comiss %4,%3;setb %0;sete %1;setp %2":"=qm"(below),"=qm"(equal),"=qm"(unordered):"x"(left),"x"(right):"cc","memory");
    return {below!=0,equal!=0,2u|std::uint32_t(below)|(std::uint32_t(unordered)<<2u)|(std::uint32_t(equal)<<6u)};
}
inline long double loaded(std::uint32_t value){
    long double result;asm volatile("flds %1;fstpt %0":"=m"(result):"m"(value):"memory","st");return result;
}
struct Absolute {std::uint32_t rounded{};long double loaded{},value{};};
inline Absolute absolute(std::uint32_t value){
    Absolute out;
    // Preserve both authored extended continuations around the intervening
    // scratch read. Duplicates are exact copies; the rounded single remains
    // owned by the same FLD/FABS/FSTP numerical sequence.
    asm volatile("flds %3;fld %%st(0);fstpt %1;fabs;fld %%st(0);fstpt %2;fstps %0"
        :"=m"(out.rounded),"=m"(out.loaded),"=m"(out.value):"m"(value):"memory","st","st(1)");
    return out;
}
inline Comparison compare_extended(long double left,long double right){
    unsigned char below{},equal{},unordered{};
    asm volatile("fldt %4;fldt %3;fcomip %%st(1),%%st;setb %0;sete %1;setp %2;fstp %%st(0)"
        :"=qm"(below),"=qm"(equal),"=qm"(unordered):"m"(left),"m"(right):"cc","memory","st","st(1)");
    return {below!=0,equal!=0,2u|std::uint32_t(below)|(std::uint32_t(unordered)<<2u)|(std::uint32_t(equal)<<6u)};
}
enum class Phase {leaves,first_index,count,initialize,entries,fraction,cache_cursor,cursor,plane,match_plane,plane_index,planes,
    normal_x,store_abs_x,normal_y,read_abs_x,store_abs_y,normal_z,store_abs_z,compare_y,compare_z,
    axis_z,store_axis_z,axis_y,store_axis_y,store_axis_x,axis,coefficient,zero,displacement_x,store_side,
    origin_x,store_x,displacement_y,origin_y,store_y,displacement_z,first_component,second_component,
    node_cursor,node,origin_z,store_z,project_x,store_project_x,project_y,store_project_y,tree,verify,polygon,
    retry_index,retry_end,next_cursor,store_index,store_cursor,finished};
struct Progress {
    std::uint32_t descriptor{},candidate{},leaves{},leaf{},index{},count{},end{},entries{},cursor{},plane{},plane_index{},planes{},plane_offset{},coefficients{};
    Scalar fraction{},abs_x{},abs_other{},coefficient{},zero{},x{},y{},z{};
    Absolute normal_x,normal_y,normal_z;
    std::uint32_t abs_y_bits{},abs_z_bits{},axis{},positive{},plane_sign{},side{},table_index{},first_component{},second_component{},node{},result{},verify{},next_cursor{},next_index{};
    std::uint32_t displacement{},origin{};std::array<std::uint32_t,3>values{};NativeTu15BspProjectedPoint point{};
    long double compare_y{},compare_z{};Comparison yz,axis_comparison,coefficient_comparison;bool accepted{},choose_z{},matched{};
};
// Iterate a leaf's plane references, choose a projection by the authored
// absolute-normal comparisons, then query its real 2D tree and polygon. The
// independent storage adapter owns caller scratch and callee ABI transport.
template<class Memory,class Inputs,class Notify>std::uint32_t query(Memory&memory,Inputs&input,Notify notify){
    Progress d{};d.descriptor=input.descriptor();d.candidate=input.candidate();
    notify(Phase::leaves,d);d.leaves=memory.word(d.descriptor+0x14u);d.leaf=d.leaves+d.candidate*4u;
    notify(Phase::first_index,d);d.index=memory.word(d.leaf+2u,2);
    notify(Phase::count,d);d.count=memory.word(d.leaf+1u,1);d.end=d.index+d.count;notify(Phase::initialize,d);
    const auto less=[](std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);};
    if(less(d.index,d.end)){
        notify(Phase::entries,d);d.entries=memory.word(d.descriptor+0x1cu);
        notify(Phase::fraction,d);d.fraction=scalar(input.fraction());d.cursor=d.entries+d.index*4u;notify(Phase::cache_cursor,d);
        do{
            notify(Phase::cursor,d);d.cursor=input.cursor(d.cursor);
            notify(Phase::plane,d);d.plane=signed_half(memory.word(d.cursor,2));
            notify(Phase::match_plane,d);d.plane_index=input.plane_index();
            d.matched=(d.plane&0x7fffu)==d.plane_index;
            if(d.matched){
                notify(Phase::plane_index,d);d.plane_index=input.plane_index();
                notify(Phase::planes,d);d.planes=memory.word(d.descriptor+12u);d.plane_offset=d.plane_index*16u;d.coefficients=d.planes+d.plane_offset;
                notify(Phase::normal_x,d);d.normal_x=absolute(memory.word(d.coefficients));notify(Phase::store_abs_x,d);
                notify(Phase::normal_y,d);d.normal_y=absolute(memory.word(d.coefficients+4u));
                notify(Phase::read_abs_x,d);d.abs_x=scalar(input.abs_x(d.normal_x.rounded));notify(Phase::store_abs_y,d);
                notify(Phase::normal_z,d);d.normal_z=absolute(memory.word(d.coefficients+8u));notify(Phase::store_abs_z,d);
                notify(Phase::compare_y,d);d.abs_y_bits=input.abs_y(d.normal_y.rounded);d.compare_y=loaded(d.abs_y_bits);
                notify(Phase::compare_z,d);d.abs_z_bits=input.abs_z(d.normal_z.rounded);d.compare_z=loaded(d.abs_z_bits);
                d.yz=compare_extended(d.compare_z,d.compare_y);d.choose_z=false;
                if(!d.yz.below){
                    notify(Phase::axis_z,d);d.abs_other=scalar(input.abs_z(d.abs_z_bits));d.axis_comparison=compare(d.abs_other,d.abs_x);
                    if(!d.axis_comparison.below){d.axis=2;d.choose_z=true;notify(Phase::store_axis_z,d);}
                }
                if(!d.choose_z){
                    notify(Phase::axis_y,d);d.abs_other=scalar(input.abs_y(d.abs_y_bits));d.axis_comparison=compare(d.abs_other,d.abs_x);
                    d.axis=1;notify(Phase::store_axis_y,d);if(d.axis_comparison.below){d.axis=0;notify(Phase::store_axis_x,d);}
                }
                notify(Phase::axis,d);d.axis=signed_half(input.axis(d.axis));
                notify(Phase::coefficient,d);d.coefficient=scalar(memory.word(d.coefficients+d.axis*4u));
                notify(Phase::zero,d);d.zero=scalar(memory.word(0x46a0e8u));d.coefficient_comparison=compare(d.coefficient,d.zero);d.positive=d.coefficient_comparison.above();
                d.displacement=input.displacement();d.origin=input.origin();notify(Phase::displacement_x,d);d.x=scalar(memory.word(d.displacement));
                d.plane_sign=(d.plane>>15u)&1u;d.side=d.positive!=d.plane_sign;notify(Phase::store_side,d);
                d.x=multiply(d.x,d.fraction);notify(Phase::origin_x,d);d.x=add(d.x,scalar(memory.word(d.origin)));d.values[0]=bits(d.x);notify(Phase::store_x,d);
                notify(Phase::displacement_y,d);d.y=scalar(memory.word(d.displacement+4u));d.y=multiply(d.y,d.fraction);
                notify(Phase::origin_y,d);d.y=add(d.y,scalar(memory.word(d.origin+4u)));d.values[1]=bits(d.y);notify(Phase::store_y,d);
                notify(Phase::displacement_z,d);d.z=scalar(memory.word(d.displacement+8u));d.table_index=(d.side+d.axis*2u)*6u;
                notify(Phase::first_component,d);d.first_component=signed_half(memory.word(0x44ac8cu+d.table_index,2));
                notify(Phase::second_component,d);d.second_component=signed_half(memory.word(0x44ac8eu+d.table_index,2));
                notify(Phase::node_cursor,d);d.cursor=input.cursor(d.cursor);
                notify(Phase::node,d);d.node=signed_half(memory.word(d.cursor+2u,2));d.z=multiply(d.z,d.fraction);
                notify(Phase::origin_z,d);d.z=add(d.z,scalar(memory.word(d.origin+8u)));d.values[2]=bits(d.z);notify(Phase::store_z,d);
                notify(Phase::project_x,d);d.point[0]=input.component(d.first_component,d.values);notify(Phase::store_project_x,d);
                notify(Phase::project_y,d);d.point[1]=input.component(d.second_component,d.values);notify(Phase::store_project_y,d);
                notify(Phase::tree,d);d.result=input.tree(d);
                notify(Phase::verify,d);d.verify=input.verify();
                if(!d.verify){d.accepted=true;notify(Phase::finished,d);return d.result;}
                notify(Phase::polygon,d);if(input.polygon(d)){d.accepted=true;notify(Phase::finished,d);return d.result;}
                notify(Phase::retry_index,d);d.index=input.retry_index(d.index);
                notify(Phase::retry_end,d);d.end=input.retry_end(d.end);
            }
            notify(Phase::next_cursor,d);d.cursor=input.cursor(d.cursor);d.next_index=d.index+1u;d.next_cursor=d.cursor+4u;
            notify(Phase::store_index,d);notify(Phase::store_cursor,d);d.index=d.next_index;d.cursor=d.next_cursor;
        }while(less(d.index,d.end));
    }
    d.result=0xffffffffu;notify(Phase::finished,d);return d.result;
}
} // namespace halo2::native_bsp_leaf_surface_detail
#endif
