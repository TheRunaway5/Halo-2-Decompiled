#include "halo2/native_tu15_contact_completion.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;
    const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t address,unsigned bytes=4)const{
        std::array<std::byte,4>raw{};memory_.read(address,std::span(raw).first(bytes));
        std::uint32_t value{};for(unsigned n=0;n<bytes;++n)value|=std::to_integer<unsigned>(raw[n])<<(n*8u);
        return value;
    }
    void store(std::uint32_t site,std::uint32_t address,std::span<const std::byte>raw){
        SourcePhysicsContactRecordStore event{site,address,{},{}};
        if(observe_){event.before.resize(raw.size());memory_.read(address,event.before);event.written.assign(raw.begin(),raw.end());}
        memory_.write(address,raw);if(observe_)observe_(event);
    }
    void word(std::uint32_t site,std::uint32_t address,std::uint32_t value){
        std::array<std::byte,4>raw{};for(unsigned n=0;n<4;++n)raw[n]=std::byte(value>>(n*8u));store(site,address,raw);
    }
    std::array<std::byte,16>vector(std::uint32_t address)const{
        if(address&15u)throw std::runtime_error("Native TU1.5 contact completion requires aligned contact storage");
        std::array<std::byte,16>raw{};memory_.read_vector(address,raw);return raw;
    }
    void vector(std::uint32_t site,std::uint32_t address,const std::array<std::byte,16>&raw){
        if(address&15u)throw std::runtime_error("Native TU1.5 contact completion requires aligned contact storage");
        store(site,address,raw);
    }
};
bool nonnegative(std::uint32_t n){return std::bit_cast<std::int32_t>(n)>=0;}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
void cohort(Memory&m,std::uint32_t subject,std::uint32_t array_offset,std::uint32_t count_offset,
    std::uint32_t count_site,std::uint32_t copy_site,NativeTu15ContactCompletionPacket packet,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ContactCompletionServices&s,NativeTu15ContactCompletionStats*stats){
    for(auto index=m.word(subject+count_offset)-1u;nonnegative(index);--index){
        const auto array=m.word(subject+array_offset);
        const NativeTu15ContactCompletionListener listener{m.word(array+index*4u)};
        if(!listener.identity)continue;
        const NativeTu15PhysicsVirtualOwner owner{m.word(m.word(listener.identity)+8u)};
        // Authenticated2E4D60 is the empty RET4 callback.
        if(owner.identity==0x2e4d60u)continue;
        if(!s.notify)throw std::runtime_error("Native TU1.5 contact completion requires its genuine listener owner");
        if(stats)++stats->custom_callbacks;s.notify(listener,owner,packet,fp);
    }
    for(auto index=m.word(subject+count_offset)-1u;nonnegative(index);--index){
        const auto array=m.word(subject+array_offset);
        if(m.word(array+index*4u))continue;
        const auto count=m.word(subject+count_offset)-1u;
        m.word(count_site,subject+count_offset,count);if(stats)++stats->null_entries_removed;
        if(!less(index,count))continue;
        auto cursor=index;
        do{
            const auto current=m.word(subject+array_offset);
            const auto next=m.word(current+cursor*4u+4u);
            m.word(copy_site,current+cursor*4u,next);
            const auto current_count=m.word(subject+count_offset);++cursor;
            if(!less(cursor,current_count))break;
        }while(true);
    }
}
}
void native_tu15_notify_world_contact_completion(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsWorld world,NativeTu15ContactCompletionPacket packet,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ContactCompletionServices&s,const SourcePhysicsContactRecordObserver&observe,NativeTu15ContactCompletionStats*stats){
    if(stats)++stats->world_notifications;Memory m(memory,observe);m.word(0x32256bu,packet.identity+16u,0);
    cohort(m,world.identity,0xb4u,0xb8u,0x3225b7u,0x3225ceu,packet,fp,s,stats);
}
void native_tu15_notify_body_contact_completion(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsBody body,NativeTu15ContactCompletionPacket packet,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ContactCompletionServices&s,const SourcePhysicsContactRecordObserver&observe,NativeTu15ContactCompletionStats*stats){
    if(stats)++stats->body_notifications;Memory m(memory,observe);m.word(0x31ff5bu,packet.identity+16u,body.identity);
    cohort(m,body.identity,0x60u,0x64u,0x31ff91u,0x31ffaau,packet,fp,s,stats);
}
void native_tu15_complete_contact_manager(SourcePhysicsContactMemory&memory,
    NativeTu15IslandCollisionAgent manager,NativeTu15PhysicsBody first,NativeTu15PhysicsBody second,
    NativeTu15IslandContactOutput output,NativeTu15ContactCompletionPacket packet,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ContactCompletionServices&s,const SourcePhysicsContactRecordObserver&observe,NativeTu15ContactCompletionStats*stats){
    if(stats)++stats->calls;Memory m(memory,observe);
    const auto header=m.word(output.identity);auto record=m.word(header);
    const auto count=m.word(header+4u);
    if(nonnegative(count-1u))for(auto remaining=count;remaining;--remaining,record+=48u){
        const auto id=m.word(record+0x20u,2),mapping=m.word(manager.identity+0x30u);
        const auto index=m.word(mapping+id,1),base=m.word(manager.identity+0x44u);
        const auto first_vector=m.vector(record);const auto destination=base+index*32u;
        m.vector(0x3230f6u,destination,first_vector);
        const auto second_vector=m.vector(record+16u);
        m.vector(0x323101u,destination+16u,second_vector);if(stats)++stats->copied_contacts;
    }
    const auto countdown=m.word(manager.identity+0xcu)-1u;m.word(0x323107u,manager.identity+0xcu,countdown);
    if(countdown)return;
    const NativeTu15PhysicsBody first_metadata{m.word(first.identity+0x20u)},second_metadata{m.word(second.identity+0x20u)};
    const auto second_period=m.word(second_metadata.identity+0x5cu,2),first_period=m.word(first_metadata.identity+0x5cu,2);
    m.word(0x323134u,manager.identity+0xcu,std::min(first_period,second_period)+1u);
    m.word(0x32313bu,packet.identity+12u,output.identity);
    const NativeTu15PhysicsWorld world{m.word(manager.identity+8u)};
    m.word(0x323144u,packet.identity,manager.identity);m.word(0x323148u,packet.identity+4u,first.identity);m.word(0x32314cu,packet.identity+8u,second.identity);
    native_tu15_notify_world_contact_completion(memory,world,packet,fp,s,observe,stats);
    if(m.word(first_metadata.identity+0x64u))native_tu15_notify_body_contact_completion(memory,first_metadata,packet,fp,s,observe,stats);
    if(m.word(second_metadata.identity+0x64u))native_tu15_notify_body_contact_completion(memory,second_metadata,packet,fp,s,observe,stats);
}
} // namespace halo2
