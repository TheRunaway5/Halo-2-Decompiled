#include "halo2/native_campaign_transition_save_metadata_tu15.hpp"
#include <optional>

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t at,unsigned width=4){
    std::array<std::byte,4> raw{};memory.read(at,std::span(raw).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(raw[n])<<(8u*n);return value;
}
void write(SourcePhysicsContactMemory &memory,std::uint32_t at,std::uint32_t value,unsigned width=4){
    std::array<std::byte,4> raw{};for(unsigned n=0;n<width;++n)raw[n]=std::byte(value>>(8u*n));memory.write(at,std::span(raw).first(width));
}
struct RetainedInput {
    const SourcePhysicsContactMemory &memory;std::uint32_t address;
    bool present(std::uint32_t offset=0)const{return address+offset!=0;}
    std::uint32_t get(std::uint32_t offset,unsigned width=4)const{return read(memory,address+offset,width);}
};
template<std::size_t N>struct ValueInput {
    const std::array<std::byte,N> &bytes;bool available=true;
    bool present(std::uint32_t=0)const{return available;}
    std::uint32_t get(std::uint32_t offset,unsigned width=4)const{
        std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[offset+n])<<(8u*n);return value;
    }
};
struct RetainedOutput {
    SourcePhysicsContactMemory &memory;std::uint32_t address;
    void set(std::uint32_t offset,std::uint32_t value,unsigned width=4){write(memory,address+offset,value,width);}
};
template<std::size_t N>struct ValueOutput {
    std::array<std::byte,N> &bytes;
    void set(std::uint32_t offset,std::uint32_t value,unsigned width=4){for(unsigned n=0;n<width;++n)bytes[offset+n]=std::byte(value>>(8u*n));}
};
template<class Output>void zero15(Output &output,std::uint32_t at){
    for(std::uint32_t n=0;n<12u;n+=4u)output.set(at+n,0);output.set(at+12u,0,2);output.set(at+14u,0,1);
}
template<class Output>void zero16(Output &output,std::uint32_t at){for(std::uint32_t n=0;n<16u;n+=4u)output.set(at+n,0);}
template<class Input,class Output>void subrecord15(const Input &input,std::uint32_t source,Output &output,std::uint32_t at){
    if(!input.present(source))zero15(output,at);
    else{
        output.set(at+4u,input.get(source));
        output.set(at+8u,input.get(source+4u,1),1);
        output.set(at+9u,input.get(source+5u,1),1);
        output.set(at+10u,input.get(source+6u,1),1);
    }
    output.set(at,0x62636f6eu);output.set(at+11u,0x65636f6eu);
}
template<class Input,class Output>void subrecord16(const Input &input,std::uint32_t source,Output &output,std::uint32_t at){
    if(!input.present(source))zero16(output,at);
    else{
        output.set(at+4u,input.get(source));
        output.set(at+8u,input.get(source+4u,1),1);
        output.set(at+9u,input.get(source+5u,1),1);
        output.set(at+10u,input.get(source+6u,1),1);
        output.set(at+11u,input.get(source+7u,1),1);
    }
    output.set(at,0x62706170u);output.set(at+12u,0x65706170u);
}
template<class Input,class Output>void profile(const Input &input,Output &output,std::uint32_t at){
    if(!input.present()){
        for(std::uint32_t n=0;n<0x84u;n+=4u)output.set(at+n,0);output.set(at+0x84u,0,2);output.set(at+0x86u,0,1);
        subrecord15(input,0,output,at+0x64u);subrecord16(input,0,output,at+0x73u);
    }else{
        for(std::uint32_t n=0;n<0x40u;n+=4u)output.set(at+4u+n,input.get(8u+n));
        for(std::uint32_t n=0;n<0x10u;n+=4u)output.set(at+0x44u+n,input.get(0x48u+n));
        for(std::uint32_t n=0;n<0x10u;n+=4u)output.set(at+0x54u+n,input.get(0xecu+n));
        subrecord15(input,0xfcu,output,at+0x64u);subrecord16(input,0x118u,output,at+0x73u);
    }
    output.set(at,0x62707072u);output.set(at+0x83u,0x65707072u);
}
template<class Options,class Output>void construct(SourcePhysicsContactMemory &memory,const Options &options,Output output){
    for(std::uint32_t n=0;n<0x338u;n+=4u)output.set(n,0xcdcdcdcdu);output.set(0x338u,0xcdcdu,2);output.set(0x33au,0xcdu,1);
    output.set(0,0x62636c63u);output.set(4,1);output.set(8,0x2651u);
    output.set(0xcu,options.get(0x14u));output.set(0x10u,options.get(0x18u));
    for(std::uint32_t n=0;n<0x104u;n+=4u)output.set(0x14u+n,options.get(0x1cu+n));
    // This actual reread follows the whole path copy, including real aliases.
    output.set(0xcu,options.get(0x14u));output.set(0x118u,options.get(0x12au,2),2);output.set(0x11au,options.get(0x12cu,1),1);
    for(std::uint32_t n=0;n<4u;++n){
        const auto row=0x55d770u+n*0xc74u;std::optional<std::array<std::byte,0x1e0>> captured;
        if(read(memory,row,1)&0x10u){
            captured.emplace();
            for(std::uint32_t offset=0;offset<captured->size();offset+=4u){
                const auto value=read(memory,row+0x18u+offset);
                for(unsigned k=0;k<4u;++k)(*captured)[offset+k]=std::byte(value>>(8u*k));
            }
            // Availability is observed after the real480-byte private copy.
            if(read(memory,row+0x1f8u)==0xffffffffu)captured.reset();
        }
        constexpr std::array<std::byte,0x1e0> empty{};
        const ValueInput<0x1e0> input{captured?*captured:empty,captured.has_value()};
        profile(input,output,0x11bu+n*0x87u);
    }
    output.set(0x337u,0x65636c63u);
}
}
void native_campaign_transition_save_metadata_tu15(SourcePhysicsContactMemory &memory,std::uint32_t options,std::uint32_t destination){construct(memory,RetainedInput{memory,options},RetainedOutput{memory,destination});}
void native_campaign_transition_save_metadata_value_tu15(SourcePhysicsContactMemory &memory,const NativeCampaignTransitionOptionsTu15 &options,NativeCampaignTransitionSaveMetadataTu15 &result){construct(memory,ValueInput<0x1118>{options.bytes},ValueOutput<0x33b>{result.bytes});}
void native_campaign_transition_profile_metadata_tu15(SourcePhysicsContactMemory &memory,std::uint32_t source,std::uint32_t destination){RetainedOutput output{memory,destination};profile(RetainedInput{memory,source},output,0);}
void native_campaign_transition_profile_subrecord15_tu15(SourcePhysicsContactMemory &memory,std::uint32_t source,std::uint32_t destination){RetainedOutput output{memory,destination};subrecord15(RetainedInput{memory,source},0,output,0);}
void native_campaign_transition_profile_subrecord16_tu15(SourcePhysicsContactMemory &memory,std::uint32_t source,std::uint32_t destination){RetainedOutput output{memory,destination};subrecord16(RetainedInput{memory,source},0,output,0);}
}
