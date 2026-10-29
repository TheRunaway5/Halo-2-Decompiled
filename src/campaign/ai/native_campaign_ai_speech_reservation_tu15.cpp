#include "halo2/native_campaign_ai_speech_reservation_tu15.hpp"
#include <bit>
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t p,unsigned size=4) {
    std::array<std::byte,4> bytes{};m.read(p,std::span(bytes).first(size));std::uint32_t value{};
    for(unsigned i=0;i<size;++i)value|=std::to_integer<unsigned>(bytes[i])<<(i*8u);return value;
}
void write(SourcePhysicsContactMemory &m,std::uint32_t p,std::uint32_t value) {
    const std::array<std::byte,4> bytes{std::byte(value),std::byte(value>>8u),std::byte(value>>16u),std::byte(value>>24u)};m.write(p,bytes);
}
std::int32_t signed32(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
std::int32_t word(const SourcePhysicsContactMemory &m,std::uint32_t p) {
    return std::bit_cast<std::int16_t>(std::uint16_t(read(m,p,2)));
}
std::uint32_t object(const SourcePhysicsContactMemory &m,std::uint32_t unit) {
    const auto pool=read(m,0x4ecbd0u),rows=read(m,pool+0x44u);return read(m,rows+(unit&0xffffu)*12u+8u);
}
std::uint32_t speech_state(const SourcePhysicsContactMemory &m,std::uint32_t object) {
    return object+std::uint32_t(word(m,object+0x342u));
}
std::uint32_t bit_word(std::uint32_t base,std::uint32_t index) {
    return base+std::uint32_t(signed32(index)>>5)*4u;
}
struct RawField {
    SourcePhysicsContactMemory &memory;std::uint32_t address;
    bool present()const{return address!=0;}
    std::uint32_t load()const{return read(memory,address);}
    void store(std::uint32_t value){write(memory,address,value);}
};
struct LocalField {
    std::uint32_t *value;
    bool present()const{return value!=nullptr;}
    std::uint32_t load()const{return *value;}
    void store(std::uint32_t next){*value=next;}
};
template<class Field> bool source(SourcePhysicsContactMemory &m,std::uint32_t unit,std::int16_t action,Field &output) {
    const auto body=object(m,unit),state=speech_state(m,body),tag=read(m,state);
    auto sound=none;bool eligible=false;
    if(tag!=none) {
        const auto table=read(m,0x4f0414u),definition=read(m,table+(tag&0xffffu)*16u+8u);
        if(action>=0&&std::int32_t(action)<signed32(read(m,definition+0xcu))) {
            const auto actions=read(m,definition+0x10u);sound=read(m,actions+std::uint32_t(action)*16u+0xcu);
            if(sound!=none)eligible=!native_campaign_ai_sound_permutation_blocked_tu15(m,sound);
        }
    }
    if(output.load()!=0)output.store(sound);return eligible;
}
template<class Field> bool reserve(SourcePhysicsContactMemory &m,std::uint32_t unit,std::int16_t action,
    std::int16_t priority,Field &prior,Field &output) {
    const auto body=object(m,unit),state=speech_state(m,body);
    auto sound=none;bool admitted=false;
    if(output.present())sound=output.load();
    bool resolved=true;
    if(sound==none&&action!=-1) {
        LocalField local{&sound};(void)source(m,unit,action,local);resolved=sound!=none;
    }
    if(resolved) {
        const auto flags=read(m,body+0x10au,1);
        if(!(flags&4u)||priority==15) {
            const auto current=word(m,state+0xcu);
            if(current==0)admitted=true;
            else if(priority==13||priority==14)admitted=std::int32_t(priority)>=current;
            else if(priority==12&&current>=10&&read(m,state+0x49u,1)==0)admitted=false;
            else admitted=std::int32_t(priority)>current;
        }
    }
    if(output.present())output.store(sound);
    if(prior.present())prior.store(read(m,state+0x44u));return admitted;
}
}
std::uint32_t native_campaign_ai_sound_history_width_tu15(std::uint32_t count) {
    std::uint32_t width=0;
    if(count>1u) {
        --count;
        if(count!=1u)do{count>>=1u;++width;}while(count!=1u);
    }
    return width+1u;
}
std::uint32_t native_campaign_ai_sound_history_prefix_tu15(const SourcePhysicsContactMemory &m,
    std::uint32_t range,std::uint32_t definition) {
    const auto globals=read(m,0x4ecc1cu),count=read(m,globals+0xc0u);
    const auto reference=count?read(m,globals+0xc4u):0u;
    const auto tag=read(m,reference+4u),table=read(m,0x4f0414u);
    const auto classes_definition=read(m,table+(tag&0xffffu)*16u+8u);
    const auto class_index=std::bit_cast<std::int8_t>(std::uint8_t(read(m,definition+2u,1)));
    const auto classes=read(m,classes_definition+4u);
    if(!(read(m,classes+std::uint32_t(class_index)*0x5cu+0xau,1)&0x20u))return none;
    const auto prefix=read(m,range+6u,2);return prefix==0xffffu?none:prefix;
}
bool native_campaign_ai_sound_permutation_blocked_tu15(const SourcePhysicsContactMemory &m,std::uint32_t sound) {
    if(sound==none)return false;
    const auto table=read(m,0x4f0414u),definition=read(m,table+(sound&0xffffu)*16u+8u);
    if(read(m,definition+0xau,1)!=1u)return false;
    const auto range_index=word(m,definition+8u);const auto gestalt=read(m,0x52d664u),ranges=read(m,gestalt+0x24u);
    const auto range=ranges+std::uint32_t(range_index)*12u;
    const auto prefix=native_campaign_ai_sound_history_prefix_tu15(m,range,definition);
    if(prefix==none)return false;
    const auto count=word(m,range+0xau);
    const auto width=std::bit_cast<std::int16_t>(std::uint16_t(native_campaign_ai_sound_history_width_tu15(std::uint32_t(count))));
    const auto flags=read(m,0x52be90u);auto index=std::uint32_t(width)+std::uint32_t(count)+prefix;
    for(unsigned offset=0;offset<5;++offset,++index)
        if(read(m,bit_word(flags,index))&(1u<<(index&31u)))return true;
    return false;
}
bool native_campaign_ai_speech_source_tu15(SourcePhysicsContactMemory &m,std::uint32_t unit,
    std::int16_t action,std::uint32_t address) {RawField field{m,address};return source(m,unit,action,field);}
bool native_campaign_ai_speech_source_tu15(SourcePhysicsContactMemory &m,std::uint32_t unit,
    std::int16_t action,NativeCampaignAiSpeechResolvedSound &value) {LocalField field{&value.value};return source(m,unit,action,field);}
bool native_campaign_ai_speech_reserve_tu15(SourcePhysicsContactMemory &m,std::uint32_t unit,
    std::int16_t action,std::int16_t priority,std::uint32_t prior_address,std::uint32_t output_address) {
    RawField prior{m,prior_address},output{m,output_address};return reserve(m,unit,action,priority,prior,output);
}
bool native_campaign_ai_speech_reserve_tu15(SourcePhysicsContactMemory &m,std::uint32_t unit,
    std::int16_t action,std::int16_t priority,NativeCampaignAiSpeechResolvedSound &value,std::uint32_t *prior_value) {
    LocalField prior{prior_value},output{&value.value};return reserve(m,unit,action,priority,prior,output);
}
}
