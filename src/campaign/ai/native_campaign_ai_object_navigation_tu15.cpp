#include "halo2/native_campaign_ai_object_navigation_tu15.hpp"
#include "halo2/native_campaign_object_world_point_tu15.hpp"
#include "halo2/native_campaign_ai_biped_refresh_tu15.hpp"
#include "halo2/native_campaign_ai_navigation_lookup_tu15.hpp"
#include "halo2/native_campaign_ai_frame_geometry_tu15.hpp"
#include "halo2/native_tu15_object_body_kernel.hpp"
#include "native_campaign_ai_goal_port_tu15.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
#include <array>

namespace halo2 {
namespace {
using namespace goal_tu15;
constexpr u32 none = 0xffffffffu;
void push(SourcePhysicsContactMemory &memory, SourcePhysicsWorldRuntimeAbi &abi, u32 value) {
    std::array<std::byte, 4> raw{};
    for (unsigned n=0; n<4; ++n) raw[n]=std::byte(value>>(8*n));
    abi.gp[4]-=4; memory.write(abi.gp[4],raw);
}
u32 signed_byte(u32 value) { return static_cast<u32>(static_cast<std::int8_t>(value)); }
void low_byte(SourcePhysicsWorldRuntimeAbi &abi,u32 value) {
    abi.gp[0]=(abi.gp[0]&0xffffff00u)|(value&255u);
}
// The shared salted-lookup algorithm uses the original lazy read order. This
// adapter retains BCDF0's actual saved words and argument read for its caller.
u32 checked_object(const Port &p,SourcePhysicsWorldRuntimeAbi &abi) {
    const auto sp=abi.gp[4],esi=abi.gp[6],edi=abi.gp[7];
    struct View {
        const Port &p;SourcePhysicsWorldRuntimeAbi &a;
        u32 begin(){push(p.memory,a,a.gp[6]);push(p.memory,a,a.gp[7]);a.gp[6]=a.gp[0];a.gp[7]=0;return a.gp[6];}
        bool present(u32 value)const{return value!=none;}
        u32 objects(){return a.gp[2]=p.r32(0x4ecbd0u);}
        u32 limit(u32 pool){return a.gp[0]=p.r32(pool+0x38u);}
        bool in_bounds(u32 value,u32 limit){a.gp[1]=value&65535u;return i32(a.gp[1])<std::bit_cast<i32>(limit);}
        u32 record(u32 pool){a.gp[0]=p.r32(pool+0x24u)*a.gp[1];return a.gp[0]+=p.r32(pool+0x44u);}
        u16 salt(u32 row){const auto value=p.r16(row);a.gp[1]=(a.gp[1]&0xffff0000u)|value;return u16(value);}
        bool live(u16 value)const{return value!=0;}
        bool matches(u32,u16 value){a.gp[1]=u32(sx16(value));a.gp[6]=u32(std::bit_cast<i32>(a.gp[6])>>16);return a.gp[1]==a.gp[6];}
        void admit(u32 row){a.gp[7]=row;}
        u32 finish(){
            a.gp[0]=0;
            if(a.gp[7]) {
                a.gp[1]=(a.gp[1]&0xffffff00u)|p.r8(a.gp[7]+3u);a.gp[2]=1u<<(a.gp[1]&31u);
                if(p.r32(a.gp[4]+0xcu)&a.gp[2])a.gp[0]=p.r32(a.gp[7]+8u);
            }
            return a.gp[0];
        }
    } view{p,abi};
    const auto result=native_object_body_detail::lookup(view);
    abi.gp[6]=esi;abi.gp[7]=edi;abi.gp[4]=sp+8u;return result;
}
void biped_dynamic_sector(const Port &p,SourcePhysicsWorldRuntimeAbi &abi) {
    const auto sp=abi.gp[4];
    abi.gp[1]=p.r32(0x4ecbd0u);abi.gp[2]=p.r32(abi.gp[1]+0x44u);
    abi.gp[0]=(abi.gp[0]&65535u)*3u;abi.gp[1]=p.r32(abi.gp[2]+abi.gp[0]*4u+8u);
    abi.gp[2]=p.r32(abi.gp[1]+0x35cu);low_byte(abi,0);
    if(abi.gp[2]==none) {
        abi.gp[2]=p.r32(abi.gp[1]+0x354u);
        if(abi.gp[2]!=none && p.r32(abi.gp[1]+0x360u)==none) {
            abi.gp[0]=p.r32(0x4ecc10u);abi.gp[1]=p.r32(abi.gp[0]+0x2cu);
            low_byte(abi,p.r8(abi.gp[1]+abi.gp[2]*8u+4u)&1u);
        }
    }
    abi.gp[4]=sp+4u;
}
void biped_reset(const Port &p,SourcePhysicsWorldRuntimeAbi &abi) {
    const auto sp=abi.gp[4];
    abi.gp[1]=p.r32(0x4ecbd0u);abi.gp[2]=p.r32(abi.gp[1]+0x44u);
    abi.gp[0]=(abi.gp[0]&65535u)*3u;abi.gp[1]=none;
    abi.gp[0]=p.r32(abi.gp[2]+abi.gp[0]*4u+8u);
    for(const auto [site,offset]:std::array<std::array<u32,2>,6>{{
        {0xe22d8u,0x354u},{0xe22deu,0x358u},{0xe22e4u,0x35cu},
        {0xe22eau,0x360u},{0xe22f0u,0x364u},{0xe22f6u,0x350u}}})p.w32(site,abi.gp[0]+offset,none);
    abi.gp[4]=sp+4u;
}
void biped_point(const Port &p,SourcePhysicsWorldRuntimeAbi &abi) {
    const auto entry=abi.gp[4];const auto saved=abi.gp;
    abi.gp[1]=p.r32(0x4ecbd0u);abi.gp[2]=p.r32(abi.gp[1]+0x44u);
    abi.gp[4]-=0xcu;push(p.memory,abi,abi.gp[3]);push(p.memory,abi,abi.gp[5]);
    push(p.memory,abi,abi.gp[6]);push(p.memory,abi,abi.gp[7]);
    abi.gp[7]=abi.gp[0];abi.gp[0]=(abi.gp[0]&65535u)*3u;
    abi.gp[3]=p.r32(abi.gp[2]+abi.gp[0]*4u+8u);const auto body=abi.gp[3],frame=abi.gp[4];
    const auto state=p.r8(body+0x3dcu);abi.gp[5]=body+0x354u;
    if(state==2u) {
        abi.gp[6]=p.r32(frame+0x28u);abi.gp[0]=none;
        p.w32(0xe2336u,abi.gp[5],none);
        for(const auto [site,offset]:std::array<std::array<u32,2>,4>{{
            {0xe2339u,0x358u},{0xe233fu,0x35cu},{0xe2345u,0x360u},{0xe234bu,0x364u}}})p.w32(site,body+offset,none);
        abi.gp[0]=abi.gp[7];push(p.memory,abi,0xe2358u);
        if(!abi.floating_point)throw std::invalid_argument("Native biped point requires retained FP controls");
        NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
        try{native_campaign_object_world_point_frame_tu15(p.memory,abi.gp[0],abi.gp[6],abi.gp[4],abi.gp[7],fp,p.observe);}
        catch(...){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;throw;}
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
        abi.gp[0]=abi.gp[6];abi.gp[4]+=4u;
    } else {
        bool refresh=p.r32(abi.gp[5])==none;
        if(!refresh) {
            abi.gp[0]=abi.gp[7];push(p.memory,abi,0xe236au);biped_dynamic_sector(p,abi);
            refresh=(abi.gp[0]&255u)!=0;
        }
        if(!refresh) {
            abi.gp[0]=p.r32(body+0x360u);
            if(abi.gp[0]!=none) {
                push(p.memory,abi,none);push(p.memory,abi,0xe2384u);checked_object(p,abi);
                low_byte(abi,abi.gp[0]!=0);
                if(!(abi.gp[0]&255u)){abi.gp[0]=abi.gp[7];push(p.memory,abi,0xe2398u);biped_reset(p,abi);}
            }
        } else {
            abi.gp[0]=p.r32(0x51df54u);abi.gp[1]=p.r32(abi.gp[0]+8u);
            if(std::bit_cast<i32>(abi.gp[1])>std::bit_cast<i32>(p.r32(body+0x350u))) {
                abi.gp[0]=body+0x368u;abi.gp[2]=p.r32(abi.gp[0]);p.w32(0xe23b9u,frame+0x10u,abi.gp[2]);
                abi.gp[2]=p.r32(abi.gp[0]+4u);abi.gp[0]=p.r32(abi.gp[0]+8u);abi.gp[6]=body+0x360u;
                p.w32(0xe23c9u,frame+0x18u,abi.gp[0]);abi.gp[0]=p.r32(abi.gp[6]);
                p.w32(0xe23d2u,frame+0x14u,abi.gp[2]);p.w32(0xe23d6u,body+0x350u,abi.gp[1]);
                if(abi.gp[0]!=none) {
                    push(p.memory,abi,none);push(p.memory,abi,0xe23e5u);checked_object(p,abi);
                    if(!abi.gp[0])p.w32(0xe23e9u,abi.gp[6],none);
                }
                abi.gp[1]=body+0x364u;push(p.memory,abi,abi.gp[1]);abi.gp[1]=p.r32(abi.gp[6]);
                push(p.memory,abi,abi.gp[6]);abi.gp[0]=body+0x35cu;push(p.memory,abi,abi.gp[0]);
                abi.gp[0]=p.r32(abi.gp[0]);push(p.memory,abi,abi.gp[5]);abi.gp[2]=abi.gp[4]+0x20u;
                push(p.memory,abi,abi.gp[2]);push(p.memory,abi,abi.gp[0]);push(p.memory,abi,abi.gp[1]);
                push(p.memory,abi,0xe240fu);
                native_campaign_ai_biped_refresh_call_tu15(p.memory,0xe20f0u,abi,p.observe);
                const bool located=p.r32(abi.gp[5])!=none;
                p.w32(0xe2413u,body+0x358u,none);
                if(located) {
                    abi.gp[2]=p.r32(frame+0x10u);abi.gp[0]=p.r32(frame+0x14u);abi.gp[1]=p.r32(frame+0x18u);
                    p.w32(0xe242bu,body+0x368u,abi.gp[2]);p.w32(0xe2431u,body+0x36cu,abi.gp[0]);p.w32(0xe2437u,body+0x370u,abi.gp[1]);
                }
            }
        }
        abi.gp[0]=p.r32(body+0x358u);abi.gp[1]=none;
        if(abi.gp[0]==none) {
            abi.gp[0]=p.r32(abi.gp[5]);
            if(abi.gp[0]!=none) {
                abi.gp[1]=p.r32(body+0x364u);abi.gp[2]=body+0x368u;
                push(p.memory,abi,abi.gp[2]);push(p.memory,abi,abi.gp[1]);abi.gp[1]=p.r32(body+0x360u);
                push(p.memory,abi,abi.gp[0]);abi.gp[0]=p.r32(body+0x35cu);push(p.memory,abi,0xe2471u);
                native_campaign_ai_navigation_lookup_call_tu15(p.memory,0x202660u,abi,p.observe);
                p.w32(0xe2471u,body+0x358u,abi.gp[0]);
            } else {
                p.w32(0xe2479u,body+0x358u,abi.gp[1]);
            }
        }
        abi.gp[6]=p.r32(frame+0x28u);
    }
    if(abi.gp[6]) {
        abi.gp[2]=body+0x368u;abi.gp[0]=p.r32(abi.gp[2]);p.w32(0xe248fu,abi.gp[6],abi.gp[0]);
        abi.gp[1]=p.r32(abi.gp[2]+4u);p.w32(0xe2494u,abi.gp[6]+4u,abi.gp[1]);
        abi.gp[2]=p.r32(abi.gp[2]+8u);p.w32(0xe249au,abi.gp[6]+8u,abi.gp[2]);
    }
    abi.gp[0]=p.r32(frame+0x20u);if(abi.gp[0]){abi.gp[1]=p.r32(abi.gp[5]);p.w32(0xe24a8u,abi.gp[0],abi.gp[1]);}
    abi.gp[0]=p.r32(frame+0x2cu);if(abi.gp[0]){abi.gp[2]=p.r32(body+0x360u);p.w32(0xe24b8u,abi.gp[0],abi.gp[2]);}
    abi.gp[0]=p.r32(frame+0x30u);if(abi.gp[0]){abi.gp[1]=p.r32(body+0x364u);p.w32(0xe24c8u,abi.gp[0],abi.gp[1]);}
    abi.gp[0]=p.r32(frame+0x24u);if(abi.gp[0]){abi.gp[2]=p.r32(body+0x358u);p.w32(0xe24d8u,abi.gp[0],abi.gp[2]);}
    for(const auto n:{3u,5u,6u,7u})abi.gp[n]=saved[n];abi.gp[4]=entry+0x18u;
}
void framed_point(const Port &p,SourcePhysicsWorldRuntimeAbi &abi) {
    const auto sp=abi.gp[4],saved_ebx=abi.gp[3];push(p.memory,abi,saved_ebx);abi.gp[3]=abi.gp[0];
    if(abi.gp[1]!=none) {
        const auto saved_edi=abi.gp[7];
        abi.gp[0]=p.r32(abi.gp[4]+8u);push(p.memory,abi,abi.gp[7]);push(p.memory,abi,0x21871eu);
        native_campaign_ai_navigation_lookup_call_tu15(p.memory,0x2185e0u,abi,p.observe);
        abi.gp[7]=abi.gp[0];
        if(u16(abi.gp[7])==0xffffu) {
            abi.gp[0]=p.r32(abi.gp[6]);abi.gp[2]=abi.gp[3];p.w32(0x21872au,abi.gp[2],abi.gp[0]);
            abi.gp[1]=p.r32(abi.gp[6]+4u);p.w32(0x21872fu,abi.gp[2]+4u,abi.gp[1]);
            abi.gp[0]=p.r32(abi.gp[6]+8u);p.w16(0x218735u,abi.gp[3]+0xcu,abi.gp[7]);p.w32(0x21873au,abi.gp[2]+8u,abi.gp[0]);
        } else {
            abi.gp[1]=abi.gp[6];abi.gp[0]=abi.gp[7];push(p.memory,abi,0x21874cu);
            native_campaign_ai_navigation_lookup_call_tu15(p.memory,0x218960u,abi,p.observe);
            const bool resolved=(abi.gp[0]&255u)!=0;
            if(resolved)p.w16(0x218750u,abi.gp[3]+0xcu,abi.gp[7]);
            else {
                abi.gp[2]=p.r32(abi.gp[6]);abi.gp[1]=abi.gp[3];p.w32(0x21875fu,abi.gp[1],abi.gp[2]);
                abi.gp[0]=p.r32(abi.gp[6]+4u);p.w32(0x218764u,abi.gp[1]+4u,abi.gp[0]);
                abi.gp[2]=p.r32(abi.gp[6]+8u);p.w16(0x21876bu,abi.gp[3]+0xcu,none);p.w32(0x218771u,abi.gp[1]+8u,abi.gp[2]);
            }
        }
        abi.gp[7]=saved_edi;low_byte(abi,1);abi.gp[3]=saved_ebx;abi.gp[4]=sp+8u;return;
    }
    abi.gp[1]=p.r32(abi.gp[6]);p.w32(0x2186fau,abi.gp[0],abi.gp[1]);
    abi.gp[2]=p.r32(abi.gp[6]+4u);p.w32(0x2186ffu,abi.gp[0]+4u,abi.gp[2]);
    abi.gp[1]=p.r32(abi.gp[6]+8u);p.w32(0x218705u,abi.gp[0]+8u,abi.gp[1]);
    p.w16(0x218708u,abi.gp[3]+0xcu,none);low_byte(abi,1);
    abi.gp[3]=saved_ebx;abi.gp[4]=sp+8u;
}
void eligibility(const Port &p, SourcePhysicsWorldRuntimeAbi &abi) {
    const auto sp=abi.gp[4], saved_esi=abi.gp[6];
    abi.gp[1]=none; push(p.memory,abi,saved_esi); abi.gp[6]=p.r32(0x4ecbd0u);
    if (abi.gp[0]!=none) {
        abi.gp[2]=p.r32(abi.gp[6]+0x44u);
        do {
            abi.gp[1]=abi.gp[0]; abi.gp[0]=(abi.gp[0]&0xffffu)*3u;
            abi.gp[0]=p.r32(abi.gp[2]+abi.gp[0]*4u+8u);
            abi.gp[0]=p.r32(abi.gp[0]+0x14u);
        } while (abi.gp[0]!=none);
    }
    abi.gp[2]=p.r32(abi.gp[6]+0x44u); abi.gp[1]=(abi.gp[1]&0xffffu)*3u;
    abi.gp[0]=p.r32(abi.gp[2]+abi.gp[1]*4u+8u);
    abi.gp[1]=signed_byte(p.r8(abi.gp[0]+0xaau));
    abi.gp[6]=saved_esi; abi.gp[4]+=4u;
    u32 allowed=1;
    if (abi.gp[1]==0 && p.r8(abi.gp[0]+0x3dcu)==1)
        allowed=(~p.r8(abi.gp[0]+0x348u))&1u;
    else if (abi.gp[1]==12 && p.r8(abi.gp[0]+0x17cu)==1)
        allowed=(~(p.r8(abi.gp[0]+0x12cu)>>7u))&1u;
    abi.gp[0]=(abi.gp[0]&0xffffff00u)|allowed; abi.gp[4]=sp+4u;
}
void point(const Port &p, SourcePhysicsWorldRuntimeAbi &abi,
    const NativeCampaignAiPerformCalleeTu15 &vehicle_point) {
    const auto sp=abi.gp[4], saved_ebp=abi.gp[5], saved_esi=abi.gp[6], saved_edi=abi.gp[7];
    abi.gp[4]-=0x74u; push(p.memory,abi,saved_ebp);
    abi.gp[5]=p.r32(abi.gp[4]+0x80u); push(p.memory,abi,saved_esi);
    abi.gp[6]=none; push(p.memory,abi,saved_edi);
    abi.gp[7]=p.r32(0x4ecbd0u); abi.gp[0]=none;
    if (abi.gp[1]!=none) {
        abi.gp[2]=p.r32(abi.gp[7]+0x44u);
        do {
            abi.gp[0]=abi.gp[1]; abi.gp[1]=(abi.gp[1]&0xffffu)*3u;
            abi.gp[1]=p.r32(abi.gp[2]+abi.gp[1]*4u+8u);
            abi.gp[1]=p.r32(abi.gp[1]+0x14u);
        } while (abi.gp[1]!=none);
    }
    abi.gp[1]=abi.gp[0]&0xffffu; abi.gp[2]=abi.gp[1]*3u;
    abi.gp[1]=p.r32(abi.gp[7]+0x44u); abi.gp[2]=p.r32(abi.gp[1]+abi.gp[2]*4u+8u);
    abi.gp[1]=signed_byte(p.r8(abi.gp[2]+0xaau)); abi.gp[7]=none;
    const auto frame=abi.gp[4];
    const auto finish=[&]{
        abi.gp[0]=p.r32(frame+0x84u);
        if (abi.gp[0]) p.w32(0x275e1bu,abi.gp[0],abi.gp[7]);
        abi.gp[7]=saved_edi; abi.gp[6]=saved_esi; abi.gp[5]=saved_ebp; abi.gp[4]=sp+12u;
    };
    const auto publish_point=[&](u32 return_pc) {
        abi.gp[7]=p.r32(frame+0xcu);
        if (!abi.gp[5]) return;
        if (abi.gp[7]!=abi.gp[6]) {
            abi.gp[0]=p.r32(frame+0x10u);abi.gp[1]=p.r32(frame+0x14u);
            push(p.memory,abi,abi.gp[0]);abi.gp[0]=abi.gp[5];abi.gp[6]=abi.gp[4]+0x1cu;
            push(p.memory,abi,return_pc);framed_point(p,abi);
        } else {
            abi.gp[2]=p.r32(frame+0x18u);abi.gp[0]=p.r32(frame+0x1cu);abi.gp[1]=abi.gp[5];
            p.w32(0x275dfau,abi.gp[1],abi.gp[2]);abi.gp[2]=p.r32(frame+0x20u);
            p.w32(0x275e00u,abi.gp[1]+4u,abi.gp[0]);p.w32(0x275e03u,abi.gp[1]+8u,abi.gp[2]);
            p.w16(0x275e06u,abi.gp[5]+0xcu,abi.gp[6]);
        }
    };
    p.w32(0x275d04u,frame+0xcu,none); p.w32(0x275d08u,frame+0x14u,none);
    p.w32(0x275d0cu,frame+0x10u,none);
    if (abi.gp[1]==0) {
        abi.gp[1]=frame+0x10u; push(p.memory,abi,abi.gp[1]);
        abi.gp[2]=abi.gp[4]+0x18u; push(p.memory,abi,abi.gp[2]);
        abi.gp[1]=abi.gp[4]+0x20u; push(p.memory,abi,abi.gp[1]);
        abi.gp[2]=abi.gp[4]+0x18u; push(p.memory,abi,abi.gp[2]);
        push(p.memory,abi,0); push(p.memory,abi,0x275dceu);
        biped_point(p,abi);
        publish_point(0x275deeu);
        finish();return;
    }
    --abi.gp[1];
    if (abi.gp[1]==0) {
        push(p.memory,abi,abi.gp[3]); abi.gp[1]=abi.gp[4]+0x14u; push(p.memory,abi,abi.gp[1]);
        abi.gp[2]=abi.gp[4]+0x1cu; push(p.memory,abi,abi.gp[2]);
        abi.gp[1]=abi.gp[4]+0x24u; push(p.memory,abi,abi.gp[1]);
        abi.gp[2]=abi.gp[4]+0x1cu; push(p.memory,abi,abi.gp[2]);
        push(p.memory,abi,0); abi.gp[3]=abi.gp[0]; push(p.memory,abi,0x275d90u);
        if (!vehicle_point) throw NativeCampaignAiPerformRequiredOwnerTu15(0x275d8bu,0xf35d0u);
        constexpr u32 arithmetic_flags=0x8d5u;
        // The type check (SUB 0; DEC 1) leaves ZF and PF set.
        abi.eflags=(abi.eflags&~arithmetic_flags)|0x44u;
        vehicle_point(0x275d8bu,0xf35d0u,abi);
        if (abi.gp[4]!=frame-4u)
            throw std::runtime_error("TU1.5 vehicle point returned another navigation stack");
        abi.gp[3]=p.r32(abi.gp[4]);abi.gp[4]+=4u;
        publish_point(0x275db1u);
        finish();
        physics_contact_runtime_detail::Flags flags;
        flags.plus(frame+12u,0x74u,4);
        abi.eflags=(abi.eflags&~arithmetic_flags)|flags.bits();
        return;
    }
    abi.gp[0]=p.r32(0x4ecc18u); abi.gp[1]=p.r32(abi.gp[0]+0xc4u);
    if (static_cast<i32>(abi.gp[1])>0) {
        abi.gp[1]=p.r32(abi.gp[0]+0xc8u);
        if (abi.gp[1]) {
            abi.gp[2]+=0x30u; abi.gp[0]=p.r32(abi.gp[2]);
            abi.gp[1]=p.r32(abi.gp[2]+4u); abi.gp[2]=p.r32(abi.gp[2]+8u);
            p.w32(0x275d45u,frame+0x18u,abi.gp[0]); abi.gp[0]=frame+0x18u;
            p.w32(0x275d4du,frame+0x1cu,abi.gp[1]); p.w32(0x275d51u,frame+0x20u,abi.gp[2]);
            push(p.memory,abi,abi.gp[0]); abi.gp[0]=p.r32(0x474d10u);
            abi.gp[1]=abi.gp[4]+0x28u; abi.gp[2]=abi.gp[5];
            p.w16(0x275d61u,abi.gp[4]+0x4cu,none); push(p.memory,abi,0x275d6bu);
            throw NativeCampaignAiPerformRequiredOwnerTu15(0x275d66u,0x276e10u);
        }
    }
    finish();
}
}
void native_campaign_ai_object_navigation_call_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t entry, SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsContactRecordObserver &observe,
    const NativeCampaignAiPerformCalleeTu15 &vehicle_point) {
    const Port port{memory,observe};
    if (entry==0x275ba0u) eligibility(port,abi);
    else if (entry==0x275cb0u) point(port,abi,vehicle_point);
    else if (entry==0xe2300u) biped_point(port,abi);
    else if (entry==0xe04b0u) biped_dynamic_sector(port,abi);
    else if (entry==0xe22c0u) biped_reset(port,abi);
    else if (entry==0x2186f0u) framed_point(port,abi);
    else throw std::invalid_argument("Native object navigation requires a qualified navigation owner");
}
}
