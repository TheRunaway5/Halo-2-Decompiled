#include "halo2/native_campaign_transition_participants_tu15.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <optional>
#include <xmmintrin.h>
#include <emmintrin.h>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t at,unsigned width=4){
    std::array<std::byte,4> bytes{};memory.read(at,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
    return value;
}
void write(SourcePhysicsContactMemory &memory,std::uint32_t at,std::uint32_t value,unsigned width=4){
    std::array<std::byte,4> bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8u*n));
    memory.write(at,std::span(bytes).first(width));
}
std::int32_t signed_value(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
std::int32_t signed_byte(std::uint32_t value){return std::bit_cast<std::int8_t>(std::uint8_t(value));}
struct RetainedOptions {
    SourcePhysicsContactMemory &memory;std::uint32_t address;
    std::uint32_t get(std::uint32_t offset,unsigned width=4)const{return read(memory,address+offset,width);}
    void set(std::uint32_t offset,std::uint32_t value,unsigned width=4){write(memory,address+offset,value,width);}
    bool nonnull(std::uint32_t offset)const{return address+offset!=0;}
};
struct PrivateOptions {
    NativeCampaignTransitionOptionsTu15 &options;
    std::uint32_t get(std::uint32_t offset,unsigned width=4)const{
        std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(options.bytes[offset+n])<<(8u*n);return value;
    }
    void set(std::uint32_t offset,std::uint32_t value,unsigned width=4){
        for(unsigned n=0;n<width;++n)options.bytes[offset+n]=std::byte(value>>(8u*n));
    }
    bool nonnull(std::uint32_t)const{return true;}
};
// The original iterator caches a high-water mark and bitmap for each advance,
// then resolves the selected row through a fresh pool/header pair. Neither
// the pool's capacity nor its occupancy is replaced by a host participant list.
std::uint32_t next_player(const SourcePhysicsContactMemory &memory,std::uint32_t previous,
    std::uint32_t &index){
    index=previous+1u;if(signed_value(index)<0)return 0;
    const auto pool=read(memory,0x4f54f4u),high=read(memory,pool+0x38u);
    if(signed_value(index)>=signed_value(high))return 0;
    const auto bitmap=read(memory,pool+0x48u);
    while(!(read(memory,bitmap+(index>>5u)*4u)&(std::uint32_t{1}<<(index&31u)))){
        ++index;if(signed_value(index)>=signed_value(high))return 0;
    }
    if(index==none)return 0;
    const auto selected=read(memory,0x4f54f4u),stride=read(memory,selected+0x24u),rows=read(memory,selected+0x44u);
    return rows+stride*index;
}
template<class Options>void capture(SourcePhysicsContactMemory &memory,Options options){
    const auto context=read(memory,0x4f54f0u);
    options.set(0x268u,read(memory,context+0x2cu));
    for(std::uint32_t n=0;n<0x60u;n+=4u)options.set(0x26cu+n,read(memory,context+0x30u+n));
    options.set(0x2ccu,read(memory,context+0x90u,1),1);
    options.set(0x2cdu,read(memory,context+0x91u));
    options.set(0x2d1u,read(memory,context+0x95u,2),2);
    for(std::uint32_t n=0;n<0xe40u;n+=4u)options.set(0x2d4u+n,0);
    std::uint32_t previous=none,count{};
    for(;;){
        std::uint32_t index{};const auto player=next_player(memory,previous,index);previous=index;
        if(!player||count>=16u)return;
        const auto output=0x2d4u+count*0xe4u;
        options.set(output,1,1);
        const auto flags=read(memory,player+2u,1);++count;
        options.set(output+1u,(flags>>1u)&1u,1);
        for(std::uint32_t n=0;n<12u;n+=4u)options.set(output+0xeu+n,read(memory,player+4u+n));
        options.set(output+8u,read(memory,player+0x14u));
        options.set(output+0xcu,read(memory,player+0x18u,2),2);
        options.set(output+2u,read(memory,player+0x1cu,2),2);
        options.set(output+4u,read(memory,player+0x20u));
        for(std::uint32_t n=0;n<0x90u;n+=4u)options.set(output+0x1cu+n,read(memory,player+0x44u+n));
    }
}
class Environment {
    alignas(16)std::array<std::byte,108> host_{};std::array<std::byte,28> native_{};
    std::uint32_t host_mxcsr_;NativeTu15PhysicsFrameFp &fp_;
public:
    Environment(NativeTu15PhysicsFrameFp &fp,unsigned occupied):fp_(fp){
        if(occupied>7||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
            (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native transition equipment requires masked retained FP and one free x87 value slot");
        host_mxcsr_=_mm_getcsr();
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),native_.size());constexpr std::uint16_t empty=0xffffu;
        std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);std::memcpy(native_.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){
        asm volatile("fnstcw %0;fnstsw %1":"=m"(fp_.control_word),"=m"(fp_.status));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);
    }
};
std::uint32_t object(const SourcePhysicsContactMemory &memory,std::uint32_t datum){
    const auto pool=read(memory,0x4ecbd0u),rows=read(memory,pool+0x44u);
    return read(memory,rows+(datum&0xffffu)*12u+8u);
}
std::uint32_t spare(const SourcePhysicsContactMemory &memory,std::uint32_t unit){
    const auto body=object(memory,unit);const auto first=signed_byte(read(memory,body+0x212u,1));
    for(std::uint32_t n=0;n<4u;++n){
        if(signed_value(n)==first)continue;
        if(signed_value(n)==signed_byte(read(memory,body+0x213u,1)))continue;
        if(read(memory,body+0x218u+n*4u)!=none)return read(memory,body+0x218u+n*4u);
    }
    return none;
}
template<class Options>void equipment(const SourcePhysicsContactMemory &memory,std::uint32_t item,
    Options &options,std::uint32_t output){
    const auto body=object(memory,item),definition=read(memory,body),tags=read(memory,0x4f0414u);
    const auto tag=read(memory,tags+(definition&0xffffu)*16u+8u);
    std::uint32_t kind{};
    if(signed_value(read(memory,tag+0x2d0u))>0){
        const auto block=read(memory,tag+0x2d4u),reference=read(memory,block+0x90u);
        if(reference!=none){const auto target=read(memory,tags+(reference&0xffffu)*16u+8u);kind=read(memory,target+0x128u,1);}
        else if(read(memory,block+0x98u)!=none)kind=read(memory,block+0x88u,1);
    }
    if(!kind)kind=read(memory,tag+0x1fcu,1);
    auto quantity=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read(memory,0x46a0d0u))));
    options.set(output,kind,2);
    const auto fraction=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read(memory,body+0x184u))));
    asm volatile("subss %1,%0":"+&x"(quantity):"x"(fraction):"memory");
    const auto factor=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read(memory,0x4500b8u))));
    asm volatile("mulss %1,%0":"+&x"(quantity):"x"(factor):"memory");
    const auto single=std::bit_cast<std::uint32_t>(_mm_cvtss_f32(quantity));std::int32_t converted;
    asm volatile("flds %1;fistpl %0":"=m"(converted):"m"(single):"memory","st");
    options.set(output+6u,std::uint16_t(converted),2);
    if(signed_value(read(memory,tag+0x2c0u))>0){
        options.set(output+2u,read(memory,body+0x22au,2),2);
        options.set(output+4u,read(memory,body+0x22cu,2),2);
    }
}
template<class Options>void prepare(SourcePhysicsContactMemory &memory,Options options){
    const auto baseline=read(memory,0x4f3218u);
    for(std::uint32_t participant=0;participant<16u;++participant)
        for(std::uint32_t n=0;n<0x38u;n+=4u)
            options.set(0x380u+participant*0xe4u+n,read(memory,baseline+0x388u+participant*0xe4u+n));
    bool special{};const auto scenario=read(memory,0x4ecc20u);
    if(signed_value(read(memory,scenario+0x100u))>0){const auto block=read(memory,scenario+0x104u);special=read(memory,block+0x2cu,2)==1u;}
    std::uint32_t previous=none;
    for(;;){
        std::uint32_t index{};const auto player=next_player(memory,previous,index);previous=index;if(!player)return;
        std::optional<std::uint32_t> selected;
        for(std::uint32_t p=0;p<16u;++p){
            bool equal=true;
            for(std::uint32_t n=0;n<12u;++n){
                const auto first=read(memory,player+4u+n,1),second=options.get(0x2e2u+p*0xe4u+n,1);
                if(first!=second){equal=false;break;}
            }
            if(equal){const auto at=0x380u+(special?0x1cu:0u)+p*0xe4u;if(options.nonnull(at))selected=at;else selected.reset();}
        }
        if(!selected)continue;
        const auto output=*selected,unit=read(memory,player+0x2cu);
        if(unit==none){for(std::uint32_t n=0;n<0x1cu;n+=4u)options.set(output+n,0);continue;}
        const auto unit_pool=read(memory,0x4ecbd0u),headers=read(memory,unit_pool+0x44u),row=(unit&0xffffu)*12u;
        const auto body=read(memory,headers+row+8u);std::uint32_t primary=none,secondary=none,other=none;
        if(read(memory,body+0x1fcu,2)!=0xffffu){
            const auto clock=read(memory,0x51df54u),now=read(memory,clock+8u),first_age=now-read(memory,body+0x228u);std::uint32_t chosen=none;
            for(unsigned pass=0;pass<2u;++pass){
                auto age=first_age,item=read(memory,body+0x218u),best=std::uint32_t{0x7fffffff};
                auto candidate=none;
                if(item!=none&&(!pass||item!=chosen)&&signed_value(age)<signed_value(best)){best=age;candidate=item;}
                for(std::uint32_t n=1;n<4u;++n){
                    age=now-read(memory,body+0x228u+n*4u);item=read(memory,body+0x218u+n*4u);
                    if(item!=none&&(!pass||item!=chosen)&&signed_value(age)<signed_value(best)){
                        if(n<3u)best=age;candidate=item;
                    }
                }
                if(!pass){primary=candidate;chosen=candidate;}else other=candidate;
            }
        }else{
            const auto selected_headers=read(memory,unit_pool+0x44u),selected_body=read(memory,selected_headers+row+8u);
            const auto slot=signed_byte(read(memory,selected_body+0x212u,1));
            if(slot!=-1)primary=read(memory,selected_body+std::uint32_t(slot)*4u+0x218u);
            const auto secondary_headers=read(memory,unit_pool+0x44u),secondary_body=read(memory,secondary_headers+row+8u);
            const auto slot2=signed_byte(read(memory,secondary_body+0x213u,1));
            if(slot2!=-1)secondary=read(memory,secondary_body+std::uint32_t(slot2)*4u+0x218u);
            other=spare(memory,unit);
        }
        if(primary!=none)equipment(memory,primary,options,output+2u);else options.set(output+2u,0,2);
        if(secondary!=none)equipment(memory,secondary,options,output+0x12u);else options.set(output+0x12u,0,2);
        if(other!=none)equipment(memory,other,options,output+0xau);else options.set(output+0xau,0,2);
        options.set(output+0x1au,read(memory,body+0x23eu,1),1);
        options.set(output+0x1bu,read(memory,body+0x23fu,1),1);
        options.set(output,1,1);
    }
}
}
void native_campaign_transition_capture_participants_tu15(SourcePhysicsContactMemory &memory,std::uint32_t options){capture(memory,RetainedOptions{memory,options});}
void native_campaign_transition_capture_participants_value_tu15(SourcePhysicsContactMemory &memory,NativeCampaignTransitionOptionsTu15 &options){capture(memory,PrivateOptions{options});}
void native_campaign_transition_prepare_participants_tu15(SourcePhysicsContactMemory &memory,std::uint32_t options,NativeTu15PhysicsFrameFp &fp,unsigned occupied){Environment environment(fp,occupied);prepare(memory,RetainedOptions{memory,options});}
void native_campaign_transition_prepare_participants_value_tu15(SourcePhysicsContactMemory &memory,NativeCampaignTransitionOptionsTu15 &options,NativeTu15PhysicsFrameFp &fp,unsigned occupied){Environment environment(fp,occupied);prepare(memory,PrivateOptions{options});}
std::uint32_t native_campaign_transition_spare_equipment_tu15(const SourcePhysicsContactMemory &memory,std::uint32_t unit){return spare(memory,unit);}
void native_campaign_transition_equipment_tu15(SourcePhysicsContactMemory &memory,std::uint32_t item,std::uint32_t destination,NativeTu15PhysicsFrameFp &fp,unsigned occupied){Environment environment(fp,occupied);RetainedOptions options{memory,destination};equipment(memory,item,options,0);}
}
