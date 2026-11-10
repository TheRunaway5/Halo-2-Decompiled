#pragma once
#include "halo2/native_tu15_simplex_face_select_kernel.hpp"

namespace halo2 {
// Actual312620. These phases describe argument, scratch, and output ownership;
// the raw view keeps their addresses backed by the genuine caller frame.
enum class NativeTu15ClosestSegmentsPhase {
    first_direction_argument,first_direction,first_point_argument,first_point,
    second_point_argument,second_point,second_direction_argument,second_direction,
    cross_store,along_second_store,along_first_store,length_second_store,length_first_store,
    length_second_load,length_first_load,product_store,cross_first,cross_second,
    cross_square_store,product_load,cross_square_load,determinant_store,
    numerator_length_second,numerator_along_first,numerator_along_second,numerator_cross,
    numerator_store,scaled_numerator,determinant_first,determinant_second,
    numerator_test,zero_test,zero_fraction,parallel_cross_square,parallel_product,
    parallel_epsilon,parallel_determinant,numerator_fraction,fraction_denominator,
    one_fraction,second_cross,second_offset,second_fraction_store,second_upper,
    second_test,second_zero,second_upper_store,second_lower_store,
    recompute_cross,recompute_fraction,recompute_offset,recompute_store,
    recompute_zero,recompute_zero_fraction,recompute_test,recompute_upper,
    recompute_one_fraction,recompute_numerator,recompute_denominator,
    second_divide_numerator,second_divide_denominator,second_ratio_store,
    output_argument,first_ratio_store,first_ratio,second_ratio,second_ratio_copy,
    second_ratio_reload,first_output,fresh_second_direction,fresh_second_point,
    separation_output,fresh_separation,distance_store,distance_reload,distance_output
};
enum class NativeTu15ClosestSegmentsMask {
    clear,first_upper,first_lower,second_upper,second_lower,add_first_lower,add_first_upper
};
struct NativeTu15ClosestSegments {
    using Vector=native_simplex_direction_numeric::Vector;
    std::uint32_t scratch{},first_direction_address{},first_point_address{},second_point_address{},second_direction_address{},output{},mask{},fraction_second{};
    Vector first_direction{},first_point{},second_point{},delta{},second_direction{};
    Vector product{},xy{},z{},cross{},along_first_product{},along_first_xy{},along_first_z{},along_first{};
    Vector along_second_product{},along_second_xy{},along_second_z{},along_second{};
    Vector length_first_product{},length_first_xy{},length_first_z{},length_first{};
    Vector length_second_product{},length_second_xy{},length_second_z{},length_second{};
    Vector fraction{},offset{},point_on_first{},fresh_second_direction{},fresh_second_point{},point_on_second{},separation{},fresh_separation{},distance_product{},distance_xy{},distance_z{},distance{};
    std::uint16_t relation{};
};
namespace native_closest_segments_numeric {
inline void subtract(std::uint32_t bits){asm volatile("fsubs %0"::"m"(bits):"memory","st");}
inline void add(std::uint32_t bits){asm volatile("fadds %0"::"m"(bits):"memory","st");}
inline void absolute(){asm volatile("fabs":::"memory","st");}
inline void subtract_pop(){asm volatile("fsubrp %%st,%%st(1)":::"memory","st","st(1)");}
inline bool parity41(std::uint16_t status){const auto bits=status&0x4100u;return bits==0u||bits==0x4100u;}
inline bool parity5(std::uint16_t status){return(status&0x500u)!=0x100u;}
}
// Private/unqualified until the complete raw frame and independent reference
// gate are supplied. The old public scalar facade remains separate for now.
template<class Access>
std::uint32_t native_tu15_closest_segments(Access&memory,NativeTu15ClosestSegments&d){
    using P=NativeTu15ClosestSegmentsPhase;using M=NativeTu15ClosestSegmentsMask;
    namespace N=native_simplex_direction_numeric;namespace X=native_simplex_projection_numeric;
    namespace F=native_simplex_face_select_numeric;namespace C=native_closest_segments_numeric;
    memory.begin(d);
    d.first_direction_address=memory.argument(P::first_direction_argument,d);
    d.first_direction=memory.vector_read(P::first_direction,d,d.first_direction_address);
    d.first_point_address=memory.argument(P::first_point_argument,d);
    d.first_point=memory.vector_read(P::first_point,d,d.first_point_address);
    d.second_point_address=memory.argument(P::second_point_argument,d);
    d.second_point=memory.vector_read(P::second_point,d,d.second_point_address);d.delta=N::subtract(d.second_point,d.first_point);
    d.second_direction_address=memory.argument(P::second_direction_argument,d);
    d.second_direction=memory.vector_read(P::second_direction,d,d.second_direction_address);
    d.product=N::multiply(d.first_direction,d.second_direction);d.xy=N::add_low(N::shuffle<0x55>(d.product),d.product);d.z=N::shuffle<0xaa>(d.product);d.cross=N::add_low(d.z,d.xy);
    memory.word_write(P::cross_store,d,X::low(d.cross));
    d.along_first_product=N::multiply(d.first_direction,d.delta);d.along_first_xy=N::add_low(N::shuffle<0x55>(d.along_first_product),d.along_first_product);d.along_first_z=N::shuffle<0xaa>(d.along_first_product);d.along_first=N::add_low(d.along_first_z,d.along_first_xy);
    d.along_second_product=N::multiply(d.second_direction,d.delta);d.along_second_xy=N::add_low(N::shuffle<0x55>(d.along_second_product),d.along_second_product);d.along_second_z=N::shuffle<0xaa>(d.along_second_product);d.along_second=N::add_low(d.along_second_z,d.along_second_xy);
    memory.word_write(P::along_second_store,d,X::low(d.along_second));
    d.length_first_product=N::multiply(d.first_direction,d.first_direction);d.length_first_xy=N::add_low(N::shuffle<0x55>(d.length_first_product),d.length_first_product);d.length_first_z=N::shuffle<0xaa>(d.length_first_product);
    d.length_second_product=N::multiply(d.second_direction,d.second_direction);d.length_first=N::add_low(d.length_first_z,d.length_first_xy);d.length_second_xy=N::shuffle<0x55>(d.length_second_product);
    memory.word_write(P::along_first_store,d,X::low(d.along_first));
    d.length_second_xy=N::add_low(d.length_second_xy,d.length_second_product);d.length_second_z=N::shuffle<0xaa>(d.length_second_product);d.length_second=N::add_low(d.length_second_z,d.length_second_xy);
    memory.word_write(P::length_second_store,d,X::low(d.length_second));
    memory.require_capacity(2u);X::load(memory.scalar_read(P::length_second_load,d));
    memory.word_write(P::length_first_store,d,X::low(d.length_first));X::multiply(memory.scalar_read(P::length_first_load,d));memory.scalar_store(P::product_store,d,true);
    X::load(memory.scalar_read(P::cross_first,d));X::multiply(memory.scalar_read(P::cross_second,d));memory.scalar_store(P::cross_square_store,d,true);
    X::load(memory.scalar_read(P::product_load,d));C::subtract(memory.scalar_read(P::cross_square_load,d));C::absolute();memory.scalar_store(P::determinant_store,d,true);
    X::load(memory.scalar_read(P::numerator_length_second,d));X::multiply(memory.scalar_read(P::numerator_along_first,d));X::load(memory.scalar_read(P::numerator_along_second,d));X::multiply(memory.scalar_read(P::numerator_cross,d));C::subtract_pop();
    memory.scalar_store(P::numerator_store,d,false);X::multiply(memory.scalar_read(P::scaled_numerator,d));X::load(memory.scalar_read(P::determinant_first,d));X::multiply(memory.scalar_read(P::determinant_second,d));d.relation=X::compare_pop_two();memory.relation(d,0x41u);
    if(!C::parity41(d.relation)){X::load(memory.scalar_read(P::one_fraction,d));d.mask=1u;memory.mask(d,M::first_upper);}
    else{
        X::load(memory.scalar_read(P::numerator_test,d));d.relation=F::compare_memory(memory.scalar_read(P::zero_test,d),true);memory.relation(d,0x41u);
        if(!C::parity41(d.relation)){X::load(memory.scalar_read(P::zero_fraction,d));d.mask=2u;memory.mask(d,M::first_lower);}
        else{
            X::load(memory.scalar_read(P::parallel_cross_square,d));C::add(memory.scalar_read(P::parallel_product,d));X::multiply(memory.scalar_read(P::parallel_epsilon,d));d.relation=F::compare_memory(memory.scalar_read(P::parallel_determinant,d),true);memory.relation(d,5u);
            if(C::parity5(d.relation)){X::load(memory.scalar_read(P::one_fraction,d));d.mask=1u;memory.mask(d,M::first_upper);}
            else{X::load(memory.scalar_read(P::numerator_fraction,d));d.mask=0u;memory.mask(d,M::clear);F::divide(memory.scalar_read(P::fraction_denominator,d));}
        }
    }
    X::load(memory.scalar_read(P::second_cross,d));F::multiply_next();C::subtract(memory.scalar_read(P::second_offset,d));memory.scalar_store(P::second_fraction_store,d,false);d.relation=F::compare_memory(memory.scalar_read(P::second_upper,d),true);memory.relation(d,1u);
    bool recompute{};
    if(!(d.relation&0x100u)){memory.word_write(P::second_upper_store,d,0x3f800000u);d.mask=4u;memory.mask(d,M::second_upper);recompute=true;}
    else{X::load(memory.scalar_read(P::second_test,d));d.relation=F::compare_memory(memory.scalar_read(P::second_zero,d),true);memory.relation(d,0x41u);if(!C::parity41(d.relation)){memory.word_write(P::second_lower_store,d,0u);d.mask=8u;memory.mask(d,M::second_lower);recompute=true;}}
    if(recompute){
        F::discard();X::load(memory.scalar_read(P::recompute_cross,d));X::multiply(memory.scalar_read(P::recompute_fraction,d));C::add(memory.scalar_read(P::recompute_offset,d));memory.scalar_store(P::recompute_store,d,false);d.relation=F::compare_memory(memory.scalar_read(P::recompute_zero,d),true);memory.relation(d,0x41u);
        if(!C::parity41(d.relation)){X::load(memory.scalar_read(P::recompute_zero_fraction,d));d.mask|=2u;memory.mask(d,M::add_first_lower);}
        else{X::load(memory.scalar_read(P::recompute_test,d));d.relation=F::compare_memory(memory.scalar_read(P::recompute_upper,d),true);memory.relation(d,1u);if(!(d.relation&0x100u)){X::load(memory.scalar_read(P::recompute_one_fraction,d));d.mask|=1u;memory.mask(d,M::add_first_upper);}else{X::load(memory.scalar_read(P::recompute_numerator,d));F::divide(memory.scalar_read(P::recompute_denominator,d));}}
    }else{X::load(memory.scalar_read(P::second_divide_numerator,d));F::divide(memory.scalar_read(P::second_divide_denominator,d));memory.scalar_store(P::second_ratio_store,d,true);}
    d.output=memory.argument(P::output_argument,d);memory.scalar_store(P::first_ratio_store,d,true);
    const auto fraction_first=memory.scalar_read(P::first_ratio,d);d.fraction_second=memory.scalar_read(P::second_ratio,d);d.fraction=F::broadcast(fraction_first);
    d.offset=N::multiply(d.fraction,d.first_direction);d.point_on_first=native_simplex_initialize_numeric::add(d.first_point,d.offset);
    memory.word_write(P::second_ratio_copy,d,d.fraction_second);const auto fraction_second=memory.scalar_read(P::second_ratio_reload,d);
    memory.vector_write(P::first_output,d,d.output,d.point_on_first);
    d.fresh_second_direction=memory.vector_read(P::fresh_second_direction,d,d.second_direction_address);d.fraction=F::broadcast(fraction_second);
    d.fresh_second_point=memory.vector_read(P::fresh_second_point,d,d.second_point_address);d.offset=N::multiply(d.fraction,d.fresh_second_direction);d.point_on_second=native_simplex_initialize_numeric::add(d.fresh_second_point,d.offset);d.separation=N::subtract(d.point_on_first,d.point_on_second);
    memory.vector_write(P::separation_output,d,d.output+0x10u,d.separation);d.fresh_separation=memory.vector_read(P::fresh_separation,d,d.output+0x10u);
    d.distance_product=N::multiply(d.fresh_separation,d.fresh_separation);d.distance_xy=N::add_low(N::shuffle<0x55>(d.distance_product),d.distance_product);d.distance_z=N::shuffle<0xaa>(d.distance_product);d.distance=N::add_low(d.distance_z,d.distance_xy);
    memory.word_write(P::distance_store,d,X::low(d.distance));const auto distance=memory.scalar_read(P::distance_reload,d);memory.word_write(P::distance_output,d,distance);memory.complete(d);return d.mask;
}
} // namespace halo2
