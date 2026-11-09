#pragma once
#include "halo2/native_tu15_node_resource_kernel.hpp"

namespace halo2 {
enum class NativeTu15ResourceTouchPhase {
    enabled,save_mode,handle,search,allocation_pool,allocate,allocated_records,
    allocated_mode,restore_mode,allocated_owner,maintenance,clock_first,deadline,
    clock_second,deadline_store,threshold,queue_pool,queue_descriptor,queue_records,
    queue_time,save_timestamp_cursor,timestamp,restore_timestamp_cursor,timestamp_store,finished
};
enum class NativeTu15ResourceTouchExit {disabled,found,allocation_deadline,deadline_updated,existing,maintenance};
struct NativeTu15ResourceTouchChild {
    std::uint32_t value{},pool{},resource{},mode{};
};
struct NativeTu15ResourceTouch {
    std::uint32_t resource{},enabled{},mode{},handle{},datum{},pool{},records{},record{},
        threshold{},queue{},queue_descriptor{},queue_records{},current{},timestamp{},
        previous{},elapsed{},clock{},deadline{},updated{},priority_byte{};
    NativeTu15ResourceTouchExit exit{NativeTu15ResourceTouchExit::disabled};
};

// Touch an existing queue record or create a request record through the real
// resource owners. Frame phases retain live stack-save/pop timing where those
// bytes can alias resource data. Every child result comes from its actual owner.
template<class Memory,class Owners,class Progress>NativeTu15ResourceTouch
native_tu15_touch_resource(const Memory&memory,std::uint32_t resource,std::uint32_t priority_byte,
    Owners&owners,Progress progress) {
    using P=NativeTu15ResourceTouchPhase;using E=NativeTu15ResourceTouchExit;
    NativeTu15ResourceTouch data;data.resource=resource;data.priority_byte=priority_byte&255u;
    const auto read=[&](P phase,std::uint32_t address,unsigned width=4) {
        progress(phase,data);return native_node_resource_value(memory,address,width);
    };
    data.enabled=read(P::enabled,0x51df21u,1);
    if(!data.enabled){progress(P::finished,data);return data;}
    data.mode=data.priority_byte?1u:2u;progress(P::save_mode,data);
    data.handle=read(P::handle,data.resource+12u);
    if(data.handle==0xffffffffu) {
        progress(P::search,data);data.datum=owners.search(data);
        if(data.datum!=0xffffffffu){data.exit=E::found;progress(P::finished,data);return data;}
        data.pool=read(P::allocation_pool,0x4f041cu);
        progress(P::allocate,data);const auto allocated=owners.allocate(data);
        data.datum=allocated.value;data.pool=allocated.pool;data.resource=allocated.resource;data.mode=allocated.mode;
        if(data.datum!=0xffffffffu) {
            data.exit=E::maintenance;
            data.records=read(P::allocated_records,data.pool+0x44u);
            data.record=data.records+(data.datum&0xffffu)*12u;
            progress(P::allocated_mode,data);owners.store(data.record+4u,data.mode,0x12624bu);
            progress(P::restore_mode,data);
            progress(P::allocated_owner,data);owners.store(data.record+8u,data.resource,0x12624fu);
            progress(P::maintenance,data);owners.maintenance(data);return data;
        }
        data.exit=E::allocation_deadline;progress(P::clock_first,data);data.clock=owners.clock(data,false);
        data.deadline=read(P::deadline,0x56e1ecu);
        if(data.clock>data.deadline) {
            progress(P::clock_second,data);data.clock=owners.clock(data,true);data.updated=data.clock+30000u;
            data.exit=E::deadline_updated;
            progress(P::restore_mode,data);progress(P::deadline_store,data);owners.store(0x56e1ecu,data.updated,0x126270u);
        }
    } else {
        data.threshold=read(P::threshold,0x474d74u+data.mode*12u);
        data.queue=read(P::queue_pool,0x4f0424u);
        data.queue_descriptor=read(P::queue_descriptor,data.queue+0x64u);
        data.queue_records=read(P::queue_records,data.queue_descriptor+0x44u);
        data.current=read(P::queue_time,data.queue+0x38u);
        data.timestamp=data.queue_records+(data.handle&0xffffu)*24u+0x14u;
        progress(P::save_timestamp_cursor,data);data.previous=read(P::timestamp,data.timestamp);
        data.elapsed=data.current-data.previous;progress(P::restore_timestamp_cursor,data);
        if(std::bit_cast<std::int32_t>(data.elapsed)>std::bit_cast<std::int32_t>(data.threshold)) {
            data.updated=data.current-data.threshold;progress(P::timestamp_store,data);
            owners.store(data.timestamp,data.updated,0x1262a9u);
        }
        data.exit=E::existing;
    }
    progress(P::finished,data);return data;
}
} // namespace halo2
