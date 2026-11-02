#include "halo2/native_tu15_phantom_added_listener.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&memory,std::uint32_t address,unsigned bytes=4) {
    std::array<std::byte,4>raw{};memory.read(address,std::span(raw).first(bytes));
    std::uint32_t value{};for(unsigned n=0;n<bytes;++n)value|=std::to_integer<unsigned>(raw[n])<<(n*8u);return value;
}
void store(SourcePhysicsContactMemory&memory,std::uint32_t address,std::uint32_t value,
    const SourcePhysicsContactRecordObserver&observer) {
    std::array<std::byte,4>raw{};for(unsigned n=0;n<4;++n)raw[n]=std::byte(value>>(n*8u));
    SourcePhysicsContactRecordStore event{0x282763u,address,{},{}};
    if(observer){event.before.resize(4);memory.read(address,event.before);event.written.assign(raw.begin(),raw.end());}
    memory.write(address,raw);if(observer)observer(event);
}
} // namespace
bool native_tu15_object_phantom_delete_protected(const SourcePhysicsContactMemory&memory,
    std::uint32_t datum,NativeTu15ObjectPhantomDeleteProtectionStats*stats) {
    if(stats)++stats->queries;
    const auto header=word(memory,0x4f3218u);
    if(word(memory,header+0xcu,1)!=4u){if(stats)++stats->other_modes;return false;}
    if(stats)++stats->mode_four;
    const auto owner=word(memory,0x4ecbd0u),entries=word(memory,owner+0x44u);
    const auto object=word(memory,entries+(datum&0xffffu)*12u+8u);
    const bool result=word(memory,object+0xd4u)!=0xffffffffu;
    if(result&&stats)++stats->protected_objects;return result;
}
void native_tu15_notify_gameplay_phantom_added(SourcePhysicsContactMemory&memory,
    NativeTu15PhantomOverlapAddedEvent event,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15PhantomAddedListenerServices&services,
    const SourcePhysicsContactRecordObserver&observer,NativeTu15PhantomAddedListenerStats*stats) {
    if(stats)++stats->calls;
    const auto wrapper=word(memory,event.identity+4u);
    if(word(memory,wrapper+0x18u)!=1u){if(stats)++stats->ignored_wrappers;return;}
    const auto body=word(memory,wrapper+0x20u);
    if(!body){if(stats)++stats->null_bodies;return;}
    if(stats)++stats->property_searches;
    const auto count=word(memory,body+0x34u);std::uint32_t datum{};
    if(std::bit_cast<std::int32_t>(count)>0) {
        const auto data=word(memory,body+0x30u);
        for(std::uint32_t n=0;std::bit_cast<std::int32_t>(n)<std::bit_cast<std::int32_t>(count);++n) {
            if(word(memory,data+n*8u)!=0x2001u)continue;
            datum=word(memory,data+n*8u+4u);if(stats)++stats->property_matches;
            if(datum==0xffffffffu){if(stats)++stats->null_datums;return;}
            break;
        }
    }
    const auto owner=word(memory,0x52be5cu),data=word(memory,owner+0x44u);
    const auto component=data+(datum&0xffffu)*160u;
    const auto flags=word(memory,component+4u),object=word(memory,component+8u);
    store(memory,component+4u,flags|0x1000u,observer);if(stats)++stats->marked_components;
    if(native_tu15_object_phantom_delete_protected(memory,object,stats?&stats->protection:nullptr)) {
        if(stats)++stats->protected_objects;return;
    }
    if(stats)++stats->delete_requests;
    if(!services.delete_object){if(stats)++stats->unbound_delete_requests;throw std::runtime_error("Native TU1.5 phantom-added listener requires nativeBA570 object="+std::to_string(object));}
    services.delete_object(object,fp);
}
} // namespace halo2
