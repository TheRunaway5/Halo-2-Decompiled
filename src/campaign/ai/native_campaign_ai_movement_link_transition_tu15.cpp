#include "halo2/native_campaign_ai_movement_link_transition_tu15.hpp"
#include "halo2/native_campaign_ai_movement_link_parameters_tu15.hpp"
#include "halo2/native_campaign_ai_movement_jump_tu15.hpp"
#include "halo2/native_campaign_ai_character_property_tu15.hpp"
#include "halo2/native_campaign_ai_vector_math_tu15.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <algorithm>
#include <bit>
#include <vector>

namespace halo2 {
namespace {
using U=std::uint32_t;using B=std::uint8_t;
struct Transition {
    SourcePhysicsContactMemory &memory;SourcePhysicsWorldRuntimeAbi &abi;
    const NativeCampaignAiPerformServicesTu15 &services;
    const SourcePhysicsContactRecordObserver &observe;
    U r(U at,unsigned width=4)const {std::array<std::byte,4>b{};memory.read(at,std::span(b).first(width));U v{};for(unsigned i=0;i<width;++i)v|=std::to_integer<U>(b[i])<<(8u*i);return v;}
    void w(U pc,U at,U v,unsigned width=4)const {
        SourcePhysicsContactRecordStore e{pc,at,{},std::vector<std::byte>(width)};
        if(observe){e.before.resize(width);memory.read(at,e.before);}
        for(unsigned i=0;i<width;++i)e.written[i]=std::byte(v>>(8u*i));memory.write(at,e.written);if(observe)observe(e);
    }
    void push(U &sp,U value)const {sp-=4u;std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(8u*i));memory.write(sp,b);}
    U multiply(U a,U b) {
        U host{},csr=abi.floating_point->mxcsr;auto x=std::bit_cast<float>(a);const auto y=std::bit_cast<float>(b);
        asm volatile("stmxcsr %0\n\tldmxcsr %1\n\tmulss %3,%2\n\tstmxcsr %1\n\tldmxcsr %0":"=m"(host),"+m"(csr),"+x"(x):"x"(y):"memory");
        abi.floating_point->mxcsr=csr;return std::bit_cast<U>(x);
    }
    bool compare(U a,U b,char condition) {
        U host{},csr=abi.floating_point->mxcsr;const auto x=std::bit_cast<float>(a),y=std::bit_cast<float>(b);B above{},below{},equal{},ordered{};
        asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(host):"m"(csr):"memory");
        if(condition=='=')asm volatile("ucomiss %3,%2\n\tsete %0\n\tsetnp %1":"=qm"(equal),"=qm"(ordered):"x"(x),"x"(y):"cc","memory");
        else asm volatile("comiss %3,%2\n\tseta %0\n\tsetb %1":"=qm"(above),"=qm"(below):"x"(x),"x"(y):"cc","memory");
        asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(csr):"m"(host):"memory");abi.floating_point->mxcsr=csr;
        return condition=='='?equal&&ordered:condition=='>'?above:condition=='<'?below:!below;
    }
    template<class F>U arithmetic(F calculate) {
        NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
        const auto publish=[&]{abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;};
        U value{};try{value=calculate(fp);}catch(...){publish();throw;}publish();return value;
    }
    U normalize(U point,U frame,U pc) {
        auto sp=frame;push(sp,pc+5u);
        return arithmetic([&](auto &fp){return native_campaign_ai_normalize_vector2_tu15(memory,point,fp,observe);});
    }
    U raised_speed(U speed) {
        return arithmetic([&](auto &fp){detail::NativeTu15ArithmeticEnvironment env(fp);const U increment=r(0x45c618u);U value{};
            asm volatile("flds %1\n\tfadds %2\n\tfstps %0":"=m"(value):"m"(speed),"m"(increment):"memory","st");return value;});
    }
    void child(U pc,U target,std::array<U,8> gp,std::initializer_list<U> args={}) {
        const auto base=gp[4];for(auto it=args.end();it!=args.begin();)push(gp[4],*--it);push(gp[4],pc+5u);abi.gp=gp;
        if(target==0x200310u)native_campaign_ai_movement_link_parameters_call_tu15(memory,target,abi,services,observe);
        else if(target==0x1ecd00u)native_campaign_ai_character_property_call_tu15(memory,target,abi);
        else if(target==0xff1a0u)native_campaign_ai_movement_jump_call_tu15(memory,target,abi,services,observe);
        else throw NativeCampaignAiPerformRequiredOwnerTu15(pc,target);
        if(abi.gp[4]!=base)throw std::runtime_error("Native link-transition child returned another frame");
    }
    B run(const std::array<U,8> &g) {
        auto sp=g[4]-0x48u;for(unsigned i:{3u,5u,6u,7u})push(sp,g[i]);const auto f=sp;
        const auto pool=r(0x5020c8u),rows=r(pool+0x44u),actor=rows+(g[0]&0xffffu)*0x888u,mask=g[1];
        abi.xmm[1]={0,0,0,0};w(0x20044bu,f+0x18u,0);w(0x200451u,f+0x13u,0,1);w(0x200456u,f+0x12u,0,1);
        child(0x20045au,0x200310u,{g[0],pool,rows,g[3]&0xffffff00u,f,actor,mask,f+0x13u});
        w(0x200461u,f+0x14u,abi.xmm[0][0]);bool permitted=!mask;
        if(mask) {
            child(0x200470u,0x1ecd00u,{abi.gp[0],r(actor+0x54u),abi.gp[2],abi.gp[3],f,actor,mask,abi.gp[7]});
            const auto property=abi.gp[0];
            if(property) {
                const auto authored=std::bit_cast<std::int16_t>(std::uint16_t(r(property+0x1au,2)));const int count=std::min<int>(authored,6);
                const auto flags=r(mask,1);
                if(r(f+0x60u,1)) {
                    const U available=(1u<<(U(count)&31u))-1u;
                    const U sign_mask=U(std::int32_t(std::bit_cast<std::int8_t>(B(flags))));
                    permitted=std::uint16_t(available|sign_mask)!=0;
                    if(permitted)w(0x2004b4u,f+0x12u,flags&1u,1);
                } else {
                    unsigned first{};while(first<6u && !(flags&(1u<<first)))++first;
                    if(first<6u) {
                        const int index=std::min(int(std::bit_cast<std::int16_t>(std::uint16_t(count-1))),int(first));U speed=0;
                        if(index>=0 && index<6) {
                            const auto scenario=r(0x4ecc1cu);
                            if(scenario && std::bit_cast<std::int32_t>(r(scenario+0xc8u))>0)speed=r(r(scenario+0xccu)+0x80u+U(index)*4u);
                        }
                        w(0x200513u,f+0x14u,speed);w(0x200519u,f+0x12u,first==0,1);permitted=true;
                    }
                }
            }
        }
        if(r(f+0x13u,1))w(0x200528u,f+0x60u,1,1);
        if(!permitted)return 0;
        const auto point=r(f+0x5cu),position=actor+0x238u;const auto speed=r(f+0x14u);
        abi.xmm[0][0]=speed;w(0x200572u,f+0x28u,speed);
        child(0x200578u,0xff1a0u,{point,position,speed,point,f,actor,f+0x2cu,position},
            {speed,0x3f800000u,f+0x18u,0,0,1,0,0,0,f+0x24u,f+0x20u});
        auto result=B(abi.gp[0]);w(0x200585u,f+0x5cu,result,1);U vertical{},duration{},launch{};bool second{};
        if(r(f+0x60u,1) || compare(multiply(r(f+0x14u),r(0x46a198u)),r(f+0x18u),'>')) {
            const auto retry=raised_speed(r(f+0x18u));w(0x2005b6u,f+0x60u,retry);
            child(0x2005dbu,0xff1a0u,{point,position,f+0x18u,point,f,actor,f+0x2cu,position},
                {retry,0x3f800000u,f+0x18u,0,0,1,0,0,0,f+0x1cu,f+0x14u});
            if(B(abi.gp[0])){vertical=r(f+0x14u);duration=r(f+0x1cu);launch=r(f+0x60u);w(0x2005f6u,f+0x5cu,1,1);result=B(r(f+0x5cu,1));second=true;}
            else result=B(r(f+0x5cu,1));
        }
        if(!second){if(!result)return result;vertical=r(f+0x20u);duration=r(f+0x24u);launch=r(f+0x28u);}
        const auto unit=r(actor+0x18u),body=r(r(r(0x4ecbd0u)+0x44u)+(unit&0xffffu)*12u+8u);
        const auto extension=U(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(r(body+0x346u,2)))));
        if(r(body+extension+0x36u,2)==5u) {
            const auto x=r(point),y=r(point+4u);w(0x20064eu,f+0x3cu,x);const auto z=r(point+8u);
            w(0x200659u,f+0x38u,0x2eu);w(0x200661u,f+0x40u,y);w(0x200665u,f+0x44u,z);
            child(0x200669u,0xe8e50u,{unit,y,z,point,f,actor,extension,f+0x38u});return B(abi.gp[0]);
        }
        if(r(f+0x12u,1))return result;
        U minimum{};const auto scenario=r(0x4ecc1cu);
        if(scenario && std::bit_cast<std::int32_t>(r(scenario+0xc8u))>0)minimum=r(r(scenario+0xccu)+0x80u);
        auto floor=r(0x46a1f8u);if(compare(multiply(minimum,floor),launch,'a'))return result;
        const bool longer=compare(duration,floor,'>');w(0x2006c3u,actor+0x464u,1,1);w(0x2006cau,actor+0x466u,1,1);w(0x2006d1u,actor+0x470u,vertical);
        if(longer)floor=duration;w(0x2006e2u,actor+0x474u,floor);w(0x2006eau,f+0x34u,0);
        auto magnitude=normalize(f+0x2cu,f,0x2006f0u);U x{},y{};
        if(compare(magnitude,0,'>')){x=r(f+0x2cu);y=r(f+0x30u);w(0x20070eu,actor+0x468u,x);w(0x200714u,actor+0x46cu,y);return B(r(f+0x5cu,1));}
        x=r(actor+0x290u);y=r(actor+0x294u);const auto z=r(actor+0x298u);w(0x200730u,f+0x34u,z);
        const bool flat=compare(z,0,'=');w(0x200741u,f+0x2cu,x);w(0x200745u,f+0x30u,y);
        if(!flat) {
            w(0x20074fu,f+0x34u,0);magnitude=normalize(f+0x2cu,f,0x200755u);
            if(!compare(0,magnitude,'<')) {
                const auto basis=r(0x474d08u);x=r(basis);y=r(basis+4u);const auto bz=r(basis+8u);
                w(0x200771u,actor+0x468u,x);w(0x200777u,actor+0x46cu,y);w(0x20077eu,f+0x34u,bz);return B(r(f+0x5cu,1));
            }
            y=r(f+0x30u);x=r(f+0x2cu);
        }
        w(0x20079bu,actor+0x468u,x);w(0x2007a1u,actor+0x46cu,y);return B(r(f+0x5cu,1));
    }
};
}
void native_campaign_ai_movement_link_transition_call_tu15(SourcePhysicsContactMemory &memory,U entry,
    SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPerformServicesTu15 &services,const SourcePhysicsContactRecordObserver &observe) {
    if(entry!=0x200420u)throw std::invalid_argument("Unknown native link-transition entry");
    if(!abi.floating_point||abi.x87_depth||abi.active_mmx||(abi.floating_point->x87_control_word&0x3fu)!=0x3fu||(abi.floating_point->mxcsr&0x1f80u)!=0x1f80u)
        throw std::invalid_argument("Native link-transition requires empty masked retained FP state");
    const auto g=abi.gp;Transition owner{memory,abi,services,observe};const auto result=owner.run(g);abi.gp=g;abi.gp[0]=(g[0]&0xffffff00u)|result;abi.gp[4]=g[4]+12u;
}
}
