#pragma once
#include "audio/native_tu15_sound_campaign_audio.hpp"
#include "halo2/native_campaign_audio_stop_callback.hpp"
namespace halo2::native_tu15_sound {
namespace campaign_stop_callback_adapter {
struct Header {
    CampaignContext& context;std::uint32_t address;
    bool present()const{return address!=0;}
    std::int16_t salt()const{return std::bit_cast<std::int16_t>(std::uint16_t(u16(context.m,address)));}
    std::uint8_t type()const{return u8(context.m,address+3u);}
    std::uint32_t body()const{return u32(context.m,address+8u);}
};
struct Objects {
    CampaignContext& context;std::uint32_t owner;
    std::int32_t extent()const{return std::bit_cast<std::int32_t>(u32(context.m,owner+0x38u));}
    Header indexed(std::uint32_t index)const {
        const auto stride=u32(context.m,owner+0x24u);const auto rows=u32(context.m,owner+0x44u);return {context,rows+index*stride};
    }
    Header direct(std::uint32_t index)const{return {context,u32(context.m,owner+0x44u)+index*12u};}
};
struct Association {
    CampaignContext& context;std::uint32_t address;
    std::uint32_t sound()const{return u32(context.m,address);}
    void clear()const{store(context.m,context.o,0xd332au,address,0xffffffffu);}
};
struct Object {
    CampaignContext& context;std::uint32_t address;
    std::int16_t sound_offset()const{return std::bit_cast<std::int16_t>(std::uint16_t(u16(context.m,address+0x33eu)));}
    Association sound_association(std::int16_t offset)const{return {context,address+std::uint32_t(std::int32_t(offset))+0x38u};}
};
struct SourceRow {CampaignContext& context;std::uint32_t address;std::uint32_t tag()const{return u32(context.m,address+0xcu);}};
struct Sources {CampaignContext& context;std::uint32_t owner;SourceRow row(std::uint32_t datum)const{return {context,u32(context.m,owner+0x44u)+(datum&0xffffu)*0xbcu};}};
struct State {
    CampaignContext& context;
    bool game_active()const{return u8(context.m,u32(context.m,0x4f9b5cu)+0x29u)!=0;}
    Objects objects()const{return {context,u32(context.m,0x4ecbd0u)};}
    Object object(std::uint32_t address)const{return {context,address};}
    Sources sources()const{return {context,u32(context.m,0x4f2c4cu)};}
};
inline void stop(CampaignContext& context,Abi& abi) {
    State state{context};const auto stack=abi.gp[4];const auto object=u32(context.m,stack+4u),source=u32(context.m,stack+8u);
    native_campaign_sound_stop_notice(state,object,source);pop_return(context.m,abi,12u);
}
inline void object(CampaignContext& context,Abi& abi) {
    State state{context};abi.gp[0]=native_campaign_sound_object(state,abi.gp[0],u32(context.m,abi.gp[4]+4u));pop_return(context.m,abi,4u);
}
inline void clear(CampaignContext& context,Abi& abi) {
    State state{context};native_campaign_clear_object_sound(state,abi.gp[0],u32(context.m,abi.gp[4]+4u));pop_return(context.m,abi,4u);
}
}
inline std::span<const CampaignRoutineEntry> campaign_stop_callback_routines() {
    static constexpr CampaignRoutineEntry entries[]{{0x18f6b0u,campaign_stop_callback_adapter::stop,true},
        {0xbcdf0u,campaign_stop_callback_adapter::object,true},{0xd3300u,campaign_stop_callback_adapter::clear,true}};return entries;
}
// Independently closed actual descriptor callback. Generic object helpers
// remain internal candidates; the forty-entry frame gate is unchanged.
inline std::span<const Kernel> campaign_stop_callback_kernels() {
    static constexpr Kernel kernels[]{{0x18f6b0u,
        +[](M& memory,Abi& abi,FP fp,const O& observer,const Services& services){
            CampaignContext context{memory,fp,observer,services};campaign_stop_callback_adapter::stop(context,abi);
        },{0xf8u,0u,0xffu}}};return kernels;
}
} // namespace halo2::native_tu15_sound
