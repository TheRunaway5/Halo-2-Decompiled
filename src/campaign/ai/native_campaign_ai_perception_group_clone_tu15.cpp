#include "halo2/native_campaign_ai_perception_group_clone_tu15.hpp"
#include "halo2/native_campaign_ai_perception_group_pressure_tu15.hpp"
#include "halo2/native_campaign_ai_perception_group_initialize_tu15.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"
#include <bit>
#include <vector>
namespace halo2 {
namespace {
using U=std::uint32_t;
struct Clone {
    SourcePhysicsContactMemory &memory;
    const SourcePhysicsContactRecordObserver &observe;
    U read(U at,unsigned width=4)const {
        std::array<std::byte,4> raw{};memory.read(at,std::span(raw).first(width));U value{};
        for(unsigned i=0;i<width;++i)value|=std::to_integer<U>(raw[i])<<(i*8u);return value;
    }
    void write(U pc,U at,U value,unsigned width=4)const {
        const bool watched=native_tu15_observes_retained_store(memory,observe,pc,at,width);
        SourcePhysicsContactRecordStore event{pc,at,{},std::vector<std::byte>(width)};
        if(watched){event.before.resize(width);memory.read(at,event.before);}
        for(unsigned i=0;i<width;++i)event.written[i]=std::byte(value>>(i*8u));
        memory.write(at,event.written);if(watched)observe(event);
    }
    void copy(U source,U destination,SourcePhysicsWorldRuntimeAbi &abi)const {
        auto &g=abi.gp;
        auto word=[&](U pc,U offset){g[1]=read(source+offset);write(pc,destination+offset,g[1]);};
        auto byte=[&](U pc,U offset){g[1]=(g[1]&0xffffff00u)|read(source+offset,1);write(pc,destination+offset,g[1],1);};
        g[1]=(g[1]&0xffff0000u)|read(source+4u,2);write(0x2744e4u,destination+4u,g[1],2);
        word(0x2744ebu,0x2cu);word(0x2744f1u,0x28u);
        for(U offset=0x58u;offset<0xc4u;offset+=4u)write(0x274501u,destination+offset,read(source+offset));
        g[1]=0;byte(0x274506u,0x52u);byte(0x27450cu,0x53u);word(0x274512u,0x54u);
        for(const auto [offset,pc]:std::array<std::pair<U,U>,5>{{{0x3eu,0x27451du},{0x42u,0x274522u},{0x46u,0x274528u},{0x4au,0x27452eu},{0x4eu,0x274534u}}}) {
            const auto value=read(source+offset);write(pc,destination+offset,value);if(offset==0x4eu)g[1]=value;
        }
        byte(0x27453au,0x30u);byte(0x274540u,0x32u);byte(0x274546u,0x33u);
        byte(0x27454cu,0x34u);byte(0x274552u,0x35u);byte(0x274558u,0x31u);byte(0x27455eu,0x36u);
        g[0]=read(source+0x38u);write(0x274565u,destination+0x38u,g[0]);
    }
};
}
void native_campaign_ai_perception_group_clone_call_tu15(SourcePhysicsContactMemory &memory,
    U entry,SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPerformServicesTu15 &,
    const SourcePhysicsContactRecordObserver &observe) {
    if(abi.eflags&0x400u)throw std::invalid_argument("Native perception group clone requires forward string direction");
    Clone clone{memory,observe};const auto saved=abi.gp;
    if(entry==0x2744e0u){clone.copy(saved[0],saved[2],abi);abi.gp[4]+=4u;return;}
    if(entry!=0x274570u)throw std::invalid_argument("Unknown native perception group clone entry");
    if(!abi.floating_point)throw std::invalid_argument("Native perception group clone requires FP state");
    const U snapshot=((saved[4]-4u)&~7u)-0xc8u;
    const auto priority=std::bit_cast<std::int16_t>(std::uint16_t(clone.read(saved[0]+0xcu,2)));
    for(U offset=0;offset<0xc4u;offset+=4u)clone.write(0x27458fu,snapshot+offset,clone.read(saved[0]+offset));
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    const auto publish=[&]{abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;};
    U group{};try{group=native_campaign_ai_perception_group_allocate_tu15(memory,saved[3],priority,fp);}catch(...){publish();throw;}publish();
    if(group!=0xffffffffu) {
        const auto pool=clone.read(0x50eef4u),rows=clone.read(pool+0x44u);
        const auto object=clone.read(snapshot+8u);
        const auto kind=std::bit_cast<std::int16_t>(std::uint16_t(clone.read(snapshot+2u)));
        const auto record=rows+(group&0xffffu)*0xc4u;
        const auto copied_priority=std::uint16_t(clone.read(snapshot+0xcu));
        native_campaign_ai_perception_group_initialize_tu15(memory,record,object,saved[3],kind,copied_priority);
        clone.copy(snapshot,record,abi);
    }
    abi.gp[0]=group;abi.gp[4]=saved[4]+4u;
}
}
