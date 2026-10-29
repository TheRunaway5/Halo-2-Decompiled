#pragma once
#include "audio/native_tu15_sound_campaign_audio.hpp"
#include "halo2/native_campaign_audio_ducking.hpp"

namespace halo2::native_tu15_sound {
namespace campaign_ducking_adapter {
struct Memory final:NativeCampaignDuckingState {
    CampaignContext& context;
    mutable std::optional<std::uint32_t> audio,game;
    explicit Memory(CampaignContext& c):context(c){}
    std::uint32_t audio_base()const{if(!audio)audio=u32(context.m,0x4f2c50u);return *audio;}
    std::uint32_t read(unsigned offset)const{return u32(context.m,audio_base()+offset);}
    void write(unsigned offset,std::uint32_t value,std::uint32_t site){store(context.m,context.o,site,audio_base()+offset,value);}
    std::uint32_t current_milliseconds()const override{return u32(context.m,u32(context.m,0x41b774u));}
    std::uint32_t last_milliseconds()const override{return read(0x80);}
    std::uint32_t elapsed_milliseconds()const override{return read(0x84);}
    std::uint32_t accumulated_seconds()const override{return read(0x1fc);}
    std::uint32_t effect_seconds()const override{return read(0x204);}
    std::uint32_t frame_seconds()const override{return read(0x1e0);}
    bool game_present()const override{game=u32(context.m,0x4f3218u);return *game!=0;}
    bool game_active()const override{return u8(context.m,game.value()+0x1120u)!=0;}
    bool campaign()const override{return u32(context.m,game.value()+8u)==1u;}
    bool paused()const override{return u8(context.m,game.value()+0x1121u)!=0;}
    std::uint32_t gain()const override{return read(0x1e4);}
    std::uint32_t delay_milliseconds()const override{return read(0x1e8);}
    std::uint32_t delay_seconds()const override{return read(0x1ec);}
    std::uint32_t fade_out_seconds()const override{return read(0x1f0);}
    std::uint32_t fade_in_seconds()const override{return read(0x1f4);}
    std::uint8_t activity_gate(unsigned index)const override {
        constexpr std::uint32_t gates[]{0x4f9c6du,0x556e03u,0x556dfeu,0x556dffu,0x556e02u,0x556e00u,0x556e01u,0x556e06u};
        return u8(context.m,gates[index]);
    }
    std::int32_t activity_counter(unsigned index)const override{return std::bit_cast<std::int32_t>(u32(context.m,index?0x4f2d70u:0x4f2d40u));}
    void last_milliseconds(std::uint32_t value)override{write(0x80,value,0x129527u);}
    void elapsed_milliseconds(std::uint32_t value)override{write(0x84,value,0x129544u);}
    void accumulated_seconds(std::uint32_t value)override{write(0x1fc,value,0x129551u);}
    void effect_seconds(std::uint32_t value)override{write(0x204,value,0x129571u);}
    void frame_seconds(std::uint32_t value)override{write(0x1e0,value,0x129569u);}
    void delay_milliseconds(std::uint32_t value,bool initial)override {
        write(0x1e8,value,initial?0x1295c2u:(value==0xffffffffu?0x1296b3u:0x1295deu));
    }
    void gain(std::uint32_t value,NativeCampaignDuckingGain phase)override {
        constexpr std::uint32_t sites[]{0x12962au,0x129645u,0x1296abu,0x1296c7u};write(0x1e4,value,sites[unsigned(phase)]);
    }
};
inline void run(CampaignContext& c,Abi& abi) {
    const auto caller=abi.floating_point.value_or(c.fp);
    NativeCampaignAudioFp fp{caller.x87_control_word,abi.x87_status,caller.mxcsr};
    Memory memory(c);native_campaign_update_ducking(memory,fp);
    abi.floating_point=FP{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;pop_return(c.m,abi);
}
}
inline std::span<const CampaignRoutineEntry> campaign_ducking_routines() {
    static constexpr CampaignRoutineEntry routines[]{{0x129500u,campaign_ducking_adapter::run,true}};return routines;
}
} // namespace halo2::native_tu15_sound
