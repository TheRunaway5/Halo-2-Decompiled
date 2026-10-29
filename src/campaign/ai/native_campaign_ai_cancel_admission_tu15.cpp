#include "halo2/native_campaign_ai_cancel_admission_tu15.hpp"
#include "halo2/native_campaign_ai_cancel_tracks_tu15.hpp"
#include "halo2/native_campaign_ai_cancel_support_tu15.hpp"
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
std::int16_t signed_word(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    return std::bit_cast<std::int16_t>(std::uint16_t(read(memory,address,2)));
}
}
bool native_campaign_ai_cancel_property_admission_campaign_tu15(
    SourcePhysicsContactMemory &memory,std::uint32_t property,std::uint32_t actor,std::uint32_t group) {
    const auto properties=read(memory,0x50eef4u),property_rows=read(memory,properties+0x44u);
    const auto row=property_rows+(property&0xffffu)*0xc4u;
    if(read(memory,row+0x34u,1))return false;
    const auto object=read(memory,row+8u),objects=read(memory,0x4ecbd0u),object_rows=read(memory,objects+0x44u);
    const auto body=read(memory,object_rows+(object&0xffffu)*12u+8u);
    if(read(memory,body+0x10au,1)&4u)return false;
    if(group==0xffffffffu)return false;
    const auto groups=read(memory,0x50eef8u),group_rows=read(memory,groups+0x44u);
    const auto team=signed_word(memory,group_rows+(group&0xffffu)*0x50u+0x12u);
    bool scan_members=false;
    if(team!=-1) {
        const auto game=read(memory,0x4f3218u),mode=read(memory,game+8u);
        if(mode==1) {
            if(team>=0&&team<16) {
                const auto policy=read(memory,0x5020c4u);
                scan_members=native_tu15_bit_array_test(memory,policy+0xc4u,std::int32_t(team)*16+1);
            }
        }else if(mode==2)throw std::runtime_error("Native campaign cancellation requires the actual mode2 game-engine owner");
        else scan_members=team==1;
    }
    if(!scan_members)return !native_campaign_ai_prop_track_conflict_campaign_tu15(memory,property,actor,group);
    const auto frames=read(memory,0x4f54f4u),clock=read(memory,0x51df54u),tick=read(memory,clock+8u);
    NativeTu15DataArrayIterator iterator{0,frames,0xffffffffu,0xffffffffu};bool quiet=true;
    while(native_tu15_data_array_advance(memory,iterator)) {
        const auto timer=native_campaign_ai_actor_timer_tu15(memory,iterator.datum);
        if(timer) {
            const auto previous=read(memory,timer+0xcu);const auto rate=signed_word(memory,clock+2u);
            if(std::bit_cast<std::int32_t>(tick-previous)<std::int32_t(rate))quiet=false;
        }
    }
    if(!quiet)return false;
    auto member=native_campaign_ai_group_first_member_tu15(memory,group);
    const auto tracks=read(memory,0x50eef0u),actors=read(memory,0x5020c8u);
    while(member!=0xffffffffu) {
        const auto actor_rows=read(memory,actors+0x44u),actor_row=actor_rows+(member&0xffffu)*0x888u;
        const auto next=read(memory,actor_row+0x80u),track=read(memory,actor_row+0x338u);
        if(track!=0xffffffffu) {
            const auto track_rows=read(memory,tracks+0x44u),track_row=track_rows+(track&0xffffu)*0x3cu;
            const auto status=signed_word(memory,track_row+0x24u);
            if(status>=1&&status<=2)return false;
        }
        member=next;
    }
    return true;
}
}
