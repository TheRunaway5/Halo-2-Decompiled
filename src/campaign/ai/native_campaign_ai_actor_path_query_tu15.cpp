#include "halo2/native_campaign_ai_actor_path_query_tu15.hpp"
#include "halo2/native_campaign_ai_character_property_tu15.hpp"
#include "halo2/native_campaign_ai_goal_evaluation_tu15.hpp"
#include "halo2/native_campaign_ai_object_navigation_tu15.hpp"
#include "native_campaign_ai_goal_port_tu15.hpp"
#include <array>
#include <stdexcept>

namespace halo2 {
namespace {
using namespace goal_tu15;
constexpr u32 none=0xffffffffu;
void stack_store(SourcePhysicsContactMemory &memory,u32 address,u32 value) {
    std::array<std::byte,4> raw{};for(unsigned n=0;n<4;++n)raw[n]=std::byte(value>>(8*n));
    memory.write(address,raw);
}
void push(SourcePhysicsContactMemory &memory,SourcePhysicsWorldRuntimeAbi &abi,u32 value) {
    abi.gp[4]-=4u;stack_store(memory,abi.gp[4],value);
}
void al(SourcePhysicsWorldRuntimeAbi &abi,u32 value){abi.gp[0]=(abi.gp[0]&0xffffff00u)|(value&255u);}
void navigation(const Port &p,u32 entry,SourcePhysicsWorldRuntimeAbi &abi,NativeTu15PhysicsFrameFp &fp) {
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
    struct Publish {SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;
        ~Publish(){fp.control_word=abi.floating_point->x87_control_word;fp.mxcsr=abi.floating_point->mxcsr;fp.status=abi.x87_status;}}publish{abi,fp};
    native_campaign_ai_object_navigation_call_tu15(p.memory,entry,abi,p.observe);
}

void position(const Port &p,SourcePhysicsWorldRuntimeAbi &abi,NativeTu15PhysicsFrameFp &fp) {
    const auto entry=abi.gp[4],saved_esi=abi.gp[6],saved_edi=abi.gp[7],saved_ebx=abi.gp[3];
    abi.gp[0]=p.r32(0x5020c8u);abi.gp[1]=p.r32(abi.gp[0]+0x44u);
    const auto offset=(abi.gp[2]&0xffffu)*0x888u;
    al(abi,p.r8(offset+abi.gp[1]+0x278u));abi.gp[2]=offset+abi.gp[1];
    if(!(abi.gp[0]&255u)) {
        const auto row=abi.gp[2];
        push(p.memory,abi,saved_esi);push(p.memory,abi,saved_edi);
        abi.gp[1]=row+0x238u;abi.gp[7]=p.r32(abi.gp[1]);abi.gp[6]=row+0x27cu;
        abi.gp[0]=abi.gp[6];p.w32(0x275ec7u,abi.gp[0],abi.gp[7]);
        abi.gp[7]=p.r32(abi.gp[1]+4u);abi.gp[1]=p.r32(abi.gp[1]+8u);
        p.w32(0x275ecfu,abi.gp[0]+4u,abi.gp[7]);p.w32(0x275ed2u,abi.gp[0]+8u,abi.gp[1]);
        al(abi,p.r8(row+0x229u));abi.gp[7]=row+0x28cu;
        p.w8(0x275ee3u,row+0x278u,1);p.w16(0x275eeau,row+0x288u,0xffffu);p.w32(0x275ef3u,abi.gp[7],none);
        if(abi.gp[0]&255u) {
            abi.gp[0]=p.r32(row+0x26cu);if(abi.gp[0]==none)abi.gp[0]=p.r32(row+0x18u);
            push(p.memory,abi,0x275f0eu);
            native_campaign_ai_object_aim_origin_tu15(p.memory,abi.gp[0],abi.gp[6],abi.gp[4],
                abi.gp[3],abi.gp[7],fp,p.observe);
            abi.gp[4]+=4u;
        } else {
            abi.gp[1]=p.r32(row+0x26cu);
            if(abi.gp[1]!=none) {
                push(p.memory,abi,abi.gp[6]);push(p.memory,abi,abi.gp[7]);push(p.memory,abi,0x275f23u);
                navigation(p,0x275cb0u,abi,fp);
            } else {
                push(p.memory,abi,saved_ebx);abi.gp[3]=p.r32(row+0x18u);
                if(abi.gp[3]!=none) {
                    abi.gp[0]=abi.gp[3];push(p.memory,abi,0x275f36u);
                    navigation(p,0x275ba0u,abi,fp);
                    if(abi.gp[0]&255u) {
                        push(p.memory,abi,abi.gp[6]);push(p.memory,abi,abi.gp[7]);abi.gp[1]=abi.gp[3];
                        push(p.memory,abi,0x275f43u);
                        navigation(p,0x275cb0u,abi,fp);
                    }
                }
            }
        }
    }
    abi.gp[3]=saved_ebx;abi.gp[6]=saved_esi;abi.gp[7]=saved_edi;abi.gp[4]=entry+4u;
}

void query(const Port &p,SourcePhysicsWorldRuntimeAbi &abi,NativeTu15PhysicsFrameFp &fp) {
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
    const auto entry=abi.gp[4],saved_ebp=abi.gp[5],saved_esi=abi.gp[6],saved_edi=abi.gp[7];
    abi.gp[0]=p.r32(0x5020c8u);abi.gp[1]=p.r32(abi.gp[0]+0x44u);
    abi.gp[4]-=0x10u;push(p.memory,abi,saved_ebp);abi.gp[5]=p.r32(abi.gp[4]+0x18u);
    push(p.memory,abi,saved_esi);abi.gp[6]=(abi.gp[5]&0xffffu)*0x888u+abi.gp[1];
    const auto row=abi.gp[6],out=abi.gp[3];
    abi.gp[1]=p.r32(row+0x54u);push(p.memory,abi,saved_edi);push(p.memory,abi,0x2013dau);
    native_campaign_ai_character_property_call_tu15(p.memory,0x1ecd00u,abi);
    abi.xmm[1]={p.r32(abi.gp[0]+4u),0,0,0};
    al(abi,p.r8(row+0x266u));abi.gp[2]=p.r32(row+0x18u);
    if(abi.gp[0]&255u) {
        abi.gp[7]=p.r32(row+0x26cu);abi.gp[2]=p.r32(0x4ecbd0u);
        abi.gp[0]=p.r32(abi.gp[2]+0x44u);
        abi.gp[1]=p.r32(abi.gp[0]+(abi.gp[7]&0xffffu)*12u+8u);abi.gp[2]=p.r32(abi.gp[1]);
        push(p.memory,abi,abi.gp[2]);abi.gp[1]=abi.gp[5];push(p.memory,abi,0x201413u);
        native_campaign_ai_character_property_call_tu15(p.memory,0x1ed700u,abi);
        abi.gp[2]=abi.gp[7];
        if(abi.gp[0]) {
            abi.xmm[0]={p.r32(abi.gp[0]+0x14u),0,0,0};
            if(Simd(fp).above(abi.xmm[0][0],p.r32(0x46a0e8u)))abi.xmm[1]=abi.xmm[0];
        }
    }
    abi.gp[0]=3u;
    if(sx16(p.r16(row+0x86u))<3)abi.xmm[0]={p.r32(0x46a100u),0,0,0};
    else {
        const auto behavior=sx16(p.r16(row+0x328u));
        abi.xmm[0]={p.r32(0x46a0d0u),0,0,0};
        if(behavior<3)abi.xmm[0]={p.r32(0x46a100u),0,0,0};
    }
    al(abi,p.r8(row+0x3e4u));p.w8(0x201457u,abi.gp[4]+0x20u,abi.gp[0]);
    abi.gp[0]=0;abi.gp[1]=0;
    for(unsigned n=0;n<20;++n)p.w32(0x201464u,out+n*4u,0);
    abi.gp[1]=p.r8(abi.gp[4]+0x20u);abi.gp[7]=none;
    p.w32(0x20146du,out,abi.xmm[1][0]);p.w8(0x201471u,out+4u,abi.gp[1]);
    p.w32(0x201474u,out+8u,abi.gp[2]);p.w32(0x201477u,out+0xcu,none);p.w32(0x20147au,out+0x4cu,abi.xmm[0][0]);
    al(abi,p.r8(row+0x229u));
    if(abi.gp[0]&255u) {
        abi.gp[0]=p.r32(row+0x238u);abi.gp[1]=p.r32(row+0x23cu);abi.gp[2]=p.r32(row+0x240u);
        p.w32(0x20149au,out+0x14u,abi.gp[0]);p.w32(0x20149cu,out+0x18u,abi.gp[1]);p.w32(0x20149fu,out+0x1cu,abi.gp[2]);
        p.w16(0x2014a2u,abi.gp[4]+0x18u,0xffffu);
        abi.gp[2]=p.r32(abi.gp[4]+0x18u); // the high half is real stack history
        p.w32(0x2014abu,out+0x24u,none);p.w32(0x2014afu,out+0x20u,abi.gp[2]);p.w8(0x2014b3u,out+0x10u,1);
    } else {
        abi.gp[2]=abi.gp[5];push(p.memory,abi,0x2014c5u);position(p,abi,fp);
        abi.gp[0]=p.r32(row+0x28cu);p.w8(0x2014d1u,out+0x10u,1);
        abi.gp[2]=p.r32(row+0x27cu);abi.gp[1]=out+0x14u;p.w32(0x2014dau,abi.gp[1],abi.gp[2]);
        abi.gp[2]=p.r32(row+0x280u);p.w32(0x2014dfu,abi.gp[1]+4u,abi.gp[2]);
        abi.gp[2]=p.r32(row+0x284u);p.w32(0x2014e5u,abi.gp[1]+8u,abi.gp[2]);
        abi.gp[2]=p.r32(row+0x288u);p.w32(0x2014edu,abi.gp[1]+0xcu,abi.gp[2]);p.w32(0x2014f0u,out+0x24u,abi.gp[0]);
    }
    abi.gp[5]=saved_ebp;abi.gp[6]=saved_esi;abi.gp[7]=saved_edi;abi.gp[4]=entry+8u;
#else
    throw std::runtime_error("Exact native2013B0 arithmetic backend unavailable");
#endif
}
}
void native_campaign_ai_actor_path_query_call_tu15(SourcePhysicsContactMemory &memory,u32 entry,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(!abi.floating_point || abi.active_mmx)throw std::invalid_argument("Native actor path query requires retained FP controls");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    struct Publication {SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;
        ~Publication(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}} publication{abi,fp};
    const Port port{memory,observe};
    if(entry==0x2013b0u)query(port,abi,fp);
    else if(entry==0x275e90u)position(port,abi,fp);
    else throw std::invalid_argument("Native actor path query requires2013B0 or275E90");
}
void native_campaign_ai_evaluate_goals_with_query_call_tu15(SourcePhysicsContactMemory &memory,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(!abi.floating_point || abi.active_mmx || abi.x87_depth>4u)
        throw std::invalid_argument("Native 26A380 requires retained FP controls and four free x87 temporaries");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    struct Publication {SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;
        ~Publication(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}} publication{abi,fp};
    const auto sp=abi.gp[4];const Port port{memory,observe};
    std::array<u32,8> words{};for(unsigned n=0;n<8;++n)words[n]=port.r32(sp+4u+n*4u);
    const NativeCampaignAiGoalEvaluationTu15 call{words[0],words[1],words[2],words[3],words[4],words[5],words[6],words[7]};
    NativeCampaignAiGoalEvaluationContextTu15 context{sp,&fp,abi.x87_depth,{}};
    context.path_query=[&](SourcePhysicsContactMemory &mem,u32,u32 out,u32 stack,
        NativeTu15PhysicsFrameFp &live,const SourcePhysicsContactRecordObserver &observer) {
        auto child=abi;
        child.gp[3]=out;child.gp[4]=stack;child.gp[5]=call.request;child.gp[6]=call.count;
        child.floating_point=SourcePhysicsFactoryFloatingPoint{live.control_word,live.mxcsr};child.x87_status=live.status;
        const auto publish=[&]{live.control_word=child.floating_point->x87_control_word;
            live.mxcsr=child.floating_point->mxcsr;live.status=child.x87_status;};
        try {native_campaign_ai_actor_path_query_call_tu15(mem,0x2013b0u,child,observer);}
        catch(...){publish();throw;}
        publish();
        if(child.gp[4]!=stack+8u)throw std::runtime_error("Native 2013B0 returned another stack");
    };
    const auto result=native_campaign_ai_evaluate_goals_tu15(memory,call,context,observe);
    abi.gp[0]=result;abi.gp[4]=sp+0x24u;
    if(!(call.count&0xffffu)){abi.gp[1]=0;abi.gp[2]=result;abi.xmm[0]={0,0,0,0};}
}
}
