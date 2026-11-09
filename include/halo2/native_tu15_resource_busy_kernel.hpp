#pragma once
#include <cstdint>

namespace halo2 {
enum class NativeTu15ResourceBusyPhase {initial_header,device,initial_parent,initial_parent_header,
    selected_header,parent,parent_header,fence,counter_owner,save_counter,counter,submitted,counter_compare,
    clear,busy,finished};
struct NativeTu15ResourceBusy {
    std::uint32_t resource{},initial_header{},device{},parent{},parent_header{},header{},fence{},counter_owner{},counter{},submitted{},elapsed{},delta{};
    bool busy{};
};
// Complete 4057F0 CPU query. Device/timeline words remain real Memory inputs;
// this operation neither creates a device nor supplies GPU completion.
template<class Access>bool native_tu15_resource_is_busy(Access &memory,NativeTu15ResourceBusy &s){
    using P=NativeTu15ResourceBusyPhase;
    s.initial_header=memory.read(P::initial_header,s,s.resource);s.device=memory.read(P::device,s,0x411568u);
    const auto busy=[&](){s.busy=true;memory.complete(P::busy,s);return true;};
    if((s.initial_header&0x70000u)==0x50000u){
        s.parent=memory.read(P::initial_parent,s,s.resource+0x14u);
        if(s.parent){
            if(s.initial_header&0x780000u)return busy();
            s.parent_header=memory.read(P::initial_parent_header,s,s.parent);
            if(s.parent_header&0x780000u)return busy();
            s.resource=s.parent;
        }
    }
    s.header=memory.read(P::selected_header,s,s.resource);if(s.header&0x780000u)return busy();
    if((s.header&0x70000u)==0x50000u){
        s.parent=memory.read(P::parent,s,s.resource+0x14u);
        if(s.parent){s.parent_header=memory.read(P::parent_header,s,s.parent);if(s.parent_header&0x780000u)return busy();}
    }
    s.fence=memory.read(P::fence,s,s.resource+8u);
    if(s.fence){
        s.counter_owner=memory.read(P::counter_owner,s,s.device+0x30u);memory.prepare(P::save_counter,s);
        s.counter=memory.read(P::counter,s,s.counter_owner);s.submitted=memory.read(P::submitted,s,s.device+0x2cu);
        s.elapsed=s.submitted-s.counter;s.delta=s.submitted-s.fence;memory.prepare(P::counter_compare,s);
        if(s.delta<s.elapsed)return busy();
    }
    memory.write(P::clear,s,s.resource+8u,0u);s.busy=false;memory.complete(P::finished,s);return false;
}
enum class NativeTu15PageBusyPhase {datum,array,rows,flags,global_policy,association,ready,query,busy,available};
struct NativeTu15PageBusy {
    std::uint32_t datum{},array{},rows{},record{},flags{},global_policy{},association{},ready{};bool busy{};
};
// Complete 12FCB0 object policy and resource query composition.
template<class Access,class Query>bool native_tu15_page_is_busy(Access &memory,NativeTu15PageBusy &s,Query &query){
    using P=NativeTu15PageBusyPhase;
    s.datum=memory.argument(P::datum,s);s.array=memory.read(P::array,s,0x4f2d24u,4u);
    s.rows=memory.read(P::rows,s,s.array+0x44u,4u);s.record=s.rows+(s.datum&0xffffu)*40u;
    s.flags=memory.read(P::flags,s,s.record+2u,1u);
    const auto busy=[&](){s.busy=true;memory.complete(P::busy,s);return true;};
    const auto available=[&](){s.busy=false;memory.complete(P::available,s);return false;};
    if(s.flags&1u)return busy();
    if(s.flags&2u){s.global_policy=memory.read(P::global_policy,s,0x4f2d49u,1u);if(!s.global_policy)return busy();}
    s.association=memory.read(P::association,s,s.record+12u,4u);if(s.association==0xffffffffu)return available();
    s.ready=memory.read(P::ready,s,s.record+3u,1u);if(!s.ready)return busy();
    memory.prepare(P::query,s);return query(s.record+0x14u,s)?busy():available();
}
} // namespace halo2
