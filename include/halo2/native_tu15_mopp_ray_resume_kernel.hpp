#pragma once
#include <array>
#include <bit>
#include <cstdint>

namespace halo2::native_mopp_ray_resume_detail {
using Vector=std::array<std::uint32_t,4>;
enum class Scalar {product_end,product_start,epsilon,fraction_start,fraction_end,fraction_numerator,
                   interpolation_fraction,current_fraction,unit_fraction,shortening_fraction,
                   saved_fraction,scale,saved_scale,axis_endpoint,axis_plane};
enum class Vectors {bounds_end,bounds_start,interpolation_one,original_end,shortening_one,
                    code_info,original_start,scaled_end,translation,translated_end};
enum class Store {interpolation_fraction,bounds_start,saved_fraction,shortened_end,saved_scale,scaled_end,translated_end};
enum class Comparison {interpolation,shortening,axis};
enum class Arithmetic {interpolation_right,interpolation_left,shortening_right,shortening_left,scale,translation};
struct Progress {
    bool increasing{},interpolated{},shortened{};
    std::uint32_t fraction{},scale{},axis{999u};
    Vector first{},last{},one{},weight{},right{},left{},code_info{},value{};
};

// The genuine child has returned. Every source field below is read from its
// current DATA owner; child aliases and overwritten scalar lifetimes are not
// replaced with an entry snapshot. Numerical choices use native comparisons.
template<class Frame>bool resume(Frame&frame,bool increasing,Progress&d){
    d={};d.increasing=increasing;
    frame.floating(frame.scalar(Scalar::product_end,d));
    frame.multiply_scalar(frame.scalar(Scalar::product_start,d));
    auto relation=frame.compare(frame.scalar(Scalar::epsilon,d));
    frame.publish_comparison(Comparison::interpolation,relation,d);d.interpolated=relation.less();
    if(d.interpolated){
        frame.floating(frame.scalar(Scalar::fraction_start,d));
        d.last=frame.vector(Vectors::bounds_end,d);
        frame.subtract_scalar(frame.scalar(Scalar::fraction_end,d));
        d.one=frame.vector(Vectors::interpolation_one,d);
        frame.reverse_divide(frame.scalar(Scalar::fraction_numerator,d));
        frame.spill(Store::interpolation_fraction,d);
        d.fraction=frame.scalar(Scalar::interpolation_fraction,d);d.weight=frame.broadcast(d.fraction,Arithmetic::interpolation_right,d);
        d.right=frame.multiply(d.weight,d.last,Arithmetic::interpolation_right,d);
        d.first=frame.vector(Vectors::bounds_start,d);
        d.left=frame.subtract(d.one,d.weight,Arithmetic::interpolation_left,d);
        d.left=frame.multiply(d.left,d.first,Arithmetic::interpolation_left,d);
        d.value=frame.add(d.left,d.right,Arithmetic::interpolation_left,d);
        frame.store_vector(Store::bounds_start,d.value,d);
    }
    frame.cache(d);frame.floating(frame.scalar(Scalar::current_fraction,d));
    relation=frame.compare(frame.scalar(Scalar::unit_fraction,d));
    frame.publish_comparison(Comparison::shortening,relation,d);d.shortened=relation.less();
    if(d.shortened){
        d.fraction=frame.scalar(Scalar::shortening_fraction,d);d.last=frame.vector(Vectors::original_end,d);
        if(increasing)frame.code(d);
        d.one=frame.vector(Vectors::shortening_one,d);
        if(!increasing)frame.code(d);
        d.code_info=frame.vector(Vectors::code_info,d);
        if(increasing)d.axis=frame.axis(d);
        frame.store_scalar(Store::saved_fraction,d.fraction,d);
        d.fraction=frame.scalar(Scalar::saved_fraction,d);d.weight=frame.broadcast(d.fraction,Arithmetic::shortening_right,d);
        d.right=frame.multiply(d.weight,d.last,Arithmetic::shortening_right,d);
        d.first=frame.vector(Vectors::original_start,d);
        d.left=frame.subtract(d.one,d.weight,Arithmetic::shortening_left,d);
        d.left=frame.multiply(d.left,d.first,Arithmetic::shortening_left,d);
        d.value=frame.add(d.left,d.right,Arithmetic::shortening_left,d);
        d.value=frame.subtract(d.value,d.code_info,Arithmetic::shortening_left,d);
        frame.store_vector(Store::shortened_end,d.value,d);
        d.scale=frame.scalar(Scalar::scale,d);d.value=frame.vector(Vectors::scaled_end,d);
        frame.store_scalar(Store::saved_scale,d.scale,d);d.scale=frame.scalar(Scalar::saved_scale,d);
        if(!increasing)d.axis=frame.axis(d);
        d.weight=frame.broadcast(d.scale,Arithmetic::scale,d);
        d.value=frame.multiply(d.weight,d.value,Arithmetic::scale,d);
        frame.store_vector(Store::scaled_end,d.value,d);
        d.first=frame.vector(Vectors::translation,d);d.last=frame.vector(Vectors::translated_end,d);
        d.value=frame.subtract(d.last,d.first,Arithmetic::translation,d);
        frame.store_vector(Store::translated_end,d.value,d);
        if(std::bit_cast<std::int32_t>(d.axis)<3){
            frame.floating(frame.scalar(Scalar::axis_endpoint,d));
            relation=frame.compare(frame.scalar(Scalar::axis_plane,d));
            frame.publish_comparison(Comparison::axis,relation,d);
            if(increasing?relation.less():relation.greater())return true;
        }
    }
    frame.advance(d);return false;
}
}
