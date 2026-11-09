#pragma once
#include "halo2/native_tu15_node_resource_kernel.hpp"

namespace halo2 {
enum class NativeTu15NodeResourceLifecyclePhase {
    enabled_first,enabled_second,save_cursor,handle,descriptor_handle,save_descriptor,
    descriptor_owner,node_table,head_index,head_table,initial_owner,initial_next,
    loop_next,resource_node_table,node_flags,resource_index,resource_table,
    resource_enabled,resource_owner,advance_next,advance_table,advance_test,advance_done,finished
};
enum class NativeTu15NodeResourceLifecycleExit {first_disabled,second_disabled,invalid_handle,complete};
struct NativeTu15NodeResourceLifecycle {
    std::uint32_t enabled_first{},enabled_second{},handle{},descriptor_handle{},index{},descriptor{},
        nodes{},node{},head_index{},cursor{},next_index{},next_node{},node_flags{},resource_index{},
        resources{},resource{},resource_enabled{},last_next{};
    NativeTu15NodeResourceLifecycleExit exit{NativeTu15NodeResourceLifecycleExit::first_disabled};
};
inline std::uint32_t native_tu15_signed_node_index(std::uint32_t value){
    return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
}
// The descriptor's +30 node array, +30 head and +32 successor fields stay
// live across real owner calls. Re-read each successor after the resource owner:
// that owner may have changed the chain. Owners provide actual complete calls,
// never test-supplied allocation, clock or resource outcomes.
template<class Memory,class Owners,class Progress>NativeTu15NodeResourceLifecycle
native_tu15_node_resource_lifecycle(const Memory&memory,std::uint32_t handle_address,
    std::uint32_t caller_descriptor,Owners&owners,Progress progress){
    using P=NativeTu15NodeResourceLifecyclePhase;using E=NativeTu15NodeResourceLifecycleExit;
    NativeTu15NodeResourceLifecycle data;data.descriptor=caller_descriptor;
    const auto read=[&](P phase,std::uint32_t address,unsigned width=4){
        progress(phase,data);return native_node_resource_value(memory,address,width);
    };
    data.enabled_first=read(P::enabled_first,0x51df20u,1);
    if(!data.enabled_first){progress(P::finished,data);return data;}
    data.exit=E::second_disabled;data.enabled_second=read(P::enabled_second,0x51df21u,1);
    if(!data.enabled_second){progress(P::finished,data);return data;}
    progress(P::save_cursor,data);data.handle=read(P::handle,handle_address);
    data.index=data.handle>>16u;data.exit=E::invalid_handle;
    if(data.index==0xffffu){progress(P::finished,data);return data;}
    data.descriptor_handle=read(P::descriptor_handle,handle_address);progress(P::save_descriptor,data);
    if((data.descriptor_handle&0xffffu)!=0xffffu){progress(P::descriptor_owner,data);data.descriptor=owners.descriptor(data);}
    data.nodes=read(P::node_table,data.descriptor+0x30u);
    data.node=data.nodes+native_tu15_signed_node_index(data.index)*0x6cu;
    data.head_index=read(P::head_index,data.node+0x30u,2);data.cursor=data.node;
    if(data.head_index!=0xffffu){
        data.nodes=read(P::head_table,data.descriptor+0x30u);
        data.cursor=data.nodes+native_tu15_signed_node_index(data.head_index)*0x6cu;
    }
    progress(P::initial_owner,data);owners.initial(data);
    data.last_next=read(P::initial_next,data.cursor+0x32u,2);
    if(data.last_next!=0xffffu)do{
        data.next_index=read(P::loop_next,data.cursor+0x32u,2);
        data.next_node=0;
        if(data.next_index!=0xffffu){
            data.nodes=read(P::resource_node_table,data.descriptor+0x30u);
            data.next_node=data.nodes+native_tu15_signed_node_index(data.next_index)*0x6cu;
        }
        data.node_flags=read(P::node_flags,data.next_node+0x16u,1)&0x10u;
        if(data.node_flags){
            data.resource_index=read(P::resource_index,data.next_node+0x24u);
            data.resources=read(P::resource_table,data.descriptor+0xb0u);
            data.resource=data.resources+data.resource_index*20u;
            data.resource_enabled=read(P::resource_enabled,data.resource+0x10u,1);
            if(data.resource_enabled){progress(P::resource_owner,data);owners.resource(data);}
        }
        data.next_index=read(P::advance_next,data.cursor+0x32u,2);data.next_node=0;
        if(data.next_index!=0xffffu){
            data.nodes=read(P::advance_table,data.descriptor+0x30u);
            data.next_node=data.nodes+native_tu15_signed_node_index(data.next_index)*0x6cu;
        }
        data.last_next=read(P::advance_test,data.next_node+0x32u,2);
        data.cursor=data.next_node;progress(P::advance_done,data);
    }while(data.last_next!=0xffffu);
    data.exit=E::complete;progress(P::finished,data);return data;
}
} // namespace halo2
