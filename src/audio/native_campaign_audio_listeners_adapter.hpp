#pragma once
#include "audio/native_tu15_sound_campaign_audio.hpp"
#include "halo2/native_campaign_audio_listeners.hpp"
#include <algorithm>
namespace halo2::native_tu15_sound::campaign_listeners_adapter {
struct ClusterQueryObservation {
    std::uint32_t count{},scale_count{},table_base{},cell{};
    bool byte_read{};
};
struct Source {
    CampaignContext& context;std::uint32_t address;
    std::uint8_t mode()const{return u8(context.m,address+3u)&15u;}
    bool ignore_clusters()const{return (u8(context.m,address)&1u)!=0;}
    bool column()const{return u8(context.m,address+2u)!=0;}
    std::uint16_t flags()const{return std::uint16_t(u16(context.m,address));}
    std::uint32_t x()const{return u32(context.m,address+0xcu);}std::uint32_t y()const{return u32(context.m,address+0x10u);}
    std::uint32_t z()const{return u32(context.m,address+0x14u);}std::uint32_t height()const{return u32(context.m,address+0x30u);}
    std::int16_t cluster()const{return std::bit_cast<std::int16_t>(std::uint16_t(u16(context.m,address+0x2cu)));}
    std::uint32_t location()const{return u32(context.m,address+0x28u);}
};
struct ListenerRow {
    CampaignContext& context;std::uint32_t address;
    std::int16_t cluster()const{return std::bit_cast<std::int16_t>(std::uint16_t(u16(context.m,address+4u)));}
    std::uint32_t location()const{return u32(context.m,address);}
    std::uint32_t x()const{return u32(context.m,address+0x30u);}std::uint32_t y()const{return u32(context.m,address+0x34u);}
    std::uint32_t z()const{return u32(context.m,address+0x38u);}
};
struct Listeners {
    CampaignContext& context;std::uint32_t address;
    ListenerRow row(unsigned index)const{return {context,address+0x88u+index*0x48u};}
    bool active(unsigned index)const{return u8(context.m,address+0x8eu+index*0x48u)!=0;}
};
struct Players {CampaignContext& context;std::uint32_t address;bool present(unsigned index)const{return u32(context.m,address+0xcu+index*4u)!=0xffffffffu;}};
struct Bytes {
    CampaignContext& context;std::uint32_t address;ClusterQueryObservation* observation{};
    std::uint8_t at(std::uint32_t index)const {
        const auto value=u8(context.m,address+index);
        if(observation){observation->cell=value;observation->byte_read=true;}return value;
    }
};
struct Scale {
    CampaignContext& context;std::uint32_t address;ClusterQueryObservation* observation{};
    Bytes bytes()const {
        const auto table=u32(context.m,address+0x28u);if(observation)observation->table_base=table;
        return {context,table,observation};
    }
    std::uint32_t lower()const{return u32(context.m,address+4u);}std::uint32_t upper()const{return u32(context.m,address+8u);}
};
struct Structure {
    CampaignContext& context;std::uint32_t address;ClusterQueryObservation* observation{};
    std::uint32_t cluster_count()const {
        const auto value=u32(context.m,address+0x9cu);if(observation)observation->count=value;return value;
    }
    std::int32_t scale_count()const {
        const auto value=u32(context.m,address+0x224u);if(observation)observation->scale_count=value;
        return std::bit_cast<std::int32_t>(value);
    }
    Bytes cluster_bytes()const {
        const auto table=u32(context.m,address+0xe8u);if(observation)observation->table_base=table;
        return {context,table,observation};
    }
    Scale scale_table()const{return {context,u32(context.m,address+0x228u),observation};}
};
struct State {
    CampaignContext& context;unsigned extended_occupied;
    void require_extended_capacity(unsigned needed)const {
        if(extended_occupied+needed>8u)throw FormatError("Required original solver x87 stack-fault owner unavailable");
    }
    Listeners listeners()const{return {context,u32(context.m,0x4f2c50u)};}
    Players players()const{return {context,u32(context.m,0x4f54f0u)};}
    Structure structure()const{return {context,u32(context.m,0x4ecc18u)};}
};
inline NativeCampaignAudioFp floating_point(const CampaignContext& context,const Abi& abi) {
    const auto fp=abi.floating_point.value_or(context.fp);return {fp.x87_control_word,abi.x87_status,fp.mxcsr};
}
inline void publish(Abi& abi,const NativeCampaignAudioFp& fp){abi.floating_point=FP{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}
inline void select(CampaignContext& context,Abi& abi) {
    const auto stack=abi.gp[4];const auto source=u32(context.m,stack+4u),limit=u32(context.m,stack+8u);
    State state{context,abi.x87_depth};auto fp=floating_point(context,abi);const auto result=native_campaign_select_listener(state,Source{context,source},limit,fp);
    const auto output=u32(context.m,stack+12u);if(output)store(context.m,context.o,0x12a920u,output,result.distance);
    abi.gp[0]=result.listener;abi.xmm[0]={result.distance,0u,0u,0u};publish(abi,fp);pop_return(context.m,abi,12u);
}
inline void distance(CampaignContext& context,Abi& abi) {
    State state{context,abi.x87_depth};auto fp=floating_point(context,abi);
    const auto result=native_campaign_listener_distance(state,Source{context,abi.gp[1]},abi.gp[2],fp);
    abi.xmm[0]={result,0u,0u,0u};publish(abi,fp);pop_return(context.m,abi);
}
inline void cut_off(CampaignContext& context,Abi& abi) {
    State state{context,abi.x87_depth};auto fp=floating_point(context,abi);
    const auto result=native_campaign_listener_cut_off(state,Source{context,abi.gp[2]},abi.gp[1],u32(context.m,abi.gp[4]+4u),fp);
    abi.gp[0]=(abi.gp[0]&0xffffff00u)|std::uint32_t(result);publish(abi,fp);pop_return(context.m,abi,4u);
}
inline void scale(CampaignContext& context,Abi& abi) {
    auto fp=floating_point(context,abi);
    const auto first=std::bit_cast<std::int32_t>(abi.gp[0]),second=std::bit_cast<std::int32_t>(abi.gp[1]);
    ClusterQueryObservation observation;
    NativeCampaignClusterScaleSimd simd;
    const auto result=native_campaign_cluster_scale(Structure{context,abi.gp[2],&observation},first,second,fp,&simd);
    abi.xmm[1]={simd.lower,0u,0u,0u};
    if(simd.span_written)abi.xmm[2]={simd.span,0u,0u,0u};
    // The register-level object-loop parents retain these volatile outputs.
    // Reuse values from the actual evaluation; never reread a mutable table.
    if(first!=second) {
        const bool scaled=std::bit_cast<std::int32_t>(observation.scale_count)>0;
        abi.gp[2]=observation.scale_count;
        if(observation.byte_read) {
            const auto a=scaled?first:std::int32_t(std::int16_t(first));
            const auto b=scaled?second:std::int32_t(std::int16_t(second));
            const auto low=std::uint32_t(std::min(a,b)),high=std::uint32_t(std::max(a,b));
            const auto triangle=std::uint32_t(std::bit_cast<std::int32_t>((low+1u)*low)/2);
            abi.gp[2]=observation.table_base+(observation.count-1u)*low-triangle;
            abi.gp[0]=((scaled?triangle:high)&0xffffff00u)|observation.cell;
            if(scaled && observation.cell!=0 && observation.cell!=255)abi.gp[0]=observation.cell-1u;
            abi.gp[1]=scaled?high:observation.cell&0x7fu;
        }else {
            abi.gp[0]&=0xffffff00u;abi.gp[1]=0u;
        }
    }
    abi.xmm[0]={result,0u,0u,0u};publish(abi,fp);pop_return(context.m,abi);
}
inline void byte(CampaignContext& context,Abi& abi) {
    const auto result=native_campaign_cluster_byte(Structure{context,abi.gp[7]},std::bit_cast<std::int16_t>(std::uint16_t(abi.gp[0])),std::bit_cast<std::int16_t>(std::uint16_t(abi.gp[1])));
    abi.gp[0]=(abi.gp[0]&0xffffff00u)|result;pop_return(context.m,abi);
}
} // namespace halo2::native_tu15_sound::campaign_listeners_adapter
