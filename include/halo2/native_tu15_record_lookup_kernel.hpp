#pragma once
#include "halo2/native_tu15_node_resource_kernel.hpp"
#include <bit>

namespace halo2 {
enum class NativeTu15RecordLookupPhase {count,count_ready,slots,datum,objects,object};
enum class NativeTu15RecordLookupExit {bounds,absent,found};
struct NativeTu15RecordLookup {
    std::uint32_t descriptor{},index{},object{},escaped_index{},count{},slots{},datum{},objects{};
    NativeTu15RecordLookupExit exit{};
};
// Signed slot indices and wrapping addresses are part of the original DATA
// contract. The node resolver and standalone record lookup share these reads;
// their caller-frame publication remains in separate adapters.
template<class Memory,class Progress>NativeTu15RecordLookup native_tu15_record_lookup(
    const Memory&memory,std::uint32_t descriptor,std::uint32_t index,Progress progress){
    using P=NativeTu15RecordLookupPhase;NativeTu15RecordLookup data;
    data.descriptor=descriptor;data.index=data.escaped_index=index;
    progress(P::count,data);data.count=native_node_resource_value(memory,descriptor+0x4cu);
    progress(P::count_ready,data);
    if(std::bit_cast<std::int32_t>(index)>=std::bit_cast<std::int32_t>(data.count))return data;
    progress(P::slots,data);data.slots=native_node_resource_value(memory,descriptor+0x50u);
    progress(P::datum,data);data.datum=native_node_resource_value(memory,data.slots+index*32u+4u);
    data.escaped_index=data.datum;
    if(data.datum==0xffffffffu){data.exit=NativeTu15RecordLookupExit::absent;return data;}
    progress(P::objects,data);data.objects=native_node_resource_value(memory,0x4f0414u);
    data.escaped_index=(data.datum&0xffffu)*16u;
    progress(P::object,data);data.object=native_node_resource_value(memory,data.objects+data.escaped_index+8u);
    data.exit=NativeTu15RecordLookupExit::found;return data;
}
} // namespace halo2
