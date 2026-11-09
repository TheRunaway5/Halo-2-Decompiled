#pragma once
#include <cstdint>

namespace halo2::native_mopp_ray_leaf_detail {
enum class Read {cache_initial,key_base,filter,filter_collection,filter_table,cache_after_filter,
                 collection,collection_table,cache_after_shape,output,shape_table,output_after_cast,
                 output_fraction,parent,parent_field,collector,collector_after_cast,collector_fraction};
enum class Write {hit,cache_fraction,output_key,parent,parent_field};
enum class Result {filtered,direct_miss,direct_hit,collected};
struct Progress {
    bool direct{};
    std::uint32_t key{},key_base{},cache{},filter{},collection{},table{},shape{},output{},
                  fraction{},parent{},parent_field{},collector{};
};

// Primitive DATA and genuine service handoffs. A retained view owns each real
// argument store, call/return and lazy return-pointer read. A value view owns
// its native service contract. Neither view supplies captured child outcomes.
template<class Frame>Result visit(Frame&frame,std::uint32_t key,Progress&d){
    d={};d.key=key;d.cache=frame.scalar(Read::cache_initial,d);
    d.key_base=frame.scalar(Read::key_base,d);d.filter=frame.scalar(Read::filter,d);
    d.key+=d.key_base;frame.key(d);
    if(d.filter){
        d.collection=frame.scalar(Read::filter_collection,d);d.table=frame.scalar(Read::filter_table,d);
        if(!frame.filter(d))return Result::filtered;
        d.cache=frame.scalar(Read::cache_after_filter,d);
    }
    d.collection=frame.scalar(Read::collection,d);d.table=frame.scalar(Read::collection_table,d);
    d.shape=frame.shape(d);d.cache=frame.scalar(Read::cache_after_shape,d);
    d.output=frame.scalar(Read::output,d);d.table=frame.scalar(Read::shape_table,d);
    if(d.output){
        d.direct=true;
        if(!frame.cast_direct(d))return Result::direct_miss;
        d.output=frame.scalar(Read::output_after_cast,d);frame.store(Write::hit,1u,d);
        d.fraction=frame.scalar(Read::output_fraction,d);frame.store(Write::cache_fraction,d.fraction,d);
        frame.store(Write::output_key,d.key,d);return Result::direct_hit;
    }
    d.parent=frame.scalar(Read::parent,d);frame.store(Write::parent,d.parent,d);
    d.parent_field=frame.scalar(Read::parent_field,d);frame.store(Write::parent_field,d.parent_field,d);
    d.collector=frame.scalar(Read::collector,d);frame.cast_collector(d);
    d.collector=frame.scalar(Read::collector_after_cast,d);d.fraction=frame.scalar(Read::collector_fraction,d);
    frame.store(Write::cache_fraction,d.fraction,d);return Result::collected;
}
}
