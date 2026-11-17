#pragma once
namespace halo2 {
enum class NativeTu15SolverHullLinePhase {
    first_vertex,first_transform,first_point,sign,direction,
    second_transform,second_point,second_vertex
};
template<class Vector>struct NativeTu15SolverHullLine {
    Vector vertex{},transformed{},point{},delta{},squared{},xy{},z{},score{};
    Vector sign{},direction{},negative{};
    bool second{};
};

// The two real support pairs bracket the two distance computations. The
// first sum finishes after the sign read, and the first score is reread only
// after the second pair returns. The view owns the actual FP comparison and
// caller-backed scratch/child transport, including unordered selection.
template<class View>void native_tu15_solver_hull_line_kernel(View&view,typename View::Data&d){
    using P=NativeTu15SolverHullLinePhase;
    const auto terms=[&]{
        d.delta=view.subtract(view.subtract(d.point,d.transformed),d.vertex);
        d.squared=view.multiply(d.delta,d.delta);
        d.xy=view.add_scalar(view.template shuffle<0x55>(d.squared,d.squared),d.squared);
        d.z=view.template shuffle<0xaa>(d.squared,d.squared);
    };
    view.first_support(d);
    d.vertex=view.vector(P::first_vertex,d);d.transformed=view.vector(P::first_transform,d);d.point=view.vector(P::first_point,d);
    terms();
    // The public DATA facade completes the first sum before the sign read.
    // The raw owner leaves the late ADDSS visible after that read.
    if constexpr(View::first_sum_before_sign)d.score=view.add_scalar(d.z,d.xy);
    d.sign=view.vector(P::sign,d);
    if constexpr(!View::first_sum_before_sign)d.score=view.add_scalar(d.z,d.xy);
    d.direction=view.vector(P::direction,d);view.prepare_second(d);d.negative=view.xor_bits(d.direction,d.sign);
    view.second_support(d);view.load_first_score(d);
    d.transformed=view.vector(P::second_transform,d);view.second_vertex_owner(d);
    d.point=view.vector(P::second_point,d);d.vertex=view.vector(P::second_vertex,d);
    terms();d.score=view.add_scalar(d.z,d.xy);d.second=view.compare_second(d);
    view.append(d);view.finish(d);
}
} // namespace halo2
