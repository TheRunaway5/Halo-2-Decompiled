#include "halo2/native_campaign_ai_perception_policy_tu15.hpp"
#include "halo2/native_campaign_ai_character_tu15.hpp"
#include <array>
#include <bit>
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};
    for(unsigned k=0;k<n;++k)v|=std::to_integer<unsigned>(b[k])<<(k*8u);return v;
}
std::int16_t signed16(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
std::int8_t signed8(std::uint32_t v){return std::bit_cast<std::int8_t>(std::uint8_t(v));}
std::int32_t signed32(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
std::uint32_t row(std::uint32_t rows,std::uint32_t datum,std::uint32_t stride){return rows+(datum&0xffffu)*stride;}
std::uint32_t actor_record(const SourcePhysicsContactMemory&m,std::uint32_t actor){
    const auto actors=read(m,0x5020c8u);return row(read(m,actors+0x44u),actor,0x888u);
}
std::uint32_t tag(const SourcePhysicsContactMemory&m,std::uint32_t registry,std::uint32_t datum){return read(m,registry+(datum&0xffffu)*16u+8u);}
}
bool native_campaign_ai_perception_has_target_tu15(const SourcePhysicsContactMemory&m,std::uint32_t perception){
    const auto properties=read(m,0x50eef0u),rows=read(m,properties+0x44u),query=read(m,row(rows,perception,0x3cu)+0x14u);
    if(query==none)return false;
    const auto queries=read(m,0x50eeecu),query_rows=read(m,queries+0x44u),record=row(query_rows,query,0x124u);
    if(!record||!(record+0x70u))return false;
    return read(m,record+0xc0u)!=none;
}
std::uint32_t native_campaign_ai_character_tracking_policy_tu15(const SourcePhysicsContactMemory&m,std::uint32_t actor){
    auto character=read(m,actor_record(m,actor)+0x54u);if(character==none)return 0;
    const auto registry=read(m,0x4f0414u);
    do{
        const auto policy=native_campaign_ai_actor_order_character_tu15(m,actor);std::uint16_t variant=0;
        if(policy){const auto authored=read(m,policy+0x2eu,1);variant=authored?std::uint16_t(std::int32_t(signed8(authored))-1):std::uint16_t(read(m,policy+0x24u,2));}
        const auto definition=tag(m,registry,character);
        if(signed32(read(m,definition+0x7cu))>0){
            const auto count=signed32(read(m,definition+0x7cu)),index=std::int32_t(signed16(variant));
            const auto rows=read(m,definition+0x80u);return index<count?rows+std::uint32_t(index)*64u:rows;
        }
        character=read(m,definition+8u);
    }while(character!=none);
    return 0;
}
std::uint32_t native_campaign_ai_character_perception_policy_tu15(const SourcePhysicsContactMemory&m,std::uint32_t actor){
    const auto record=actor_record(m,actor);auto character=read(m,record+0x54u);if(character==none)return 0;
    const auto registry=read(m,0x4f0414u);
    do{
        const auto definition=tag(m,registry,character);const auto count=signed32(read(m,definition+0x4cu));
        if(count>1&&read(m,record+0x26cu)!=none)return read(m,definition+0x50u)+0x34u;
        if(count>2&&signed16(read(m,record+0x86u,2))>=4)return read(m,definition+0x50u)+0x68u;
        if(count>3&&signed16(read(m,record+0x86u,2))<=1)return read(m,definition+0x50u)+0x9cu;
        if(count>0)return read(m,definition+0x50u);
        character=read(m,definition+8u);
    }while(character!=none);
    return 0;
}
std::uint32_t native_campaign_ai_actor_vehicle_tu15(const SourcePhysicsContactMemory&m,std::uint32_t actor){
    const auto record=actor_record(m,actor);const auto secondary=read(m,record+0x268u,1),objects=read(m,0x4ecbd0u);
    if(secondary){
        const auto object=read(m,record+0x274u);
        if(object!=none){
            const auto rows=read(m,objects+0x44u),body=read(m,row(rows,object,12u)+8u);const auto seat=signed8(read(m,body+0x212u,1));
            if(seat!=-1){const auto vehicle=read(m,body+0x218u+std::uint32_t(std::int32_t(seat))*4u);if(vehicle!=none)return vehicle;}
        }
    }
    const auto object=read(m,record+0x18u);if(object==none)return none;
    const auto rows=read(m,objects+0x44u),body=read(m,row(rows,object,12u)+8u);const auto seat=signed8(read(m,body+0x212u,1));
    return seat==-1?none:read(m,body+0x218u+std::uint32_t(std::int32_t(seat))*4u);
}
bool native_campaign_ai_actor_vehicle_policy_tu15(const SourcePhysicsContactMemory&m,std::uint32_t actor){
    const auto record=actor_record(m,actor),vehicle=native_campaign_ai_actor_vehicle_tu15(m,actor);if(vehicle==none)return false;
    const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u),body=read(m,row(rows,vehicle,12u)+8u);
    const auto definition=read(m,body),registry=read(m,0x4f0414u),flags=read(m,tag(m,registry,definition)+0x12fu,1);
    if(flags&1u)return false;
    const auto unit=read(m,record+0x18u);if(unit==none)return true;
    const auto unit_body=read(m,row(rows,unit,12u)+8u);return !(read(m,unit_body+0x19u,1)&2u);
}
std::uint32_t native_campaign_ai_character_vehicle_perception_tu15(const SourcePhysicsContactMemory&m,std::uint32_t actor,std::uint32_t vehicle){
    auto character=read(m,actor_record(m,actor)+0x54u);if(character==none)return 0;
    const auto registry=read(m,0x4f0414u);
    do{
        const auto definition=tag(m,registry,character);const auto count=signed32(read(m,definition+0xccu));
        if(count>0){
            const auto rows=read(m,definition+0xd0u);std::uint16_t index=0;
            do{
                const auto block=rows+std::uint32_t(std::int32_t(signed16(index)))*0xccu;
                if(read(m,block+8u)==vehicle)return block;
                ++index;
            }while(std::int32_t(signed16(index))<count);
        }
        character=read(m,definition+8u);
    }while(character!=none);
    return 0;
}
std::uint32_t native_campaign_ai_actor_vehicle_perception_tu15(const SourcePhysicsContactMemory&m,std::uint32_t actor){
    const auto vehicle=native_campaign_ai_actor_vehicle_tu15(m,actor);if(vehicle==none)return 0;
    const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u),body=read(m,row(rows,vehicle,12u)+8u),definition=read(m,body);
    return native_campaign_ai_character_vehicle_perception_tu15(m,actor,definition);
}
std::int16_t native_campaign_ai_order_perception_variant_tu15(const SourcePhysicsContactMemory&m,std::uint32_t policy){
    const auto authored=signed8(read(m,policy+0x31u,1));return authored>0?std::int16_t(authored-1):signed16(read(m,policy+0x24u,2));
}
NativeCampaignAiChildPlayerTu15 native_campaign_ai_child_player_tu15(const SourcePhysicsContactMemory&m,std::uint32_t parent){
    const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u),body=read(m,row(rows,parent,12u)+8u);
    auto child=read(m,body+0x10u);if(child==none)return {none,none};
    const auto child_rows=read(m,objects+0x44u);
    while(child!=none){
        const auto member=read(m,row(child_rows,child,12u)+8u);
        if(read(m,member+0xaau,1)==0u&&read(m,member+0x13cu)!=none)return {read(m,member+0x13cu),child};
        child=read(m,member+0xcu);
    }
    return {none,none};
}
}
