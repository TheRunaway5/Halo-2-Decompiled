#include "halo2/native_campaign_ai_area_tu15.hpp"
#include <array>
#include <bit>

namespace halo2 {
namespace {
constexpr std::uint16_t none=0xffffu;
class AuthoredAreas {
    const SourcePhysicsContactMemory &memory_;
    std::uint32_t read(std::uint32_t address,unsigned width=4)const {
        std::array<std::byte,4> bytes{};
        memory_.read(address,std::span(bytes).first(width));
        std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
        return value;
    }
    std::uint16_t word(std::uint32_t address)const{return std::uint16_t(read(address,2));}
    static std::int32_t signed_word(std::uint16_t value){return std::bit_cast<std::int16_t>(value);}
    std::int32_t count(std::uint32_t address)const{return std::bit_cast<std::int32_t>(read(address));}
    static std::uint32_t scaled_word(std::uint16_t value,unsigned stride){return std::uint32_t(signed_word(value))*stride;}
    bool matches(std::uint32_t area,bool mode)const{return (read(area+0x20u)&1u)==unsigned(mode);}
    bool explicit_pairs(std::uint32_t squad,bool mode)const {
        if(std::bit_cast<std::int8_t>(std::uint8_t(read(squad+0x30u,1)))<0)return false;
        std::uint16_t index=0;
        while(true) {
            const auto pair=squad+scaled_word(index,4u);
            const auto zone=word(pair+0x32u),area=word(pair+0x34u);
            const auto scenario=read(0x4ecc20u);
            const auto zones=read(scenario+0x16cu);
            const auto areas=read(zones+std::uint32_t(zone)*0x38u+0x34u);
            const auto area_row=areas+scaled_word(area,0x88u);
            ++index;
            const auto length=std::bit_cast<std::int8_t>(std::uint8_t(read(squad+0x30u,1)));
            if(signed_word(index)>=length)index=none;
            if(matches(area_row,mode))return area!=none;
            if(index==none)return false;
        }
    }
    bool order_list(std::uint32_t squad,std::uint32_t order,bool mode)const {
        const auto length_at=order+(read(squad+0x60u,1)?0x5cu:0x54u);
        if(count(length_at)<=0)return false;
        std::uint16_t index=0;
        while(true) {
            const auto entries=read(order+(read(squad+0x60u,1)?0x60u:0x58u));
            const auto entry=entries+scaled_word(index,8u);
            ++index;
            if(signed_word(index)>=count(length_at))index=none;
            const auto kind=read(entry,1);
            if((5u&(1u<<(kind&31u)))!=0) {
                const auto zone=word(entry+4u);
                if(signed_word(zone)>=0) {
                    const auto scenario=read(0x4ecc20u);
                    if(signed_word(zone)<count(scenario+0x168u)) {
                        const auto zones=read(scenario+0x16cu);
                        const auto zone_row=zones+std::uint32_t(zone)*0x38u;
                        const auto area=word(entry+6u);
                        if(signed_word(area)>=0 && signed_word(area)<count(zone_row+0x30u)) {
                            const auto areas=read(zone_row+0x34u);
                            if(matches(areas+scaled_word(area,0x88u),mode))return true;
                        }
                    }
                }
            }
            if(index==none)return false;
        }
    }
    bool default_zone(std::uint16_t zone,bool mode)const {
        const auto scenario=read(0x4ecc20u);
        const auto zone_offset=std::uint32_t(zone)*0x38u;
        std::uint16_t index=0;
        while(true) {
            const auto zone_row=read(scenario+0x16cu)+zone_offset;
            if(signed_word(index)<0)return false;
            if(signed_word(index)>=count(zone_row+0x30u))return false;
            const auto areas=read(zone_row+0x34u);
            const auto area_row=areas+scaled_word(index,0x88u);
            ++index;
            const auto current_zones=read(scenario+0x16cu);
            if(signed_word(index)>=count(current_zones+zone_offset+0x30u))index=none;
            if(matches(area_row,mode))return true;
            if(index==none)return false;
        }
    }
    bool first_area(std::uint16_t squad,bool mode)const {
        if(squad==none)return false;
        const auto initial_pool=read(0x52be7cu);
        const auto initial_rows=read(initial_pool+0x44u);
        std::uint32_t order{};
        while(true) {
            const auto row=initial_rows+std::uint32_t(squad)*0x98u;
            const auto order_index=word(row+0x2au);
            if(order_index==none){order=0;break;}
            const auto scenario=read(0x4ecc20u);
            order=read(scenario+0x244u)+scaled_word(order_index,0x7cu);
            if(!order || !(read(order+0x24u,1)&0x20u))break;
            const auto inherited=word(order+0x4eu);
            if(inherited==none)break;
            squad=inherited;
        }
        std::uint16_t zone=none;
        if(!order) {
            const auto scenario=read(0x4ecc20u);
            const auto definitions=read(scenario+0x164u);
            const auto definition=definitions+std::uint32_t(squad)*0x74u;
            zone=word(definition+0x38u);
            if(word(definition+0x38u)==none)return false;
            const auto zones=read(scenario+0x16cu);
            if(count(zones+std::uint32_t(zone)*0x38u+0x30u)<=0)return false;
        }
        // The next owner does a fresh lookup after initialization. Its cached
        // explicit-pair path still depends on initialization enabling a cursor.
        const auto pool=read(0x52be7cu);
        const auto row=read(pool+0x44u)+std::uint32_t(squad)*0x98u;
        if(read(row+0x80u)!=0xffffffffu)return explicit_pairs(row,mode);
        return order?order_list(row,order,mode):default_zone(zone,mode);
    }
public:
    explicit AuthoredAreas(const SourcePhysicsContactMemory &memory):memory_(memory){}
    bool has_area(std::uint32_t actor)const {
        const auto pool=read(0x5020c8u);
        const auto row=read(pool+0x44u)+(actor&0xffffu)*0x888u;
        const bool mode=read(row+0x26cu)!=0xffffffffu;
        const auto squad=word(row+0x30u);
        return first_area(squad,mode);
    }
    std::uint16_t behavior(std::uint32_t actor)const {
        const auto pool=read(0x5020c8u);
        const auto row=read(pool+0x44u)+(actor&0xffffu)*0x888u;
        const auto enabled=read(row+0x225u,1);
        const auto fallback=read(0x47c1a4u);
        return enabled && has_area(actor)?0x6bu:std::uint16_t(fallback);
    }
};
}
bool native_campaign_ai_has_area_tu15(const SourcePhysicsContactMemory &memory,std::uint32_t actor){return AuthoredAreas(memory).has_area(actor);}
std::uint16_t native_campaign_ai_area_behavior_tu15(const SourcePhysicsContactMemory &memory,std::uint32_t actor){return AuthoredAreas(memory).behavior(actor);}
}
