#pragma once
#include "halo2/native_tu15_record_lookup_kernel.hpp"
#include "halo2/native_tu15_node_resource_lifecycle_kernel.hpp"

namespace halo2 {
enum class NativeTu15NodeDescriptorLookupPhase {
    handle,decoded,save_descriptor,descriptor_handle,save_result,direct_node,
    foreign_lookup,foreign_completed,indirect_node,node_table,node_ready,
    restore_foreign,consider_touch,touch,finished
};
struct NativeTu15NodeDescriptorLookup {
    std::uint32_t descriptor{},handle{},index{},descriptor_index{},foreign_descriptor{},
        node_descriptor{},signed_index{},nodes{},value{};
    NativeTu15RecordLookup lookup;
    NativeTu15RecordLookupPhase lookup_phase{};
    bool valid{},foreign{};
};
// Resolve a signed node index from either the caller descriptor or its actual
// tag-backed foreign descriptor. Resource scheduling is a genuine native owner
// call; its returned cursor can differ from the pre-call node through aliases.
template<class Memory,class Inputs,class Progress>NativeTu15NodeDescriptorLookup
native_tu15_node_descriptor_lookup(const Memory&memory,Inputs&input,Progress progress){
    using P=NativeTu15NodeDescriptorLookupPhase;NativeTu15NodeDescriptorLookup data;
    data.descriptor=input.descriptor();progress(P::handle,data);data.handle=input.handle();
    data.index=data.handle>>16u;progress(P::decoded,data);
    if(data.index==0xffffu){progress(P::finished,data);return data;}
    data.valid=true;progress(P::save_descriptor,data);progress(P::descriptor_handle,data);
    data.handle=input.handle();data.descriptor_index=data.handle&0xffffu;progress(P::save_result,data);
    data.foreign=data.descriptor_index!=0xffffu;data.node_descriptor=data.descriptor;
    data.signed_index=native_tu15_signed_node_index(data.index);
    if(data.foreign){
        data.lookup=native_tu15_record_lookup(memory,data.descriptor,native_tu15_signed_node_index(data.descriptor_index),
            [&](auto phase,const auto&state){data.lookup_phase=phase;data.lookup=state;progress(P::foreign_lookup,data);});
        data.foreign_descriptor=data.lookup.object;data.node_descriptor=data.foreign_descriptor;
        progress(P::foreign_completed,data);progress(P::indirect_node,data);
    }else progress(P::direct_node,data);
    if(data.signed_index!=0xffffffffu){
        progress(P::node_table,data);data.nodes=native_node_resource_value(memory,data.node_descriptor+0x30u);
        data.value=data.nodes+data.signed_index*0x6cu;progress(P::node_ready,data);
    }
    if(data.foreign)progress(P::restore_foreign,data);
    progress(P::consider_touch,data);
    if(data.value){progress(P::touch,data);data.value=input.touch(data);}
    progress(P::finished,data);return data;
}
} // namespace halo2
