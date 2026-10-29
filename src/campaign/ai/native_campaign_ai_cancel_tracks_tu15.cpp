#include "halo2/native_campaign_ai_cancel_tracks_tu15.hpp"
#include "halo2/native_tu15_data_array_iteration.hpp"
#include <array>
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
std::int16_t signed_word_at(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    return std::bit_cast<std::int16_t>(std::uint16_t(read(memory,address,2)));
}
}
bool native_campaign_ai_prop_has_other_active_track_tu15(const SourcePhysicsContactMemory &memory,
    std::uint32_t property,std::uint32_t excluded_actor) {
    const auto properties=read(memory,0x50eef4u),rows=read(memory,properties+0x44u);
    auto track=read(memory,rows+(property&0xffffu)*0xc4u+0x18u);
    const auto tracks=read(memory,0x50eef0u);
    while(track!=0xffffffffu) {
        const auto track_rows=read(memory,tracks+0x44u),row=track_rows+(track&0xffffu)*0x3cu;
        const auto next=read(memory,row+0x34u);const auto status=signed_word_at(memory,row+0x24u);
        if(status>=1&&status<=2&&read(memory,row+4u)!=excluded_actor)return true;
        track=next;
    }
    return false;
}
std::uint32_t native_campaign_ai_group_find_object_prop_tu15(const SourcePhysicsContactMemory &memory,
    std::uint32_t group,std::uint32_t object) {
    const auto groups=read(memory,0x50eef8u),rows=read(memory,groups+0x44u);
    auto property=read(memory,rows+(group&0xffffu)*0x50u+0x14u);
    if(property==0xffffffffu)return 0xffffffffu;
    const auto properties=read(memory,0x50eef4u),property_rows=read(memory,properties+0x44u);
    while(property!=0xffffffffu) {
        const auto row=property_rows+(property&0xffffu)*0xc4u;
        if(read(memory,row+8u)==object)return property;
        property=read(memory,row+0x14u);
    }
    return 0xffffffffu;
}
std::uint16_t native_campaign_ai_group_policy_class_tu15(const SourcePhysicsContactMemory &memory,std::int16_t value) {
    if(value==1)return 0;
    if(value==-1)return 1;
    const auto game=read(memory,0x4f3218u),mode=read(memory,game+8u);
    if(mode==1) {
        if(value<0||value>=16)return 1;
        const auto state=read(memory,0x5020c4u);
        return native_tu15_bit_array_test(memory,state+0xa4u,std::int32_t(value)*16+1)?0:1;
    }
    if(mode==2)throw std::runtime_error("Native campaign group policy requires the actual mode2 game-engine owner");
    return 1;
}
bool native_campaign_ai_prop_track_conflict_campaign_tu15(const SourcePhysicsContactMemory &memory,
    std::uint32_t property,std::uint32_t excluded_actor,std::uint32_t group) {
    if(native_campaign_ai_prop_has_other_active_track_tu15(memory,property,excluded_actor))return true;
    const auto initial_groups=read(memory,0x50eef8u),initial_rows=read(memory,initial_groups+0x44u);
    const auto category=native_campaign_ai_group_policy_class_tu15(memory,signed_word_at(memory,initial_rows+(group&0xffffu)*0x50u+0x12u));
    const auto properties=read(memory,0x50eef4u),property_rows=read(memory,properties+0x44u);
    const auto globals=read(memory,0x5020a8u),target=property_rows+(property&0xffffu)*0xc4u;
    const auto enabled=read(memory,globals+1u,1);
    if(!enabled) {
        const auto current_globals=read(memory,0x5020a8u);
        if(!read(memory,current_globals+1u,1))return false;
        throw std::runtime_error("Native campaign track scan requires an owned iterator for the inactive-to-active transition");
    }
    const auto groups=read(memory,0x50eef8u);std::uint32_t cursor=0xffffffffu;
    for(;;) {
        const auto current_globals=read(memory,0x5020a8u);
        if(!read(memory,current_globals+1u,1))return false;
        const auto index=native_tu15_data_array_next(memory,groups,std::bit_cast<std::int32_t>(cursor+1u));
        if(index==0xffffffffu){(void)read(memory,groups+0x20u);return false;}
        const auto stride=read(memory,groups+0x24u),rows=read(memory,groups+0x44u),row=rows+stride*index;
        const auto salt=read(memory,row,2);cursor=index;const auto candidate=(salt<<16u)|index;
        if(row==0)return false;
        if(candidate==group)continue;
        const auto candidate_category=native_campaign_ai_group_policy_class_tu15(memory,signed_word_at(memory,row+0x12u));
        if(candidate_category!=category)continue;
        const auto object=read(memory,target+8u);
        const auto matched=native_campaign_ai_group_find_object_prop_tu15(memory,candidate,object);
        if(matched!=0xffffffffu&&native_campaign_ai_prop_has_other_active_track_tu15(memory,matched,0xffffffffu))return true;
    }
}
}
