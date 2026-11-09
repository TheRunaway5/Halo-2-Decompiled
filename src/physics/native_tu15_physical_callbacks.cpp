#include "halo2/native_tu15_physical_callbacks.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;
}
}
void native_tu15_notify_physical_allocations(SourcePhysicsContactMemory &memory,NativeTu15PhysicsFrameFp &fp,
    const NativeTu15PhysicalCallbackServices &services,const NativeTu15PhysicalCallbackPhases &phases,
    NativeTu15PhysicalCallbackStats *stats) {
    if(stats)++stats->calls;NativeTu15PhysicalCallbackPhaseData data;using Phase=NativeTu15PhysicalCallbackPhase;
    const auto notify=[&](Phase phase){if(phases.observe)phases.observe(phase,data);};
    data.head=word(memory,0x4f2d2cu);data.current=data.head;notify(Phase::binding);
    while(data.current) {
        if(stats)++stats->visited;data.owner=word(memory,data.current+20u);notify(Phase::callback_owner);
        if(data.owner) {
            data.context=word(memory,data.current+16u);notify(Phase::callback_context);
            data.address=word(memory,data.current+8u);notify(Phase::callback_address);notify(Phase::callback_call);
            if(!services.invoke){if(stats){++stats->unbound_callbacks;++stats->unbound_owners[data.owner];}
                throw std::runtime_error("Physical allocation callback requires actual owner="+std::to_string(data.owner));}
            if(stats)++stats->callbacks;
            data.current=services.invoke({data.owner,data.address,data.context,data.current},fp).current;
            data.head=word(memory,0x4f2d2cu);notify(Phase::reacquired_head);
        }
        data.scan=data.head;notify(Phase::head_check);
        if(data.scan)for(;;) {
            notify(Phase::search_compare);if(data.scan==data.current)break;
            data.scan=word(memory,data.scan+32u);notify(Phase::search_next);if(!data.scan)break;
        }
        if(data.scan){data.current=word(memory,data.current+32u);notify(Phase::next);}
        else{data.current=data.head;if(stats)++stats->restarts;notify(Phase::restart);}
        notify(Phase::current_check);
    }
}
} // namespace halo2
