#include "halo2/native_campaign_ai_behavior_129_probe_tu15.hpp"
#include "halo2/native_campaign_ai_perception_refs_tu15.hpp"
#include "halo2/native_campaign_ai_vector_math_tu15.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"
#include "native_campaign_ai_behavior_129_obstacle_probe.hpp"
#include <bit>
#include <vector>
namespace halo2 {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace {
using U=std::uint32_t;using V=std::array<U,3>;constexpr U none=0xffffffffu;
struct Probe {
    SourcePhysicsContactMemory &memory;SourcePhysicsWorldRuntimeAbi &abi;
    const SourcePhysicsContactRecordObserver &observe;NativeTu15PhysicsFrameFp &fp;
    const NativeCampaignAiPerformServicesTu15 &services;
    U r(U at,unsigned width=4)const {
        std::array<std::byte,4> raw{};memory.read(at,std::span(raw).first(width));U value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<U>(raw[n])<<(n*8u);return value;
    }
    void w(U pc,U at,U value)const {
        const bool watched=native_tu15_observes_retained_store(memory,observe,pc,at,4);
        SourcePhysicsContactRecordStore event{pc,at,{},std::vector<std::byte>(4)};
        if(watched){event.before.resize(4);memory.read(at,event.before);}
        for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(n*8u));
        memory.write(at,event.written);if(watched)observe(event);
    }
    V point(U at)const{return {r(at),r(at+4u),r(at+8u)};}
    void point(U at,const V &value)const{for(unsigned n=0;n<3;++n)w(0,at+n*4u,value[n]);}
    template<class F>U math(U a,U b,F operation) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        U host{},csr=fp.mxcsr;asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(host):"m"(csr):"memory");
        auto x=std::bit_cast<float>(a);operation(x,std::bit_cast<float>(b));
        asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(fp.mxcsr):"m"(host):"memory");return std::bit_cast<U>(x);
#else
        throw std::runtime_error("Native behavior probes require an x86 scalar FP owner");
#endif
    }
    U add(U a,U b){return math(a,b,[](float &x,float y){asm volatile("addss %1,%0":"+x"(x):"x"(y):"memory");});}
    U sub(U a,U b){return math(a,b,[](float &x,float y){asm volatile("subss %1,%0":"+x"(x):"x"(y):"memory");});}
    U mul(U a,U b){return math(a,b,[](float &x,float y){asm volatile("mulss %1,%0":"+x"(x):"x"(y):"memory");});}
    bool compare(U a,U b,bool ordered_above) {
        U host{},csr=fp.mxcsr;std::uint8_t answer{},ordered{};const auto x=std::bit_cast<float>(a),y=std::bit_cast<float>(b);
        if(ordered_above)asm volatile("stmxcsr %[h]\n\tldmxcsr %[c]\n\tcomiss %[y],%[x]\n\tseta %[a]\n\tstmxcsr %[c]\n\tldmxcsr %[h]"
            :[h]"+m"(host),[c]"+m"(csr),[a]"=&q"(answer):[x]"x"(x),[y]"x"(y):"memory","cc");
        else {
            asm volatile("stmxcsr %[h]\n\tldmxcsr %[c]\n\tucomiss %[y],%[x]\n\tsete %[a]\n\tsetnp %[o]\n\tstmxcsr %[c]\n\tldmxcsr %[h]"
                :[h]"+m"(host),[c]"+m"(csr),[a]"=&q"(answer),[o]"=&q"(ordered):[x]"x"(x),[y]"x"(y):"memory","cc");
            answer=answer&&ordered;
        }
        fp.mxcsr=csr;return answer;
    }
    [[noreturn]] void missing(U site,U actor,U direction,U request,U sp) {
        w(0,sp-4u,request);w(0,sp-8u,direction);w(0,sp-12u,actor);w(0,sp-16u,site+5u);
        abi.gp[4]=sp-16u;throw NativeCampaignAiPerformRequiredOwnerTu15(site,0x1ca560u);
    }
    bool query(U site,U first_push,U second_push,U third_push,U actor,U direction,U request,U sp) {
        if(!services.behavior_direction_geometry)missing(site,actor,direction,request,sp);
        w(first_push,sp-4u,request);w(second_push,sp-8u,direction);
        w(third_push,sp-12u,actor);w(site,sp-16u,site+5u);
        abi.gp[4]=sp-16u;
        behavior_129_direction::ObstacleProbe(memory,abi,fp,services,observe).run();
        if(abi.gp[4]!=sp)throw std::runtime_error("Native behavior129 probe returned another stack");
        return (abi.gp[0]&0xffu)!=0;
    }
    void nearest(const std::array<U,8>&g) {
        const auto actor=r(g[4]+4u),offset=(actor&0xffffu)*0x888u;auto best_distance=r(0x45695cu);
        const auto properties=r(0x50eef0u),body=r(r(0x5020c8u)+0x44u)+offset;
        U current=r(body+0x58u),best=none;
        while(current!=none) {
            const auto row=r(properties+0x44u)+(current&0xffffu)*0x3cu,target=current,tracking=r(row+0x14u);
            current=r(row+0x2cu);
            if(tracking==none || std::bit_cast<std::int8_t>(std::uint8_t(r(row+0x27u,1)))<2)continue;
            if(compare(best_distance,r(row+0x28u),true)){best_distance=r(row+0x28u);best=target;}
        }
        if(best==none){abi.gp[0]=0xffffff00u;abi.gp[4]=g[4]+12u;return;}
        const auto rows=r(properties+0x44u),record=rows+(best&0xffffu)*0x3cu;
        const auto position=native_campaign_ai_perception_position_tu15(memory,record)+4u;
        const auto f=g[4]-0x18u,direction=f+0xcu;
        w(0x1ca99eu,direction,sub(r(position),r(body+0x238u)));
        w(0x1ca9b1u,direction+4u,sub(r(position+4u),r(body+0x23cu)));w(0x1ca9beu,direction+8u,0);
        const auto length=native_campaign_ai_normalize_vector_tu15(memory,direction,fp,observe);
        const bool fallback=compare(length,r(0x46a0e8u),false);
        if(fallback)point(direction,point(body+0x290u));
        const auto request=r(f+0x20u),actor_argument=r(f+0x1cu);
        abi.gp={request,direction,actor_argument,body+(fallback?0x290u:0u),f,properties,rows,g[7]};
        if(services.behavior_direction_geometry) {
            w(0x1ca918u,g[4]-16u,g[3]);
            w(0x1ca91fu,g[4]-20u,g[5]);
            w(0x1ca926u,g[4]-24u,g[6]);
        }
        query(0x1ca9ffu,0x1ca9f8u,0x1ca9fdu,0x1ca9feu,actor_argument,direction,request,f);
        abi.gp[6]=r(f);abi.gp[5]=r(f+4u);abi.gp[3]=r(f+8u);
        abi.gp[0]=(abi.gp[0]&0xffffff00u)|1u;
        (void)r(g[4]);abi.gp[4]=g[4]+12u;
    }
    void fan(const std::array<U,8>&g) {
        const auto f=g[4]-0x84u,body=r(r(0x5020c8u)+0x44u)+(g[7]&0xffffu)*0x888u;
        auto facing=point(body+0x290u);
        if(!compare(facing[2],0,false)) {
            facing[2]=0;point(f,facing);
            const auto length=native_campaign_ai_normalize_vector_tu15(memory,f,fp,observe);
            facing=point(f);
            if(compare(length,r(0x46a0e8u),false))facing=point(r(0x474d08u));
        }
        const auto s=r(0x46a794u),x=facing[0],y=facing[1],z=facing[2];
        const auto ny=sub(0,y),sx=mul(x,s),sy=mul(y,s),sz=mul(z,s),sny=mul(ny,s);
        std::array<V,8> directions{};directions[0]=facing;
        directions[1]={add(sny,sx),add(sx,sy),add(sz,sz)};
        directions[2]={sub(sx,sny),sub(sy,sx),sub(sz,sz)};
        const auto negative=r(0x46a0dcu),inverse_ny=mul(ny,negative),nx=mul(x,negative),nz=mul(z,negative);
        const auto t=r(0x46a7e4u),tx=mul(x,t),tz=mul(z,t),ty=mul(y,t);
        directions[3]={ny,x,z};directions[4]={inverse_ny,nx,nz};
        directions[5]={add(sny,tx),add(sx,ty),add(sz,tz)};
        const auto inverse_y=mul(y,r(0x46a0dcu)),six_y=sub(ty,sx),six_x=sub(tx,sny),six_z=sub(tz,sz);
        directions[6]={six_x,six_y,six_z};directions[7]={nx,inverse_y,nz};
        for(unsigned n=0;n<8;++n)point(f+0x24u+n*12u,directions[n]);
        abi.gp=g;abi.gp[0]=0;abi.gp[1]=0;abi.gp[2]=f+0x24u;abi.gp[6]=0;
        if(services.behavior_direction_geometry)w(0x1cac05u,f-4u,g[6]);
        for(U index=0;index<8u;++index) {
            abi.gp[0]=index;abi.gp[1]=index*3u;abi.gp[6]=index;
            abi.gp[2]=f+0x24u+index*12u;
            if(query(0x1cac7du,0x1cac76u,0x1cac7bu,0x1cac7cu,g[7],abi.gp[2],g[3],f-4u))break;
        }
        abi.gp[0]=(abi.gp[0]&0xffffff00u)|1u;
        abi.gp[6]=r(f-4u);(void)r(g[4]);abi.gp[4]=g[4]+4u;
    }
};
}
void native_campaign_ai_behavior_129_probe_call_tu15(SourcePhysicsContactMemory &memory,U entry,
    SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPerformServicesTu15 &services,
    const SourcePhysicsContactRecordObserver &observe) {
    if(!abi.floating_point || abi.active_mmx)throw std::invalid_argument("Native behavior probes require retained x87 FP");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))throw std::runtime_error("Native behavior probes require SIMD trap owner");
    const auto publish=[&]{abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;};
    Probe probe{memory,abi,observe,fp,services};const auto saved=abi.gp;
    try{if(entry==0x1ca900u)probe.nearest(saved);else if(entry==0x1caa20u)probe.fan(saved);else throw std::invalid_argument("Unknown native behavior probe entry");}
    catch(...){publish();throw;}publish();
}
#else
void native_campaign_ai_behavior_129_probe_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t,SourcePhysicsWorldRuntimeAbi &,const NativeCampaignAiPerformServicesTu15 &,
    const SourcePhysicsContactRecordObserver &) {
    throw std::runtime_error("Native behavior probes require a qualified scalar FP owner");
}
#endif
}
