#include "halo2/native_campaign_ai_route_obstacle_geometry_tu15.hpp"
#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "native_campaign_ai_goal_port_tu15.hpp"
#include "physics/runtime/native_tu15_host_fp_io.hpp"
namespace halo2 {
namespace {
using namespace goal_tu15;
struct Geometry {
    Port p;SourcePhysicsWorldRuntimeAbi &a;
    void push(u32 value) {
        std::array<std::byte,4> bytes{};
        for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8*n));
        const auto next=a.gp[4]-4; p.memory.write(next,bytes);a.gp[4]=next;
    }
    void al(u32 value){a.gp[0]=(a.gp[0]&0xffffff00u)|(value&255u);}
    void marker(u32 site) {
        push(site+5);
        // Only the existing native marker closure is admitted. This service
        // selects compiled algorithms; it cannot execute an original image.
        SourcePhysicsWorldRuntimeServices services;
        services.required_owner=[&](auto &memory,const auto &call,auto &state,const auto &observe) {
            switch(call.target) {
            case 0xbacd0:case 0xfc510:case 0x1e11b0:case 0x1e1200:
            case 0x144910:case 0x145560:
                native_weapon_effect_dispatch_tu15_run(memory,call.target,state,services,observe);return;
            default:throw std::runtime_error("Native obstacle marker requires an unowned child");
            }
        };
        // The marker provider returns its source FP outcome in a. Park the
        // independent C++ host environment across its complete ABI boundary.
        native_host_fp_io_detail::preserve([&] {
            native_weapon_effect_dispatch_tu15_run(p.memory,0xbad60,a,services,p.observe);
        });
    }
    void markers(u32 entry) {
        const auto sp=a.gp[4];const auto saved=a.gp;
        push(a.gp[3]);push(a.gp[5]);a.gp[5]=p.r32(a.gp[4]+0x10);
        push(a.gp[6]);push(a.gp[7]);a.gp[7]=a.gp[0];
        if(entry==0x279b30)a.gp[0]=p.r32(a.gp[4]+0x1c);else al(p.r8(a.gp[4]+0x1c));
        a.gp[6]=0;
        const auto first=entry==0x279640?1u:entry==0x279900?0x20u:0x800u;
        const auto second=entry==0x279640?2u:entry==0x279900?0x40u:0x1000u;
        if(a.gp[0]&first) {
            a.gp[0]=p.r32(a.gp[4]+0x14);push(a.gp[7]);
            push(entry==0x279640?0x0f000526u:entry==0x279900?0x0f00052au:0x1100052fu);
            push(a.gp[0]);a.gp[1]&=0xffffff00u;a.gp[3]=a.gp[5];
            marker(entry==0x279640?0x279663:entry==0x279900?0x279923:0x279b54);
            a.gp[6]=a.gp[0];
        }
        if(sx16(a.gp[6])<sx16(a.gp[7])) {
            u32 flags;
            if(entry==0x279b30){a.gp[0]=p.r32(a.gp[4]+0x1c);flags=a.gp[0];}
            else flags=p.r8(a.gp[4]+0x1c);
            if(flags&second) {
                const auto object=p.r32(a.gp[4]+0x14);
                a.gp[3]=u32(sx16(a.gp[6]))*0x70u;
                if(entry==0x279900) {
                    a.gp[2]=object;a.gp[1]=a.gp[7]-a.gp[6];push(a.gp[1]);push(0x1100052bu);
                    a.gp[3]+=a.gp[5];push(a.gp[2]);a.gp[1]&=0xffffff00u;marker(0x27994f);
                } else {
                    a.gp[1]=object;a.gp[7]-=a.gp[6];push(a.gp[7]);
                    push(entry==0x279640?0x11000527u:0x10000530u);push(a.gp[1]);a.gp[3]+=a.gp[5];
                    a.gp[1]&=0xffffff00u;marker(entry==0x279640?0x27968d:0x279b80);
                }
                a.gp[6]+=a.gp[0];
            }
            if(entry==0x279900 && sx16(a.gp[6])<sx16(a.gp[7])) {
                al(p.r8(a.gp[4]+0x1c));
                if(a.gp[0]&0x80u) {
                    a.gp[0]=p.r32(a.gp[4]+0x14);a.gp[3]=u32(sx16(a.gp[6]))*0x70u;
                    a.gp[7]-=a.gp[6];push(a.gp[7]);push(0x1000052cu);a.gp[3]+=a.gp[5];
                    push(a.gp[0]);a.gp[1]&=0xffffff00u;marker(0x27997a);a.gp[6]+=a.gp[0];
                }
            }
        }
        a.gp[0]=(a.gp[0]&0xffff0000u)|(a.gp[6]&65535u);
        for(unsigned n:{3u,5u,6u,7u})a.gp[n]=saved[n];a.gp[4]=sp+16;
    }
    void append() {
        const auto sp=a.gp[4];a.gp[1]=(a.gp[1]&0xffff0000u)|p.r16(a.gp[2]+2);
        if(u16(a.gp[1])==64){al(0);a.gp[4]=sp+12;return;}
        a.gp[0]=u32(sx16(a.gp[1]));++a.gp[1];a.gp[0]=a.gp[2]+a.gp[0]*20+12;
        p.w16(0x2cab9d,a.gp[2]+2,a.gp[1]);
        if(a.gp[3]&1)p.w16(0x2caba3,a.gp[2]+4,p.r16(a.gp[2]+4)+1);
        if(a.gp[3]&8)p.w16(0x2cabac,a.gp[2]+6,p.r16(a.gp[2]+6)+1);
        a.gp[1]=p.r32(sp+4);p.w16(0x2cabb4,a.gp[0],a.gp[3]);p.w16(0x2cabb7,a.gp[0]+2,0xffff);
        p.w32(0x2cabbd,a.gp[0]+4,a.gp[1]);a.xmm[0]={p.r32(a.gp[6]+4),0,0,0};a.gp[2]=p.r32(a.gp[6]);
        p.w32(0x2cabc7,a.gp[0]+12,a.xmm[0][0]);a.xmm[0]={p.r32(sp+8),0,0,0};
        p.w32(0x2cabd2,a.gp[0]+8,a.gp[2]);p.w32(0x2cabd5,a.gp[0]+16,a.xmm[0][0]);al(1);a.gp[4]=sp+12;
    }
    void orientation() {
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
        if(!a.floating_point)throw std::invalid_argument("Native obstacle orientation requires FP controls");
        NativeTu15PhysicsFrameFp fp{a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr};
        struct Publish{SourcePhysicsWorldRuntimeAbi&a;NativeTu15PhysicsFrameFp&fp;~Publish(){a.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};a.x87_status=fp.status;}}publish{a,fp};
        Simd s(fp);a.gp[2]=p.r32(0x474d10);a.xmm[0]={p.r32(a.gp[2]+8),0,0,0};
        a.xmm[0][0]=s.mul(a.xmm[0][0],p.r32(a.gp[1]+0x44));a.xmm[1]={p.r32(a.gp[2]+4),0,0,0};
        a.xmm[1][0]=s.mul(a.xmm[1][0],p.r32(a.gp[1]+0x40));a.xmm[0][0]=s.add(a.xmm[0][0],a.xmm[1][0]);
        a.xmm[1]={p.r32(a.gp[1]+0x3c),0,0,0};a.xmm[1][0]=s.mul(a.xmm[1][0],p.r32(a.gp[2]));
        al(0);a.xmm[0][0]=s.add(a.xmm[0][0],a.xmm[1][0]);if(s.above(a.xmm[0][0],p.r32(0x46a2bc)))al(1);a.gp[4]+=4;
#else
        throw std::runtime_error("Native obstacle orientation requires an x86 arithmetic backend");
#endif
    }
};
}
void native_campaign_ai_route_obstacle_geometry_call_tu15(SourcePhysicsContactMemory &memory,u32 entry,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    Geometry g{{memory,observe},abi};
    switch(entry){case 0x279640:case 0x279900:case 0x279b30:g.markers(entry);return;
    case 0x279600:g.orientation();return;case 0x2cab80:g.append();return;
    default:throw std::invalid_argument("Unknown native obstacle geometry owner");}
}
}
