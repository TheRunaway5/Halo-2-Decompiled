#pragma once
#include <bit>
#include <cstdint>

namespace halo2 {
enum class NativeTu15ResourceMaintenancePhase {
    pool,enabled,limit,count_owner,initialize,initial_count,primary_next,primary_stride,primary_rows,
    primary_salt,primary_count,primary_mode,primary_enabled,primary_owner,primary_handle,
    primary_allocate,primary_schedule,primary_delete_pool,primary_delete,primary_increment,primary_store_count,
    secondary_pool,secondary_initialize,secondary_cursor,secondary_next,secondary_stride,secondary_rows,
    secondary_salt,secondary_store_cursor,secondary_count,secondary_enabled,secondary_mode,secondary_owner,
    secondary_handle,secondary_allocate,secondary_schedule,secondary_delete_pool,secondary_delete,
    secondary_increment,secondary_store_count,finished
};
struct NativeTu15ResourceMaintenance {
    std::uint32_t pool{},enabled{},limit{},inactive{},index{},stride{},rows{},product{},record{},salt{},datum{},
        mode{},resource{},handle{},allocation{},erase_pool{},secondary_cursor{};
    bool active{};
};
struct NativeTu15ResourceMaintenanceCursor {std::uint32_t pool{},index{};};

// Service queued resource requests in the original two passes. The first pass
// prioritizes mode1; the second accepts the remaining modes. Both stop at eight
// inactive records. Saved count/cursor words remain live, and every page, job,
// iteration and deletion result belongs to its real owner.
template<class Access,class Owners>void native_tu15_maintain_resources(
    Access&memory,Owners&owners,NativeTu15ResourceMaintenance&data) {
    using P=NativeTu15ResourceMaintenancePhase;
    const auto signed_word=[](std::uint32_t value){return std::bit_cast<std::int32_t>(value);};
    const auto read=[&](P phase,std::uint32_t address,unsigned width=4){return memory.read(phase,data,address,width);};
    const auto record=[&](bool secondary){
        data.stride=read(secondary?P::secondary_stride:P::primary_stride,data.pool+0x24u);
        data.rows=read(secondary?P::secondary_rows:P::primary_rows,data.pool+0x44u);
        data.product=data.stride*data.index;data.record=data.rows+data.product;
        data.salt=read(secondary?P::secondary_salt:P::primary_salt,data.record,2);
        data.datum=(data.salt<<16u)|data.index;
    };
    data.pool=read(P::pool,0x4f041cu);data.enabled=read(P::enabled,data.pool+0x29u,1);
    if(!data.enabled){memory.prepare(P::finished,data);return;}
    data.limit=read(P::limit,0x4ef1c0u);
    if(signed_word(data.limit)>25){memory.prepare(P::finished,data);return;}
    memory.prepare(P::count_owner,data);data.inactive=owners.count(data);data.active=true;
    memory.prepare(P::initialize,data);memory.store_count(P::initial_count,data,data.inactive);
    for(;;) {
        const auto cursor=owners.primary_cursor();data.pool=cursor.pool;
        memory.prepare(P::primary_next,data);data.index=owners.next(data,cursor.index+1u,false);
        if(data.index==0xffffffffu)break;
        data.pool=owners.primary_cursor().pool;record(false);
        data.inactive=memory.load_count(P::primary_count,data);
        if(signed_word(data.inactive)>=8)break;
        data.mode=read(P::primary_mode,data.record+4u);
        if(data.mode!=1)continue;
        data.enabled=read(P::primary_enabled,0x51df21u,1);
        data.resource=read(P::primary_owner,data.record+8u);
        if(!data.enabled)continue;
        data.handle=read(P::primary_handle,data.resource+12u);
        if(data.handle==0xffffffffu) {
            memory.prepare(P::primary_allocate,data);data.allocation=owners.allocate(data,1u,false);
            if(!data.allocation)continue;
            memory.prepare(P::primary_schedule,data);owners.schedule(data,1u,false);
        }
        data.erase_pool=read(P::primary_delete_pool,0x4f041cu);
        memory.prepare(P::primary_delete,data);owners.erase(data,false);
        data.inactive=memory.load_count(P::primary_increment,data)+1u;
        memory.store_count(P::primary_store_count,data,data.inactive);
    }
    data.pool=read(P::secondary_pool,0x4f041cu);
    memory.prepare(P::secondary_initialize,data);memory.store_cursor(P::secondary_initialize,data,0xffffffffu);
    for(;;) {
        data.secondary_cursor=memory.load_cursor(data);data.pool=owners.secondary_pool();
        memory.prepare(P::secondary_next,data);data.index=owners.next(data,data.secondary_cursor+1u,true);
        if(data.index==0xffffffffu)break;
        data.pool=owners.secondary_pool();record(true);
        memory.prepare(P::secondary_store_cursor,data);memory.store_cursor(P::secondary_store_cursor,data,data.index);
        data.inactive=memory.load_count(P::secondary_count,data);
        if(signed_word(data.inactive)>=8)break;
        data.enabled=read(P::secondary_enabled,0x51df21u,1);
        data.mode=read(P::secondary_mode,data.record+4u);data.resource=read(P::secondary_owner,data.record+8u);
        if(!data.enabled)continue;
        data.handle=read(P::secondary_handle,data.resource+12u);
        if(data.handle==0xffffffffu) {
            memory.prepare(P::secondary_allocate,data);data.allocation=owners.allocate(data,data.mode,true);
            if(!data.allocation)continue;
            memory.prepare(P::secondary_schedule,data);owners.schedule(data,owners.secondary_mode(),true);
        }
        data.erase_pool=read(P::secondary_delete_pool,0x4f041cu);
        memory.prepare(P::secondary_delete,data);owners.erase(data,true);
        data.inactive=memory.load_count(P::secondary_increment,data)+1u;
        memory.store_count(P::secondary_store_count,data,data.inactive);
    }
    memory.prepare(P::finished,data);
}
} // namespace halo2
