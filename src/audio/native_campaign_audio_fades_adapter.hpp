#pragma once
#include "audio/native_tu15_sound_campaign_audio.hpp"
#include "halo2/native_campaign_audio_fades.hpp"
namespace halo2::native_tu15_sound {
namespace campaign_fades_adapter {
struct SoundRow {
    CampaignContext& context;std::uint32_t address;
    std::uint8_t flags()const{return u8(context.m,address+4u);}
    std::uint32_t start()const{return u32(context.m,address+0xb4u);}
    std::uint32_t end()const{return u32(context.m,address+0xb8u);}
    std::uint32_t gain()const{return u32(context.m,address+0xb0u);}
    std::int16_t curve()const{return std::bit_cast<std::int16_t>(std::uint16_t(u16(context.m,address+0xaeu)));}
    void gain(std::uint32_t value,bool target,bool active){store(context.m,context.o,target?0x12999cu:(active?0x129930u:0x129938u),address+0xb0u,value);}
    void curve(std::int16_t value,bool target){store16(context.m,context.o,target?0x1299a6u:0x129954u,address+0xaeu,std::bit_cast<std::uint16_t>(value));}
    void start(std::uint32_t value){store(context.m,context.o,0x12d374u,address+0xb4u,value);}
    void end(std::uint32_t value){store(context.m,context.o,0x12d389u,address+0xb8u,value);}
    void start(std::uint32_t value,bool target){store(context.m,context.o,target?0x1299adu:0x12995bu,address+0xb4u,value);}
    void end(std::uint32_t value,bool target){store(context.m,context.o,target?0x1299b3u:0x129965u,address+0xb8u,value);}
    void activate(bool target){store8(context.m,context.o,target?0x1299bdu:0x12996bu,address+4u,std::uint8_t(flags()|0x80u));}
};
struct SoundTable {
    CampaignContext& context;std::uint32_t owner;
    SoundRow row(std::uint32_t datum)const{return {context,u32(context.m,owner+0x44u)+(datum&0xffffu)*0xbcu};}
};
struct Clock {CampaignContext& context;std::uint32_t owner;std::uint32_t milliseconds()const{return u32(context.m,owner+0x84u);}};
struct State {
    CampaignContext& context;
    SoundTable sounds()const{return {context,u32(context.m,0x4f2c4cu)};}
    Clock clock()const{return {context,u32(context.m,0x4f2c50u)};}
};
inline NativeCampaignAudioFp transport_fp(const CampaignContext& context,const Abi& abi) {
    const auto fp=abi.floating_point.value_or(context.fp);return {fp.x87_control_word,abi.x87_status,fp.mxcsr};
}
inline void publish_fp(Abi& abi,const NativeCampaignAudioFp& fp){abi.floating_point=FP{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}
inline void request(CampaignContext& context,Abi& abi) {
    auto fp=transport_fp(context,abi);State state{context};
    native_campaign_request_fade(state,abi.gp[0],abi.gp[7],std::bit_cast<std::int16_t>(std::uint16_t(abi.gp[3])),u32(context.m,abi.gp[4]+4u),fp);
    publish_fp(abi,fp);pop_return(context.m,abi,4u);
}
inline void sample(CampaignContext& context,Abi& abi) {
    auto fp=transport_fp(context,abi);State state{context};abi.gp[0]=native_campaign_sample_fade(state,abi.gp[0],fp);
    publish_fp(abi,fp);pop_return(context.m,abi);
}
inline void fraction(CampaignContext& context,Abi& abi) {
    auto fp=transport_fp(context,abi);const auto stack=abi.gp[4];
    const auto first=u32(context.m,stack+4u),second=u32(context.m,stack+8u),current=u32(context.m,stack+12u);
    const auto result=native_campaign_fade_fraction(first,second,current,std::uint8_t(abi.gp[1]),fp);
    abi.xmm[0]={result,0u,0u,0u};publish_fp(abi,fp);pop_return(context.m,abi,12u);
}
inline void decibels(CampaignContext& context,Abi& abi) {
    auto fp=transport_fp(context,abi);abi.gp[0]=native_campaign_fade_decibels(abi.xmm[0][0],fp);
    publish_fp(abi,fp);pop_return(context.m,abi);
}
}
// Closed fade-request/sampling owners and their shared numerical fraction.
// Qualification is bounded by docs/NATIVE_CAMPAIGN_AUDIO_FADES_20261028.md.
inline std::span<const CampaignRoutineEntry> campaign_fade_routines() {
    static constexpr CampaignRoutineEntry routines[]{{0x129900u,campaign_fades_adapter::request,true},
        {0x12d320u,campaign_fades_adapter::sample,true},{0x12db00u,campaign_fades_adapter::fraction,true}};return routines;
}
} // namespace halo2::native_tu15_sound
