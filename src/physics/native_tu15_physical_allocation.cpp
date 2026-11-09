#include "halo2/native_tu15_physical_allocation.hpp"
#include <stdexcept>

namespace halo2 {namespace {
class Memory {
    SourcePhysicsContactMemory &m_;const SourcePhysicsContactRecordObserver &o_;
public:
    Memory(SourcePhysicsContactMemory &m,const SourcePhysicsContactRecordObserver &o):m_(m),o_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const {
        std::array<std::byte,4>b{};m_.read(p,std::span(b).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(8u*n);return value;
    }
    void store(std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width=4) {
        SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o_){e.before.resize(width);m_.read(p,e.before);}
        std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(8u*n));
        m_.write(p,std::span(b).first(width));if(o_){e.written.assign(b.begin(),b.begin()+width);o_(e);}
    }
};
template<class Callback>void require(const Callback &callback,std::uint32_t owner,NativeTu15PhysicalAllocationStats *stats) {
    if(callback)return;if(stats){++stats->unbound_services;++stats->unbound_owners[owner];}
    throw std::runtime_error("Native physical allocation requires actual owner="+std::to_string(owner));
}
}
std::uint32_t native_tu15_allocate_physical_block(SourcePhysicsContactMemory &memory,
    NativeTu15PhysicalAllocationFrame &frame,NativeTu15PhysicsFrameFp &fp,
    const NativeTu15PhysicalAllocationServices &services,const NativeTu15PhysicalAllocationPhases &phases,
    const SourcePhysicsContactRecordObserver &observe,NativeTu15PhysicalAllocationStats *stats) {
    if(stats)++stats->calls;Memory m(memory,observe);NativeTu15PhysicalAllocationPhaseData data;
    using Phase=NativeTu15PhysicalAllocationPhase;
    const auto notify=[&](Phase phase){if(phases.observe)phases.observe(phase,data);};
    data.pool=m.word(0x4f2d34u);notify(Phase::partition_binding);
    data.bytes=frame.requested_bytes();notify(Phase::requested);
    require(services.pages,0x13fe50u,stats);data.handle=services.pages(data.pool,data.bytes+0x2023u,1u,fp);notify(Phase::handle);
    if(data.handle==0xffffffffu){if(stats)++stats->refusals;data.value=0;notify(Phase::returning);return 0;}
    data.pool=m.word(0x4f2d34u);notify(Phase::partition);
    data.table=m.word(data.pool+0x64u);notify(Phase::page_table);
    data.records=m.word(data.table+0x44u);notify(Phase::page_records);
    data.shift=m.word(data.pool+0x34u);notify(Phase::page_shift);
    data.offset=m.word(data.records+(data.handle&0xffffu)*24u+8u);notify(Phase::page_offset);
    data.array=m.word(0x4f2d24u);notify(Phase::descriptor_binding);
    data.offset<<=(data.shift&31u);notify(Phase::shifted_offset);
    data.base=m.word(0x4f2d30u);notify(Phase::base);
    data.address=(data.offset+data.base+0x1023u)&0xfffff000u;data.header=data.address-36u;notify(Phase::address);
    require(services.allocate_specific,0x16e770u,stats);const auto datum=services.allocate_specific(data.array,data.handle,fp);
    data.records=m.word(data.array+0x44u);notify(Phase::record_array);
    data.record=data.records+(datum&0xffffu)*40u;data.value=datum;notify(Phase::record);
    const auto second=frame.attribute(1);data.value=second;data.field=1;notify(Phase::attribute);
    data.value=data.address^0x02281972u;notify(Phase::encoded_address);m.store(0x12fe83u,data.header+4u,data.value);
    const auto first=frame.attribute(0);data.value=first;data.field=0;notify(Phase::attribute);m.store(0x12fe8au,data.header+16u,first);
    data.previous=m.word(0x4f2d2cu);notify(Phase::previous);
    frame.record_slot(data.record,0x12fe94u);notify(Phase::scratch_record);
    const auto third=frame.attribute(2);data.value=third;data.field=2;notify(Phase::attribute);
    m.store(0x12fe9cu,data.header,data.handle);m.store(0x12fe9eu,data.header+8u,data.address);
    m.store(0x12fea1u,data.header+12u,data.bytes);m.store(0x12fea4u,data.header+20u,second);
    m.store(0x12fea7u,data.header+24u,third);m.store(0x12feaau,data.header+28u,0);
    m.store(0x12feb1u,data.header+32u,data.previous);
    if(data.previous)m.store(0x12feb6u,data.previous+28u,data.header);
    data.protection_bytes=m.word(data.header+12u);data.field=0;notify(Phase::link_inputs);
    data.protection_address=m.word(data.header+8u);data.field=1;notify(Phase::link_inputs);
    notify(Phase::publish_list);m.store(0x12fec3u,0x4f2d2cu,data.header);
    require(services.protect,0x2db4cau,stats);services.protect(data.protection_address,data.protection_bytes,4u,fp);
    data.record=frame.record_slot();notify(Phase::clear_record);
    constexpr std::array<std::uint32_t,9> clear_sites{0x12fed9u,0x12fedbu,0x12fedeu,0x12fee1u,
        0x12fee4u,0x12fee7u,0x12feeau,0x12feedu,0x12fef0u};
    for(unsigned n=0;n<clear_sites.size();++n)m.store(clear_sites[n],data.record+2u+n*4u,0);
    m.store(0x12fef3u,data.record+38u,0,2);
    data.value=m.word(data.record+2u,1)|1u;notify(Phase::active_byte);m.store(0x12fef7u,data.record+2u,data.value,1);
    data.value=data.address;notify(Phase::returning);if(stats)++stats->allocated;return data.address;
}
} // namespace halo2
