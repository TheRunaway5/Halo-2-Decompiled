#include "halo2/native_campaign_ai_route_corridor_tu15.hpp"
#include "halo2/native_campaign_ai_route_traversal_tu15.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <bit>
#include <cstring>
#include <vector>
namespace halo2 {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace {
using u8=std::uint8_t;using u16=std::uint16_t;using u32=std::uint32_t;using i16=std::int16_t;using i32=std::int32_t;
constexpr u32 none=0xffffffffu;
i32 sw(u32 v){return std::bit_cast<i16>(u16(v));}
float real(u32 bits){return std::bit_cast<float>(bits);}u32 bits(float value){return std::bit_cast<u32>(value);}
u32 node(u32 context,u32 index){return context+0x2cu+u32(sw(index))*0x34u;}
u32 obstacle(u32 list,u32 index){return list+0xcu+u32(sw(index))*20u;}
struct Scalar {
    NativeTu15PhysicsFrameFp &fp;
    template<class F> u32 operation(u32 a,u32 b,F calculate)const {
        u32 host{},csr=fp.mxcsr;asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(host):"m"(csr):"memory");
        auto x=real(a);calculate(x,real(b));
        asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(fp.mxcsr):"m"(host):"memory");return bits(x);
    }
    u32 add(u32 a,u32 b)const{return operation(a,b,[](float &x,float y){asm volatile("addss %1,%0":"+x"(x):"x"(y):"memory");});}
    u32 sub(u32 a,u32 b)const{return operation(a,b,[](float &x,float y){asm volatile("subss %1,%0":"+x"(x):"x"(y):"memory");});}
    u32 mul(u32 a,u32 b)const{return operation(a,b,[](float &x,float y){asm volatile("mulss %1,%0":"+x"(x):"x"(y):"memory");});}
    u32 div(u32 a,u32 b)const{return operation(a,b,[](float &x,float y){asm volatile("divss %1,%0":"+x"(x):"x"(y):"memory");});}
    bool above(u32 a,u32 b)const {
        u32 host{},csr=fp.mxcsr;u8 answer{};const auto x=real(a),y=real(b);
        asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\tcomiss %[y],%[x]\n\tseta %[answer]\n\tstmxcsr %[csr]\n\tldmxcsr %[host]"
          :[host]"+m"(host),[csr]"+m"(csr),[answer]"=&q"(answer):[x]"x"(x),[y]"x"(y):"memory","cc");
        fp.mxcsr=csr;return answer!=0;
    }
    bool zero(u32 a,u32 b)const {
        u32 host{},csr=fp.mxcsr;u8 equal{},ordered{};const auto x=real(a),y=real(b);
        asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\tucomiss %[y],%[x]\n\tsete %[equal]\n\tsetnp %[ordered]\n\tstmxcsr %[csr]\n\tldmxcsr %[host]"
          :[host]"+m"(host),[csr]"+m"(csr),[equal]"=&q"(equal),[ordered]"=&q"(ordered):[x]"x"(x),[y]"x"(y):"memory","cc");
        fp.mxcsr=csr;return equal&&ordered;
    }
    bool below(u32 a,u32 b)const {
        u32 host{},csr=fp.mxcsr;u8 answer{};const auto x=real(a),y=real(b);
        asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\tcomiss %[y],%[x]\n\tsetb %[answer]\n\tstmxcsr %[csr]\n\tldmxcsr %[host]"
          :[host]"+m"(host),[csr]"+m"(csr),[answer]"=&q"(answer):[x]"x"(x),[y]"x"(y):"memory","cc");fp.mxcsr=csr;return answer!=0;
    }
};

struct Corridor {
    SourcePhysicsContactMemory &memory;SourcePhysicsWorldRuntimeAbi &abi;
    const NativeCampaignAiPerformServicesTu15 &services;NativeTu15PhysicsFrameFp &fp;
    const SourcePhysicsContactRecordObserver &observe;Scalar scalar{fp};
    u32 r(u32 address,unsigned width=4)const {
        std::array<std::byte,4> raw{};memory.read(address,std::span(raw).first(width));u32 value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<u32>(raw[n])<<(n*8u);return value;
    }
    void w(u32 site,u32 address,u32 value,unsigned width=4)const {
        const bool observed=native_tu15_observes_retained_store(memory,observe,site,address,width);
        SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
        if(observed){event.before.resize(width);memory.read(address,event.before);}
        for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
        memory.write(address,event.written);if(observed)observe(event);
    }
    void push(u32 &sp,u32 value)const {sp-=4u;std::array<std::byte,4> raw{};for(unsigned n=0;n<4;++n)raw[n]=std::byte(value>>(n*8u));memory.write(sp,raw);}
    void call(u32 site,u32 target,std::array<u32,8> gp,std::initializer_list<u32> args) {
        const auto base=gp[4];for(auto it=args.end();it!=args.begin();)push(gp[4],*--it);push(gp[4],site+5u);abi.gp=gp;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
        auto import=[&](){if(abi.floating_point){fp.control_word=abi.floating_point->x87_control_word;fp.mxcsr=abi.floating_point->mxcsr;}fp.status=abi.x87_status;};
        try {if(target==0x2769c0u||target==0x2762a0u)native_campaign_ai_route_traversal_call_tu15(memory,target,abi,services,observe);
            else native_campaign_ai_route_corridor_call_tu15(memory,target,abi,services,observe);
        }catch(...){import();throw;}
        import();if(abi.gp[4]!=base)throw std::runtime_error("Native corridor child returned another frame");
    }
    // X difference is rounded by FSTP; Y stays extended across its FST and
    // multiply by the rounded Y. This distinction matters under 24-bit CW.
    bool direction(u32 to,u32 from,u32 out,u32 magnitude,std::array<u32,3> sites) {
        detail::NativeTu15ArithmeticEnvironment environment(fp);auto a=real(r(to)),b=real(r(from));float value{};
        asm volatile("flds %1\n\tfsubs %2\n\tfstps %0":"=m"(value):"m"(a),"m"(b):"memory","st");w(sites[0],out,bits(value));
        a=real(r(to+4u));b=real(r(from+4u));asm volatile("flds %1\n\tfsubs %2\n\tfsts %0":"=m"(value):"m"(a),"m"(b):"memory","st");w(sites[1],out+4u,bits(value));a=real(r(out+4u));
        asm volatile("fmuls %0"::"m"(a):"memory","st");a=real(r(out));asm volatile("flds %0"::"m"(a):"memory","st");a=real(r(out));
        asm volatile("fmuls %1\n\tfaddp\n\tfsqrt\n\tfsts %0":"=m"(value):"m"(a):"memory","st","st(1)");w(sites[2],magnitude,bits(value));
        asm volatile("fabs":::"st");a=real(r(0x46a0ecu));u8 small{};asm volatile("flds %1\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0":"=q"(small):"m"(a):"memory","cc","st","st(1)");return small;
    }
    bool margin(u32 x,u32 y,u32 radius) {
        detail::NativeTu15ArithmeticEnvironment environment(fp);auto a=real(r(x)),b=real(r(y)),rr=real(r(radius)),half=real(r(0x46a0ccu));u8 yes{};
        asm volatile("flds %1\n\tfmuls %1\n\tflds %2\n\tfmuls %2\n\tfaddp\n\tfsqrt\n\tfsubrs %3\n\tflds %3\n\tfmuls %4\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0":"=q"(yes):"m"(a),"m"(b),"m"(rr),"m"(half):"memory","cc","st","st(1)");return yes;
    }
    void heap_up(u32 context,u32 index);
    void heap_down(u32 context,u32 index);
    u32 contains(u32 list,u32 point,u32 exclude,u32 radius);
    void tangents(u32 entry,const std::array<u32,8>&);
    void obstacle_tangents(u32 entry,const std::array<u32,8>&);
    u32 append(u32 entry,const std::array<u32,8>&);
    void initialize(u32 entry,const std::array<u32,8>&);
    void exhausted(u32 entry,const std::array<u32,8>&);
    u32 expand(u32 entry,const std::array<u32,8>&);
    u32 step(u32 entry,const std::array<u32,8>&);
    u32 run(u32 entry,const std::array<u32,8>&);
};
void Corridor::heap_up(u32 c,u32 index) {
    while(sw(index)>0){const auto child=c+0xd2eu+u32(sw(index))*2u;auto ci=r(child,2);index=u32((sw(index)-1)>>1);auto parent=c+0xd2eu+u32(sw(index))*2u,pi=r(parent,2);
        if(!scalar.above(r(node(c,pi)+0x24u),r(node(c,ci)+0x24u)))break;
        w(0x2cd426u,parent,r(child,2),2);w(0x2cd429u,child,pi,2);
    }
}
void Corridor::heap_down(u32 c,u32 index) {
    if(sw(index)>=sw(r(c+0xd2cu,2)))return;
    for(;;){auto count=r(c+0xd2cu,2),best=index,left=index*2u+1u,right=index*2u+2u;
        if(sw(left)<sw(count)&&scalar.above(r(node(c,r(c+0xd2eu+u32(sw(index))*2u,2))+0x24u),r(node(c,r(c+0xd2eu+u32(sw(left))*2u,2))+0x24u)))best=left;
        if(sw(right)<sw(count)&&scalar.above(r(node(c,r(c+0xd2eu+u32(sw(best))*2u,2))+0x24u),r(node(c,r(c+0xd2eu+u32(sw(right))*2u,2))+0x24u)))best=right;
        if(u16(best)==u16(index))break;auto a=c+0xd2eu+u32(sw(index))*2u,b=c+0xd2eu+u32(sw(best))*2u,av=r(a,2),bv=r(b,2);
        w(0x2cd4ebu,b,av,2);w(0x2cd4eeu,a,bv,2);index=best;
    }
}
u32 Corridor::contains(u32 list,u32 point,u32 exclude,u32 radius) {
    const auto count=sw(r(list+2u,2));for(u32 n=0;sw(n)<count;++n){if(u16(n)==u16(exclude))continue;auto p=obstacle(list,n);
        auto dx=scalar.sub(r(p+8u),r(point)),dy=scalar.sub(r(p+0xcu),r(point+4u));auto rr=scalar.add(r(p+0x10u),radius);
        auto distance=scalar.add(scalar.mul(dx,dx),scalar.mul(dy,dy));if(!scalar.below(scalar.mul(rr,rr),distance))return n;
    }return 0xffffu;
}
void Corridor::tangents(u32 entry,const std::array<u32,8>&g) {
    const auto f=entry-4u,vector=g[0],first=g[2],second=g[1];auto one=r(0x46a0d0u),distance=r(entry+4u),sine=one;
    w(0x2caad8u,f,sine);if(scalar.above(distance,0)){sine=scalar.div(r(entry+8u),distance);w(0x2caaecu,f,sine);if(scalar.above(sine,one)){sine=one;w(0x2caaf6u,f,sine);}}
    u32 cosine{},first_y{},minus_y{};
    {detail::NativeTu15ArithmeticEnvironment environment(fp);auto s=real(r(f)),o=real(one);float v{};asm volatile("flds %0\n\tfmuls %0"::"m"(s):"memory","st");
        // SSE alongside the live x87 expression uses this same MXCSR.
        auto mx=real(r(vector)),my=real(r(vector+4u)),minus=0.f,ss=real(sine);asm volatile("subss %3,%0\n\tmulss %0,%1\n\tmulss %2,%0":"+x"(minus),"+x"(mx):"x"(my),"x"(ss):"memory");first_y=bits(mx);minus_y=bits(minus);
        asm volatile("fsubrs %1\n\tfsqrt\n\tfstps %0":"=m"(v):"m"(o):"memory","st");cosine=bits(v);w(0x2cab1au,entry+8u,cosine);
    }
    auto y=scalar.add(first_y,scalar.mul(cosine,r(vector+4u)));w(0x2cab38u,first+4u,y);auto x=scalar.sub(scalar.mul(r(vector),cosine),minus_y);w(0x2cab45u,first,x);
    {detail::NativeTu15ArithmeticEnvironment environment(fp);auto co=real(r(entry+8u)),vx=real(r(vector));asm volatile("flds %0\n\tfmuls %1"::"m"(co),"m"(vx):"memory","st");auto si=real(r(f)),vy=real(r(vector+4u));asm volatile("flds %0\n\tfmuls %1"::"m"(si),"m"(vy):"memory","st");
        auto sy=real(r(vector)),s=real(sine),cy=real(r(vector+4u)),c=real(cosine);asm volatile("mulss %2,%0\n\tmulss %3,%1\n\taddss %1,%0":"+x"(sy),"+x"(cy):"x"(s),"x"(c):"memory");
        asm volatile("fsubrp":::"st","st(1)");w(0x2cab68u,second+4u,bits(sy));float xx{};asm volatile("fstps %0":"=m"(xx)::"memory","st");w(0x2cab6du,second,bits(xx));
        co=real(r(entry+8u));auto d=real(r(entry+4u));asm volatile("flds %1\n\tfmuls %2\n\tfstps %0":"=m"(xx):"m"(co),"m"(d):"memory","st");w(0x2cab77u,r(entry+0xcu),bits(xx));
    }
}
void Corridor::obstacle_tangents(u32 entry,const std::array<u32,8>&g) {
    const auto f=entry-8u,list=r(entry+4u),p=obstacle(list,r(entry+8u,2));const bool small=direction(p+8u,g[6],f,entry+8u,{0x2cbc35u,0x2cbc3eu,0x2cbc50u});
    auto one=r(0x46a0d0u);if(!small){auto length=r(entry+8u);const auto positive=scalar.above(length,0);auto inverse=scalar.div(one,length);w(0x2cbc7au,f,scalar.mul(inverse,r(f)));w(0x2cbc89u,f+4u,scalar.mul(r(f+4u),inverse));
        if(positive){auto radius=scalar.add(scalar.add(r(p+0x10u),r(entry+0xcu)),r(0x46a4d4u));call(0x2cbcb4u,0x2caac0u,{f,g[1],g[2],g[3],f,g[5],g[6],g[7]},{length,radius,g[7]});return;}}
    auto minus=r(0x46a0dcu);w(0x2cbcc7u,g[2],0);w(0x2cbccbu,g[2]+4u,minus);w(0x2cbcd0u,g[1],0);w(0x2cbcd4u,g[1]+4u,one);w(0x2cbcd9u,g[7],0);
}
u32 Corridor::append(u32 entry,const std::array<u32,8>&g) {
    const auto c=g[1],side=g[3]&255u,identity=r(entry+0xcu,2);if(sw(r(c+0x2au,2))>=64)return none;
    auto sp=entry-0xcu;push(sp,g[5]);const auto f=sp,point=r(entry+4u);
    w(0x2cd526u,entry+0xcu,0,1);
    const bool small=direction(c+0x10u,point,f+8u,f+4u,{0x2cd52bu,0x2cd535u,0x2cd54fu});
    u32 dx=r(f+8u),dy=r(f+0xcu),distance=0;
    if(!small){distance=r(f+4u);auto inverse=scalar.div(r(0x46a0d0u),distance);dx=scalar.mul(inverse,dx);dy=scalar.mul(dy,inverse);w(0x2cd580u,f+8u,dx);w(0x2cd586u,f+0xcu,dy);}
    push(sp,g[6]);push(sp,g[7]);const auto parent=r(entry+0x18u);auto ancestor=parent;bool tracks_goal=false;
    while(u16(ancestor)!=0xffffu){const auto previous=node(c,ancestor);
        if(r(previous+0x18u,2)==identity){if(r(previous+0x1cu,1)!=side)return none;
            const auto goal=r(c+0x1cu,2);if(identity==goal&&goal!=0xffffu&&scalar.above(distance,r(previous+0x14u)))return none;
            ancestor=r(previous+0x28u,2);continue;}
        const auto goal=r(c+0x1cu,2);if(identity!=goal||goal==0xffffu)break;
        tracks_goal=true;w(0x2cd610u,entry+0xcu,1,1);auto other=r(previous+0x1eu+(side?0u:2u),2);
        if(other!=0xffffu){const auto opposite=node(c,other);auto dot=scalar.add(scalar.mul(r(opposite+0x10u),dy),scalar.mul(r(opposite+0xcu),dx));
            if(scalar.above(dot,0)){auto parent_node=node(c,parent);auto cross1=scalar.sub(scalar.mul(r(parent_node+0x10u),r(opposite+0xcu)),scalar.mul(r(opposite+0x10u),r(parent_node+0xcu)));
                dy=scalar.mul(dy,r(opposite+0xcu));dy=scalar.sub(dy,scalar.mul(r(opposite+0x10u),dx));
                if(scalar.above(0,scalar.mul(cross1,dy)))return none;
            }}
        const auto link=previous+0x1eu+side*2u,value=r(link,2);if(value==u16(parent)||value==0xffffu)w(0x2cd6a7u,link,r(c+0x2au,2),2);break;
    }
    const auto index=r(c+0x2au,2),out=node(c,index);w(0x2cd6bdu,c+0x2au,index+1u,2);
    w(0x2cd6c7u,out,r(point));w(0x2cd6d0u,out+4u,r(point+4u));w(0x2cd6d7u,out+8u,r(entry+8u));
    w(0x2cd6deu,out+0xcu,r(f+8u));w(0x2cd6e5u,out+0x10u,r(f+0xcu));w(0x2cd6edu,out+0x1au,r(entry+0x10u,2),2);
    w(0x2cd6f3u,out+0x2au,0,2);w(0x2cd6f7u,out+0x2cu,0,2);w(0x2cd6feu,out+0x14u,distance);auto total=scalar.add(distance,r(entry+0x14u));
    w(0x2cd709u,out+0x2eu,none,2);w(0x2cd70du,out+0x18u,identity,2);w(0x2cd711u,out+0x1cu,side,1);w(0x2cd714u,out+0x24u,total);
    w(0x2cd719u,out+0x28u,parent,2);w(0x2cd71du,out+0x30u,0,1);w(0x2cd721u,out+0x1eu,none);
    if(tracks_goal&&scalar.above(r(c+0x24u),r(out+0x14u))){w(0x2cd73au,c+0x24u,r(out+0x14u));w(0x2cd73du,c+0x20u,index,2);}
    const auto heap_count=r(c+0xd2cu,2);if(sw(heap_count)<64){w(0x2cd753u,c+0xd2cu,heap_count+1u,2);w(0x2cd75du,c+0xd2eu+u32(sw(heap_count))*2u,index,2);call(0x2cd765u,0x2cd3d0u,{heap_count,c,abi.gp[2],g[3],sp,identity,index,parent},{});}
    return index;
}
void Corridor::initialize(u32 entry,const std::array<u32,8>&g) {
    auto sp=entry;push(sp,g[3]);push(sp,g[5]);push(sp,g[6]);const auto c=g[7],list=g[2],target=r(entry+0x10u),radius=r(entry+4u);
    w(0x2cd78fu,c+4u,g[1],1);w(0x2cd796u,c,radius);w(0x2cd79au,c+8u,list);w(0x2cd79du,c+0xcu,g[0]);w(0x2cd7a0u,c+0x28u,0,1);
    w(0x2cd7a7u,c+0x10u,r(target));w(0x2cd7b2u,c+0x14u,r(target+4u));w(0x2cd7b5u,c+0x18u,r(entry+0x14u));
    abi.xmm[3][0]=radius;call(0x2cd7b8u,0x2cba80u,{g[0],g[1],target,none,sp,target,list,c},{});
    const auto hit=abi.gp[0]&0xffffu,identity=hit==0xffffu?none:u32(sw(r(obstacle(list,hit)+2u,2)));
    w(0x2cd7e1u,c+0x1cu,identity,2);w(0x2cd7eau,c+0x29u,r(entry+0x18u,1),1);
    w(0x2cd7f3u,c+0x1eu,none,2);w(0x2cd7f7u,c+0x20u,none,2);w(0x2cd7fbu,c+0xdaeu,none,2);w(0x2cd807u,c+0x24u,r(0x45695cu));
    w(0x2cd80cu,c+0x2au,0,2);w(0x2cd810u,c+0xd2cu,0,2);call(0x2cd817u,0x2cd500u,{0,c,abi.gp[2],0xffffff00u,sp,target,list,c},{r(entry+8u),r(entry+0xcu),none,none,0,none});
    for(u32 n=0;sw(n)<sw(r(list+2u,2));++n){const auto p=obstacle(list,n);bool inside{};
        {detail::NativeTu15ArithmeticEnvironment environment(fp);auto x=real(r(p+8u)),tx=real(r(target)),y=real(r(p+0xcu)),ty=real(r(target+4u)),a=real(r(entry+4u)),b=real(r(p+0x10u));u8 yes{};
            asm volatile("flds %1\n\tfsubs %2\n\tflds %3\n\tfsubs %4\n\tfld %%st(1)\n\tfmulp %%st,%%st(2)\n\tfld %%st(0)\n\tfmul %%st(1)\n\tfaddp %%st,%%st(2)\n\tfxch %%st(1)\n\tfsqrt\n\tfstp %%st(1)\n\tflds %5\n\tfadds %6\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0":"=q"(yes):"m"(x),"m"(tx),"m"(y),"m"(ty),"m"(a),"m"(b):"memory","cc","st","st(1)","st(2)");inside=yes;}
        if(inside)w(0x2cd857u,p,r(p,1)|2u,1);
    }
}
void Corridor::exhausted(u32 entry,const std::array<u32,8>&g) {
    auto index=g[0];const auto identity=r(entry+4u,2),c=g[6];
    while(u16(index)!=0xffffu){auto n=node(c,index);bool unseen=r(n+0x2eu,2)==0xffffu;w(0x2cdb60u,n+0x30u,1,1);if(unseen&&identity!=0xffffu)w(0x2cdb6cu,n+0x2eu,identity,2);
        const auto parent=r(n+0x28u,2);if(parent==0xffffu){w(0x2cdba7u,c+0xdaeu,r(n+0x2eu,2),2);break;}
        const auto p=node(c,parent);w(0x2cdb80u,p+0x2cu,r(p+0x2cu,2)+1u,2);if(sw(r(p+0x2cu,2))<sw(r(p+0x2au,2)))break;index=r(n+0x28u,2);
    }
}
u32 Corridor::expand(u32 entry,const std::array<u32,8>&g) {
    auto sp=entry-0x100u;push(sp,g[3]);push(sp,g[5]);push(sp,g[6]);push(sp,g[7]);const auto f=sp,nav=r(entry+4u),c=r(entry+8u),index=r(entry+0xcu),current=node(c,index),list=r(c+8u);
    const auto words=u32((sw(r(list+2u,2))+31)>>5);for(u32 n=0;n<words;++n)w(0x2cd8abu,f+0x2cu+n*4u,0);
    auto first=r(entry+0x10u),bit=1u<<(u32(sw(first))&31u),word=f+0x2cu+u32(sw(first)>>5)*4u;
    w(0x2cd8d3u,f+0x24u,0);w(0x2cd8dbu,f+0x90u,first,2);w(0x2cd8e5u,word,r(word)|bit);u32 pending=1;
    do {
        --pending;w(0x2cd8f6u,f+0x10u,pending);auto active=r(f+0x90u+u32(sw(pending))*2u,2);w(0x2cd908u,f+0x28u,active);
        if(active!=0xffffu)w(0x2cd91cu,f+0x1cu,r(obstacle(r(c+8u),active)+2u,2),2);else w(0x2cd923u,f+0x1cu,none);
        call(0x2cd941u,0x2cbc10u,{active,f+0x64u,f+0x5cu,c,f,current,current,f+0x18u},{r(c+8u),active,r(c)});
        if(scalar.above(r(c),r(f+0x18u)))w(0x2cd951u,f+0x18u,r(c));
        w(0x2cd972u,f+0x14u,scalar.add(scalar.add(r(obstacle(r(c+8u),r(f+0x28u,2))+0x10u),r(c)),r(f+0x18u)));w(0x2cd978u,f+0x20u,0);
        for(u32 side=0;sw(side)<2;++side){const auto vector=f+0x5cu+side*8u;
            call(0x2cd9b6u,0x2769c0u,{nav,active,r(c+8u),c,f,current,abi.gp[6],vector},{nav,r(c+8u),r(f+0x28u),current,r(current+8u),none,vector,r(c),r(f+0x14u),0,0,r(c+0x29u,1),0,f+0x4cu});
            const auto hit=r(f+0x58u,2);if(hit!=0xffffu){word=f+0x2cu+u32(sw(hit)>>5)*4u;bit=1u<<(u32(sw(hit))&31u);if(!(r(word)&bit)){w(0x2cd9fcu,word,r(word)|bit);w(0x2cda02u,f+0x90u+u32(sw(r(f+0x10u)))*2u,hit,2);w(0x2cda0au,f+0x10u,r(f+0x10u)+1u);}}
            auto extent=r(f+0x4cu),minimum=r(f+0x18u);if(scalar.above(extent,minimum)&&r(f+0x5au,2)!=r(f+0x1cu,2)){
                extent=scalar.mul(scalar.add(extent,minimum),r(0x46a0ccu));w(0x2cda43u,f+0x14u,extent);
                w(0x2cda67u,f+0x40u,r(current));w(0x2cda73u,f+0x44u,r(current+4u));w(0x2cda7cu,f+0x34u,r(vector));w(0x2cda92u,f+0x48u,0);w(0x2cda98u,f+0x38u,r(vector+4u));w(0x2cda9eu,f+0x3cu,0);
                call(0x2cdaa4u,0x2762a0u,{r(current+8u),abi.gp[1],f+0x40u,c,f,current,f+0x6cu,vector},{nav,f+0x40u,none,f+0x34u,r(f+0x14u),0});
                const auto cumulative=scalar.add(scalar.sub(r(current+0x24u),r(current+0x14u)),r(f+0x14u));
                call(0x2cdaf4u,0x2cd500u,{r(f+0x1cu),c,f+0x70u,(c&0xffffff00u)|u32(sw(side)>0),f,current,f+0x6cu,vector},{f+0x70u,r(f+0x7cu),r(f+0x1cu),r(entry+0x10u),cumulative,index});
                if(u16(abi.gp[0])!=0xffffu)w(0x2cdb01u,f+0x24u,r(f+0x24u)+1u);
            }
            w(0x2cdb11u,f+0x20u,side+1u);
        }
        pending=r(f+0x10u);
    }while(sw(pending)>0);
    return r(f+0x24u,2);
}
u32 Corridor::step(u32 entry,const std::array<u32,8>&g) {
    const auto c=g[0];auto sp=entry-0x24u;push(sp,g[6]);auto count=r(c+0xd2cu,2);
    if(sw(count)>0){push(sp,g[7]);const auto index=r(c+0xd2eu,2);--count;w(0x2cdbe3u,c+0xd2cu,count,2);w(0x2cdbf5u,c+0xd2eu,r(c+0xd2eu+u32(sw(count))*2u,2),2);w(0x2cdc00u,sp+8u,index);
        call(0x2cdc04u,0x2cd440u,{c,0,abi.gp[2],g[3],sp,g[5],c,index},{});
        if(index!=0xffffu){push(sp,g[3]);push(sp,g[5]);const auto f=sp,n=node(c,index),nav=r(entry+4u);u32 added=0;
            const auto initial=r(n+0x28u,2)==0xffffu;
            call(0x2cdc57u,0x2769c0u,{r(n+8u),r(c+8u),nav,0,f,n+0xcu,c,n},{nav,r(c+8u),none,n,r(n+8u),r(c+0x18u),n+0xcu,r(c),r(n+0x14u),u32(initial),1,r(c+0x29u,1),0,f+0x24u});
            const auto edge=r(f+0x2cu);if(edge==0xffffu){auto hit=r(f+0x30u,2);
                if(hit==0xffffu){auto target=r(c+0x18u);if(r(f+0x28u)==target||scalar.above(r(0x46a16cu),r(n+0x14u))){
                    const auto extent=r(f+0x24u);w(0x2cdca7u,f+0x1cu,scalar.add(scalar.mul(r(n+0xcu),extent),r(n)));w(0x2cdcbcu,f+0x20u,scalar.add(scalar.mul(r(n+0x10u),extent),r(n+4u)));
                    const auto cumulative=scalar.add(scalar.sub(r(n+0x24u),r(n+0x14u)),extent);
                    call(0x2cdddeu,0x2cd500u,{target,c,f+0x1cu,0,f,n+0xcu,c,n},{f+0x1cu,target,none,none,cumulative,index});w(0x2cdde3u,c+0x1eu,abi.gp[0],2);added=1;
                }}else{const auto ob=obstacle(r(c+8u),hit);auto identity=r(f+0x32u,2);bool success=false;
                    if(r(f+0x3cu,1)&&identity==r(c+0x1cu,2)){
                        w(0x2cdd1eu,f+0x1cu,scalar.sub(r(c+0x10u),r(ob+8u)));w(0x2cdd2eu,f+0x20u,scalar.sub(r(c+0x14u),r(ob+0xcu)));
                        const auto extent=r(f+0x24u);auto dy=scalar.mul(r(n+0x10u),extent),dx=scalar.mul(extent,r(n+0xcu));w(0x2cdd46u,f+0x14u,r(n));dx=scalar.add(r(f+0x14u),dx);w(0x2cdd57u,f+0x18u,r(n+4u));dy=scalar.add(r(f+0x18u),dy);auto rr=r(ob+0x10u);w(0x2cdd6au,f+0x18u,dy);dy=scalar.sub(dy,r(c+0x14u));w(0x2cdd75u,f+0x14u,dx);dx=scalar.sub(dx,r(c+0x10u));
                        if(scalar.above(scalar.mul(rr,rr),scalar.add(scalar.mul(dy,dy),scalar.mul(dx,dx)))&&margin(f+0x20u,f+0x1cu,ob+0x10u)){
                            call(0x2cdddeu,0x2cd500u,{index,c,r(c+0x18u),0,f,n+0xcu,c,n},{f+0x14u,r(c+0x18u),none,none,r(n+0x24u),index});w(0x2cdde3u,c+0x1eu,abi.gp[0],2);added=1;success=true;
                        }
                    }
                    if(!success){if(identity==r(c+0x1cu,2)&&scalar.above(r(c+0x24u),r(n+0x14u))&&r(n+0x18u,2)==identity){w(0x2cde0du,c+0x24u,r(n+0x14u));w(0x2cde10u,c+0x20u,index,2);}
                        call(0x2cde24u,0x2cd870u,{r(f+0x30u),index,nav,0,f,n+0xcu,c,n},{nav,c,index,r(f+0x30u)});added=abi.gp[0];
                    }
                }
            }else{auto current_obstacle=r(n+0x1au,2);if(current_obstacle!=0xffffu){call(0x2cde43u,0x2cd870u,{index,nav,abi.gp[2],0,f,n+0xcu,c,n},{nav,c,index,current_obstacle});added=abi.gp[0];w(0x2cde4cu,f+0x30u,r(n+0x1au,2),2);}}
            w(0x2cde54u,n+0x2au,added,2);
            if(u16(added)==0){auto hit=r(f+0x30u,2),identity=hit==0xffffu?none:u32(sw(r(obstacle(r(c+8u),hit)+2u,2)));call(0x2cde7fu,0x2cdb40u,{index,abi.gp[1],abi.gp[2],g[3],f+8u,g[5],c,n},{identity});}
        }
    }
    return r(c+0x1eu,2)==0xffffu&&sw(r(c+0xd2cu,2))>0;
}
u32 Corridor::run(u32 entry,const std::array<u32,8>&g) {
    auto sp=entry-8u;push(sp,g[3]);push(sp,g[5]);push(sp,g[7]);const auto f=sp,c=g[0],nav=r(entry+4u),obstacles=r(entry+0x10u);
    call(0x2cdeddu,0x2cd780u,{r(0x4ecc18u),r(entry+0xcu,1),obstacles,nav,f,obstacles,g[6],c},{r(entry+0x14u),r(entry+0x18u),g[2],g[6],g[1],r(entry+0x20u)});
    do{call(0x2cdeeau,0x2cdbc0u,{c,abi.gp[1],r(entry+0x1cu),nav,f,obstacles,g[6],c},{nav,r(entry+0x1cu)});}while(abi.gp[0]&255u);
    if(r(c+0x1eu,2)!=0xffffu){w(0x2cdf01u,c+0x28u,1,1);return r(c+0x1eu,2)!=0xffffu;}
    const auto best=r(c+0x20u,2);if(best!=0xffffu){if(!r(entry+0x1cu,1))w(0x2ce00bu,c+0x1eu,best,2);
        else{const auto n=node(c,best);auto threshold=r(r(entry+8u)+0x6cu);bool finish=scalar.above(threshold,r(0x46a0e8u))&&scalar.above(threshold,r(n+0x14u));
            if(!finish){const auto ob=obstacle(obstacles,r(n+0x1au,2));w(0x2cdf62u,f+0xcu,scalar.sub(r(g[6]),r(ob+8u)));w(0x2cdf72u,f+0x10u,scalar.sub(r(g[6]+4u),r(ob+0xcu)));finish=scalar.above(r(ob+0x10u),r(n+0x14u))&&margin(f+0xcu,f+0x10u,ob+0x10u);}
            if(finish){const auto length=r(n+0x14u);w(0x2cdfc4u,f+0xcu,scalar.add(scalar.mul(r(n+0xcu),length),r(n)));w(0x2cdfe9u,f+0x10u,scalar.add(scalar.mul(r(n+0x10u),length),r(n+4u)));
                call(0x2cdfefu,0x2cd500u,{r(c+0x18u),c,abi.gp[2],nav&0xffffff00u,f,obstacles,g[6],c},{f+0xcu,r(c+0x18u),none,none,length,best});w(0x2cdff4u,c+0x1eu,abi.gp[0],2);
            }
        }
    }return r(c+0x1eu,2)!=0xffffu;
}
struct Publication {SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;~Publication(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}};
}
void native_campaign_ai_route_corridor_call_tu15(SourcePhysicsContactMemory &memory,u32 entry,SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPerformServicesTu15 &services,const SourcePhysicsContactRecordObserver &observe) {
    if(!abi.floating_point||abi.active_mmx||abi.x87_depth)throw std::invalid_argument("Native corridor requires retained FP and empty x87");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};Publication publication{abi,fp};
    if((fp.control_word&0x3fu)!=0x3fu||(fp.mxcsr&0x1f80u)!=0x1f80u)throw std::invalid_argument("Native corridor requires masked FP exceptions");
    const auto g=abi.gp;Corridor owner{memory,abi,services,fp,observe};u32 result=0,cleanup=0,mask=0;
    switch(entry){
    case 0x2cdeb0:result=owner.run(g[4],g);cleanup=0x20;mask=0xff;break;
    case 0x2cd780:owner.initialize(g[4],g);cleanup=0x18;break;
    case 0x2cdbc0:result=owner.step(g[4],g);cleanup=8;mask=0xff;break;
    case 0x2cd870:result=owner.expand(g[4],g);cleanup=0x10;mask=0xffff;break;
    case 0x2cd500:result=owner.append(g[4],g);cleanup=0x18;mask=0xffff;break;
    case 0x2cd3d0:owner.heap_up(g[1],g[0]);break;
    case 0x2cd440:owner.heap_down(g[0],g[1]);break;
    case 0x2cdb40:owner.exhausted(g[4],g);cleanup=4;break;
    case 0x2cbc10:owner.obstacle_tangents(g[4],g);cleanup=0xc;break;
    case 0x2caac0:owner.tangents(g[4],g);cleanup=0xc;break;
    case 0x2cba80:result=owner.contains(g[6],g[2],g[3],abi.xmm[3][0]);mask=0xffff;break;
    default:throw std::invalid_argument("Native corridor has no such entry");
    }
    abi.gp=g;abi.gp[0]=(g[0]&~mask)|(result&mask);abi.gp[4]=g[4]+4u+cleanup;
}
#else
void native_campaign_ai_route_corridor_call_tu15(SourcePhysicsContactMemory &,std::uint32_t,SourcePhysicsWorldRuntimeAbi &,const NativeCampaignAiPerformServicesTu15 &,const SourcePhysicsContactRecordObserver &){throw std::runtime_error("Exact native corridor arithmetic unavailable");}
#endif
}
