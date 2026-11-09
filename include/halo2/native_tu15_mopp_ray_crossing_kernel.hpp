#pragma once
#include <array>
#include <cstdint>

namespace halo2::native_mopp_ray_crossing_detail {
using Vector=std::array<std::uint32_t,4>;
enum class Scalar {low_initial,high_start,low_end,high_end,end_for_low,end_for_high,start_low,direction_end_low,
                   product_end,product_start,epsilon,fraction_start,fraction_end,fraction_numerator,fraction_bits};
enum class Vectors {start,end,one};
enum class Store {child_start,child_end,start_low,start_high,end_low,end_high,fraction};
enum class Comparison {direction,product};
enum class Result {left,right};
struct Progress {
    bool increasing{},clipped{};
    Vector first{},last{},weight{},weighted_last{},one{},weighted_first{},child_end{};
    std::uint32_t fraction{};
};

// Native crossing geometry before the first real recursive child. Intermediate
// storage is an explicit DATA lifetime: raw callers can alias it, value callers
// own it. No CPU state or supplied child outcome participates in decisions.
template<class Frame>
void prepare_child(Frame&frame,Progress&d){
    d={};
    frame.floating(frame.scalar(Scalar::low_initial,d));
    d.first=frame.vector(Vectors::start,d);frame.reverse_subtract_second();
    d.last=frame.vector(Vectors::end,d);
    frame.store_vector(Store::child_start,d.first,d);frame.store_vector(Store::child_end,d.last,d);
    frame.spill(Store::start_low,d);
    frame.subtract_scalar(frame.scalar(Scalar::high_start,d));frame.spill(Store::start_high,d);
    frame.floating(frame.scalar(Scalar::end_for_low,d));
    frame.subtract_scalar(frame.scalar(Scalar::low_end,d));frame.spill(Store::end_low,d);
    frame.floating(frame.scalar(Scalar::end_for_high,d));
    frame.subtract_scalar(frame.scalar(Scalar::high_end,d));frame.spill(Store::end_high,d);
    frame.floating(frame.scalar(Scalar::start_low,d));
    const auto direction=frame.compare(frame.scalar(Scalar::direction_end_low,d));
    frame.publish_comparison(Comparison::direction,direction);d.increasing=direction.less();
    frame.floating(frame.scalar(Scalar::product_end,d));
    frame.multiply_scalar(frame.scalar(Scalar::product_start,d));
    const auto product=frame.compare(frame.scalar(Scalar::epsilon,d));
    frame.publish_comparison(Comparison::product,product);d.clipped=product.less();
    if(d.clipped){
        frame.floating(frame.scalar(Scalar::fraction_start,d));
        frame.subtract_scalar(frame.scalar(Scalar::fraction_end,d));
        frame.reverse_divide(frame.scalar(Scalar::fraction_numerator,d));
        frame.spill(Store::fraction,d);d.fraction=frame.scalar(Scalar::fraction_bits,d);
        d.weight=frame.broadcast(d.fraction);
        d.weighted_last=frame.multiply(d.weight,d.last,Result::right);
        d.one=frame.vector(Vectors::one,d);
        d.weighted_first=frame.subtract(d.one,d.weight,Result::left);
        d.weighted_first=frame.multiply(d.weighted_first,d.first,Result::left);
        d.child_end=frame.add(d.weighted_first,d.weighted_last,Result::left);
        frame.store_vector(Store::child_end,d.child_end,d);
    }
}
}
