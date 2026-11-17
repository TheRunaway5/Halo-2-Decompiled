#pragma once
#include "halo2/native_tu15_simplex_paired_kernel.hpp"

namespace halo2 {
// Private actual30DC10 semantic policy. A raw view must adopt the genuine
// Triangle/Closest return and keep every local slot lazy across those calls.
enum class NativeTu15SimplexBoundaryPhase {
    initial_second_count,first_scan_count,first_scan_count_again,
    initial_first_count,second_scan_count,second_scan_count_again,
    pair_first_count,pair_second_count,first_endpoint,second_endpoint,
    first_point,first_end,second_point,first_delta_store,second_end,
    distance,minimum,first_reduction_count,first_reduction_choice,
    first_reduction_endpoint,first_reduction_source,first_reduction_store,
    second_reduction_count,second_reduction_choice,second_reduction_endpoint,
    second_position_source,second_position_store,second_count_again,
    second_relative_source,second_relative_store,early_source,early_position,
    early_relative,early_relative_store,first_singleton,second_singleton,
    normal_exit,first_overlap_exit,second_overlap_exit
};
struct NativeTu15SimplexBoundary {
    using Vector=native_simplex_direction_numeric::Vector;
    std::uint32_t work{},scratch{},index{},row{},first_count{},second_count{},
        first_edges{},second_edges{},outer_index{},inner_index{},outer_point{},
        second_point{},first_endpoint{},second_endpoint{},choice{},count{},
        source{},destination{},mask{};
    Vector first_point{},first_end{},first_delta{},second_point_value{},
        second_end{},second_delta{},promoted{};
    std::uint16_t relation{};
};
template<class Access>
void native_tu15_reduce_simplex_boundary(Access&memory,NativeTu15SimplexBoundary&d){
    using P=NativeTu15SimplexBoundaryPhase;
    namespace N=native_simplex_direction_numeric;namespace X=native_simplex_projection_numeric;
    namespace F=native_simplex_face_select_numeric;namespace C=native_closest_segments_numeric;
    const auto positive=[](std::uint32_t value){return std::bit_cast<std::int32_t>(value)>0;};
    memory.begin(d);
    d.second_count=memory.count(P::initial_second_count,d,false);
    if(d.second_count==3u){
        d.first_count=memory.count(P::first_scan_count,d,true);
        memory.prepare_triangle_scan(d,true);
        if(positive(d.first_count))do{
            d.mask=memory.triangle_child(d,true);
            memory.classify_triangle(d);
            if(d.mask==7u){
                memory.prepare_early_source(d,true);
                d.promoted=memory.vector_read(P::early_source,d,d.source);
                memory.vector_write(P::early_position,d,d.work+0x20u,d.promoted);
                memory.write_count(P::first_singleton,d,true,1u);
                memory.finish(P::first_overlap_exit,d);return;
            }
            d.first_count=memory.count(P::first_scan_count_again,d,true);
        }while(memory.advance_triangle_scan(d,true,d.first_count));
    }
    d.first_count=memory.count(P::initial_first_count,d,true);
    if(d.first_count==3u){
        d.second_count=memory.count(P::second_scan_count,d,false);
        memory.prepare_triangle_scan(d,false);
        if(positive(d.second_count))do{
            d.mask=memory.triangle_child(d,false);
            memory.classify_triangle(d);
            if(d.mask==7u){
                memory.prepare_early_source(d,false);
                d.promoted=memory.vector_read(P::early_source,d,d.source);
                memory.vector_write(P::early_position,d,d.work+0xe0u,d.promoted);
                memory.prepare_early_relative(d);
                d.promoted=memory.vector_read(P::early_relative,d,d.source);
                memory.before_early_relative_store(d);
                memory.vector_write(P::early_relative_store,d,d.work+0xa0u,d.promoted);
                memory.write_count(P::second_singleton,d,false,1u);
                memory.finish(P::second_overlap_exit,d);return;
            }
            d.second_count=memory.count(P::second_scan_count_again,d,false);
        }while(memory.advance_triangle_scan(d,false,d.second_count));
    }
    d.first_count=memory.count(P::pair_first_count,d,true);
    d.second_count=memory.count(P::pair_second_count,d,false);
    d.first_edges=d.first_count==3u?3u:1u;
    d.second_edges=d.second_count==3u?3u:1u;
    memory.initialize_pairs(d);
    if(positive(d.first_edges)){
        memory.initialize_outer_point(d);
        do{
            memory.begin_outer_pair(d);
            if(positive(d.second_edges)){
                do{
                    memory.acquire_outer_index(d);
                    d.first_endpoint=memory.index_read(P::first_endpoint,d,d.outer_index);
                    d.second_endpoint=memory.index_read(P::second_endpoint,d,d.inner_index);
                    d.first_point=memory.vector_read(P::first_point,d,d.outer_point);
                    memory.prepare_first_endpoint(d);
                    d.first_end=memory.vector_read(P::first_end,d,d.source);
                    d.first_delta=N::subtract(d.first_end,d.first_point);
                    d.second_point_value=memory.vector_read(P::second_point,d,d.second_point);
                    memory.prepare_second_endpoint(d);
                    memory.vector_write(P::first_delta_store,d,d.scratch+0x40u,d.first_delta);
                    d.second_end=memory.vector_read(P::second_end,d,d.source);
                    memory.prepare_closest_arguments(d);
                    d.second_delta=N::subtract(d.second_end,d.second_point_value);
                    memory.closest_child(d);
                    memory.require_capacity(1u);X::load(memory.scalar_read(P::distance,d));
                    d.relation=F::compare_memory(memory.scalar_read(P::minimum,d),true);
                    memory.distance_classification(d);
                    if(!C::parity5(d.relation))memory.publish_minimum(d);
                }while(memory.advance_inner_pair(d));
                memory.reload_pair_limits(d);
            }
        }while(memory.advance_outer_pair(d));
    }
    memory.classify_first_edges(d);
    if(d.first_edges==3u){
        d.count=memory.count(P::first_reduction_count,d,true);
        d.choice=memory.selected_index(P::first_reduction_choice,d);
        d.count-=1u;memory.write_count(P::first_reduction_count,d,true,d.count);
        d.first_endpoint=memory.index_read(P::first_reduction_endpoint,d,d.choice);
        memory.prepare_first_reduction_source(d);
        d.promoted=memory.vector_read(P::first_reduction_source,d,d.source);
        memory.prepare_first_reduction_destination(d);
        memory.vector_write(P::first_reduction_store,d,d.destination,d.promoted);
    }
    memory.classify_second_edges(d);
    if(d.second_edges==3u){
        d.count=memory.count(P::second_reduction_count,d,false);
        d.choice=memory.selected_index(P::second_reduction_choice,d);
        d.count-=1u;memory.write_count(P::second_reduction_count,d,false,d.count);
        d.second_endpoint=memory.index_read(P::second_reduction_endpoint,d,d.choice);
        memory.prepare_second_position_source(d);
        d.promoted=memory.vector_read(P::second_position_source,d,d.source);
        memory.prepare_second_position_destination(d);
        memory.vector_write(P::second_position_store,d,d.destination,d.promoted);
        d.count=memory.count(P::second_count_again,d,false);
        memory.prepare_second_relative_source(d);
        d.promoted=memory.vector_read(P::second_relative_source,d,d.source);
        memory.prepare_second_relative_destination(d);
        memory.vector_write(P::second_relative_store,d,d.destination,d.promoted);
    }
    memory.finish(P::normal_exit,d);
}
} // namespace halo2
