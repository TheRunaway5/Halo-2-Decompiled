#pragma once
#include "halo2/native_tu15_closest_segments_kernel.hpp"

namespace halo2 {
// Private actual30D830 policy. Access must transport genuine30D3F0/312620
// children and adopt their resumed semantic pointers; no result is supplied.
enum class NativeTu15SimplexPairedPhase {
    triangle_first,triangle_tip,triangle_second,segment_argument,edge_first,
    edge_second,segment_first,segment_second,ones,intersection,
    cached_weights,axis,cached_weights_store,segment_first_fresh,
    distance_first_store,distance_first_load,distance_second_store,distance_second_load,
    selected_endpoint,swap_endpoint_first,swap_endpoint_second,
    payload_first,payload_second,swap_payload_first,swap_payload_second,
    closest_segment_first,closest_segment_second,closest_distance_first,closest_distance_second,
    triangle_tip_fresh,indexed_tip,indexed_payload,tip_payload,
    endpoint_inside_point,exit_flag,endpoint_inside_point_store,
    endpoint_inside_payload,endpoint_inside_payload_store,work_flag,
    fresh_weight_first,saved_weight_first,saved_weight_second,fresh_weight_second,
    intersection_child,endpoint_child,closest_first_child,closest_second_child,
    overlap_exit,derived_endpoint_exit,derived_reduce_exit,cached_endpoint_exit,
    cached_keep_exit,cached_second_exit,cached_first_exit
};
enum class NativeTu15SimplexPairedPromotion { keep,first,second };
struct NativeTu15SimplexPaired {
    using Vector=native_simplex_direction_numeric::Vector;
    bool derive_plane{};std::uint32_t work{},triangle{},segment{},selected_point{},tip{},scratch{},mask{},edge_index{},exit_flag{};
    unsigned weight_first{},weight_second{};NativeTu15SimplexPairedPromotion promotion{};
    Vector triangle_first{},triangle_second{},triangle_tip{},edge_first{},edge_second{},negative{},positive{},normal{},axis{};
    Vector segment_first{},segment_second{},delta_first{},delta_second{},product_first{},xy_first{},z_first{},distance_first{},product_second{},xy_second{},z_second{},distance_second{};
    Vector denominator{},ratio{},ones{},fraction{},complement{},scaled_first{},scaled_second{},intersection{},cached_weights{},payload_first{},payload_second{},promoted{};
    std::uint16_t relation{};
};
namespace native_simplex_paired_numeric {
inline __m128 subtract_low(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 multiply_low(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 divide_low(__m128 a,__m128 b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
}
template<class Access>
std::uint32_t native_tu15_reduce_paired_simplex(Access&memory,NativeTu15SimplexPaired&d){
    using P=NativeTu15SimplexPairedPhase;using Promotion=NativeTu15SimplexPairedPromotion;
    namespace N=native_simplex_direction_numeric;namespace X=native_simplex_projection_numeric;
    namespace F=native_simplex_face_select_numeric;namespace C=native_closest_segments_numeric;namespace Q=native_simplex_paired_numeric;
    memory.begin(d);
    const auto finish=[&](P phase,std::uint32_t value){memory.finish(phase,d,value);return value;};
    const auto intersect=[&]{
        d.denominator=Q::subtract_low(d.distance_first,d.distance_second);
        if(d.derive_plane)d.ratio=Q::divide_low(d.distance_first,d.denominator);
        d.ones=memory.vector_read(P::ones,d,0u);
        if(!d.derive_plane)d.ratio=Q::divide_low(d.distance_first,d.denominator);
        memory.begin_intersection(d);
        d.fraction=N::shuffle<0>(d.ratio);d.complement=N::subtract(d.ones,d.fraction);
        memory.prepare_intersection_argument(d);
        d.scaled_second=N::multiply(d.fraction,d.segment_second);d.scaled_first=N::multiply(d.complement,d.segment_first);
        d.intersection=native_simplex_initialize_numeric::add(d.scaled_first,d.scaled_second);
        memory.vector_write(P::intersection,d,0u,d.intersection);
        return memory.triangle_child(P::intersection_child,d);
    };
    const auto decrement=[&](bool first){memory.decrement_count(d,first);};
    const auto reduce_by_distances=[&]{
        d.segment_first=memory.vector_read(P::closest_segment_first,d,d.segment);
        d.segment_second=memory.vector_read(P::closest_segment_second,d,d.segment+0x10u);
        memory.prepare_closest_first(d);d.delta_first=N::subtract(d.segment_second,d.segment_first);
        memory.closest_child(P::closest_first_child,d);memory.closest_child(P::closest_second_child,d);
        memory.require_capacity(1u);X::load(memory.scalar_read(P::closest_distance_first,d));
        d.relation=F::compare_memory(memory.scalar_read(P::closest_distance_second,d),true);
        d.edge_index=F::greater(d.relation)?1u:0u;memory.distance_choice(d);
        d.promoted=memory.vector_read(P::triangle_tip_fresh,d,d.tip);memory.prepare_indexed_tip(d);
        memory.vector_write(P::indexed_tip,d,d.triangle+d.edge_index*16u,d.promoted);
        d.promoted=memory.vector_read(P::tip_payload,d,d.triangle+0x60u);memory.prepare_indexed_payload(d);
        memory.vector_write(P::indexed_payload,d,d.triangle+(d.edge_index+4u)*16u,d.promoted);
        decrement(false);return finish(P::derived_reduce_exit,2u);
    };
    if(d.derive_plane){
        d.triangle_first=memory.vector_read(P::triangle_first,d,d.triangle);
        d.triangle_tip=memory.vector_read(P::triangle_tip,d,d.triangle+0x20u);
        d.triangle_second=memory.vector_read(P::triangle_second,d,d.triangle+0x10u);
        d.segment=memory.argument(P::segment_argument,d);d.selected_point=d.segment;
        d.edge_second=N::subtract(d.triangle_second,d.triangle_tip);d.edge_first=N::subtract(d.triangle_first,d.triangle_tip);
        d.negative=N::multiply(N::shuffle<0xd2>(d.edge_second),N::shuffle<0xc9>(d.edge_first));memory.capture_tip(d);
        memory.vector_write(P::edge_first,d,d.scratch+0x30u,d.edge_first);
        memory.vector_write(P::edge_second,d,d.scratch+0x40u,d.edge_second);
        d.positive=N::multiply(N::shuffle<0xc9>(d.edge_second),N::shuffle<0xd2>(d.edge_first));
        d.segment_first=memory.vector_read(P::segment_first,d,d.segment);d.normal=N::subtract(d.positive,d.negative);
        d.delta_first=N::subtract(d.segment_first,d.triangle_first);d.product_first=N::multiply(d.normal,d.delta_first);d.xy_first=N::add_low(N::shuffle<0x55>(d.product_first),d.product_first);d.z_first=N::shuffle<0xaa>(d.product_first);d.distance_first=N::add_low(d.z_first,d.xy_first);
        d.segment_second=memory.vector_read(P::segment_second,d,d.segment+0x10u);d.delta_second=N::subtract(d.segment_second,d.triangle_first);d.product_second=N::multiply(d.normal,d.delta_second);d.xy_second=N::add_low(N::shuffle<0x55>(d.product_second),d.product_second);d.z_second=N::shuffle<0xaa>(d.product_second);d.distance_second=N::add_low(d.z_second,d.xy_second);
        const bool crossing=((std::uint32_t(_mm_movemask_ps(d.distance_first))^std::uint32_t(_mm_movemask_ps(d.distance_second)))&1u)!=0u;memory.crossing_decision(d,crossing);
        if(crossing){d.mask=intersect();memory.classification(d);if(d.mask==7u)return finish(P::overlap_exit,1u);return reduce_by_distances();}
        memory.word_write(P::distance_first_store,d,X::low(d.distance_first));memory.require_capacity(2u);X::load(memory.scalar_read(P::distance_first_load,d));C::absolute();
        memory.word_write(P::distance_second_store,d,X::low(d.distance_second));X::load(memory.scalar_read(P::distance_second_load,d));C::absolute();d.relation=X::compare_pop_two();memory.nearest_endpoint(d);
        if(!C::parity5(d.relation)){
            d.promoted=memory.vector_read(P::selected_endpoint,d,d.segment+0x10u);memory.vector_write(P::swap_endpoint_first,d,d.segment,d.promoted);memory.vector_write(P::swap_endpoint_second,d,d.segment+0x10u,d.segment_first);
            d.payload_first=memory.vector_read(P::payload_first,d,d.segment+0x40u);d.payload_second=memory.vector_read(P::payload_second,d,d.segment+0x50u);memory.vector_write(P::swap_payload_first,d,d.segment+0x40u,d.payload_second);memory.vector_write(P::swap_payload_second,d,d.segment+0x50u,d.payload_first);
        }
        d.mask=memory.triangle_child(P::endpoint_child,d);memory.classification(d);
        if(d.mask==7u){decrement(true);return finish(P::derived_endpoint_exit,0u);}return reduce_by_distances();
    }
    d.segment=memory.argument(P::segment_argument,d);d.cached_weights=memory.vector_read(P::cached_weights,d,d.work+0x160u);d.triangle_first=memory.vector_read(P::triangle_first,d,d.triangle);d.segment_second=memory.vector_read(P::segment_second,d,d.segment+0x10u);d.axis=memory.vector_read(P::axis,d,d.work+0x120u);
    memory.vector_write(P::cached_weights_store,d,d.scratch+0x10u,d.cached_weights);memory.select_second_endpoint(d);
    d.delta_second=N::subtract(d.segment_second,d.triangle_first);d.product_second=N::multiply(d.axis,d.delta_second);d.xy_second=N::add_low(N::shuffle<0x55>(d.product_second),d.product_second);d.z_second=N::shuffle<0xaa>(d.product_second);d.distance_second=N::add_low(d.z_second,d.xy_second);
    d.segment_first=memory.vector_read(P::segment_first_fresh,d,d.segment);d.delta_first=N::subtract(d.segment_first,d.triangle_first);d.product_first=N::multiply(d.axis,d.delta_first);d.xy_first=N::add_low(N::shuffle<0x55>(d.product_first),d.product_first);d.z_first=N::shuffle<0xaa>(d.product_first);d.distance_first=N::add_low(d.z_first,d.xy_first);
    d.positive=Q::multiply_low(d.distance_first,d.distance_second);const bool crossing=(std::uint32_t(_mm_movemask_ps(d.positive))&1u)!=0u;memory.crossing_decision(d,crossing);
    if(crossing){d.mask=intersect();memory.classification(d);if(d.mask==7u)return finish(P::overlap_exit,1u);}
    d.mask=memory.triangle_child(P::endpoint_child,d);memory.classification(d);
    if(d.mask==7u){
        decrement(true);d.promoted=memory.vector_read(P::endpoint_inside_point,d,d.selected_point);d.exit_flag=memory.argument(P::exit_flag,d);memory.vector_write(P::endpoint_inside_point_store,d,d.segment,d.promoted);
        d.promoted=memory.vector_read(P::endpoint_inside_payload,d,d.segment+0x50u);memory.vector_write(P::endpoint_inside_payload_store,d,d.segment+0x40u,d.promoted);memory.word_write(P::work_flag,d,d.exit_flag);return finish(P::cached_endpoint_exit,0u);
    }
    d.promotion=d.mask==3u?Promotion::keep:d.mask==5u?Promotion::second:Promotion::first;
    if(d.mask==1u||d.mask==2u||d.mask==4u){
        d.weight_first=d.mask==1u?1u:d.mask==2u?2u:0u;d.weight_second=d.mask==1u?2u:d.mask==2u?0u:1u;
        memory.require_capacity(2u);X::load(memory.scalar_read(P::fresh_weight_first,d));C::subtract(memory.scalar_read(P::saved_weight_first,d));X::multiply(memory.scalar_read(P::saved_weight_second,d));X::load(memory.scalar_read(P::fresh_weight_second,d));C::subtract(memory.scalar_read(P::saved_weight_second,d));X::multiply(memory.scalar_read(P::saved_weight_first,d));d.relation=X::compare_pop_two();memory.cached_selection(d);
        if(d.mask==1u)d.promotion=C::parity5(d.relation)?Promotion::second:Promotion::keep;
        else if(d.mask==2u)d.promotion=C::parity5(d.relation)?Promotion::keep:Promotion::first;
        else d.promotion=C::parity5(d.relation)?Promotion::first:Promotion::second;
    }
    decrement(false);
    if(d.promotion==Promotion::keep)return finish(P::cached_keep_exit,2u);
    const auto index=d.promotion==Promotion::first?0u:1u;
    d.promoted=memory.vector_read(P::triangle_tip_fresh,d,d.triangle+0x20u);memory.vector_write(P::indexed_tip,d,d.triangle+index*16u,d.promoted);
    d.promoted=memory.vector_read(P::tip_payload,d,d.triangle+0x60u);memory.before_payload_copy(d);memory.vector_write(P::indexed_payload,d,d.triangle+(index+4u)*16u,d.promoted);
    return finish(d.promotion==Promotion::first?P::cached_first_exit:P::cached_second_exit,2u);
}
} // namespace halo2
