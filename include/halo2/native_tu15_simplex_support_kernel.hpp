#pragma once
#include "halo2/native_tu15_simplex_initialize_kernel.hpp"

namespace halo2 {
enum class NativeTu15SimplexSupportEntry { first,second,last_first,transform_second };
enum class NativeTu15SimplexSupportNext { first_candidate,second_candidate,last_first,second_support,finalize,iteration };
enum class NativeTu15SimplexSupportPhase {
    parity,cache,limit,count,progress,shape,table,owner,matrix,axis,
    inverse_z,inverse_x,inverse_y,inverse_sign,inverse_y_reload,
    transformed_point,translation,transform_z,transform_y,transform_x,
    transformed_count
};
struct NativeTu15SimplexSupport {
    NativeTu15SimplexSupportEntry entry{};
    std::uint32_t work{},count_address{},cache{},limit{},count{},shape{},table{},owner{},matrix{},output{};
    __m128 z{},x{},y{},sign{},y_reload{},z_low{},z_high{},xy_rows{},axis{},z_products{},y_products{},x_products{},sum_xy{},direction{};
    __m128 point{},translation{},transform_z{},transform_y{},transform_x{},z_term{},zt{},y_term{},x_term{},point_x{},point_y{},point_z{},transformed{};
};
// The support policy retains each live input acquisition, the two distinct
// first-shape attempts, and the actual child call. The inverse direction and
// newly accepted point transform share the same exact ordered numeric leaves
// as initialization; no child result is synthesized by this policy.
template<class Access>
NativeTu15SimplexSupportNext native_tu15_simplex_support(Access&memory,NativeTu15SimplexSupport&d){
    using P=NativeTu15SimplexSupportPhase;
    using E=NativeTu15SimplexSupportEntry;
    using N=NativeTu15SimplexSupportNext;
    using namespace native_simplex_initialize_numeric;
    using native_simplex_direction_numeric::multiply;
    if(d.entry==E::transform_second){
        d.point=memory.vector_read(P::transformed_point,d);
        d.matrix=memory.word_read(P::matrix,d);
        d.translation=memory.vector_read(P::translation,d);
        d.transform_z=memory.vector_read(P::transform_z,d);
        d.point_z=native_simplex_direction_numeric::shuffle<0xaa>(d.point);
        d.z_term=multiply(d.transform_z,d.point_z);
        d.transform_y=memory.vector_read(P::transform_y,d);
        d.zt=add(d.z_term,d.translation);
        d.point_y=native_simplex_direction_numeric::shuffle<0x55>(d.point);
        d.y_term=multiply(d.transform_y,d.point_y);
        d.point_x=native_simplex_direction_numeric::shuffle<0>(d.point);
        d.transform_x=memory.vector_read(P::transform_x,d);
        memory.prepare_transform(d);
        d.x_term=multiply(d.transform_x,d.point_x);
        d.transformed=add(add(d.x_term,d.y_term),d.zt);
        memory.store_transformed(d);
        memory.increment_transformed_count(d);
        memory.complete(N::iteration,d);return N::iteration;
    }
    const bool second=d.entry==E::second;
    const auto unavailable=second?N::last_first:d.entry==E::last_first?N::finalize:N::second_support;
    if(d.entry==E::last_first&&memory.word_read(P::parity,d)!=0u){memory.complete(unavailable,d);return unavailable;}
    d.cache=memory.word_read(P::cache,d);
    d.limit=memory.word_read(P::limit,d);
    d.count=memory.word_read(P::count,d);
    if(memory.at_limit(d)){memory.complete(unavailable,d);return unavailable;}
    if(memory.progress_blocks(memory.word_read(P::progress,d),d)){memory.complete(unavailable,d);return unavailable;}
    if(!second){
        d.shape=memory.word_read(P::shape,d);d.table=memory.word_read(P::table,d);
        memory.prepare_first_output(d);memory.push_output(d);memory.push_direction(d);
    }else{
        d.matrix=memory.word_read(P::matrix,d);
        d.z=memory.vector_read(P::inverse_z,d);d.x=memory.vector_read(P::inverse_x,d);d.y=memory.vector_read(P::inverse_y,d);
        d.shape=memory.word_read(P::shape,d);d.sign=memory.vector_read(P::inverse_sign,d);
        d.z_low=_mm_shuffle_ps(d.z,d.z,0x44);d.z_high=_mm_shuffle_ps(d.z,d.z,0xee);
        d.y_reload=memory.vector_read(P::inverse_y_reload,d);
        d.table=memory.word_read(P::table,d);
        d.xy_rows=_mm_shuffle_ps(d.x,d.y,0x44);
        d.axis=memory.vector_read(P::axis,d);
        const auto axis_z=native_simplex_direction_numeric::shuffle<0xaa>(d.axis);
        memory.prepare_second_index(d);
        d.z_products=multiply(_mm_shuffle_ps(_mm_shuffle_ps(d.x,d.y_reload,0xee),d.z_high,0x88),axis_z);
        memory.scale_second_index(d);
        d.y_products=multiply(_mm_shuffle_ps(d.xy_rows,d.z_low,0xdd),native_simplex_direction_numeric::shuffle<0x55>(d.axis));
        memory.locate_second_output(d);
        d.x_products=multiply(_mm_shuffle_ps(d.xy_rows,d.z_low,0x88),native_simplex_direction_numeric::shuffle<0>(d.axis));
        memory.push_output(d);
        d.sum_xy=add(d.x_products,d.y_products);memory.prepare_direction_address(d);
        d.direction=_mm_xor_ps(add(d.sum_xy,d.z_products),d.sign);
        memory.push_direction(d);memory.store_direction(d);
    }
    d.owner=memory.word_read(P::owner,d);
    memory.invoke_support(d);
    const auto next=second?N::second_candidate:N::first_candidate;
    memory.complete(next,d);return next;
}
} // namespace halo2
