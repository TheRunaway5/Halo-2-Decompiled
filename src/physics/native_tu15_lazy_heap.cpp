#include "halo2/native_tu15_lazy_heap.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
using Slot=NativeTu15LazyHeapSlot;using Phase=NativeTu15LazyHeapPhase;
std::int32_t signed_word(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
class Memory {
    SourcePhysicsContactMemory &memory_;const SourcePhysicsContactRecordObserver &observe_;
public:
    Memory(SourcePhysicsContactMemory &m,const SourcePhysicsContactRecordObserver &o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p)const {
        std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t value{};
        for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(b[n])<<(8u*n);return value;
    }
    void store(std::uint32_t pc,std::uint32_t p,std::uint32_t value) {
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe_){event.before.resize(4);memory_.read(p,event.before);}
        std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(8u*n));
        memory_.write(p,b);if(observe_){event.written.assign(b.begin(),b.end());observe_(event);}
    }
};
template<class Callback> void require(const Callback &callback,std::uint32_t owner,NativeTu15LazyHeapStats *stats) {
    if(callback)return;if(stats){++stats->unbound_services;++stats->unbound_owners[owner];}
    throw std::runtime_error("Native lazy heap requires actual owner="+std::to_string(owner));
}
}
std::uint32_t native_tu15_lazy_default_heap(SourcePhysicsContactMemory &memory,NativeTu15LazyHeapFrame &frame,
    NativeTu15PhysicsFrameFp &fp,const NativeTu15LazyHeapServices &services,const NativeTu15LazyHeapPhases &phases,
    const SourcePhysicsContactRecordObserver &observe,NativeTu15LazyHeapStats *stats) {
    if(stats)++stats->calls;Memory m(memory,observe);NativeTu15LazyHeapPhaseData data;
    const auto notify=[&](Phase phase){if(phases.observe)phases.observe(phase,data);};
    const auto initial=m.word(0x486074u);data.value=initial;notify(Phase::binding);
    if(initial){if(stats)++stats->initialized_returns;notify(Phase::returning);return initial;}
    data.slow=true;data.bytes=0x800000u;notify(Phase::slow_begin);
    for(;;) {
        if(stats)++stats->attempts;
        frame.store(Slot::start_padding,0,0x149950u);frame.store(Slot::start_high,0,0x149954u);
        require(services.timestamp_counter,0x149958u,stats);const auto start=services.timestamp_counter(0x149958u);
        data.low=std::uint32_t(start);data.high=std::uint32_t(start>>32u);data.start_low=data.low;notify(Phase::attempt_clock);
        frame.store(Slot::start_high,data.high,0x14995eu);std::uint32_t allocation{};
        if(signed_word(data.bytes)>0) {
            data.pointer=m.word(0x4f2d34u);notify(Phase::pressure_binding);
            data.count=m.word(data.pointer+0x30u);notify(Phase::pressure_count);
            if(signed_word(data.count)>0) {
                frame.store(Slot::retries,0,0x149978u);
                for(;;) {
                    notify(Phase::allocation);require(services.allocate,0x12fe00u,stats);
                    if(stats)++stats->allocations;allocation=services.allocate(data.bytes,fp);
                    data.allocation=allocation;notify(Phase::allocated);if(allocation)break;
                    const auto retries=frame.load(Slot::retries);data.value=retries;notify(Phase::retry);
                    if(signed_word(retries)<90) {
                        if(stats)++stats->retries;frame.store(Slot::retries,retries+1u,0x14999fu);
                        require(services.cleanup_first,0x12ef10u,stats);services.cleanup_first(fp);
                        require(services.cleanup_second,0x12ef60u,stats);services.cleanup_second(fp);
                        require(services.cleanup_third,0x12f0c0u,stats);services.cleanup_third(fp);
                        require(services.milliseconds,0x33b590u,stats);data.value=services.milliseconds(fp);
                        data.previous=m.word(0x4f2d50u);notify(Phase::clock_deadline);
                        if(data.value>data.previous) {
                            const auto now=services.milliseconds(fp);data.value=now;notify(Phase::notification_clock);
                            data.value=now+200u;notify(Phase::notification_value);m.store(0x1499c9u,0x4f2d50u,data.value);
                            require(services.notify,0x12edf0u,stats);services.notify(fp);if(stats)++stats->notifications;
                        }
                        m.store(0x1499d3u,0x4f2d54u,0);continue;
                    }
                    frame.store(Slot::finish_padding,0,0x1499dbu);frame.store(Slot::finish_high,0,0x1499dfu);
                    require(services.timestamp_counter,0x1499e3u,stats);const auto finish=services.timestamp_counter(0x1499e3u);
                    data.low=std::uint32_t(finish);data.high=std::uint32_t(finish>>32u);notify(Phase::elapsed_clock);
                    data.start_high=frame.load(Slot::start_high);notify(Phase::elapsed_inputs);
                    data.delta_low=data.low-data.start_low;
                    data.delta_high=data.high-data.start_high-std::uint32_t(data.low<data.start_low);
                    if(signed_word(data.delta_high)<0)data.delta_low=data.delta_high=0;
                    notify(Phase::elapsed_delta);
                    require(services.deadline_reached,0x22a5c0u,stats);if(stats)++stats->elapsed_queries;
                    data.deadline_reached=services.deadline_reached((std::uint64_t(data.delta_high)<<32u)|data.delta_low,fp);
                    notify(Phase::deadline);if(data.deadline_reached)break;
                    data.pointer=m.word(0x411568u);notify(Phase::yield_binding);
                    require(services.yield_context,0x404d10u,stats);const auto context=services.yield_context(data.pointer,fp);
                    data.pointer=m.word(context+0x934u);notify(Phase::yield_process);
                    const auto current=m.word(data.pointer+0x3240u);data.value=current;notify(Phase::yield_cursor);
                    data.previous=m.word(data.pointer+0x3244u);notify(Phase::yield_bound);
                    if(current==data.previous){data.value=m.word(data.pointer+0x400700u);notify(Phase::yield_fallback);}
                    require(services.yield,0x2dbf74u,stats);services.yield(data.value,fp);if(stats)++stats->yields;
                }
            }
        }
        data.allocation=allocation;notify(Phase::backing);m.store(0x149a3fu,0x486060u,allocation);
        if(allocation)break;
        data.previous=data.bytes;data.bytes=std::uint32_t(signed_word(data.bytes)>>1);notify(Phase::reduce_request);
        if(signed_word(data.bytes)<0x100000)break;
    }
    data.value=m.word(0x48605cu);notify(Phase::constructor_binding);
    data.object=data.value+0x105440u;data.adjustment=((data.allocation+15u)&0xfffffff0u)-data.allocation;
    notify(Phase::constructor_inputs);
    if(data.object) {
        if(stats)++stats->constructions;
        m.store(0x149a6cu,data.object+4u,1);m.store(0x149a73u,data.object+8u,0);
        m.store(0x149a76u,data.object+12u,0);m.store(0x149a79u,data.object+16u,0);
        m.store(0x149a7cu,data.object+20u,0xffffffffu);
        data.bytes=(data.bytes-data.adjustment)&0xfffffffcu;notify(Phase::constructor_size);
        m.store(0x149a8bu,data.object+24u,data.allocation);m.store(0x149a8eu,data.object+28u,data.bytes);
        m.store(0x149a91u,data.object+32u,0);m.store(0x149a94u,data.object,0x46393cu);
    }
    notify(Phase::publish_binding);m.store(data.object?0x149a9cu:0x149ab4u,0x486074u,data.object);
    require(services.trim,0x1cd130u,stats);if(stats)++stats->trims;services.trim(true,fp);
    data.value=m.word(0x486074u);notify(Phase::returning);return data.value;
}
} // namespace halo2
