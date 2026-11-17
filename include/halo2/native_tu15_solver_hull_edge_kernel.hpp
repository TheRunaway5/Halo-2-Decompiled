#pragma once
#include <cstdint>
namespace halo2 {
enum class NativeTu15SolverHullEdgePhase {
    sign,axis,origin,first_point,first_transformed,axis_again,
    second_transformed,negative_axis,second_point,first_local_second
};
template<class Vector>struct NativeTu15SolverHullEdge {
    std::uint32_t context{},direction{},origin{},output{},selected_projection{};
    Vector sign{},axis{},negative{},position{},first{},first_transformed{},axis_again{};
    Vector first_delta{},first_offset{},first_product{},first_y{},first_xy{},first_z{},first_sum{};
    Vector second_transformed{},negative_again{},second{},second_delta{},second_offset{};
    Vector second_product{},second_y{},second_xy{},second_z{},second_sum{},first_local_second{};
    bool selected_first{};
};
// Two genuine support pairs bound opposite directions. Compare their rounded
// projections against the origin, retaining the selected pair and its score.
// The view owns caller-backed acquisition, child continuation and publication;
// no register file or supplied child outcome enters this ordinary DATA policy.
template<class View>void native_tu15_solver_hull_edge_kernel(View&view,typename View::Data&d){
    using P=NativeTu15SolverHullEdgePhase;
    view.begin(d);
    d.sign=view.vector(P::sign,d);
    d.direction=view.direction_owner(d);d.axis=view.vector(P::axis,d);
    view.prepare_first(d);d.negative=view.xor_bits(d.axis,d.sign);view.support_first(d);
    d.output=view.output_owner(d);view.support_second(d);
    d.origin=view.origin_owner(d);d.position=view.vector(P::origin,d);
    d.first=view.vector(P::first_point,d);d.first_transformed=view.vector(P::first_transformed,d);
    d.axis_again=view.vector(P::axis_again,d);
    d.first_delta=view.subtract(d.first,d.first_transformed);
    d.first_offset=view.subtract(d.first_delta,d.position);
    d.first_product=view.multiply(d.first_offset,d.axis_again);
    d.first_y=view.template shuffle<0x55>(d.first_product,d.first_product);
    d.first_xy=view.add_scalar(d.first_y,d.first_product);
    d.first_z=view.template shuffle<0xaa>(d.first_product,d.first_product);
    // Source acquires the current pair's transformed point before completing
    // this Z+XY sum. Its provider sees the still-partial SIMD publication.
    d.second_transformed=view.vector(P::second_transformed,d);
    d.first_sum=view.add_scalar(d.first_z,d.first_xy);
    d.negative_again=view.vector(P::negative_axis,d);
    view.record_first_and_load_comparison(d);
    d.second=view.vector(P::second_point,d);
    d.second_delta=view.subtract(d.second,d.second_transformed);
    d.second_offset=view.subtract(d.second_delta,d.position);
    d.second_product=view.multiply(d.second_offset,d.negative_again);
    d.second_y=view.template shuffle<0x55>(d.second_product,d.second_product);
    d.second_xy=view.add_scalar(d.second_y,d.second_product);
    d.second_z=view.template shuffle<0xaa>(d.second_product,d.second_product);
    d.second_sum=view.add_scalar(d.second_z,d.second_xy);
    view.record_second(d);d.selected_first=view.select_first(d);
    if(d.selected_first){
        d.first_local_second=view.vector(P::first_local_second,d);
        d.selected_projection=view.first_projection_again(d);view.promote_first(d);
    }else{
        d.selected_projection=view.second_projection_again(d);view.keep_second(d);
    }
    view.finish(d);
}
} // namespace halo2
