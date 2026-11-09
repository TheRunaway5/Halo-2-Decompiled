#include "halo2/native_tu15_physical_epoch.hpp"
#include <bit>

namespace halo2 {namespace {
using Phase=NativeTu15PhysicalEpochPhase;using Data=NativeTu15PhysicalEpochPhaseData;
class Memory {
    SourcePhysicsContactMemory &memory_;const SourcePhysicsContactRecordObserver &observe_;const NativeTu15PhysicalEpochPhases &phases_;
public:
    Memory(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,const NativeTu15PhysicalEpochPhases &phases):memory_(memory),observe_(observe),phases_(phases){}
    std::uint32_t word(std::uint32_t pc,std::uint32_t address,unsigned width=4)const {
        if(phases_.before_memory)phases_.before_memory(pc);
        std::array<std::byte,4>b{};memory_.read(address,std::span(b).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(8u*n);return value;
    }
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){
        if(phases_.before_memory)phases_.before_memory(pc);
        SourcePhysicsContactRecordStore e{pc,address,{},{}};if(observe_){e.before.resize(4);memory_.read(address,e.before);}
        std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(8u*n));
        memory_.write(address,b);if(observe_){e.written.assign(b.begin(),b.end());observe_(e);}
    }
};
std::int32_t signed_value(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
template<class Notify,class Record>void allocated_records(Memory &memory,Data &data,bool reload_scan_limit,Notify notify,Record record){
    for(;;){
        data.previous=data.index;++data.index;data.scan=false;notify(Phase::index_advance);
        if(signed_value(data.index)<0)break;
        data.limit=memory.word(reload_scan_limit?0x12f05cu:0x13fd23u,data.array+0x38u);notify(Phase::limit);
        if(signed_value(data.index)>=signed_value(data.limit))break;
        data.bitmap=memory.word(reload_scan_limit?0x12f061u:0x13fd2au,data.array+0x48u);notify(Phase::bitmap);
        bool occupied{};
        for(;;){
            data.mask=1u<<(data.index&31u);notify(Phase::bit_address);
            data.value=memory.word(reload_scan_limit?0x12f075u:0x13fd41u,data.bitmap+(data.index>>5u)*4u);notify(Phase::bit_test);
            if(data.value&data.mask){occupied=true;break;}
            data.scan=true;
            if(reload_scan_limit){data.limit=memory.word(0x12f07au,data.array+0x38u);notify(Phase::limit);}
            data.previous=data.index;++data.index;notify(Phase::index_advance);
            if(signed_value(data.index)>=signed_value(data.limit))break;
        }
        if(!occupied)break;
        notify(Phase::occupied);if(data.index==0xffffffffu)break;
        data.stride=memory.word(reload_scan_limit?0x12f095u:0x13fd54u,data.array+0x24u);notify(Phase::record_size);
        data.records=memory.word(reload_scan_limit?0x12f098u:0x13fd57u,data.array+0x44u);notify(Phase::record_base);
        data.record=data.stride*data.index+data.records;notify(Phase::record_address);
        if(!data.record)break;record();
    }
}
}
void native_tu15_reset_physical_object_epochs(SourcePhysicsContactMemory &memory,const NativeTu15PhysicalEpochPhases &phases,
    const SourcePhysicsContactRecordObserver &observe,NativeTu15PhysicalEpochStats *stats){
    if(stats)++stats->object_resets;Memory m(memory,observe,phases);Data data;
    const auto notify=[&](Phase phase){if(phases.observe)phases.observe(phase,data);};notify(Phase::begin);
    data.array=m.word(0x12f042u,0x4f2d24u);notify(Phase::array);data.value=m.word(0x12f048u,data.array+0x29u,1);data.enabled=data.value!=0;notify(Phase::enabled);
    if(data.enabled){notify(Phase::iteration_begin);allocated_records(m,data,true,notify,[&]{
        data.field=0;data.object=m.word(0x12f0a2u,data.record+16u);notify(Phase::object);
        if(!data.object)return;
        m.store(0x12f0a9u,data.object+0x54u,0);
        data.field=1;data.object=m.word(0x12f0acu,data.record+16u);notify(Phase::object);m.store(0x12f0afu,data.object+0x50u,0);
        data.field=2;data.object=m.word(0x12f0b2u,data.record+16u);notify(Phase::object);m.store(0x12f0b5u,data.object+0x70u,0);
        if(stats)++stats->objects_cleared;
    });}
    notify(Phase::finish);m.store(0x12f084u,0x4f2d58u,4u);
}
void native_tu15_rebase_physical_page_epochs(SourcePhysicsContactMemory &memory,std::uint32_t pool,
    const NativeTu15PhysicalEpochPhases &phases,const SourcePhysicsContactRecordObserver &observe,NativeTu15PhysicalEpochStats *stats){
    if(stats)++stats->page_rebases;Memory m(memory,observe,phases);Data data;data.pool=pool;
    const auto notify=[&](Phase phase){if(phases.observe)phases.observe(phase,data);};notify(Phase::begin);
    data.array=m.word(0x13fd13u,pool+0x64u);notify(Phase::array);m.store(0x13fd16u,pool+0x38u,0x101u);notify(Phase::iteration_begin);
    allocated_records(m,data,false,notify,[&]{
        data.value=m.word(0x13fd61u,data.record+20u);data.age=0x7fffffffu-data.value;notify(Phase::timestamp);
        if(data.age>0x100u)m.store(0x13fd73u,data.record+20u,0);
        else{data.value=0x100u-data.age;notify(Phase::adjusted);m.store(0x13fd83u,data.record+20u,data.value);}
        if(stats)++stats->pages_rebased;
    });
}
void native_tu15_advance_physical_epochs(SourcePhysicsContactMemory &memory,NativeTu15PhysicsFrameFp &fp,
    const NativeTu15PhysicalEpochServices &services,const NativeTu15PhysicalEpochPhases &phases,
    const SourcePhysicsContactRecordObserver &observe,NativeTu15PhysicalEpochStats *stats){
    if(stats)++stats->ticks;Memory m(memory,observe,phases);Data data;
    const auto notify=[&](Phase phase){if(phases.observe)phases.observe(phase,data);};
    data.previous=m.word(0x12f0c0u,0x4f2d58u);data.value=data.previous+1u;notify(Phase::counter);m.store(0x12f0c0u,0x4f2d58u,data.value);
    if(!data.value){if(services.reset_objects)services.reset_objects(fp);else native_tu15_reset_physical_object_epochs(memory,{},observe,stats);}
    notify(Phase::tick_begin);data.pool=m.word(0x12f0ceu,0x4f2d34u);notify(Phase::pool);
    data.value=m.word(0x12f0d4u,data.pool+0x38u);notify(Phase::epoch);
    if(data.value==0x7fffffffu){
        notify(Phase::rebase_call);
        if(services.rebase_pages){const auto resumed=services.rebase_pages(data.pool,fp);data.pool=resumed.pool;data.policy=resumed.policy;}
        else native_tu15_rebase_physical_page_epochs(memory,data.pool,{},observe,stats);
    }else{++data.value;notify(Phase::epoch_advance);m.store(0x12f0ebu,data.pool+0x38u,data.value);}
    constexpr std::array<std::uint32_t,8> sites{0x12f0eeu,0x12f0f1u,0x12f0f4u,0x12f0f7u,0x12f0fau,0x12f0fdu,0x12f100u,0x12f103u};
    for(unsigned index=0;index<sites.size();++index)m.store(sites[index],data.pool+0x44u+index*4u,data.policy);
}
} // namespace halo2
