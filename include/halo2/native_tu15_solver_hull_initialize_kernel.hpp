#pragma once
#include <cstdint>
namespace halo2 {
enum class NativeTu15SolverHullInitializePhase {
    sign,translation,difference,fresh_difference,rounded_length,direction,point
};
template<class Vector>struct NativeTu15SolverHullInitialize {
    std::uint32_t context{},differences{},output{};
    Vector sign{},translation{},direction{},product{},y{},xy{},z{},length{};
    Vector first{},fresh{},squared{},squared_y{},squared_xy{},squared_z{},square{};
    Vector root{},rounded{},direction_again{},point{};
    bool produced_plane{};
};
// Source313560 chooses a nonzero initial direction, computes one genuine
// support pair, appends it, then tests its actual difference against tolerance.
// Each view owns caller-backed acquisition, real child transport and its own
// retained or physical publication protocol. No child result is supplied.
// Private: the full raw/source/value views still need independent proof.
template<class View>void native_tu15_solver_hull_initialize_kernel(View&view,typename View::Data&d){
    using P=NativeTu15SolverHullInitializePhase;
    view.begin(d);
    d.sign=view.vector(P::sign,d);
    d.translation=view.vector(P::translation,d);
    d.direction=view.xor_bits(d.translation,d.sign);view.save_direction(d);
    d.product=view.multiply(d.direction,d.direction);
    d.y=view.template shuffle<0x55>(d.product,d.product);
    d.xy=view.add_scalar(d.y,d.product);
    d.z=view.template shuffle<0xaa>(d.product,d.product);
    d.length=view.add_scalar(d.z,d.xy);
    if(!view.initial_length_above_or_unordered(d))view.default_direction(d);
    view.support(d);view.append(d);
    d.differences=view.differences_owner(d);d.first=view.vector(P::difference,d);
    d.product=view.multiply(d.first,d.first);
    d.y=view.template shuffle<0x55>(d.product,d.product);
    d.xy=view.add_scalar(d.y,d.product);
    d.z=view.template shuffle<0xaa>(d.product,d.product);
    d.length=view.add_scalar(d.z,d.xy);
    d.produced_plane=!view.difference_above_or_unordered(d);
    if(d.produced_plane){
        d.fresh=view.vector(P::fresh_difference,d);
        // The output formal is fetched after this vector but before squaring.
        d.output=view.output_owner(d);
        d.squared=view.multiply(d.fresh,d.fresh);
        d.squared_y=view.template shuffle<0x55>(d.squared,d.squared);
        d.squared_xy=view.add_scalar(d.squared_y,d.squared);
        d.squared_z=view.template shuffle<0xaa>(d.squared,d.squared);
        d.square=view.add_scalar(d.squared_z,d.squared_xy);
        view.save_square(d);d.root=view.square_root(d.square);view.save_root(d);
        d.rounded=view.vector(P::rounded_length,d);view.save_and_load_length(d);
        d.direction_again=view.vector(P::direction,d);view.output_length(d);
        view.output_direction(d);d.point=view.vector(P::point,d);
        view.output_point(d);view.output_weight(d);
    }
    view.output_flag(d);view.finish(d);
}
} // namespace halo2
