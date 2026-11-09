#pragma once
#include "halo2/native_tu15_bsp_leaf_surface_kernel.hpp"
#include "halo2/native_tu15_bsp_segment_query.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_bsp_segment_detail {
using native_bsp_surface_2d_detail::Scalar;
using native_bsp_surface_2d_detail::scalar;
using native_bsp_surface_2d_detail::multiply;
using native_bsp_surface_2d_detail::add;
using native_bsp_surface_2d_detail::subtract;
using native_bsp_surface_polygon_detail::signed_half;
using native_bsp_leaf_surface_detail::bits;
using native_bsp_leaf_surface_detail::loaded;
using native_bsp_leaf_surface_detail::compare;
using Comparison=native_bsp_leaf_surface_detail::Comparison;

// Keep the extended quotient until its authored single-store boundary. An
// exact extended subnormal may first raise underflow/precision at that store.
inline long double crossing_ratio(long double distance,std::uint32_t slope){
    long double value;
    asm volatile("fldt %1;fdivs %2;fchs;fstpt %0":"=m"(value):"m"(distance),"m"(slope):"memory","st");
    return value;
}
struct SingleStore {std::uint32_t value{};std::uint16_t status{};};
inline SingleStore single_store(long double value){
    SingleStore out;
    asm volatile("fldt %2;fstps %0;fnstsw %1":"=m"(out.value),"=m"(out.status):"m"(value):"memory","st");
    return out;
}
inline std::uint32_t child(std::uint32_t packed){return std::uint32_t(std::bit_cast<std::int32_t>(packed)>>8u);}

enum class Phase {
    node_argument,context_argument,save_index,node,descriptor,nodes,plane_index,planes,origin,
    origin_z,origin_y,plane_y,plane_z,plane_x,origin_x,displacement,displacement_y,displacement_plane_y,
    displacement_z,displacement_plane_z,plane_distance,displacement_x,displacement_plane_x,
    maximum,minimum,distance_store,slope_store,same_child,cross_distance,cross_minimum,cross_slope,
    cross_store,cross_reload,near_child,recurse,recursed,near_output,near_cross,near_limit,
    far_plane,far_plane_store,far_child,minimum_store,next_node,
    leaf,leaf_descriptor,leaves,leaf_flags,leaf_store,leaf_reload,kind_store,flags,save_flags,
    enter_previous,consider_enter,enter_leaf,candidate_store,
    exit_previous,consider_exit,inside_previous,consider_inside,inside_leaf,verify_store,
    candidate,verify,query_minimum,query_descriptor,query_displacement,query_origin,push_verify,
    query_plane,push_minimum,query_bitmap,push_plane,query_limit,surface,surface_returned,
    surfaces,surface_flags,filter_first,filter_second,saved_flags_first,saved_flags_second,
    hit_output,hit_minimum,fraction_store,hit_descriptor,hit_planes,hit_plane_index,plane_output,plane_store,
    candidate_output,hit_candidate,candidate_output_store,surface_output,surface_store,
    material,material_output,material_store,flags_output,hit_flags,flags_store,
    brush_output,brush,brush_store,partition_output,partition,partition_store,
    cached_kind,cached_leaf,record_leaf,record_output,record_count,record_store,
    counter_output,counter_read,counter_store,overflow_store,previous_leaf_store,previous_kind_store,finished
};
enum class Write {distance,slope,crossing,minimum,leaf,kind,flags,candidate,verify,
    far_plane,fraction,plane,hit_candidate,surface,material,hit_flags,brush,partition,
    record,count,overflow,previous_leaf,previous_kind};
enum class Exit {near_hit,pruned,hit,recorded,overflow,empty};
enum class Candidate {none,enter,exit,inside_previous,inside_current};
struct RecursiveContinuation {bool accepted{};std::uint32_t context{},record{},side{};};
struct SurfaceContinuation {std::uint32_t surface{},context{},descriptor{};};
struct Progress {
    std::uint32_t context{},node{},descriptor{},nodes{},record{},plane_index{},planes{},plane{},origin{},displacement{};
    std::uint32_t minimum{},maximum{},packed{},side{},crossing{},distance_word{},slope_word{},output{},limit{},far_plane{};
    Scalar distance{},y{},x{},slope{},dy{},dx{},at_min{},at_max{},zero{},cross_scalar{};
    Comparison min_comparison,max_comparison,slope_comparison,limit_comparison;
    long double crossing_distance{},crossing_extended{};
    bool minimum_side{},maximum_side{},slope_positive{},accepted{},queried{};
    std::uint32_t leaf{0xffffffffu},kind{3},leaf_flags{},flags{},first{},previous_kind{},candidate{0xffffffffu},verify{};
    Candidate selection{Candidate::none};Exit exit{Exit::empty};
    std::uint32_t bitmap{},bitmap_limit{},surface{0xffffffffu},surfaces{},surface_row{},surface_flags{},saved_flags{};
    std::uint32_t material{},flags_byte{},brush{},partition{},count{},current_count{},updated_count{},current_planes{},current_plane{};
};

// Actual1E6980's typed recursive DATA walk. Inputs owns the distinct public
// value contract or live caller-frame reads, real child continuations and
// stores. Notify carries boundary publication, never instruction execution.
template<class Memory,class Inputs,class Notify>
bool traverse(Memory&memory,Inputs&input,Notify notify){
    Progress d{};
    const auto read=[&](Phase phase,std::uint32_t address,unsigned width=4){notify(phase,d);return memory.word(address,width);};
    const auto write=[&](Phase phase,Write kind,std::uint32_t address,std::uint32_t value,unsigned width=4){notify(phase,d);input.store(kind,address,value,width);};
    notify(Phase::node_argument,d);d.node=input.node();
    notify(Phase::context_argument,d);d.context=input.context();notify(Phase::save_index,d);
    while(!(d.node&0x800000u)){
        notify(Phase::node,d);
        d.descriptor=read(Phase::descriptor,d.context+4u);d.nodes=read(Phase::nodes,d.descriptor+4u);d.record=d.nodes+(d.node&0x7fffffu)*8u;
        d.plane_index=signed_half(read(Phase::plane_index,d.record,2));d.planes=read(Phase::planes,d.descriptor+12u);
        d.origin=read(Phase::origin,d.context+16u);d.plane=d.planes+d.plane_index*16u;
        d.distance=scalar(read(Phase::origin_z,d.origin+8u));d.y=scalar(read(Phase::origin_y,d.origin+4u));
        d.y=multiply(d.y,scalar(read(Phase::plane_y,d.plane+4u)));d.distance=multiply(d.distance,scalar(read(Phase::plane_z,d.plane+8u)));d.distance=add(d.distance,d.y);
        d.x=scalar(read(Phase::plane_x,d.plane));d.x=multiply(d.x,scalar(read(Phase::origin_x,d.origin)));
        d.displacement=read(Phase::displacement,d.context+20u);d.dy=scalar(read(Phase::displacement_y,d.displacement+4u));d.dy=multiply(d.dy,scalar(read(Phase::displacement_plane_y,d.plane+4u)));d.distance=add(d.distance,d.x);
        d.slope=scalar(read(Phase::displacement_z,d.displacement+8u));d.slope=multiply(d.slope,scalar(read(Phase::displacement_plane_z,d.plane+8u)));d.distance=subtract(d.distance,scalar(read(Phase::plane_distance,d.plane+12u)));d.slope=add(d.slope,d.dy);
        d.dx=scalar(read(Phase::displacement_x,d.displacement));d.dx=multiply(d.dx,scalar(read(Phase::displacement_plane_x,d.plane)));d.slope=add(d.slope,d.dx);
        notify(Phase::maximum,d);d.maximum=input.maximum(d.maximum);d.at_max=multiply(d.slope,scalar(d.maximum));
        notify(Phase::minimum,d);d.minimum=input.minimum(d.minimum);d.at_min=multiply(d.slope,scalar(d.minimum));d.at_min=add(d.at_min,d.distance);
        d.distance_word=bits(d.distance);write(Phase::distance_store,Write::distance,0,d.distance_word);
        d.at_max=add(d.at_max,d.distance);d.min_comparison=compare(d.at_min,d.zero);d.slope_word=bits(d.slope);write(Phase::slope_store,Write::slope,0,d.slope_word);
        d.minimum_side=!d.min_comparison.below;d.max_comparison=compare(d.at_max,d.zero);d.maximum_side=!d.max_comparison.below;
        if(d.minimum_side==d.maximum_side){
            d.side=d.minimum_side;d.packed=read(Phase::same_child,d.record+1u+d.side*3u);
        }else{
            d.slope_comparison=compare(d.slope,d.zero);d.slope_positive=d.slope_comparison.above();d.side=!d.slope_positive;
            notify(Phase::cross_distance,d);d.crossing_distance=loaded(input.distance(d.distance_word));
            notify(Phase::cross_minimum,d);d.minimum=input.minimum(d.minimum);
            notify(Phase::cross_slope,d);d.slope_word=input.slope(d.slope_word);d.crossing_extended=crossing_ratio(d.crossing_distance,d.slope_word);
            notify(Phase::cross_store,d);input.store_crossing(d.crossing_extended);
            notify(Phase::cross_reload,d);d.crossing=input.crossing(d.crossing);
            d.packed=read(Phase::near_child,d.record+1u+d.side*3u);
            notify(Phase::recurse,d);const auto continuation=input.recurse(d,child(d.packed));
            d.context=continuation.context;d.record=continuation.record;d.side=continuation.side;d.accepted=continuation.accepted;notify(Phase::recursed,d);
            if(d.accepted){d.exit=Exit::near_hit;notify(Phase::finished,d);return true;}
            d.output=read(Phase::near_output,d.context+24u);notify(Phase::near_cross,d);d.crossing=input.crossing(d.crossing);d.cross_scalar=scalar(d.crossing);
            d.limit=read(Phase::near_limit,d.output);d.limit_comparison=compare(d.cross_scalar,scalar(d.limit));
            if(!d.limit_comparison.below){d.exit=Exit::pruned;notify(Phase::finished,d);return false;}
            d.far_plane=signed_half(read(Phase::far_plane,d.record,2));write(Phase::far_plane_store,Write::far_plane,d.context+36u,d.far_plane);
            d.packed=read(Phase::far_child,d.record+1u+d.side*3u);d.minimum=d.crossing;write(Phase::minimum_store,Write::minimum,0,d.minimum);
        }
        d.node=child(d.packed);notify(Phase::next_node,d);
    }
    notify(Phase::leaf,d);d.leaf=0xffffffffu;d.kind=3;
    write(Phase::leaf_store,Write::leaf,0,d.leaf);write(Phase::kind_store,Write::kind,0,d.kind,1);write(Phase::verify_store,Write::verify,0,0,1);
    if(d.node!=0xffffffffu){
        d.descriptor=read(Phase::leaf_descriptor,d.context+4u);d.nodes=read(Phase::leaves,d.descriptor+20u);d.node&=0x7fffffu;
        d.leaf_flags=read(Phase::leaf_flags,d.nodes+d.node*4u,1);write(Phase::leaf_store,Write::leaf,0,d.node);
        notify(Phase::leaf_reload,d);d.leaf=input.leaf(d.node);d.kind=(d.leaf_flags&1u)+1u;write(Phase::kind_store,Write::kind,0,d.kind,1);
    }
    d.flags=read(Phase::flags,d.context);d.first=d.flags&1u;write(Phase::save_flags,Write::flags,0,d.flags);
    if(d.first){
        d.previous_kind=read(Phase::enter_previous,d.context+32u,1);notify(Phase::consider_enter,d);
        if((d.previous_kind==1u||d.previous_kind==2u)&&d.kind==3u){d.selection=Candidate::enter;d.candidate=read(Phase::enter_leaf,d.context+28u);write(Phase::candidate_store,Write::candidate,0,d.candidate);}
    }
    if(d.selection==Candidate::none&&(d.flags&2u)){
        d.previous_kind=read(Phase::exit_previous,d.context+32u,1);notify(Phase::consider_exit,d);
        if(d.previous_kind==3u&&(d.kind==1u||d.kind==2u)){d.selection=Candidate::exit;d.candidate=d.leaf;write(Phase::candidate_store,Write::candidate,0,d.candidate);}
    }
    if(d.selection==Candidate::none&&!(d.flags&4u)){
        d.previous_kind=read(Phase::inside_previous,d.context+32u,1);notify(Phase::consider_inside,d);
        if(d.previous_kind==2u&&d.kind==2u){
            d.selection=d.first?Candidate::inside_previous:Candidate::inside_current;
            d.candidate=d.first?read(Phase::inside_leaf,d.context+28u):d.leaf;write(Phase::candidate_store,Write::candidate,0,d.candidate);
            d.verify=1;write(Phase::verify_store,Write::verify,0,d.verify,1);
        }
    }
    if(d.selection!=Candidate::none){notify(Phase::candidate,d);d.candidate=input.candidate(d.candidate);}
    if(d.selection!=Candidate::none&&d.candidate!=0xffffffffu){
        notify(Phase::verify,d);d.verify=input.verify(d.verify);notify(Phase::query_minimum,d);d.minimum=input.minimum(d.minimum);
        d.descriptor=read(Phase::query_descriptor,d.context+4u);d.displacement=read(Phase::query_displacement,d.context+20u);d.origin=read(Phase::query_origin,d.context+16u);
        notify(Phase::push_verify,d);d.plane_index=read(Phase::query_plane,d.context+36u);notify(Phase::push_minimum,d);
        d.bitmap=read(Phase::query_bitmap,d.context+12u);notify(Phase::push_plane,d);d.bitmap_limit=read(Phase::query_limit,d.context+8u,2);
        notify(Phase::surface,d);const auto continuation=input.surface(d);d.surface=continuation.surface;d.context=continuation.context;d.descriptor=continuation.descriptor;d.queried=true;notify(Phase::surface_returned,d);
        if(d.surface!=0xffffffffu){
            d.surfaces=read(Phase::surfaces,d.descriptor+44u);d.surface_row=d.surfaces+d.surface*8u;d.surface_flags=read(Phase::surface_flags,d.surface_row+4u,1);
            bool filtered=false;notify(Phase::filter_first,d);
            if(d.surface_flags&2u){notify(Phase::saved_flags_first,d);d.saved_flags=input.flags(d.flags);filtered=(d.saved_flags&8u)!=0;}
            if(!filtered){notify(Phase::filter_second,d);if(d.surface_flags&8u){notify(Phase::saved_flags_second,d);d.saved_flags=input.flags(d.flags);filtered=(d.saved_flags&16u)!=0;}}
            if(!filtered){
                d.output=read(Phase::hit_output,d.context+24u);notify(Phase::hit_minimum,d);d.minimum=input.minimum(d.minimum);write(Phase::fraction_store,Write::fraction,d.output,d.minimum);
                d.descriptor=read(Phase::hit_descriptor,d.context+4u);d.current_planes=read(Phase::hit_planes,d.descriptor+12u);d.current_plane=read(Phase::hit_plane_index,d.context+36u);d.output=read(Phase::plane_output,d.context+24u);write(Phase::plane_store,Write::plane,d.output+4u,d.current_planes+d.current_plane*16u);
                d.output=read(Phase::candidate_output,d.context+24u);notify(Phase::hit_candidate,d);d.candidate=input.candidate(d.candidate);write(Phase::candidate_output_store,Write::hit_candidate,d.output+8u,d.candidate);
                d.output=read(Phase::surface_output,d.context+24u);write(Phase::surface_store,Write::surface,d.output+12u,d.surface);
                d.material=signed_half(read(Phase::material,d.surface_row,2));d.output=read(Phase::material_output,d.context+24u);write(Phase::material_store,Write::material,d.output+16u,d.material);
                d.output=read(Phase::flags_output,d.context+24u);d.flags_byte=read(Phase::hit_flags,d.surface_row+4u,1);write(Phase::flags_store,Write::hit_flags,d.output+20u,d.flags_byte,1);
                d.output=read(Phase::brush_output,d.context+24u);d.brush=read(Phase::brush,d.surface_row+5u,1);write(Phase::brush_store,Write::brush,d.output+21u,d.brush,1);
                d.output=read(Phase::partition_output,d.context+24u);d.partition=read(Phase::partition,d.surface_row+6u,2);write(Phase::partition_store,Write::partition,d.output+22u,d.partition,2);
                d.exit=Exit::hit;d.accepted=true;notify(Phase::finished,d);return true;
            }
        }
        notify(Phase::cached_kind,d);d.kind=input.kind(d.kind);notify(Phase::cached_leaf,d);d.leaf=input.leaf(d.leaf);
    }
    notify(Phase::record_leaf,d);
    if(d.leaf!=0xffffffffu){
        d.output=read(Phase::record_output,d.context+24u);d.count=read(Phase::record_count,d.output+24u);
        if(std::bit_cast<std::int32_t>(d.count)<0x100){
            write(Phase::record_store,Write::record,d.output+28u+d.count*4u,d.leaf);d.output=read(Phase::counter_output,d.context+24u);d.current_count=read(Phase::counter_read,d.output+24u);d.updated_count=d.current_count+1u;write(Phase::counter_store,Write::count,d.output+24u,d.updated_count);d.exit=Exit::recorded;
        }else{write(Phase::overflow_store,Write::overflow,d.output+0x418u,d.leaf);d.exit=Exit::overflow;}
    }
    write(Phase::previous_leaf_store,Write::previous_leaf,d.context+28u,d.leaf);write(Phase::previous_kind_store,Write::previous_kind,d.context+32u,d.kind,1);notify(Phase::finished,d);return false;
}

template<class Memory,class Surface>
bool traverse_values(Memory&memory,std::uint32_t context,std::uint32_t node,
    std::uint32_t minimum,std::uint32_t maximum,Surface&surface,NativeTu15BspSegmentQueryStats*stats){
    struct Inputs {
        Memory&memory;Surface&surface_owner;NativeTu15BspSegmentQueryStats*stats;
        std::uint32_t context_value,node_value,minimum_value,maximum_value,crossing_word{};bool recorded{};
        std::uint32_t context()const{return context_value;}
        std::uint32_t node()const{return node_value;}
        std::uint32_t minimum(std::uint32_t)const{return minimum_value;}
        std::uint32_t maximum(std::uint32_t)const{return maximum_value;}
        std::uint32_t distance(std::uint32_t value)const{return value;}
        std::uint32_t slope(std::uint32_t value)const{return value;}
        std::uint32_t crossing(std::uint32_t)const{return crossing_word;}
        std::uint32_t leaf(std::uint32_t value)const{return value;}
        std::uint32_t kind(std::uint32_t value)const{return value;}
        std::uint32_t candidate(std::uint32_t value)const{return value;}
        std::uint32_t verify(std::uint32_t value)const{return value;}
        std::uint32_t flags(std::uint32_t value)const{return value;}
        void store_crossing(long double value){crossing_word=single_store(value).value;}
        RecursiveContinuation recurse(const Progress&data,std::uint32_t child_node){
            return {traverse_values(memory,data.context,child_node,data.minimum,data.crossing,surface_owner,stats),data.context,data.record,std::uint32_t(data.slope_positive)};
        }
        SurfaceContinuation surface(const Progress&data){return {surface_owner(data),data.context,data.descriptor};}
        void store(Write kind,std::uint32_t address,std::uint32_t value,unsigned width){
            std::uint32_t site{};
            switch(kind){
            case Write::minimum:minimum_value=value;return;
            case Write::far_plane:site=0x1e6ab0u;break;
            case Write::fraction:site=0x1e6c05u;break;
            case Write::plane:site=0x1e6c1au;break;
            case Write::hit_candidate:site=0x1e6c24u;break;
            case Write::surface:site=0x1e6c2au;break;
            case Write::material:site=0x1e6c33u;break;
            case Write::hit_flags:site=0x1e6c3cu;break;
            case Write::brush:site=0x1e6c46u;break;
            case Write::partition:site=0x1e6c52u;break;
            case Write::record:site=0x1e6c7au;break;
            case Write::count:site=0x1e6c81u;recorded=true;break;
            case Write::overflow:site=0x1e6c96u;break;
            case Write::previous_leaf:site=recorded?0x1e6c85u:0x1e6c9du;break;
            case Write::previous_kind:site=recorded?0x1e6c89u:0x1e6ca1u;break;
            default:return; // The public DATA interface has no source frame.
            }
            memory.word(site,address,value,width);
        }
    }inputs{memory,surface,stats,context,node,minimum,maximum};
    return traverse(memory,inputs,[&](Phase phase,const Progress&data){
        if(!stats)return;
        if(phase==Phase::node)++stats->nodes;
        if(phase==Phase::leaf)++stats->leaves;
        if(phase==Phase::surface)++stats->surface_queries;
        if(phase==Phase::finished&&data.exit==Exit::hit)++stats->hits;
    });
}
} // namespace halo2::native_bsp_segment_detail
#endif
