#include "halo2/native_campaign_ai_command_movement_tu15.hpp"
#include "halo2/native_campaign_ai_command_lifecycle_tu15.hpp"
#include "halo2/native_campaign_ai_publication_tu15.hpp"
#include "halo2/native_campaign_ai_movement_resume_tu15.hpp"
#include "halo2/native_campaign_ai_vector_math_tu15.hpp"
#include <array>
#include <bit>
#include <vector>

namespace halo2 {
namespace {
using u32=std::uint32_t;using u8=std::uint8_t;
constexpr u32 none=0xffffffffu;
int signed_word(u32 n){return std::bit_cast<std::int16_t>(std::uint16_t(n));}
struct Movement {
    SourcePhysicsContactMemory &memory;SourcePhysicsWorldRuntimeAbi &abi;
    const NativeCampaignAiPerformServicesTu15 &services;
    const SourcePhysicsContactRecordObserver &observe;
    u32 r(u32 address,unsigned size=4)const {
        std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(size));u32 value{};
        for(unsigned n=0;n<size;++n)value|=std::to_integer<u32>(bytes[n])<<(n*8u);return value;
    }
    void w(u32 pc,u32 address,u32 value,unsigned size=4)const {
        SourcePhysicsContactRecordStore event{pc,address,{},std::vector<std::byte>(size)};
        if(observe){event.before.resize(size);memory.read(address,event.before);}
        for(unsigned n=0;n<size;++n)event.written[n]=std::byte(value>>(n*8u));
        memory.write(address,event.written);if(observe)observe(event);
    }
    void push(u32 &sp,u32 value)const {
        sp-=4u;std::array<std::byte,4> bytes{};
        for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8u));memory.write(sp,bytes);
    }
    u32 actor_row(u32 actor)const{return r(r(0x5020c8u)+0x44u)+(actor&0xffffu)*0x888u;}
    bool equal_zero(u32 value) {
        auto csr=abi.floating_point->mxcsr;u32 host{};u8 equal{},ordered{};
        const auto a=std::bit_cast<float>(value),b=std::bit_cast<float>(r(0x46a0e8u));
        asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\tucomiss %[b],%[a]\n\tsete %[equal]\n\tsetnp %[ordered]\n\tstmxcsr %[csr]\n\tldmxcsr %[host]"
            :[host]"+m"(host),[csr]"+m"(csr),[equal]"=&q"(equal),[ordered]"=&q"(ordered)
            :[a]"x"(a),[b]"x"(b):"cc","memory");
        abi.floating_point->mxcsr=csr;return equal&&ordered;
    }
    u32 scalar(u32 left,u32 right,char operation) {
        auto a=std::bit_cast<float>(left);const auto b=std::bit_cast<float>(right);u32 host{},csr=abi.floating_point->mxcsr;
        asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(host):"m"(csr):"memory");
        if(operation=='+')asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");
        else asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");
        asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(csr):"m"(host):"memory");abi.floating_point->mxcsr=csr;return std::bit_cast<u32>(a);
    }
    bool above(u32 left,u32 right) {
        const auto a=std::bit_cast<float>(left),b=std::bit_cast<float>(right);u32 host{},csr=abi.floating_point->mxcsr;u8 result{};
        asm volatile("stmxcsr %0\n\tldmxcsr %1\n\tcomiss %4,%3\n\tseta %2\n\tstmxcsr %1\n\tldmxcsr %0"
            :"=m"(host),"+m"(csr),"=qm"(result):"x"(a),"x"(b):"memory","cc");abi.floating_point->mxcsr=csr;return result;
    }
    u32 normalize(u32 vector,bool planar) {
        NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
        const auto publish=[&]{abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;};
        u32 value{};try{value=planar?native_campaign_ai_normalize_vector2_tu15(memory,vector,fp,observe):native_campaign_ai_normalize_vector_tu15(memory,vector,fp,observe);}catch(...){publish();throw;}publish();return value;
    }
    void child(u32 site,u32 entry,std::array<u32,8> gp,u32 cleanup) {
        const auto sp=gp[4];push(gp[4],site+5u);abi.gp=gp;
        if(entry==0x1fc720u || entry==0x1fc540u)
            native_campaign_ai_publication_call_tu15(memory,entry,abi,services,observe);
        else if(entry==0x218eb0u)native_campaign_ai_movement_resume_call_tu15(memory,entry,abi,services,observe);
        else if(entry==0x263b80u)native_campaign_ai_command_reset_tu15(memory,gp[0],observe),abi.gp[4]+=4u;
        else throw NativeCampaignAiPerformRequiredOwnerTu15(site,entry);
        if(abi.gp[4]!=sp+cleanup)throw std::runtime_error("Native command movement child returned another stack");
    }
    bool valid(u32 point) {
        if(point==none)return false;
        const auto scenario=r(0x4ecc20u);
        if(std::bit_cast<std::int32_t>(r(scenario+0x1d8u))<=0)return false;
        const auto set=(point>>16)&0xffffu,block=r(scenario+0x1dcu);
        if(std::int32_t(set)>=std::bit_cast<std::int32_t>(r(block)))return false;
        const auto rows=r(block+4u),index=point&0xffffu;
        return std::int32_t(index)<std::bit_cast<std::int32_t>(r(rows+set*0x30u+0x20u));
    }
    u8 goal(u32 entry,const std::array<u32,8> &caller) {
        auto sp=entry-8u;const auto commands=r(0x50eee0u);push(sp,caller[3]);
        const auto actor=caller[1],actors=r(0x5020c8u);push(sp,caller[5]);push(sp,caller[6]);
        const auto scenario=r(0x4ecc20u),block=r(scenario+0x1dcu),sets=r(block+4u);
        push(sp,caller[7]);const auto f=sp,ar=r(actors+0x44u)+(actor&0xffffu)*0x888u;
        const auto cr=r(commands+0x44u)+(caller[0]&0xffffu)*0xd4u;
        const auto reference=r(cr+0x28u),point=r(sets+(reference>>16)*0x30u+0x24u)+(reference&0xffffu)*0x3cu;
        const auto unit=r(ar+0x18u);w(0x263c44u,f+0x13u,1,1);
        if(unit==none)return u8(r(f+0x13u,1));
        const auto kind=r(cr+4u,2);
        if(kind!=1u && kind!=3u)throw std::invalid_argument("Native command goal requires command kind 1 or 3");
        w(0x263c5cu,f+0x14u,0,1);const auto ignore=r(cr+0x75u,1);w(0x263c6au,ar+0x478u,ignore,1);
        const auto nav_reference=r(point+0x30u);
        if(nav_reference==none){w(0x263d9fu,f+0x13u,0,1);return u8(r(f+0x13u,1));}
        const auto radius=r(cr+8u);const u8 pass=equal_zero(radius)?0:1;
        w(0x263d62u,f+0x14u,r(f+0x14u,1)|pass,1);
        const auto flag=r(f+0x14u);auto call_sp=f;push(call_sp,flag);push(call_sp,none);push(call_sp,nav_reference);push(call_sp,point+0x20u);
        child(0x263d72u,0x1fc720u,{point+0x20u,nav_reference,flag,actor,call_sp,point,ar,cr},16u);
        const auto result=u8(abi.gp[0]);w(0x263d79u,f+0x13u,result,1);
        if(result && r(f+0x14u,1)){const auto margin=r(cr+8u);w(0x263d92u,ar+0x4b0u,margin);}
        if(result && kind==3u) {
            const auto face=r(cr+0x2cu),scene=r(0x4ecc20u),points=r(r(scene+0x1dcu)+4u);
            const auto target=r(points+(face>>16)*0x30u+0x24u)+(face&0xffffu)*0x3cu+0x20u;
            child(0x263decu,0x218eb0u,{ar+0x4d8u,point+0x20u,ar+0x4d8u,target,f,point,ar,points},0);
            {auto p=f;push(p,0x263dfcu);}const auto magnitude=normalize(ar+0x4d8u,false);
            if(!equal_zero(magnitude))w(0x263e09u,ar+0x4d5u,1,1);
        }
        return u8(r(f+0x13u,1));
    }
    u32 initialize(u32 entry,const std::array<u32,8> &caller) {
        const auto actors=r(0x5020c8u);auto sp=entry;push(sp,caller[3]);const auto commands=r(0x50eee0u);push(sp,caller[5]);
        const auto actor=r(entry+4u),rows=r(commands+0x44u);push(sp,caller[6]);push(sp,caller[7]);
        const auto ar=r(actors+0x44u)+(actor&0xffffu)*0x888u,command=r(entry+0x10u),cr=rows+(command&0xffffu)*0xd4u;
        const auto swarm=r(ar+0x1cu),kind=r(cr+4u,2);
        if(kind!=1u && kind!=3u)throw std::invalid_argument("Native command initializer requires command kind 1 or 3");
        if(swarm!=none && kind==3u)return 2;
        child(0x262a70u,0x263b80u,{command,0,none,ar,sp,actor,caller[6],cr},0);
        const auto state=r(entry+0xcu);
        if(kind==3u){w(0x262aebu,state,0,2);const auto face=r(cr+0x2cu);{auto p=sp;push(p,0x262af8u);}if(!valid(face))return 2;}
        const auto point=r(cr+0x28u);
        {auto p=sp;push(p,0x262b08u);}if(!valid(point))return 2;
        if(r(ar+0x18u)!=none) {
            const auto id=r(entry+0x10u);auto p=sp;push(p,0x262b21u);
            const auto ok=goal(p,{id,actor,point,ar,p,actor,state,cr});
            if(ok){w(0x262ad5u,cr+0x77u,1,1);return 0;}
            if(r(cr+0x76u,1)){w(0x262b2cu,cr+0x77u,0,1);return 0;}return 2;
        }
        const auto ref=r(cr+0x28u),scenario=r(0x4ecc20u),block=r(scenario+0x1dcu),sets=r(block+4u);
        const auto point_row=r(sets+(ref>>16)*0x30u+0x24u)+(ref&0xffffu)*0x3cu+0x20u;
        for(unsigned n=0;n<3;++n)w(std::array{0x262b72u,0x262b77u,0x262b7du}[n],state+8u+n*4u,r(point_row+n*4u));
        const auto flags=r(state+3u,1),frame=r(point_row+0xcu);w(0x262b89u,state+3u,flags|0x40u,1);w(0x262b8eu,state+0x14u,frame);return 0;
    }
    bool status(u32 actor,u32 wanted)const{return r(actor_row(actor)+0x504u,2)==wanted;}
    bool moving(u32 actor)const {const auto ar=actor_row(actor);return r(ar+0x50cu,1) && r(ar+0x504u,2)==1u;}
    u32 finish(u32 entry,const std::array<u32,8> &caller) {
        const auto actor=r(entry+4u),pool=r(0x5020c8u);auto sp=entry;push(sp,caller[6]);
        const auto rows=r(pool+0x44u),command=r(entry+0x10u),ar=rows+(actor&0xffffu)*0x888u;
        const auto commands=r(0x50eee0u),command_rows=r(commands+0x44u),kind=r(command_rows+(command&0xffffu)*0xd4u+4u,2);sp+=4u;
        if(kind!=1u && kind!=3u)throw std::invalid_argument("Native command cleanup requires command kind 1 or 3");
        if(r(ar+0x18u)!=none){push(sp,actor);child(0x263af7u,0x1fc540u,{ar,0,actor,caller[3],sp,caller[5],caller[6],caller[7]},4u);}
        else{const auto state=r(entry+0xcu);w(0x263b07u,state+3u,r(state+3u,1)&0xbfu,1);}
        return 1;
    }
    u32 update(u32 entry,const std::array<u32,8> &caller) {
        auto sp=entry-0x1cu;const auto actors=r(0x5020c8u);push(sp,caller[3]);const auto actor=r(entry+4u),commands=r(0x50eee0u);
        push(sp,caller[5]);const auto ar=r(actors+0x44u)+(actor&0xffffu)*0x888u,command=r(entry+0x10u);push(sp,caller[6]);
        const auto cr=r(commands+0x44u)+(command&0xffffu)*0xd4u,kind=r(cr+4u,2);push(sp,caller[7]);const auto f=sp;
        w(0x263186u,f+0x10u,ar);w(0x26318au,f+0x3cu,1,1);
        if(kind!=1u && kind!=3u)throw std::invalid_argument("Native command updater requires command kind 1 or 3");
        const auto state=r(f+0x38u);
        if(kind==1u && r(ar+0x1cu)!=none)child(0x2631b2u,0xbbe00u,{r(f+0x34u),1,0,ar,f,cr,f+0x10u,state},0);
        if(!r(cr+0x77u,1)) {
            const auto active=r(ar+0x40u,1);w(0x263309u,f+0x3cu,0,1);
            if(active) {
                auto p=f;push(p,0x263317u);const auto ok=goal(p,{command,actor,abi.gp[2],ar,p,cr,actor,state});
                if(ok)w(0x26331bu,cr+0x77u,1,1);
                else if(!r(cr+0x76u,1))w(0x263328u,f+0x3cu,1,1);
            }
        }
        if(!r(cr+0x77u,1)) {if(r(f+0x3cu,1))return 1;}
        else {
            {auto p=f;push(p,0x26333fu);}bool done=status(actor,2u);
            if(!done){{auto p=f;push(p,0x26334au);}done=status(actor,3u) && !r(cr+0x76u,1);}
            if(done) {
                if(r(ar+0x4d5u,1) && r(ar+0x270u,2)==0u) {
                    const auto y=r(ar+0x4dcu),x=r(ar+0x4d8u);w(0x263388u,f+0x14u,y);w(0x263390u,f+0x10u,x);
                    {auto p=f;push(p,0x263399u);}const auto magnitude=normalize(f+0x10u,true);
                    if(above(magnitude,r(0x46a0e8u))) {
                        const auto facing_y=r(ar+0x294u),desired_x=r(f+0x10u);
                        const auto xdot=scalar(desired_x,r(ar+0x290u),'*'),ydot=scalar(facing_y,r(f+0x14u),'*');
                        if(above(r(0x46aaccu),scalar(ydot,xdot,'+'))) {
                            const auto dx=r(ar+0x4d8u),dy=r(ar+0x4dcu);w(0x2633e5u,ar+0x424u,dx);const auto dz=r(ar+0x4e0u);
                            w(0x2633edu,ar+0x428u,dy);w(0x2633f0u,ar+0x41cu,3,2);w(0x2633f9u,ar+0x420u,4,2);w(0x263402u,ar+0x42cu,dz);w(0x263405u,ar+0x44du,1,1);done=false;
                        }
                    }
                }
                if(done){auto p=f;push(p,actor);child(0x263460u,0x1fc540u,{abi.gp[0],abi.gp[1],abi.gp[2],ar,p,cr,actor,state},4u);return 1;}
            }
            {auto p=f;push(p,0x263413u);}if(!status(actor,2u)) {
                {auto p=f;push(p,0x263424u);}if(!moving(actor)) {
                    {auto p=f;push(p,0x263435u);}if(!status(actor,3u))w(0x26343du,cr+0x77u,0,1);
                }
            }
        }
        auto count=r(state,2);if(signed_word(count)>0)w(0x2631e1u,state,count-1u,2);
        const auto flags=r(state+3u,1);
        if(flags&4u){count=r(state+8u,2);if(signed_word(count)>0)w(0x2631f8u,state+8u,count-1u,2);}
        if((flags&3u)==3u)child(0x263210u,0x263e40u,{r(f+0x30u),state,r(f+0x34u),ar,f,cr,actor,state},0);
        if(r(ar+7u,1))return 0;
        if(r(cr+0x8cu,1))child(0x26322eu,0x1fd3a0u,{r(f+0x30u),abi.gp[1],abi.gp[2],ar,f,cr,actor,state},0);
        if(r(state+3u,1)&1u) {
            w(0x26327bu,ar+0x456u,1,1);
            for(unsigned n=0;n<3;++n)w(std::array{0x26328du,0x263292u,0x263298u}[n],ar+0x458u+n*4u,r(state+0xcu+n*4u));
            w(0x26329fu,ar+0x454u,r(state+8u,2),2);
        }
        const auto look=r(state+3u,1);
        if(look&4u) {
            if(!(look&8u))child(0x26395eu,0x113190u,{abi.gp[0],abi.gp[1],abi.gp[2],ar,f,cr,actor,state},0);
            if(signed_word(r(state+8u,2))>0) {
                const auto x=r(ar+0x290u);w(0x2632cfu,ar+0x458u,x);const auto y=r(ar+0x294u),z=r(ar+0x298u);
                w(0x2632d7u,ar+0x45cu,y);w(0x2632dau,ar+0x456u,1,1);w(0x2632e1u,ar+0x460u,z);w(0x2632e4u,ar+0x454u,0,2);
            }
        }
        return 0;
    }
};
}
void native_campaign_ai_command_go_to_tu15(SourcePhysicsContactMemory &memory,u32 point,const SourcePhysicsContactRecordObserver &observe) {
    SourcePhysicsWorldRuntimeAbi abi;const NativeCampaignAiPerformServicesTu15 services;Movement m{memory,abi,services,observe};
    const auto command=m.r(0x50eee8u);if(command==none)return;
    const auto row=m.r(m.r(0x50eee0u)+0x44u)+(command&0xffffu)*0xd4u;
    m.w(0x2b0c3cu,row+4u,1,2);m.w(0x2b0c42u,row+0x28u,point);m.w(0x2b0c45u,row+8u,0);
}
void native_campaign_ai_command_movement_call_tu15(SourcePhysicsContactMemory &memory,u32 entry,SourcePhysicsWorldRuntimeAbi &abi,
    const NativeCampaignAiPerformServicesTu15 &services,const SourcePhysicsContactRecordObserver &observe) {
    if(!abi.floating_point || abi.x87_depth || abi.active_mmx)throw std::invalid_argument("Native command movement requires empty retained FP state");
    if((abi.floating_point->mxcsr&0x1f80u)!=0x1f80u || (abi.floating_point->x87_control_word&0x3fu)!=0x3fu)
        throw std::invalid_argument("Native command movement requires masked FP exceptions");
    const auto caller=abi.gp;const auto sp=caller[4];Movement m{memory,abi,services,observe};u32 result{},mask{},cleanup{};
    switch(entry) {
    case 0x264860u:result=m.valid(caller[2]);mask=0xffu;cleanup=4u;break;
    case 0x263bd0u:result=m.goal(sp,caller);mask=0xffu;cleanup=4u;break;
    case 0x262a00u:result=m.initialize(sp,caller);mask=0xffffu;cleanup=0x14u;break;
    case 0x263140u:result=m.update(sp,caller);mask=0xffffffffu;cleanup=0x14u;break;
    case 0x263aa0u:result=m.finish(sp,caller);mask=0xffffu;cleanup=0x14u;break;
    default:throw std::invalid_argument("Native command movement has no such entry");
    }
    abi.gp=caller;abi.gp[0]=(caller[0]&~mask)|(result&mask);abi.gp[4]=sp+cleanup;
}
}
