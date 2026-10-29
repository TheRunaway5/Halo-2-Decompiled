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
std::uint32_t actor_record(const SourcePhysicsContactMemory &memory,std::uint32_t actor) {
    const auto pool=read(memory,0x5020c8u),rows=read(memory,pool+0x44u);
    return rows+(actor&0xffffu)*0x888u;
}
std::uint32_t tag(const SourcePhysicsContactMemory &memory,std::uint32_t registry,std::uint32_t datum) {
    return read(memory,registry+(datum&0xffffu)*16u+8u);
}
}
std::uint32_t native_campaign_ai_character_navigation_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t character) {
    if(character==0xffffffffu)return 0;
    const auto registry=read(memory,0x4f0414u);
    do {
        const auto definition=tag(memory,registry,character);
        if(std::int32_t(read(memory,definition+0x5c))>0)return read(memory,definition+0x60);
        character=read(memory,definition+8u);
    }while(character!=0xffffffffu);
    return 0;
}
std::uint32_t native_campaign_ai_character_vehicle_navigation_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t actor,std::uint32_t vehicle_character) {
    const auto record=actor_record(memory,actor);
    auto character=read(memory,record+0x54u);
    if(character==0xffffffffu)return 0;
    const auto registry=read(memory,0x4f0414u);
    do {
        const auto definition=tag(memory,registry,character);
        const auto count=std::int32_t(read(memory,definition+0xe4u));
        if(count>0) {
            const auto rows=read(memory,definition+0xe8u);
            std::uint16_t index{};
            do {
                const auto row=rows+std::uint32_t(std::int32_t(std::int16_t(index)))*0xb4u;
                if(read(memory,row+4u)==vehicle_character)return row;
                ++index;
            }while(std::int32_t(std::int16_t(index))<count);
        }
        character=read(memory,definition+8u);
    }while(character!=0xffffffffu);
    return 0;
}
std::uint32_t native_campaign_ai_order_character_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t order) {
    const auto index=std::int16_t(read(memory,order+0x20u,2));
    if(index<0)return 0;
    const auto scenario=read(memory,0x4ecc20u);
    if(std::int32_t(index)>=std::int32_t(read(memory,scenario+0x150u)))return 0;
    const auto rows=read(memory,scenario+0x154u);
    const auto character=read(memory,rows+std::uint32_t(index)*8u+4u);
    if(character==0xffffffffu)return 0;
    return tag(memory,read(memory,0x4f0414u),character);
}
std::uint32_t native_campaign_ai_actor_order_character_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t actor) {
    const auto record=actor_record(memory,actor);
    const auto action=read(memory,record+0x858u);
    if(action!=0xffffffffu) {
        const auto actions=read(memory,0x50eee0u),rows=read(memory,actions+0x44u);
        const auto action_record=rows+(action&0xffffu)*0xd4u;
        if(read(memory,action_record+0x86u,1)) {
            const auto character=read(memory,action_record+0x88u);
            const auto definition=tag(memory,read(memory,0x4f0414u),character);
            if(definition)return definition;
        }
    }
    const auto squad=read(memory,record+0x30u);
    if(squad==0xffffffffu)return 0;
    const auto squads=read(memory,0x52be7cu),rows=read(memory,squads+0x44u);
    const auto index=std::uint16_t(read(memory,rows+(squad&0xffffu)*0x98u+0x2au,2));
    if(index==0xffffu)return 0;
    const auto scenario=read(memory,0x4ecc20u),orders=read(memory,scenario+0x244u);
    const auto order=orders+std::uint32_t(std::int32_t(std::int16_t(index)))*0x7cu;
    return order ? native_campaign_ai_order_character_tu15(memory,order) : 0;
}
std::uint32_t native_campaign_ai_character_movement_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t actor) {
    const auto record=actor_record(memory,actor);
    auto character=read(memory,record+0x54u);
    if(character==0xffffffffu)return 0;
    const auto registry=read(memory,0x4f0414u);
    do {
        // The action/order character is looked up again for every ancestor;
        // it is not a frozen selector shared across inherited definitions.
        const auto policy=native_campaign_ai_actor_order_character_tu15(memory,actor);
        std::uint16_t variant{};
        if(policy) {
            const auto authored=std::uint8_t(read(memory,policy+0x2cu,1));
            variant=authored ? std::uint16_t(std::int32_t(std::int8_t(authored))-1) :
                std::uint16_t(read(memory,policy+0x24u,2));
        }
        const auto definition=tag(memory,registry,character);
        if(std::int32_t(read(memory,definition+0xa4u))>0) {
            const auto count=std::int32_t(read(memory,definition+0xa4u));
            const auto index=std::int32_t(std::int16_t(variant));
            const auto rows=read(memory,definition+0xa8u);
            return index<count ? rows+std::uint32_t(index)*36u : rows;
        }
        character=read(memory,definition+8u);
    }while(character!=0xffffffffu);
    return 0;
}
}
