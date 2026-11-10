#pragma once
#include "halo2/native_tu15_simplex_initialize_kernel.hpp"

namespace halo2 {
enum class NativeTu15SimplexTrianglePhase {
    vertices_argument,point_argument,orientation_argument,
    first,second,third,scratch_store,point,third_fresh,scratch_read,first_fresh,
    weights,normal,orientation_first,orientation_second,sign,flipped_normal,
    swap_first,swap_second,publish_first,publish_second,
    payload_first,payload_second,publish_payload_first,publish_payload_second,
    weight_first,weight_second,publish_weight_first,publish_weight_second
};
struct NativeTu15SimplexTriangle {
    using Vector=native_simplex_direction_numeric::Vector;
    std::uint32_t work{},vertices{},point_address{},orient{},mask{},weight{};
    Vector a{},b{},c{},ac{},ac_yzx{},ac_zxy{},cb{},ba{},cb_zxy{},cb_yzx{};
    Vector negative_normal{},positive_normal{},normal{},p{},pb{},pb_yzx{},negative_pb{};
    Vector c_fresh{},positive_pb{},pc{},pc_yzx{},negative_pc{},scratch{},positive_pc{};
    Vector a_fresh{},pa{},pc_cross{},negative_pa{},pb_cross{},positive_pa{},pa_cross{};
    Vector ab_low{},ab_high{},pa_low{},pa_high{},normal_z{},z_term{},normal_y{},y_term{},normal_x{},x_rows{},x_term{},weights{};
    Vector orientation_first{},orientation_second{},offset{},products{},xy{},dot{},sign{};
    Vector swap_first{},swap_second{},payload_first{},payload_second{};
};
// Actual30D3F0 policy: signed triangle products, optional normal orientation,
// paired vertex/payload swaps and a true single-precision weight exchange.
// Scratch and argument acquisitions are semantic phases: raw callers provide
// their real frame backing; a value caller can own a local scratch vector.
// This core accepts no instruction stream, register bank or supplied result.
template<class Access>
std::uint32_t native_tu15_classify_simplex_triangle(Access&memory,NativeTu15SimplexTriangle&d){
    using P=NativeTu15SimplexTrianglePhase;
    namespace N=native_simplex_direction_numeric;
    using native_simplex_initialize_numeric::add;
    memory.begin(d);
    d.vertices=memory.argument(P::vertices_argument,d);
    d.a=memory.vector_read(P::first,d,d.vertices);
    d.b=memory.vector_read(P::second,d,d.vertices+0x10u);
    d.c=memory.vector_read(P::third,d,d.vertices+0x20u);
    d.point_address=memory.argument(P::point_argument,d);
    d.ac=N::subtract(d.a,d.c);d.ac_yzx=N::shuffle<0xc9>(d.ac);d.ac_zxy=N::shuffle<0xd2>(d.ac);
    d.cb=N::subtract(d.c,d.b);d.ba=N::subtract(d.b,d.a);
    d.cb_zxy=N::shuffle<0xd2>(d.cb);d.cb_yzx=N::shuffle<0xc9>(d.cb);
    d.negative_normal=N::multiply(d.cb_zxy,d.ac_yzx);
    memory.vector_write(P::scratch_store,d,memory.scratch_address(d),d.ac_zxy);
    d.positive_normal=N::multiply(d.cb_yzx,d.ac_zxy);
    d.p=memory.vector_read(P::point,d,d.point_address);
    d.normal=N::subtract(d.positive_normal,d.negative_normal);
    d.pb=N::subtract(d.p,d.b);d.negative_pb=N::multiply(N::shuffle<0xd2>(d.pb),d.cb_yzx);d.pb_yzx=N::shuffle<0xc9>(d.pb);
    d.c_fresh=memory.vector_read(P::third_fresh,d,d.vertices+0x20u);
    d.positive_pb=N::multiply(d.pb_yzx,d.cb_zxy);d.pc=N::subtract(d.p,d.c_fresh);
    d.negative_pc=N::multiply(N::shuffle<0xd2>(d.pc),d.ac_yzx);d.pc_yzx=N::shuffle<0xc9>(d.pc);
    d.scratch=memory.vector_read(P::scratch_read,d,memory.scratch_address(d));
    d.positive_pc=N::multiply(d.pc_yzx,d.scratch);
    d.a_fresh=memory.vector_read(P::first_fresh,d,d.vertices);
    d.pa=N::subtract(d.p,d.a_fresh);d.pc_cross=N::subtract(d.positive_pc,d.negative_pc);
    d.negative_pa=N::multiply(N::shuffle<0xd2>(d.pa),N::shuffle<0xc9>(d.ba));
    d.pb_cross=N::subtract(d.positive_pb,d.negative_pb);
    d.positive_pa=N::multiply(N::shuffle<0xc9>(d.pa),N::shuffle<0xd2>(d.ba));
    d.pa_cross=N::subtract(d.positive_pa,d.negative_pa);
    d.ab_high=_mm_shuffle_ps(d.pb_cross,d.pc_cross,0xee);d.pa_high=N::shuffle<0xee>(d.pa_cross);
    d.ab_high=_mm_shuffle_ps(d.ab_high,d.pa_high,0x88);
    d.ab_low=_mm_shuffle_ps(d.pb_cross,d.pc_cross,0x44);d.pa_low=N::shuffle<0x44>(d.pa_cross);
    d.normal_z=N::shuffle<0xaa>(d.normal);d.z_term=N::multiply(d.ab_high,d.normal_z);
    d.normal_y=N::shuffle<0x55>(d.normal);d.y_term=N::multiply(_mm_shuffle_ps(d.ab_low,d.pa_low,0xdd),d.normal_y);
    d.normal_x=N::shuffle<0>(d.normal);d.x_rows=_mm_shuffle_ps(d.ab_low,d.pa_low,0x88);
    d.orient=memory.argument(P::orientation_argument,d);
    d.x_term=N::multiply(d.x_rows,d.normal_x);d.weights=add(add(d.x_term,d.y_term),d.z_term);
    d.mask=std::uint32_t(_mm_movemask_ps(d.weights))&7u;
    memory.prepare_weights(d);memory.vector_write(P::weights,d,d.work+0x160u,d.weights);
    if(!d.orient){memory.finish(d,false);return d.mask;}
    memory.prepare_normal(d);memory.vector_write(P::normal,d,d.work+0x120u,d.normal);
    if(d.mask!=7u){memory.finish(d,false);return d.mask;}
    d.orientation_first=memory.vector_read(P::orientation_first,d,d.work+0x20u);
    d.orientation_second=memory.vector_read(P::orientation_second,d,d.work+0xa0u);
    d.offset=N::subtract(d.orientation_second,d.orientation_first);d.products=N::multiply(d.normal,d.offset);
    d.xy=N::add_low(N::shuffle<0x55>(d.products),d.products);d.dot=N::add_low(N::shuffle<0xaa>(d.products),d.xy);
    memory.prepare_orientation(d);
    if(!(_mm_movemask_ps(d.dot)&1u)){memory.finish(d,false);return d.mask;}
    d.sign=memory.vector_read(P::sign,d,0x461960u);d.normal=_mm_xor_ps(d.normal,d.sign);
    memory.vector_write(P::flipped_normal,d,d.work+0x120u,d.normal);
    d.swap_first=memory.vector_read(P::swap_first,d,d.vertices);d.swap_second=memory.vector_read(P::swap_second,d,d.vertices+0x10u);
    memory.vector_write(P::publish_first,d,d.vertices,d.swap_second);memory.vector_write(P::publish_second,d,d.vertices+0x10u,d.swap_first);
    d.payload_first=memory.vector_read(P::payload_first,d,d.vertices+0x40u);d.payload_second=memory.vector_read(P::payload_second,d,d.vertices+0x50u);
    memory.vector_write(P::publish_payload_first,d,d.vertices+0x40u,d.payload_second);memory.vector_write(P::publish_payload_second,d,d.vertices+0x50u,d.payload_first);
    memory.load_weight(d);d.weight=memory.word_read(P::weight_second,d,d.work+0x164u);
    memory.word_write(P::publish_weight_first,d,d.work+0x160u,d.weight);memory.store_weight(d);
    memory.finish(d,true);return d.mask;
}
} // namespace halo2
