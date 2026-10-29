#include "halo2/native_campaign_ai_navigation_filter_tu15.hpp"
#include "halo2/native_campaign_ai_character_tu15.hpp"
#include <array>

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));
    std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width=4) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);memory.read(address,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observe)observe(event);
}
std::uint32_t tag(const SourcePhysicsContactMemory &memory,std::uint32_t registry,std::uint32_t datum) {
    return read(memory,registry+(datum&0xffffu)*16u+8u);
}
}
void native_campaign_ai_navigation_filter_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t actor,std::uint32_t destination,const SourcePhysicsContactRecordObserver &observe) {
    const auto actors=read(memory,0x5020c8u),rows=read(memory,actors+0x44u);
    const auto record=rows+(actor&0xffffu)*0x888u;
    const auto character=read(memory,record+0x54u);
    const auto registry=read(memory,0x4f0414u);
    const auto definition=tag(memory,registry,character);
    const auto physics_character=read(memory,definition+0x20u);
    const auto physics=physics_character==0xffffffffu ? 0u : tag(memory,registry,physics_character);
    if(physics) {
        if(std::int32_t(read(memory,physics+0x4cu))>0) {
            const auto flags=read(memory,physics+0x50u);
            publish(memory,observe,0x20155c,destination,read(memory,flags)<<1u);
        }else publish(memory,observe,0x201560,destination,0);
    }else publish(memory,observe,0x201564,destination,0xff);

    bool action_restriction{};
    const auto action=read(memory,record+0x858u);
    if(action!=0xffffffffu) {
        const auto actions=read(memory,0x50eee0u);
        const auto action_rows=read(memory,actions+0x44u);
        const auto action_record=action_rows+(action&0xffffu)*0xd4u;
        if(read(memory,action_record+0x86u,1)) {
            const auto action_character=read(memory,action_record+0x88u);
            const auto action_definition=tag(memory,registry,action_character);
            if(std::int32_t(read(memory,action_definition+0x4cu))>0) {
                const auto flags=read(memory,action_definition+0x50u);
                const auto restriction=read(memory,flags);
                const auto current=read(memory,destination);
                publish(memory,observe,0x2015b3,destination,current&(restriction<<1u));
                action_restriction=true;
            }
        }
    }
    if(!action_restriction) {
        const auto squad=read(memory,record+0x30u);
        if(squad!=0xffffffffu) {
            const auto squads=read(memory,0x52be7cu),squad_rows=read(memory,squads+0x44u);
            const auto order_index=std::uint16_t(read(memory,squad_rows+(squad&0xffffu)*0x98u+0x2au,2));
            if(order_index!=0xffffu) {
                const auto scenario=read(memory,0x4ecc20u),orders=read(memory,scenario+0x244u);
                const auto order=orders+std::uint32_t(std::int32_t(std::int16_t(order_index)))*0x7cu;
                if(order) {
                    std::uint32_t flags{};
                    if(std::int32_t(read(memory,order+0x6cu))>0)flags=read(memory,order+0x70u);
                    else {
                        const auto order_character=native_campaign_ai_order_character_tu15(memory,order);
                        if(order_character && std::int32_t(read(memory,order_character+0x4cu))>0)
                            flags=read(memory,order_character+0x50u);
                    }
                    if(flags) {
                        const auto restriction=read(memory,flags),current=read(memory,destination);
                        publish(memory,observe,0x20161e,destination,current&(restriction<<1u));
                    }
                }
            }
        }
    }
    publish(memory,observe,0x201620,destination,read(memory,destination)|1u);
    const auto navigation=native_campaign_ai_character_navigation_tu15(memory,read(memory,record+0x54u));
    publish(memory,observe,0x20162b,destination+0xc,0,1);
    publish(memory,observe,0x20162e,destination+8,0);
    publish(memory,observe,0x201631,destination+4,0);
    publish(memory,observe,0x201634,destination+0xe,0,2);
    publish(memory,observe,0x201638,destination+0x10,0,2);
    publish(memory,observe,0x20163c,destination+0x12,0,2);
    publish(memory,observe,0x201640,destination+0x14,0,2);
    if(read(memory,record+0x266u,1)) {
        const auto vehicle=read(memory,record+0x26cu),objects=read(memory,0x4ecbd0u);
        const auto object_rows=read(memory,objects+0x44u);
        const auto body=read(memory,object_rows+(vehicle&0xffffu)*12u+8u);
        const auto vehicle_character=read(memory,body);
        const auto vehicle_navigation=native_campaign_ai_character_vehicle_navigation_tu15(memory,actor,vehicle_character);
        if(vehicle_navigation)publish(memory,observe,0x20167c,destination+0xe,read(memory,vehicle_navigation+0xb0u,2),2);
        publish(memory,observe,0x201680,destination+8,1);
        publish(memory,observe,0x20168f,destination+0x18,read(memory,record+0x300u));return;
    }
    if(navigation) {
        const auto kinds=std::int16_t(read(memory,navigation+0x1au,2));
        if(kinds>0) {
            const auto limited=kinds>6 ? 6u : unsigned(kinds);
            publish(memory,observe,0x2016bd,destination+8,(1u<<limited)-1u);
            if(std::int16_t(read(memory,navigation+0x1au,2))>=7)
                publish(memory,observe,0x2016c7,destination+0xc,1,1);
        }
        publish(memory,observe,0x2016ce,destination+4,read(memory,navigation+0x1cu));
        publish(memory,observe,0x2016d5,destination+0xe,read(memory,navigation+0x14u,2),2);
        publish(memory,observe,0x2016dd,destination+0x10,read(memory,navigation+0x16u,2),2);
        publish(memory,observe,0x2016e5,destination+0x12,read(memory,navigation+0x10u,2),2);
        publish(memory,observe,0x2016ed,destination+0x14,read(memory,navigation+0x12u,2),2);
    }
    publish(memory,observe,0x2016f9,destination+0x18,read(memory,record+0x300u));
}
}
