#include "halo2/native_campaign_ai_route_special_link_tu15.hpp"
#include "halo2/native_campaign_ai_route_traversal_tu15.hpp"
#include "halo2/native_campaign_ai_vector_math_tu15.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"
#include <bit>
#include <vector>

namespace halo2 {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace {
using U=std::uint32_t;using B=std::uint8_t;
constexpr U none=0xffffffffu;
struct Scalar {
    NativeTu15PhysicsFrameFp &fp;
    template<class F>U operation(U a,U b,F calculate)const {
        U host{},csr=fp.mxcsr;asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(host):"m"(csr):"memory");
        auto x=std::bit_cast<float>(a);calculate(x,std::bit_cast<float>(b));
        asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(fp.mxcsr):"m"(host):"memory");return std::bit_cast<U>(x);
    }
    U add(U a,U b)const{return operation(a,b,[](float &x,float y){asm volatile("addss %1,%0":"+x"(x):"x"(y):"memory");});}
    U sub(U a,U b)const{return operation(a,b,[](float &x,float y){asm volatile("subss %1,%0":"+x"(x):"x"(y):"memory");});}
    U mul(U a,U b)const{return operation(a,b,[](float &x,float y){asm volatile("mulss %1,%0":"+x"(x):"x"(y):"memory");});}
    U div(U a,U b)const{return operation(a,b,[](float &x,float y){asm volatile("divss %1,%0":"+x"(x):"x"(y):"memory");});}
    bool above(U a,U b)const {
        U host{},csr=fp.mxcsr;B result{};const auto x=std::bit_cast<float>(a),y=std::bit_cast<float>(b);
        asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\tcomiss %[y],%[x]\n\tseta %[result]\n\tstmxcsr %[csr]\n\tldmxcsr %[host]"
            :[host]"+m"(host),[csr]"+m"(csr),[result]"=&q"(result):[x]"x"(x),[y]"x"(y):"memory","cc");
        fp.mxcsr=csr;return result!=0;
    }
};
struct Link {
    SourcePhysicsContactMemory &memory;SourcePhysicsWorldRuntimeAbi &abi;
    const NativeCampaignAiPerformServicesTu15 &services;NativeTu15PhysicsFrameFp &fp;
    const SourcePhysicsContactRecordObserver &observe;Scalar scalar{fp};
    U r(U address,unsigned width=4)const {
        std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));U value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<U>(bytes[n])<<(n*8u);return value;
    }
    void w(U pc,U address,U value,unsigned width=4)const {
        const bool observed=native_tu15_observes_retained_store(memory,observe,pc,address,width);
        SourcePhysicsContactRecordStore event{pc,address,{},std::vector<std::byte>(width)};
        if(observed){event.before.resize(width);memory.read(address,event.before);}
        for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
        memory.write(address,event.written);if(observed)observe(event);
    }
    void push(U &sp,U value)const {
        sp-=4u;std::array<std::byte,4> bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8u));memory.write(sp,bytes);
    }
    void copy(U source,U destination,std::initializer_list<U> sites)const {
        for(auto pc:sites){const auto value=r(source);w(pc,destination,value);source+=4u;destination+=4u;}
    }
    void call(U site,U target,std::array<U,8> gp,std::initializer_list<U> args) {
        const auto base=gp[4];for(auto it=args.end();it!=args.begin();)push(gp[4],*--it);push(gp[4],site+5u);
        abi.gp=gp;abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
        const auto import=[&](){if(abi.floating_point){fp.control_word=abi.floating_point->x87_control_word;fp.mxcsr=abi.floating_point->mxcsr;}fp.status=abi.x87_status;};
        try {
            if(target==0x2762a0u)native_campaign_ai_route_traversal_call_tu15(memory,target,abi,services,observe);
            else if(services.callee)services.callee(site,target,abi);
            else throw NativeCampaignAiPerformRequiredOwnerTu15(site,target);
        }catch(...){import();throw;}
        import();if(abi.gp[4]!=base)throw std::runtime_error("Native special-link child returned another frame");
    }
    U normalize(U point,U frame,U site) {
        auto sp=frame;push(sp,site+5u);return native_campaign_ai_normalize_vector_tu15(memory,point,fp,observe);
    }
    bool run(const std::array<U,8>&g);
};

bool Link::run(const std::array<U,8>&g) {
    const auto entry=g[4];auto sp=entry-0x40u;push(sp,g[3]);push(sp,g[5]);
    const auto second=r(entry+0x18u);push(sp,g[6]);const auto f=sp;
    const auto link=r(g[0]+0x3cu)+g[1]*20u,kind=r(link,2),first=g[7];
    w(0x279120u,f+0x64u,0,1);
    if(kind==1u || kind==6u) {
        const auto previous=r(f+0x58u);if(!previous)return false;
        // Source MOVZX/CMP -1 checks cannot reject a WORD FFFF. Preserve
        // those unsigned indices; backing services own invalid accesses.
        const auto ia=r(link+4u,2),ib=r(link+6u,2),ic=r(link+8u,2),id=r(link+0xau,2);
        const auto vertices=r(g[0]+0x2cu),a=vertices+ia*12u,b=vertices+ib*12u;
        const auto c=vertices+ic*12u,d=vertices+id*12u;
        w(0x27918au,f+0x50u,c);
        w(0x27919du,f+0x10u,scalar.sub(r(b),r(a)));
        w(0x2791b3u,f+0x14u,scalar.sub(r(b+4u),r(a+4u)));
        w(0x2791c3u,f+0xcu,d);
        w(0x2791cbu,f+0x18u,scalar.sub(r(b+8u),r(a+8u)));
        const auto length=normalize(f+0x10u,f,0x2791d1u);
        const bool first_valid=scalar.above(length,r(0x46a0e8u));w(0x2791ddu,f+0x64u,length);
        bool second_valid=false;
        if(first_valid) {
            const auto endpoint=r(f+0xcu),origin=r(f+0x50u);
            w(0x2791f9u,f+0x1cu,scalar.sub(r(endpoint),r(origin)));
            w(0x279209u,f+0x20u,scalar.sub(r(endpoint+4u),r(origin+4u)));
            w(0x27921du,f+0x24u,scalar.sub(r(endpoint+8u),r(origin+8u)));
            const auto other_length=normalize(f+0x1cu,f,0x279223u);
            second_valid=scalar.above(other_length,r(0x46a0e8u));w(0x27922fu,f+0xcu,other_length);
        }
        if(!first_valid || !second_valid) {
            copy(a,first,{0x27939au,0x27939fu,0x2793a9u});
            const auto frame=r(previous+0xcu,2),sector=r(f+0x5cu),out=r(f+0x60u);
            w(0x2793b8u,first+0xcu,frame,2);w(0x2793bcu,out,sector);
            copy(r(f+0x50u),second,{0x2793c6u,0x2793cbu,0x2793d1u});
            w(0x2793dcu,second+0xcu,r(link+0x12u,2),2);
            const auto next_sector=r(link+0xeu,2),next_out=r(f+0x68u);
            w(0x2793e5u,f+0x64u,1,1);w(0x2793ebu,next_out,next_sector);return true;
        }
        const auto point=r(f+0x58u),vy=r(f+0x14u),vz=r(f+0x18u),vx=r(f+0x10u);
        auto pz=scalar.mul(r(point+8u),vz),py=scalar.mul(r(point+4u),vy);
        auto projection=scalar.add(py,pz);projection=scalar.add(projection,scalar.mul(r(point),vx));
        auto az=scalar.mul(r(a+8u),vz),ay=scalar.mul(vy,r(a+4u));
        auto origin_projection=scalar.add(ay,az);origin_projection=scalar.add(origin_projection,scalar.mul(r(a),vx));
        const auto request=r(f+0x54u);const auto twice_radius=scalar.mul(r(request),r(0x46a11cu));
        projection=scalar.sub(projection,origin_projection);const auto magnitude=r(f+0x64u),one=r(0x46a0d0u);
        const bool wide=scalar.above(magnitude,twice_radius);auto inverse=scalar.div(one,magnitude);projection=scalar.mul(projection,inverse);
        if(wide) {
            inverse=scalar.mul(inverse,r(request));
            if(scalar.above(inverse,projection))projection=inverse;
            else {const auto upper=scalar.sub(one,inverse);if(scalar.above(projection,upper))projection=upper;}
        } else projection=r(0x46a0ccu);
        const auto other=r(f+0x50u);const auto second_distance=scalar.mul(projection,r(f+0xcu));
        const auto first_distance=scalar.mul(projection,magnitude);
        w(0x27930bu,first,scalar.add(scalar.mul(vx,first_distance),r(a)));
        auto second_x=scalar.mul(r(f+0x1cu),second_distance);
        w(0x27931cu,first+4u,scalar.add(scalar.mul(vy,first_distance),r(a+4u)));
        w(0x27932au,first+8u,scalar.add(scalar.mul(vz,first_distance),r(a+8u)));
        w(0x279333u,second,scalar.add(second_x,r(other)));
        w(0x279347u,second+4u,scalar.add(scalar.mul(r(f+0x20u),second_distance),r(other+4u)));
        const auto second_z=scalar.add(scalar.mul(r(f+0x24u),second_distance),r(other+8u));
        const auto sector=r(f+0x5cu);w(0x27935fu,second+8u,second_z);
        w(0x279368u,first+0xcu,r(point+0xcu,2),2);
        const auto next_frame=r(link+0x12u,2),out=r(f+0x60u);w(0x279374u,second+0xcu,next_frame,2);w(0x279378u,out,sector);
        const auto next_sector=r(link+0xeu,2),next_out=r(f+0x68u);
        w(0x279383u,f+0x64u,1,1);w(0x279389u,next_out,next_sector);return true;
    }
    if(kind==5u) {
        const auto actor=r(f+0x50u),vertices=r(g[0]+0x2cu),point=vertices+r(link+8u,2)*12u;
        const auto direction=vertices+r(link+0xau,2)*12u,negative=r(0x46a0dcu);
        w(0x27941cu,f+0x10u,r(direction));
        auto y=r(direction+4u);w(0x279430u,f+0x10u,scalar.mul(r(f+0x10u),negative));w(0x279436u,f+0x14u,y);
        auto z=r(direction+8u);w(0x279444u,f+0x18u,z);w(0x279448u,f+0x14u,scalar.mul(r(f+0x14u),negative));
        w(0x279458u,f+0x18u,scalar.mul(r(f+0x18u),negative));
        if(actor!=none) {
            call(0x279479u,0x27a110u,{actor,f+0x10u,0,point,f,second,link,first},
                {0x5000534u,0x5000049u,point,first,0,r(f+0x60u)});
            if(abi.gp[0]&255u) {
                copy(point,second,{0x279486u,0x27948bu,0x279495u});
                const auto frame=r(r(f+0x58u)+0xcu,2),out=r(f+0x68u);w(0x2794a0u,second+0xcu,frame,2);
                const auto sector=r(link+4u,2);w(0x2794a9u,f+0x64u,1,1);w(0x2794b3u,out,sector);return true;
            }
        }
        const auto previous=r(f+0x58u);copy(previous,first,{0x2794c4u,0x2794c9u,0x2794cfu,0x2794d9u});
        const auto sector=r(f+0x5cu),out=r(f+0x60u);w(0x2794e0u,out,sector);
        copy(point,second,{0x2794e6u,0x2794ebu,0x2794f1u});
        w(0x2794fcu,second+0xcu,r(previous+0xcu,2),2);const auto next_out=r(f+0x68u),next_sector=r(link+4u,2);
        w(0x279505u,f+0x64u,1,1);w(0x27950bu,next_out,next_sector);return true;
    }
    if(kind==2u) {
        const auto request=r(f+0x54u),navigation=r(request+0x8cu),vertices=r(navigation+0x2cu);
        const auto direction=vertices+r(link+8u,2)*12u,point=vertices+r(link+6u,2)*12u;
        copy(point,second,{0x27953fu,0x279544u,0x27954au});
        const auto previous=r(f+0x58u),frame=r(previous+0xcu,2),next_out=r(f+0x68u),sector=r(f+0x5cu);
        w(0x27955du,second+0xcu,frame,2);const auto out=r(f+0x60u),actor=r(f+0x50u);
        // This retained publication precedes the marker child in source.
        w(0x27956eu,next_out,sector);
        call(0x27957eu,0x27a110u,{actor,direction,0,direction,f,out,point,first},
            {0x50000cbu,0x60000ccu,point,first,0,out});
        if(!(abi.gp[0]&255u)) {
            const auto nav=r(r(f+0x54u)+0x8cu),start_sector=r(f+0x5cu);
            call(0x2795a5u,0x2762a0u,{start_sector,nav,abi.gp[2],direction,f,out,f+0x28u,first},
                {nav,point,none,direction,0x3e4ccccdu,0});
            const auto x=r(f+0x2cu),y=r(f+0x30u);w(0x2795b4u,first,x);
            const auto z=r(f+0x34u);w(0x2795bau,first+4u,y);const auto old=r(f+0x58u);w(0x2795c1u,first+8u,z);
            const auto output_frame=r(old+0xcu,2),output_sector=r(f+0x38u);w(0x2795ccu,first+0xcu,output_frame,2);w(0x2795d0u,out,output_sector);
        }
        w(0x2795d3u,f+0x64u,1,1);return true;
    }
    return false;
}
struct Publication {SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;~Publication(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}};
}
void native_campaign_ai_route_special_link_call_tu15(SourcePhysicsContactMemory &memory,U entry,
    SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPerformServicesTu15 &services,
    const SourcePhysicsContactRecordObserver &observe) {
    if(entry!=0x279100u)throw std::invalid_argument("Unknown native special-link entry");
    if(!abi.floating_point||abi.active_mmx||abi.x87_depth)throw std::invalid_argument("Native special-link requires retained FP and empty x87");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};Publication publication{abi,fp};
    if((fp.control_word&0x3fu)!=0x3fu||(fp.mxcsr&0x1f80u)!=0x1f80u)throw std::invalid_argument("Native special-link requires masked FP exceptions");
    const auto g=abi.gp;Link owner{memory,abi,services,fp,observe};const auto result=owner.run(g);
    abi.gp=g;abi.gp[0]=(g[0]&0xffffff00u)|U(result);abi.gp[4]=g[4]+0x20u;
}
#else
void native_campaign_ai_route_special_link_call_tu15(SourcePhysicsContactMemory &,std::uint32_t,SourcePhysicsWorldRuntimeAbi &,const NativeCampaignAiPerformServicesTu15 &,const SourcePhysicsContactRecordObserver &){throw std::runtime_error("Exact native special-link arithmetic unavailable");}
#endif
}
