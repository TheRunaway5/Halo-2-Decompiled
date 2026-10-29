#pragma once
#include "audio/native_tu15_sound_campaign_audio.hpp"
#include "halo2/native_campaign_audio_retirement.hpp"
#include "halo2/native_tu15_data_array_delete.hpp"
namespace halo2::native_tu15_sound {
namespace campaign_retirement_adapter {
inline void publish_byte(CampaignContext& context,std::uint32_t address,std::uint8_t value,std::uint32_t site) {
    const std::array<std::byte,1> after{std::byte(value)};std::array<std::byte,1> before{};
    if(context.o)context.m.read(address,before);context.m.write(address,after);
    if(context.o)context.o({site,address,{before.begin(),before.end()},{after.begin(),after.end()}});
}
struct SourceRow {
    CampaignContext& context;std::uint32_t address;
    std::uint32_t controller()const{return u32(context.m,address+0xa8u);}
    std::uint8_t category()const{return u8(context.m,address+3u);}
};
struct ControllerRow {
    CampaignContext& context;std::uint32_t address;
    std::uint8_t references()const{return u8(context.m,address+2u);}
    std::uint8_t side_references()const{return u8(context.m,address+3u);}
    std::uint32_t owner()const{return u32(context.m,address+4u);}
    void references(std::uint8_t value){publish_byte(context,address+2u,value,0x2206bbu);}
    void side_references(std::uint8_t value){publish_byte(context,address+3u,value,0x129ee9u);}
};
template<class Row,unsigned Stride> struct Pool {
    CampaignContext& context;std::uint32_t owner;
    Row row(std::uint32_t datum)const{return {context,u32(context.m,owner+0x44u)+(datum&0xffffu)*Stride};}
    void erase(std::uint32_t datum)const{native_tu15_data_array_delete(context.m,owner,datum);}
};
struct State {
    CampaignContext& context;
    Pool<SourceRow,0xbc> sources()const{return {context,u32(context.m,0x4f2c4cu)};}
    Pool<ControllerRow,0x1c> controllers()const{return {context,u32(context.m,0x52d668u)};}
};
inline void forward_direction(const Abi& abi){if(abi.eflags&0x400u)throw FormatError("Native audio data-array retirement requires original DF=0");}
inline void source(CampaignContext& context,Abi& abi) {
    forward_direction(abi);State state{context};native_campaign_retire_source(state,abi.gp[7]);pop_return(context.m,abi);
}
inline void controller(CampaignContext& context,Abi& abi) {
    forward_direction(abi);State state{context};native_campaign_release_controller(state,abi.gp[6]);pop_return(context.m,abi);
}
inline void erase(CampaignContext& context,Abi& abi) {
    forward_direction(abi);native_tu15_data_array_delete(context.m,abi.gp[2],abi.gp[0]);pop_return(context.m,abi);
}
}
// Complete unchanged original source/controller chain independently proven;
// see docs/NATIVE_CAMPAIGN_AUDIO_RETIREMENT_20261028.md.
inline std::span<const CampaignRoutineEntry> campaign_retirement_routines() {
    static constexpr CampaignRoutineEntry entries[]{{0x129ea0u,campaign_retirement_adapter::source,true},
        {0x220690u,campaign_retirement_adapter::controller,true}};return entries;
}
} // namespace halo2::native_tu15_sound
