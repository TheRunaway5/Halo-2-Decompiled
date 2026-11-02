#include "halo2/native_tu15_contact_world_update.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& memory_;const SourcePhysicsContactRecordObserver& observe_;
public:
    Memory(SourcePhysicsContactMemory& memory,const SourcePhysicsContactRecordObserver& observe):memory_(memory),observe_(observe){}
    std::uint32_t word(std::uint32_t address,unsigned size=4)const{
        std::array<std::byte,4>bytes{};memory_.read(address,std::span(bytes).first(size));
        std::uint32_t value{};for(unsigned n=0;n<size;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
    }
    void word(std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned size=4){
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<size;++n)bytes[n]=std::byte(value>>(n*8u));
        SourcePhysicsContactRecordStore event{site,address,{},{}};
        if(observe_){event.before.resize(size);memory_.read(address,event.before);event.written.assign(bytes.begin(),bytes.begin()+size);}
        memory_.write(address,std::span(bytes).first(size));if(observe_)observe_(event);
    }
};
bool negative(std::uint32_t value){return std::bit_cast<std::int32_t>(value)<0;}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
std::uint32_t argument(const std::function<std::uint32_t()>& input){
    if(!input)throw std::runtime_error("Native TU1.5 contact world update requires its current formal DATA");return input();
}
}

void native_tu15_notify_contact_world_update(SourcePhysicsContactMemory& memory,
    std::uint32_t world,std::uint32_t collector,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15ContactWorldUpdateServices& services,const SourcePhysicsContactRecordObserver& observe,
    NativeTu15ContactWorldUpdateStats* stats){
    Memory m(memory,observe);auto index=m.word(world+0x64u)-1u;
    while(!negative(index)){
        const auto data=m.word(world+0x60u),receiver=m.word(data+index*4u);
        if(receiver){
            const auto table=m.word(receiver);const NativeTu15ContactVirtualOwner owner{m.word(table+0xcu)};
            if(!services.listener)throw std::runtime_error("Native TU1.5 contact world update requires its actual listener owner");
            if(stats)++stats->listeners;services.listener(receiver,owner,collector,fp);
        }
        --index;
    }
    index=m.word(world+0x64u)-1u;
    while(!negative(index)){
        const auto data=m.word(world+0x60u);
        if(!m.word(data+index*4u)){
            const auto count=m.word(world+0x64u)-1u;m.word(0x322411u,world+0x64u,count);
            auto destination=index;
            if(less(destination,count))for(;;){
                const auto current_data=m.word(world+0x60u),next=m.word(current_data+destination*4u+4u);
                m.word(0x32242au,current_data+destination*4u,next);
                const auto current_count=m.word(world+0x64u);++destination;
                if(!less(destination,current_count))break;
            }
        }
        --index;
    }
}

void native_tu15_update_contact_world(SourcePhysicsContactMemory& memory,
    std::uint32_t world,const NativeTu15ContactWorldUpdateArguments& inputs,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15ContactWorldUpdateServices& services,
    const SourcePhysicsContactRecordObserver& observe,NativeTu15ContactWorldUpdateStats* stats){
    if(stats)++stats->attempts;Memory m(memory,observe);
    if(m.word(world+0x12cu,1)==1u){
        if(stats)++stats->queued;
        const auto capacity=m.word(world+0x104u),header=world+0xfcu,count=m.word(header+4u);
        if(count==(capacity&0x7fffffffu)){
            if(stats)++stats->growths;NativeTu15ArrayGrowthMemory array(memory,{header},observe);
            native_tu15_grow_array(array,4u,fp,services.allocation);
        }
        const auto current_count=m.word(header+4u),data=m.word(header),collector=argument(inputs.collector);
        m.word(0x316ad6u,data+current_count*4u,collector);
        const auto after=m.word(header+4u);m.word(0x316ad9u,header+4u,after+1u);
        const auto references=m.word(collector+6u,2);m.word(0x316adcu,collector+6u,references+1u,2);
        const auto result=argument(inputs.result_byte);m.word(0x316ae4u,result,0,1);
    }else{
        if(stats)++stats->immediate;
        const auto collector=argument(inputs.collector),first=m.word(collector+0xcu);
        auto island=m.word(first+0x44u);
        if(m.word(island+0x24u)==0xffffffffu){const auto second=m.word(collector+0x10u);island=m.word(second+0x44u);}
        native_tu15_notify_contact_world_update(memory,world,collector,fp,services,observe,stats);
        const auto table=m.word(island);const NativeTu15ContactVirtualOwner owner{m.word(table+4u)};
        if(!services.remove_from_island)throw std::runtime_error("Native TU1.5 contact world update requires its actual island removal owner");
        if(stats)++stats->islands;services.remove_from_island(island,owner,collector,fp);
        const auto result=argument(inputs.result_byte);m.word(0x316b19u,result,1,1);
    }
    if(stats)++stats->completed;
}
void native_tu15_update_contact_world(SourcePhysicsContactMemory& memory,
    std::uint32_t world,std::uint32_t result,std::uint32_t collector,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15ContactWorldUpdateServices& services,const SourcePhysicsContactRecordObserver& observe,
    NativeTu15ContactWorldUpdateStats* stats){
    native_tu15_update_contact_world(memory,world,{[result]{return result;},[collector]{return collector;}},fp,services,observe,stats);
}
} // namespace halo2
