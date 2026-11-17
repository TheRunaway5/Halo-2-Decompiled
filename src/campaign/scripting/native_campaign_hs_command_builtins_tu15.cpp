#include "halo2/native_campaign_hs_command_builtins_tu15.hpp"
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"
#include <bit>

namespace halo2 {
namespace {
using U=std::uint32_t;
struct Builtins {
    SourcePhysicsContactMemory &memory;
    SourcePhysicsWorldRuntimeAbi &abi;
    const NativeCampaignHsCommandChildrenTu15 &children;
    const SourcePhysicsContactRecordObserver &observe;
    U read(U address,unsigned width=4)const {
        std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));U value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<U>(bytes[n])<<(n*8u);
        return value;
    }
    void write(U pc,U address,U value,unsigned width=4) {
        const bool observed=native_tu15_observes_retained_store(memory,observe,pc,address,width);
        SourcePhysicsContactRecordStore event{pc,address,{},std::vector<std::byte>(width)};
        if(observed){event.before.resize(width);memory.read(address,event.before);}
        for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
        memory.write(address,event.written);if(observed)observe(event);
    }
    void push(U pc,U value){abi.gp[4]-=4u;write(pc,abi.gp[4],value);}
    U pop(){const auto result=read(abi.gp[4]);abi.gp[4]+=4u;return result;}
    void call(U pc,U target,unsigned arguments) {
        const auto expected=abi.gp[4]+arguments;
        push(pc,pc+5u);
        if(!children.call)throw NativeCampaignAiPerformRequiredOwnerTu15(pc,target);
        children.call(pc,target,abi);
        if(abi.gp[4]!=expected)throw std::runtime_error("Native HS child returned another source frame");
    }
    static U signed16(U value){return U(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));}
    void flag(U entry) {
        auto &g=abi.gp;const bool moving=entry==0x2b2470u,looking=entry==0x2b2410u;
        g[0]=signed16(read(g[4]+4u,2));g[0]=read(0x480c00u+g[0]*4u);
        g[1]=read(g[4]+12u);push(entry+0x10u,g[6]);g[6]=read(g[4]+12u);
        push(entry+0x15u,g[1]);g[2]=g[0]+14u;g[0]=signed16(read(g[0]+12u,2));
        push(entry+0x1du,g[2]);push(entry+0x1eu,g[0]);push(entry+0x1fu,g[6]);
        call(entry+0x20u,0x212010u,16u);
        if(g[0]) {
            g[1]=read(0x50eee8u);
            if(g[1]!=0xffffffffu) {
                g[2]=read(0x50eee0u);g[2]=read(g[2]+0x44u);
                g[0]=(g[0]&0xffffff00u)|read(g[0],1);
                g[1]=(g[1]&0xffffu)*0xd4u;
                write(entry+0x4bu,g[1]+g[2]+(looking?0x80u:(moving?0x81u:0x76u)),g[0],1);
            }
            g[0]=0;g[1]=g[6];call(entry+(moving||looking?0x56u:0x53u),0x211da0u,0);
        }
        g[6]=pop();pop();g[4]+=12u;
    }
    void face(U entry) {
        auto &g=abi.gp;
        g[0]=signed16(read(g[4]+4u,2));g[0]=read(0x480c00u+g[0]*4u);
        g[1]=read(g[4]+12u);push(entry+0x10u,g[3]);g[3]=read(g[4]+12u);
        push(entry+0x15u,g[1]);g[2]=g[0]+14u;g[0]=signed16(read(g[0]+12u,2));
        push(entry+0x1du,g[2]);push(entry+0x1eu,g[0]);push(entry+0x1fu,g[3]);
        call(entry+0x20u,0x212010u,16u);
        if(g[0]) {
            g[1]=read(0x50eee8u);g[2]=(g[2]&0xffffff00u)|read(g[0],1);
            if(g[1]!=0xffffffffu) {
                g[1]=(g[1]&0xffffu)*0xd4u;push(entry+0x42u,g[6]);g[6]=read(0x50eee0u);
                push(entry+0x49u,g[7]);g[1]+=read(g[6]+0x44u);g[7]=read(g[0]+4u);g[6]=g[1];
                call(entry+0x52u,entry==0x2b1200u?0x280c90u:0x280d70u,0);
                if(g[2]&0xffu) {
                    g[0]=(g[0]&0xffffff00u)|1u;write(entry+0x5du,g[6]+0x50u,g[0],1);
                    write(entry+0x60u,g[6]+0x51u,g[0],1);
                }
                g[7]=pop();g[6]=pop();
            }
            g[0]=0;g[1]=g[3];call(entry+0x69u,0x211da0u,0);
        }
        g[3]=pop();pop();g[4]+=12u;
    }
    void look(U entry) {
        auto &g=abi.gp;
        g[0]=signed16(read(g[4]+4u,2));g[0]=read(0x480c00u+g[0]*4u);
        g[1]=read(g[4]+12u);push(entry+0x10u,g[6]);g[6]=read(g[4]+12u);
        push(entry+0x15u,g[1]);g[2]=g[0]+14u;g[0]=signed16(read(g[0]+12u,2));
        push(entry+0x1du,g[2]);push(entry+0x1eu,g[0]);push(entry+0x1fu,g[6]);
        call(entry+0x20u,0x212010u,16u);
        if(g[0]) {
            g[2]=read(g[0]+4u);g[1]=(g[1]&0xffffff00u)|read(g[0],1);g[0]=read(0x50eee8u);
            if(g[0]!=0xffffffffu) {
                g[0]=(g[0]&0xffffu)*0xd4u;push(entry+0x43u,g[7]);g[7]=read(0x50eee0u);
                g[0]+=read(g[7]+0x44u);g[7]=pop();write(entry+0x50u,g[0]+0x46u,g[1],1);
                if(g[1]&0xffu) {
                    write(entry+0x55u,g[0]+0x48u,entry==0x2b1000u?2u:1u,2);
                    write(entry+0x5bu,g[0]+0x4cu,g[2]);
                }
            }
            g[0]=0;g[1]=g[6];call(entry+0x62u,0x211da0u,0);
        }
        g[6]=pop();pop();g[4]+=12u;
    }
    void aim_object() {
        auto &g=abi.gp;
        g[0]=signed16(read(g[4]+4u,2));g[0]=read(0x480c00u+g[0]*4u);
        g[1]=read(g[4]+12u);push(0x2b11c0u,g[6]);g[6]=read(g[4]+12u);
        push(0x2b11c5u,g[1]);g[2]=g[0]+14u;g[0]=signed16(read(g[0]+12u,2));
        push(0x2b11cdu,g[2]);push(0x2b11ceu,g[0]);push(0x2b11cfu,g[6]);
        call(0x2b11d0u,0x212010u,16u);
        if(g[0]) {
            g[2]=(g[2]&0xffffff00u)|read(g[0],1);push(0x2b11dbu,g[7]);g[7]=read(g[0]+4u);
            call(0x2b11dfu,0x280d70u,0);g[0]=0;g[1]=g[6];call(0x2b11e8u,0x211da0u,0);g[7]=pop();
        }
        g[6]=pop();pop();g[4]+=12u;
    }
    void go_to_and_face() {
        auto &g=abi.gp;
        g[0]=signed16(read(g[4]+4u,2));g[0]=read(0x480c00u+g[0]*4u);
        g[1]=read(g[4]+12u);push(0x2b0e40u,g[6]);g[6]=read(g[4]+12u);
        push(0x2b0e45u,g[1]);g[2]=g[0]+14u;g[0]=signed16(read(g[0]+12u,2));
        push(0x2b0e4du,g[2]);push(0x2b0e4eu,g[0]);push(0x2b0e4fu,g[6]);
        call(0x2b0e50u,0x212010u,16u);
        if(g[0]) {
            g[1]=read(g[0]+4u);g[2]=read(g[0]);g[0]=read(0x50eee8u);
            if(g[0]!=0xffffffffu) {
                g[0]=(g[0]&0xffffu)*0xd4u;push(0x2b0e73u,g[7]);g[7]=read(0x50eee0u);
                g[0]+=read(g[7]+0x44u);g[7]=pop();write(0x2b0e7eu,g[0]+4u,3,2);
                write(0x2b0e84u,g[0]+0x28u,g[2]);write(0x2b0e87u,g[0]+0x2cu,g[1]);
            }
            g[0]=0;g[1]=g[6];call(0x2b0e8eu,0x211da0u,0);
            g[1]=read(0x505e5cu);g[2]=read(g[1]+0x44u);g[6]=(g[6]&0xffffu)*0x418u;
            write(0x2b0ea8u,g[6]+g[2]+8u,0xfffffffdu);
        }
        g[6]=pop();pop();g[4]+=12u;
    }
    void sleep_forever() {
        auto &g=abi.gp;
        g[0]=read(0x505e6cu);g[1]=read(g[0]+0x44u);
        g[0]=read(0x505e5cu);g[0]=read(g[0]+0x44u);
        push(0x210a20u,g[6]);g[6]=read(g[4]+12u);g[2]=(g[6]&0xffffu)*0x418u;
        g[2]=read(g[2]+g[0]+0x10u);g[0]=read(g[2]+4u)&0xffffu;
        g[0]*=5u;g[0]=read(g[1]+g[0]*4u+0x10u)&0xffffu;
        g[2]=g[0]*5u;g[1]=read(g[1]+g[2]*4u+8u);g[0]=g[6];
        if(g[1]!=0xffffffffu) {
            push(0x210a59u,g[3]);push(0x210a5au,g[1]);g[3]=g[4]+0x14u;g[2]=g[6];
            call(0x210a61u,0x211cb0u,4u);g[3]=read(g[4]+0x10u);
            call(0x210a6au,0x212130u,0);g[3]=pop();
        }
        if(g[0]!=0xffffffffu) {
            g[1]=read(0x505e5cu);g[2]=read(g[1]+0x44u);g[0]=(g[0]&0xffffu)*0x418u;
            write(0x210a89u,g[0]+g[2]+8u,0xfffffffeu);
        }
        g[0]=0;g[1]=g[6];call(0x210a95u,0x211da0u,0);
        g[6]=pop();pop();g[4]+=12u;
    }
};
}
void native_campaign_hs_command_builtins_call_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
    const NativeCampaignHsCommandChildrenTu15 &children,const SourcePhysicsContactRecordObserver &observe) {
    Builtins body{memory,abi,children,observe};
    switch(entry) {
    case 0x2b2410u:case 0x2b2470u:case 0x2b2670u:body.flag(entry);return;
    case 0x210a10u:body.sleep_forever();return;
    case 0x2b1200u:case 0x2b1300u:body.face(entry);return;
    case 0x2b1000u:case 0x2b10b0u:body.look(entry);return;
    case 0x2b11b0u:body.aim_object();return;
    case 0x2b0e30u:body.go_to_and_face();return;
    default:throw std::invalid_argument("Unknown native HS command builtin");
    }
}
}
