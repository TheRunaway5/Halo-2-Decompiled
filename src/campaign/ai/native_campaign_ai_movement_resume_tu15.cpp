#include "halo2/native_campaign_ai_movement_resume_tu15.hpp"
#include "halo2/native_campaign_ai_character_property_tu15.hpp"
#include "halo2/native_campaign_ai_point_geometry_tu15.hpp"
#include <array>
#include <bit>
#include <vector>

namespace halo2 {
namespace {
using u32=std::uint32_t;using u8=std::uint8_t;
struct Resume {
    SourcePhysicsContactMemory &memory;SourcePhysicsWorldRuntimeAbi &abi;
    const SourcePhysicsContactRecordObserver &observe;
    u32 r(u32 at,unsigned width=4)const {std::array<std::byte,4>b{};memory.read(at,std::span(b).first(width));u32 v{};for(unsigned i=0;i<width;++i)v|=std::to_integer<u32>(b[i])<<(i*8u);return v;}
    void w(u32 pc,u32 at,u32 v,unsigned width=4)const {SourcePhysicsContactRecordStore e{pc,at,{},std::vector<std::byte>(width)};if(observe){e.before.resize(width);memory.read(at,e.before);}for(unsigned i=0;i<width;++i)e.written[i]=std::byte(v>>(i*8u));memory.write(at,e.written);if(observe)observe(e);}
    void push(u32 &sp,u32 value)const {sp-=4u;std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(i*8u));memory.write(sp,b);}
    u32 actor(u32 id)const{return r(r(0x5020c8u)+0x44u)+(id&0xffffu)*0x888u;}
    u32 op(u32 left,u32 right,char operation) {
        auto a=std::bit_cast<float>(left);const auto b=std::bit_cast<float>(right);u32 host{},csr=abi.floating_point->mxcsr;
        asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(host):"m"(csr):"memory");
        if(operation=='+')asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");
        else if(operation=='-')asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");
        else asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");
        asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(csr):"m"(host):"memory");abi.floating_point->mxcsr=csr;return std::bit_cast<u32>(a);
    }
    // Source COMISS (including unordered flags) and UCOMISS equality.
    bool cmp(u32 left,u32 right,char condition) {
        const auto a=std::bit_cast<float>(left),b=std::bit_cast<float>(right);u8 above{},below{},equal{},ordered{};u32 host{},csr=abi.floating_point->mxcsr;
        asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(host):"m"(csr):"memory");
        if(condition=='=')asm volatile("ucomiss %3,%2\n\tsete %0\n\tsetnp %1":"=qm"(equal),"=qm"(ordered):"x"(a),"x"(b):"cc","memory");
        else asm volatile("comiss %3,%2\n\tseta %0\n\tsetb %1":"=qm"(above),"=qm"(below):"x"(a),"x"(b):"cc","memory");
        asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(csr):"m"(host):"memory");abi.floating_point->mxcsr=csr;
        return condition=='='?equal&&ordered:condition=='>'?above:below;
    }
    void project(u32 point,u32 output) {
        NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
        const auto publish=[&]{abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;};
        try{native_campaign_ai_project_point_tu15(memory,point,output,fp,observe);}catch(...){publish();throw;}publish();
    }
    u32 property(u32 site,u32 entry,u32 actor_id,u32 definition,u32 sp) {
        auto child=abi;child.gp[1]=actor_id;if(entry==0x1ed700u)push(sp,definition);push(sp,site+5u);child.gp[4]=sp;
        native_campaign_ai_character_property_call_tu15(memory,entry,child);return child.gp[0];
    }
    u32 radius(const std::array<u32,8>&caller) {
        const auto ar=actor(caller[1]);auto sp=caller[4];push(sp,caller[6]);push(sp,caller[7]);auto value=r(ar+0x4ccu);
        if(cmp(value,0,'=')) {
            const auto unit=r(ar+0x18u),headers=r(r(0x4ecbd0u)+0x44u),body=r(headers+(unit&0xffffu)*12u+8u),vehicle=r(ar+0x26cu);
            if(vehicle!=0xffffffffu) {
                const auto other=r(headers+(vehicle&0xffffu)*12u+8u),p=property(0x1ebc35u,0x1ed700u,caller[1],r(other),sp);
                if(p)value=r(p+0x18u);if(!cmp(0,value,'<'))return r(0x46a0ccu);
            } else {
                bool resolved=false;
                if(r(ar+0x229u,1)) {const auto p=property(0x1ebc60u,0x1ed700u,caller[1],r(body),sp);if(p){value=r(p+0x18u);resolved=true;}}
                if(!resolved){const auto p=property(0x1ebc81u,0x1ecd00u,r(ar+0x54u),0,sp);if(p)value=r(p+8u);}
                if(!cmp(0,value,'<'))return r(0x456918u);
            }
        }
        const auto minimum=r(0x46a16cu);return cmp(value,minimum,'>')?value:minimum;
    }
    void difference_world(u32 framed,u32 world,u32 out,u32 scratch) {
        auto source=framed;const bool projected=r(framed+0xcu,2)!=0xffffu;
        if(projected){project(framed,scratch);source=scratch;}
        const auto pcs=projected?std::array{0x218fafu,0x218fbeu,0x218fceu}:std::array{0x218f75u,0x218f83u,0x218f92u};
        for(unsigned i=0;i<3;++i)w(pcs[i],out+i*4u,op(r(source+i*4u),r(world+i*4u),'-'));
    }
    void difference(u32 first,u32 second,u32 out,u32 scratch) {
        const auto frame=r(first+0xcu,2);const bool same=frame==r(second+0xcu,2);auto a=first,b=second;
        if(!same){project(first,scratch);project(second,scratch+0x10u);a=scratch;b=scratch+0x10u;}
        const auto pcs=same?std::array{0x218ecbu,0x218ed9u,0x218eeau}:std::array{0x218f26u,0x218f36u,0x218f47u};
        for(unsigned i=0;i<3;++i)w(pcs[i],out+i*4u,op(r(b+i*4u),r(a+i*4u),'-'));
        if(same && frame!=0xffffu)throw NativeCampaignAiPerformRequiredOwnerTu15(0x218efbu,0x218880u);
    }
    bool reached(const std::array<u32,8>&caller) {
        const auto ar=actor(caller[0]),goal=ar+0x4ecu,position=ar+0x238u;auto sp=caller[4]-0x1cu;
        for(unsigned i:{3u,5u,6u,7u})push(sp,caller[i]);const auto f=sp;
        auto point=goal;if(r(ar+0x4f8u,2)!=0xffffu){project(goal,f+0x14u);point=f+0x14u;}
        const auto dx=op(r(position),r(point),'-'),dy=op(r(position+4u),r(point+4u),'-'),dz=op(r(position+8u),r(point+8u),'-');
        const auto vehicle=r(ar+0x26cu);const auto yy=op(dy,dy,'*'),zz=op(dz,dz,'*'),zy=op(zz,yy,'+'),xx=op(dx,dx,'*'),distance=op(zy,xx,'+');
        w(0x1fb595u,f+0x13u,0,1);
        if(vehicle==0xffffffffu && r(ar+0x4aeu,1)) {
            const auto count=std::bit_cast<std::int8_t>(u8(r(ar+0x539u,1))),current=std::bit_cast<std::int8_t>(u8(r(ar+0x53au,1)));
            if(int(current)!=int(count)-1)return false;const auto previous=current-1>=0?ar+0x52cu+u32(current)*0x1cu:ar+0x528u;
            difference(previous,goal,f+0x14u,f-0x60u);difference_world(goal,position,f+0x20u,f-0x20u);
            const auto z=op(r(f+0x28u),r(f+0x1cu),'*'),y=op(r(f+0x24u),r(f+0x18u),'*'),zydot=op(z,y,'+');
            const auto x=op(r(f+0x14u),r(f+0x20u),'*');return cmp(0,op(zydot,x,'+'),'>');
        }
        auto radius=r(caller[4]+4u);if(cmp(op(radius,radius,'*'),distance,'>'))return true;
        if(!r(ar+0x4aeu,1))return false;radius=op(radius,r(0x46a0ccu),'+');if(!cmp(op(radius,radius,'*'),distance,'>'))return false;
        const auto unit=r(ar+0x18u),body=r(r(r(0x4ecbd0u)+0x44u)+(unit&0xffffu)*12u+8u);
        difference_world(goal,position,f+0x20u,f-0x20u);
        const auto y=op(r(body+0x8cu),r(f+0x24u),'*'),z=op(r(body+0x90u),r(f+0x28u),'*'),yz=op(y,z,'+'),x=op(r(body+0x88u),r(f+0x20u),'*');
        return cmp(0,op(yz,x,'+'),'>');
    }
    void resume(const std::array<u32,8>&caller) {
        const auto ar=actor(caller[0]);auto sp=caller[4]-0x24u;for(unsigned i:{5u,6u,7u})push(sp,caller[i]);
        const auto active=r(ar+0x227u,1);w(0x200a6bu,ar+0x50cu,1,1);w(0x200a72u,ar+0x504u,2,2);
        if(!active){const auto mode=r(ar+0x4acu,2);if(mode==4u || mode==5u || mode==6u)throw NativeCampaignAiPerformRequiredOwnerTu15(0x200aa1u,0x26c2a0u);
            if(r(ar+0x4e8u,1))w(0x200aeau,ar+0x227u,1,1);}
        if(!r(caller[4]+4u,1))return;
        const auto unit=r(ar+0x18u),header=r(r(0x4ecbd0u)+0x44u)+(unit&0xffffu)*12u;
        if(r(header+3u,1) || r(r(header+8u)+0x3dcu,1)!=2u)return;
        const auto mode=r(ar+0x4acu,2);if(mode==6u)throw NativeCampaignAiPerformRequiredOwnerTu15(0x200b56u,0x26c7a0u);
        if(mode==4u){const auto p=property(0x200b81u,0x1ecc40u,r(ar+0x54u),0,sp);if(!p || (r(p,1)&2u))return;throw NativeCampaignAiPerformRequiredOwnerTu15(0x200b96u,0x26c850u);}
    }
};
}
void native_campaign_ai_movement_resume_call_tu15(SourcePhysicsContactMemory &memory,u32 entry,SourcePhysicsWorldRuntimeAbi &abi,
    const NativeCampaignAiPerformServicesTu15 &,const SourcePhysicsContactRecordObserver &observe) {
    if(!abi.floating_point || abi.x87_depth || abi.active_mmx || (abi.floating_point->mxcsr&0x1f80u)!=0x1f80u || (abi.floating_point->x87_control_word&0x3fu)!=0x3fu)
        throw std::invalid_argument("Native movement resume requires empty masked retained FP state");
    const auto caller=abi.gp;Resume m{memory,abi,observe};u32 cleanup=4u;
    switch(entry){
    case 0x1ebbd0u:abi.xmm[0][0]=m.radius(caller);break;
    case 0x1fb4f0u:abi.gp[0]=(caller[0]&0xffffff00u)|u32(m.reached(caller));cleanup=8u;break;
    case 0x200a40u:m.resume(caller);cleanup=8u;break;
    case 0x218eb0u:m.difference(caller[1],caller[3],caller[0],caller[4]-0x80u);break;
    case 0x218f60u:m.difference_world(caller[0],caller[3],caller[7],caller[4]-0x40u);break;
    default:throw std::invalid_argument("Native movement resume has no such entry");}
    abi.gp[4]=caller[4]+cleanup;
}
}
