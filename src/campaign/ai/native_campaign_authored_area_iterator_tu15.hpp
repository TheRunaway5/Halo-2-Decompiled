#pragma once
#include "halo2/physics_contact_record.hpp"
#include <array>
#include <bit>

namespace halo2::detail {
// Semantic private cursor for the actual flags5 area-query profile. It keeps
// the initialized order/squad identity, while every next() resolves the fresh
// retained squad, selected list and scenario operands required by20D2D0.
class CampaignAuthoredAreaIteratorTu15 {
    static constexpr std::uint16_t none=0xffffu;
    const SourcePhysicsContactMemory &memory_;
    std::uint16_t squad_,zone_=none,index_=none;
    std::uint32_t order_{};
    bool mode_;

    std::uint32_t read(std::uint32_t address,unsigned width=4)const {
        std::array<std::byte,4> bytes{};
        memory_.read(address,std::span(bytes).first(width));
        std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
        return value;
    }
    std::uint16_t word(std::uint32_t address)const{return std::uint16_t(read(address,2));}
    static std::int32_t signed_word(std::uint16_t value){return std::bit_cast<std::int16_t>(value);}
    static std::uint32_t scaled_word(std::uint16_t value,unsigned stride){return std::uint32_t(signed_word(value))*stride;}
    std::int32_t count(std::uint32_t address)const{return std::bit_cast<std::int32_t>(read(address));}
    bool matches(std::uint32_t area)const{return (read(area+0x20u)&1u)==unsigned(mode_);}

    std::uint16_t explicit_pairs(std::uint32_t squad) {
        if(std::bit_cast<std::int8_t>(std::uint8_t(read(squad+0x30u,1)))<0)return none;
        while(true) {
            const auto pair=squad+scaled_word(index_,4u);
            const auto zone=word(pair+0x32u),area=word(pair+0x34u);
            const auto scenario=read(0x4ecc20u);
            const auto zones=read(scenario+0x16cu);
            const auto areas=read(zones+std::uint32_t(zone)*0x38u+0x34u);
            const auto area_row=areas+scaled_word(area,0x88u);
            ++index_;
            const auto length=std::bit_cast<std::int8_t>(std::uint8_t(read(squad+0x30u,1)));
            if(signed_word(index_)>=length)index_=none;
            if(matches(area_row)){zone_=zone;return area;}
            if(index_==none)return none;
        }
    }
    std::uint16_t order_list(std::uint32_t squad) {
        const auto length_at=order_+(read(squad+0x60u,1)?0x5cu:0x54u);
        if(count(length_at)<=0)return none;
        while(true) {
            const auto entries=read(order_+(read(squad+0x60u,1)?0x60u:0x58u));
            const auto entry=entries+scaled_word(index_,8u);
            ++index_;
            if(signed_word(index_)>=count(length_at))index_=none;
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
                            if(matches(areas+scaled_word(area,0x88u))){zone_=zone;return area;}
                        }
                    }
                }
            }
            if(index_==none)return none;
        }
    }
    std::uint16_t default_zone() {
        const auto scenario=read(0x4ecc20u);
        const auto zone_offset=std::uint32_t(zone_)*0x38u;
        while(true) {
            const auto zone_row=read(scenario+0x16cu)+zone_offset;
            if(signed_word(index_)<0)return none;
            if(signed_word(index_)>=count(zone_row+0x30u)){index_=none;return none;}
            const auto areas=read(zone_row+0x34u);
            const auto area=index_;
            const auto area_row=areas+scaled_word(area,0x88u);
            ++index_;
            const auto current_zones=read(scenario+0x16cu);
            if(signed_word(index_)>=count(current_zones+zone_offset+0x30u))index_=none;
            if(matches(area_row))return area;
            if(index_==none)return none;
        }
    }
public:
    CampaignAuthoredAreaIteratorTu15(const SourcePhysicsContactMemory &memory,
        std::uint16_t squad,bool mode):memory_(memory),squad_(squad),mode_(mode) {
        if(squad_==none)return;
        const auto initial_pool=read(0x52be7cu);
        const auto initial_rows=read(initial_pool+0x44u);
        while(true) {
            const auto row=initial_rows+std::uint32_t(squad_)*0x98u;
            const auto order_index=word(row+0x2au);
            if(order_index==none)break;
            const auto scenario=read(0x4ecc20u);
            order_=read(scenario+0x244u)+scaled_word(order_index,0x7cu);
            if(!order_ || !(read(order_+0x24u,1)&0x20u))break;
            const auto inherited=word(order_+0x4eu);
            if(inherited==none)break;
            squad_=inherited;
            order_=0;
        }
        index_=0;
        if(!order_) {
            const auto scenario=read(0x4ecc20u);
            const auto definitions=read(scenario+0x164u);
            const auto definition=definitions+std::uint32_t(squad_)*0x74u;
            zone_=word(definition+0x38u);
            if(word(definition+0x38u)==none){index_=none;return;}
            const auto zones=read(scenario+0x16cu);
            if(count(zones+std::uint32_t(zone_)*0x38u+0x30u)<=0)index_=none;
        }
    }
    std::uint16_t next() {
        if(index_==none)return none;
        const auto pool=read(0x52be7cu);
        const auto row=read(pool+0x44u)+std::uint32_t(squad_)*0x98u;
        if(read(row+0x80u)!=0xffffffffu)return explicit_pairs(row);
        return order_?order_list(row):default_zone();
    }
    std::uint16_t zone()const noexcept{return zone_;}
};
}
