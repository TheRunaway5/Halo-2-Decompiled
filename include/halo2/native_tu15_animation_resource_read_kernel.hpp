#pragma once
#include "halo2/native_tu15_node_resource_kernel.hpp"

namespace halo2 {
enum class NativeTu15AnimationReadPhase {
    cache,resource,pending,pages,handle,rows,shift,offset,base,bytes,ready,
    priority,file,epoch,job,refresh,time,age,resource_again,handle_again,pages_again,rows_again,finished
};
struct NativeTu15AnimationRead {
    std::uint32_t tracking{},mode{},cache{},resource{},pages{},handle{},rows{},shift{},offset{},
        base{},destination{},bytes{},rounded_bytes{},mode_offset{},priority{},file{},epoch{},job{},time{},age{};
};
struct NativeTu15AnimationReadResume {std::uint32_t tracking{},mode_offset{},job{};};
// Frame access remains lazy so stack/descriptor aliases are read at their
// actual acquisition points. The real I/O and wait owners determine outcomes.
template<class Memory,class Owners,class Progress>NativeTu15AnimationRead
native_tu15_read_animation_resource(const Memory&memory,std::uint32_t tracking,
    Owners&owners,Progress progress) {
    using P=NativeTu15AnimationReadPhase;NativeTu15AnimationRead data;data.tracking=tracking;
    const auto read=[&](P phase,std::uint32_t address){progress(phase,data);return native_node_resource_value(memory,address);};
    const auto store=[&](P phase,std::uint32_t address,std::uint32_t value,unsigned width,std::uint32_t site){progress(phase,data);owners.store(address,value,width,site);};
    data.cache=read(P::cache,0x4f0424u);data.mode=owners.mode(data);
    data.resource=read(P::resource,data.tracking+12u);
    store(P::pending,data.tracking+4u,0,1,0x1260d6u);
    data.pages=read(P::pages,data.cache+0x64u);data.handle=read(P::handle,data.resource+12u);
    data.rows=read(P::rows,data.pages+0x44u);data.shift=read(P::shift,data.cache+0x34u);
    data.offset=read(P::offset,data.rows+(data.handle&65535u)*24u+8u);
    data.base=read(P::base,0x4f0420u);data.destination=data.base+(data.offset<<(data.shift&31u));
    data.bytes=read(P::bytes,data.resource+4u);
    data.rounded_bytes=(data.bytes&511u)?(data.bytes|511u)+1u:data.bytes;
    store(P::ready,data.tracking+4u,0,1,0x12610fu);
    data.mode_offset=data.mode*12u;data.priority=read(P::priority,0x474d70u+data.mode_offset);
    owners.priority(data);data.file=read(P::file,data.resource+8u);data.epoch=read(P::epoch,data.resource);
    owners.epoch(data);data.priority=owners.saved_priority(data);
    const auto result=owners.read(data);data.tracking=result.tracking;data.mode_offset=result.mode_offset;data.job=result.job;
    store(P::job,data.tracking+8u,data.job,4,0x12613eu);
    if(!owners.wait_mode(data)){
        const auto waited=owners.wait(data);data.tracking=waited.tracking;data.mode_offset=waited.mode_offset;
    }
    data.cache=read(P::refresh,0x4f0424u);data.time=read(P::time,data.cache+0x38u);
    data.age=read(P::age,0x474d74u+data.mode_offset);data.resource=read(P::resource_again,data.tracking+12u);
    data.handle=read(P::handle_again,data.resource+12u);data.pages=read(P::pages_again,data.cache+0x64u);
    data.rows=read(P::rows_again,data.pages+0x44u);progress(P::finished,data);return data;
}
} // namespace halo2
