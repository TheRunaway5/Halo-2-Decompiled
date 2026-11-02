#include "halo2/native_tu15_contact_point_removal.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& memory_;
    const SourcePhysicsContactRecordObserver& observe_;
public:
    Memory(SourcePhysicsContactMemory& memory,const SourcePhysicsContactRecordObserver& observe)
        :memory_(memory),observe_(observe){}
    std::uint32_t word(std::uint32_t address,unsigned size=4)const{
        std::array<std::byte,4> bytes{};memory_.read(address,std::span(bytes).first(size));
        std::uint32_t value{};for(unsigned n=0;n<size;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
        return value;
    }
    void word(std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned size=4){
        std::array<std::byte,4> bytes{};for(unsigned n=0;n<size;++n)bytes[n]=std::byte(value>>(8u*n));
        SourcePhysicsContactRecordStore event{site,address,{},{}};
        if(observe_){event.before.resize(size);memory_.read(address,event.before);event.written.assign(bytes.begin(),bytes.begin()+size);}
        memory_.write(address,std::span(bytes).first(size));if(observe_)observe_(event);
    }
};
bool negative(std::uint32_t value){return std::bit_cast<std::int32_t>(value)<0;}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
}

void native_tu15_notify_removed_contact_point(SourcePhysicsContactMemory& memory,
    bool world,std::uint32_t object,std::uint32_t event,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15ContactPointRemovalServices& services,const SourcePhysicsContactRecordObserver& observe){
    Memory m(memory,observe);const auto offset=world?0xb4u:0x60u;
    m.word(world?0x3224dbu:0x31feebu,event+0x10u,world?0u:object);
    auto index=m.word(object+offset+4u)-1u;
    while(!negative(index)){
        const auto data=m.word(object+offset),receiver=m.word(data+index*4u);
        if(receiver){
            const auto table=m.word(receiver);const NativeTu15ContactVirtualOwner owner{m.word(table+4u)};
            if(!services.listener_removed)throw std::runtime_error("Native TU1.5 point removal requires its actual listener owner");
            services.listener_removed(receiver,owner,event,fp);
        }
        --index;
    }
    index=m.word(object+offset+4u)-1u;
    while(!negative(index)){
        const auto data=m.word(object+offset);
        if(!m.word(data+index*4u)){
            const auto count=m.word(object+offset+4u)-1u;
            m.word(world?0x322527u:0x31ff21u,object+offset+4u,count);
            auto destination=index;
            if(less(destination,count))for(;;){
                const auto current_data=m.word(object+offset),next=m.word(current_data+destination*4u+4u);
                m.word(world?0x32253eu:0x31ff3au,current_data+destination*4u,next);
                const auto current_count=m.word(object+offset+4u);++destination;
                if(!less(destination,current_count))break;
            }
        }
        --index;
    }
}

void native_tu15_remove_collector_contact_point(SourcePhysicsContactMemory& memory,
    std::uint32_t collector,std::uint32_t point,std::uint32_t info,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15ContactPointRemovalServices& services,
    const SourcePhysicsContactRecordObserver& observe){
    Memory m(memory,observe);const auto map=m.word(collector+0x20u);
    const auto index=m.word(map+(point&0xffffu),1);m.word(0x325da5u,map+(point&0xffffu),0xffu,1);
    const auto old_count=m.word(collector+0x94u);
    m.word(0x325db7u,info,0);m.word(0x325dbbu,info+4u,0);
    m.word(0x325dbfu,info+8u,0);m.word(0x325dc3u,info+12u,0);
    if(old_count==2u){m.word(0x325dc9u,info+8u,4);m.word(0x325dd1u,info+4u,0x20);m.word(0x325dd9u,info+12u,1);}
    const auto count=old_count-1u;
    if(less(index,count)){
        auto offset=index*28u,remaining=count-index;
        do{
            const auto data=m.word(collector+0x90u),destination=data+offset;
            offset+=28u;--remaining;
            for(unsigned n=0;n<7;++n){const auto value=m.word(destination+28u+n*4u);m.word(0x325e05u,destination+n*4u,value);}
        }while(remaining);
    }
    const auto data=m.word(collector+0x90u);m.word(0x325e1bu,data+index*28u+0x18u,0,2);
    m.word(0x325e20u,collector+0x94u,count);
    const auto active=m.word(collector+0x38u);m.word(0x325e26u,collector+0x38u,active-1u);
    auto current=m.word(collector+0x24u)-1u;
    while(!negative(current)){
        const auto current_map=m.word(collector+0x20u),value=m.word(current_map+current,1);
        if(value!=0xffu&&less(index,value)){
            const auto before=m.word(current_map+current,1);
            m.word(0x325e44u,current_map+current,before-1u,1);
        }
        --current;
    }
    const auto bytes=m.word(info+4u),words=m.word(info+8u),entries=m.word(info+12u);
    m.word(0x325e5bu,info+4u,bytes+0x30u);m.word(0x325e5fu,info+8u,words+4u);
    const auto receiver=m.word(collector+0x14u);
    m.word(0x325e6cu,info+12u,entries+1u);
    const auto table=m.word(receiver);const NativeTu15ContactVirtualOwner owner{m.word(table+0xcu)};
    if(!services.collector_removed)throw std::runtime_error("Native TU1.5 point removal requires its actual collector update owner");
    services.collector_removed(receiver,owner,collector,info,fp);
    const auto flags=m.word(collector+0x50u);m.word(0x325e7eu,collector+0x50u,flags|5u);
}

void native_tu15_remove_contact_point(SourcePhysicsContactMemory& memory,
    std::uint32_t manager,std::uint32_t point,NativeTu15ContactPointRemovalScratch scratch,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15ContactPointRemovalServices& services,
    const SourcePhysicsContactRecordObserver& observe){
    Memory m(memory,observe);const auto world=m.word(manager+8u),second=m.word(manager+0x20u),first=m.word(manager+0x1cu);
    m.word(0x322f8cu,scratch.event,point,2);m.word(0x322f91u,scratch.event+4u,manager);
    m.word(0x322f95u,scratch.event+8u,first);m.word(0x322f99u,scratch.event+12u,second);
    native_tu15_notify_removed_contact_point(memory,true,world,scratch.event,fp,services,observe);
    if(m.word(first+0x64u))native_tu15_notify_removed_contact_point(memory,false,first,scratch.event,fp,services,observe);
    if(m.word(second+0x64u))native_tu15_notify_removed_contact_point(memory,false,second,scratch.event,fp,services,observe);
    const auto collector=manager+0x10u;
    native_tu15_remove_collector_contact_point(memory,collector,point,scratch.removal_info,fp,services,observe);
    if(!m.word(manager+0xa4u)){
        const auto current_world=m.word(manager+8u),saved=m.word(current_world+0x12cu,1);
        m.word(0x322ff3u,current_world+0x12cu,0,1);const auto input_world=m.word(manager+8u);
        if(!services.update_world)throw std::runtime_error("Native TU1.5 point removal requires its actual world update owner");
        services.update_world(input_world,scratch.result_byte,collector,fp);
        const auto returned_world=m.word(manager+8u);m.word(0x323006u,returned_world+0x12cu,saved,1);
    }
}
} // namespace halo2
