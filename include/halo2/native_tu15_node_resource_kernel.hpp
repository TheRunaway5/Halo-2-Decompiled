#pragma once
#include "halo2/physics_contact_runtime.hpp"

namespace halo2 {
enum class NativeTu15NodeResourcePhase {node_table,node_flags,resource_index,resource_table,resource_enabled};
struct NativeTu15NodeResource {
    std::uint32_t index{},descriptor{},nodes{},node{},resource_index{},resources{},value{},escaped{},tested{};
    bool tail{};
};
template<class Memory>inline std::uint32_t native_node_resource_value(const Memory&memory,std::uint32_t at,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(at,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8*n);return value;
}
// Resolve the node's enabled resource before handing the real resource owner
// its tail call. Source descriptor fields remain lazy and can alias previous
// reads. A sentinel node index selects the original zero node address.
template<class Memory,class Progress>inline NativeTu15NodeResource native_tu15_node_resource(
    const Memory&memory,std::uint32_t descriptor,std::uint32_t index,Progress progress) {
    using P=NativeTu15NodeResourcePhase;NativeTu15NodeResource data;
    data.index=index;data.descriptor=descriptor;data.value=index;
    if(index!=0xffffffffu) {
        progress(P::node_table,data);data.nodes=native_node_resource_value(memory,descriptor+0x30u);
        data.value=data.node=data.escaped=data.nodes+index*0x6cu;
    }
    progress(P::node_flags,data);data.tested=native_node_resource_value(memory,data.node+0x16u,1)&0x10u;
    if(!data.tested)return data;
    progress(P::resource_index,data);data.resource_index=native_node_resource_value(memory,data.node+0x24u);
    progress(P::resource_table,data);data.resources=native_node_resource_value(memory,descriptor+0xb0u);
    data.value=data.resources+data.resource_index*20u;
    progress(P::resource_enabled,data);data.tested=native_node_resource_value(memory,data.value+0x10u,1);
    data.escaped=(data.resources&0xffffff00u)|data.tested;
    if(data.tested){data.escaped=(data.escaped&0xffffff00u)|1u;data.tail=true;}
    return data;
}
} // namespace halo2
