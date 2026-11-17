#pragma once
#include <cstdint>
namespace halo2 {
enum class NativeTu15SolverHullSupportPairPhase {
    first_z,sign,direction,first_x,first_y,second_local,translation,second_z,second_y,second_x
};
template<class Vector>struct NativeTu15SolverHullSupportPair {
    std::uint32_t context{},direction{},point_pair{},second_output{},transformed_direction{};
    std::uint32_t first_shape{},first_table{},first_owner{},second_shape{},second_table{},second_owner{};
    Vector z{},sign{},input{},x{},y{},negative{},xy_low{},xy_high{},z_low{},z_high{};
    Vector transposed_x{},transposed_y{},transposed_z{},negative_x{},negative_y{},negative_z{};
    Vector x_term{},y_term{},z_term{},xy_sum{},transformed{};
    Vector local{},translation{},final_z{},local_z{},final_z_term{},final_y{},translated{};
    Vector local_y{},final_y_term{},local_x{},final_x{},final_x_term{},final_xy{},result{};
};
// The view owns lazy arguments, true support-owner calls and their continuation
// identities. Geometry is shared ordinary DATA; no CPU/stack representation is
// carried here. In particular each child may resume a different live context,
// direction or output owner before the following reads.
template<class View>void native_tu15_solver_hull_support_pair_kernel(View&view,typename View::Data&d){
    using P=NativeTu15SolverHullSupportPairPhase;
    view.begin(d);d.first_shape=view.first_shape(d);d.first_table=view.first_table(d);view.support_first(d);
    d.z=view.vector(P::first_z,d,d.context+0x20u);
    d.sign=view.vector(P::sign,d,0x461960u);
    d.input=view.vector(P::direction,d,d.direction);
    d.x=view.vector(P::first_x,d,d.context);
    d.y=view.vector(P::first_y,d,d.context+0x10u);
    d.second_shape=view.second_shape(d);
    d.negative=view.xor_bits(d.input,d.sign);
    d.z_low=view.template shuffle<0x44>(d.z,d.z);
    d.xy_low=view.template shuffle<0x44>(d.x,d.y);
    d.z_high=view.template shuffle<0xee>(d.z,d.z);
    d.xy_high=view.template shuffle<0xee>(d.x,d.y);
    d.negative_y=view.template shuffle<0x55>(d.negative,d.negative);
    d.transposed_y=view.template shuffle<0xdd>(d.xy_low,d.z_low);
    d.y_term=view.multiply(d.transposed_y,d.negative_y);
    d.transposed_x=view.template shuffle<0x88>(d.xy_low,d.z_low);
    d.negative_x=view.template shuffle<0x00>(d.negative,d.negative);
    d.x_term=view.multiply(d.transposed_x,d.negative_x);
    d.negative_z=view.template shuffle<0xaa>(d.negative,d.negative);
    d.transposed_z=view.template shuffle<0x88>(d.xy_high,d.z_high);
    d.z_term=view.multiply(d.transposed_z,d.negative_z);
    d.xy_sum=view.add(d.x_term,d.y_term);
    d.transformed=view.add(d.xy_sum,d.z_term);
    view.prepare_second(d);d.second_table=view.second_table(d);view.support_second(d);
    d.local=view.vector(P::second_local,d,d.second_output);
    d.translation=view.vector(P::translation,d,d.context+0x30u);
    d.final_z=view.vector(P::second_z,d,d.context+0x20u);
    d.local_z=view.template shuffle<0xaa>(d.local,d.local);
    d.final_z_term=view.multiply(d.final_z,d.local_z);
    // The Y row is acquired before adding translation, and the X row only
    // after the Y product. These are observable FP/SIMD acquisition boundaries.
    // The original raw owner reads Y before translation; the existing value
    // facade publishes that addition before its Y provider access. Keep both
    // authored schedules explicit while sharing the remaining DATA policy.
    if constexpr(View::translation_before_second_y)d.translated=view.add(d.final_z_term,d.translation);
    d.final_y=view.vector(P::second_y,d,d.context+0x10u);
    if constexpr(!View::translation_before_second_y)d.translated=view.add(d.final_z_term,d.translation);
    d.local_y=view.template shuffle<0x55>(d.local,d.local);
    d.final_y_term=view.multiply(d.final_y,d.local_y);
    d.local_x=view.template shuffle<0x00>(d.local,d.local);
    d.final_x=view.vector(P::second_x,d,d.context);
    d.final_x_term=view.multiply(d.final_x,d.local_x);
    d.final_xy=view.add(d.final_x_term,d.final_y_term);
    d.result=view.add(d.final_xy,d.translated);
    view.store_result(d);view.finish(d);
}
} // namespace halo2
