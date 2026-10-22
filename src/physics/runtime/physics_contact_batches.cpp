#include "halo2/physics_contact_batches.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M=SourcePhysicsContactMemory;using O=SourcePhysicsContactRecordObserver;
using S=SourcePhysicsContactBatchServices;using FP=SourcePhysicsFactoryFloatingPoint;
std::uint32_t r(const M&m,std::uint32_t p,unsigned n=4) {
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t x=0;
    for(unsigned k=0;k<n;++k)x|=std::to_integer<std::uint32_t>(b[k])<<(k*8u);
    return x;
}
std::int32_t si(std::uint32_t x){return std::bit_cast<std::int32_t>(x);}
void w(M&m,const O&o,std::uint32_t site,std::uint32_t p,unsigned n,std::uint64_t x) {
    SourcePhysicsContactRecordStore a{site,p,{},{}};a.before.resize(n);a.written.resize(n);m.read(p,a.before);
    for(unsigned k=0;k<n;++k)a.written[k]=std::byte((x>>(k*8u))&255u);
    m.write(p,a.written);if(o)o(a);
}
[[noreturn]]void missing(const char*s){throw std::runtime_error(s);}
std::uint32_t required(M&m,const S&s,std::uint32_t site,std::uint32_t entry,
    std::optional<std::uint32_t> ecx,std::uint32_t esp,std::vector<std::uint32_t>args,const O&o) {
    if(!s.required_owner)missing("Required original contact batch owner unavailable");
    return s.required_owner(m,{site,entry,esp,ecx,std::move(args)},o);
}
struct FpScope {
    unsigned mx;std::fenv_t env;
    explicit FpScope(FP p):mx(_mm_getcsr()) {
        if((p.x87_control_word&0x3fu)!=0x3fu||(p.mxcsr&0x1f80u)!=0x1f80u)
            missing("Unmasked original contact batch FP effects unavailable");
        if(std::fegetenv(&env)!=0)missing("Original contact batch FP environment unavailable");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("fldcw %0" : :"m"(p.x87_control_word):"memory");
#else
        missing("Original contact batch x87 backend unavailable");
#endif
        _mm_setcsr(p.mxcsr);
    }
    ~FpScope(){std::fesetenv(&env);_mm_setcsr(mx);}
};
float f(std::uint32_t x){return std::bit_cast<float>(x);}
std::uint32_t bits(float x){return std::bit_cast<std::uint32_t>(x);}
__m128 v(const M&m,std::uint32_t p) {
    std::array<float,4>x{};for(unsigned k=0;k<4;++k)x[k]=f(r(m,p+k*4u));return _mm_loadu_ps(x.data());
}
void vw(M&m,const O&o,std::uint32_t site,std::uint32_t p,__m128 x) {
    std::array<float,4>a{};_mm_storeu_ps(a.data(),x);const auto b=std::bit_cast<std::array<std::uint32_t,4>>(a);
    for(unsigned k=0;k<2;++k)w(m,o,site,p+k*8u,8,std::uint64_t(b[k*2])|(std::uint64_t(b[k*2+1])<<32));
}
float dot(__m128 a,__m128 b) {
    const auto x=_mm_mul_ps(a,b);
    return _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(x,x,0xaa),_mm_add_ss(_mm_shuffle_ps(x,x,0x55),x)));
}
__m128 transform(const M&m,std::uint32_t matrix,__m128 a) {
    const auto zt=_mm_add_ps(_mm_mul_ps(v(m,matrix+32),_mm_shuffle_ps(a,a,0xaa)),v(m,matrix+48));
    const auto y=_mm_mul_ps(v(m,matrix+16),_mm_shuffle_ps(a,a,0x55));
    const auto x=_mm_mul_ps(v(m,matrix),_mm_shuffle_ps(a,a,0));
    return _mm_add_ps(_mm_add_ps(x,y),zt);
}
long double square_extended(float x) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double y;asm volatile("flds %1\n\tfmul %%st(0)\n\tfstpt %0":"=m"(y):"m"(x):"st");return y;
#else
    (void)x;missing("Original contact batch x87 square unavailable");
#endif
}
float square_spill(float x) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    float y;asm volatile("flds %1\n\tfmul %%st(0)\n\tfstps %0":"=m"(y):"m"(x):"st");return y;
#else
    (void)x;missing("Original contact batch x87 square unavailable");
#endif
}
std::uint32_t x87_copy(std::uint32_t x) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    float y;const float a=f(x);asm volatile("flds %1\n\tfstps %0":"=m"(y):"m"(a):"st");return bits(y);
#else
    (void)x;missing("Original contact batch x87 copy unavailable");
#endif
}
std::uint32_t root(const M&m,std::uint32_t p){while(r(m,p+12))p=r(m,p+12);return p;}
}
std::uint32_t source_copy_physics_contact_packet_1_1(M&m,std::uint32_t d,std::uint32_t p,const O&o) {
    vw(m,o,0x300109,d,v(m,p));vw(m,o,0x300110,d+16,v(m,p+16));
    w(m,o,0x300117,d+32,4,r(m,p+32));w(m,o,0x30011d,d+36,4,r(m,p+36));
    for(unsigned k=0;k<4;++k)vw(m,o,0x300124+k*8u,d+48+k*16u,v(m,p+48+k*16u));
    w(m,o,0x300143,d+112,4,r(m,p+112));w(m,o,0x300149,d+116,4,r(m,p+116));
    w(m,o,0x30014f,d+120,4,r(m,p+120));w(m,o,0x300155,d+124,1,r(m,p+124,1));w(m,o,0x30015b,d+125,1,r(m,p+125,1));return d;
}
std::uint32_t source_construct_physics_contact_scratch_1_1(M&m,std::uint32_t d,std::uint32_t count,std::uint32_t esp,const S&s,const O&o) {
    w(m,o,0x3001f0,d,4,0);w(m,o,0x3001f2,d+4,4,0);w(m,o,0x3001f5,d+8,4,0x80000000);
    const auto adapter=r(m,0x487638),n=((count<<8)+15u)&~15u;auto p=r(m,adapter+8);
    if(si(n)>si(r(m,adapter+12)))p=required(m,s,0x30021f,r(m,r(m,adapter)+0x24),adapter,esp-32u,{n},o);
    else {w(m,o,0x300231,adapter+8,4,p+n);w(m,o,0x300234,adapter+12,4,r(m,adapter+12)-n);}
    w(m,o,0x30023a,d,4,p);w(m,o,0x300242,d+12,4,p);w(m,o,0x300245,d+8,4,count|0x80000000);w(m,o,0x30024c,d+16,4,count);return d;
}
void source_dispose_physics_contact_scratch_1_1(M&m,std::uint32_t d,std::uint32_t esp,const S&s,const O&o) {
    const auto adapter=r(m,0x487638),n=((r(m,d+16)<<8)+15u)&~15u,p=r(m,d+12),alternate=r(m,adapter+20);
    if(n&&(p==alternate||!alternate))required(m,s,0x3002ac,r(m,r(m,adapter)+0x28),adapter,esp-36u,{p,n},o);
    else {w(m,o,0x3002bb,adapter+8,4,r(m,adapter+8)-n);w(m,o,0x3002be,adapter+12,4,r(m,adapter+12)+n);}
    if(si(r(m,d+8))>=0)required(m,s,0x3002e6,r(m,r(m,adapter)+20),adapter,esp-40u,{r(m,d),(r(m,d+8)&0x7fffffff)<<8,0x12},o);
}
std::uint32_t source_append_physics_contact_packet_1_1(M&m,std::uint32_t d,std::uint32_t esp,const S&s,const O&o) {
    const auto frame=((esp-4u)&~15u)-0x90u;
    w(m,o,0x300414,frame+0x8d,1,0);w(m,o,0x30041b,frame+0x8c,1,0);
    if(r(m,d+4)==(r(m,d+8)&0x7fffffffu))required(m,s,0x300434,0x2df780,std::nullopt,frame-12u,{d,128},o);
    const auto p=r(m,d)+r(m,d+4)*128u;w(m,o,0x30044e,d+4,4,r(m,d+4)+1u);
    source_copy_physics_contact_packet_1_1(m,p,frame+16,o);return r(m,d)+(r(m,d+4)-1u)*128u;
}
void source_transform_physics_contact_point_1_1(M&m,std::uint32_t out,std::uint32_t matrix,std::uint32_t point,FP fp,const O&o) {
    FpScope scope(fp);vw(m,o,0x2e0be5,out,transform(m,matrix,v(m,point)));
}
void source_physics_contact_point_velocity_1_1(M&m,std::uint32_t motion,std::uint32_t point,std::uint32_t out,FP fp,const O&o) {
    FpScope scope(fp);const auto d=_mm_sub_ps(v(m,point),v(m,motion+112)),angular=v(m,motion+80);
    const auto a=_mm_mul_ps(_mm_shuffle_ps(angular,angular,0xc9),_mm_shuffle_ps(d,d,0xd2));
    const auto b=_mm_mul_ps(_mm_shuffle_ps(angular,angular,0xd2),_mm_shuffle_ps(d,d,0xc9));
    const auto cross=_mm_sub_ps(a,b);vw(m,o,0x31feec,out,cross);vw(m,o,0x31fef9,out,_mm_add_ps(cross,v(m,motion+64)));
}
std::uint32_t source_prune_physics_contact_packets_1_1(M&m,std::uint32_t parent,const std::array<std::uint32_t,5>&a,std::uint32_t esp,FP fp,const S&s,const O&o) {
    FpScope scope(fp);const auto frame=((esp-4u)&~15u)-0xc0u;auto total=a[3],index=a[2];
    w(m,o,0x300667,frame+44,4,parent);w(m,o,0x30066b,frame+20,4,index);
    while(si(index)<si(total)) {
        const auto base=r(m,a[1]),current=base+index*128u;
        if(r(m,current+124,1)==1) {
            const auto result=required(m,s,0x3006a6,0x2ffd60,parent,frame-20u,{frame+19,a[0],current+80,a[4]},o);
            if(r(m,result,1)==0) {
                const auto last=base+(total-1u)*128u;
                const auto c0=v(m,current),c10=v(m,current+16),c30=v(m,current+48),c40=v(m,current+64),c50=v(m,current+80),c60=v(m,current+96);
                const auto c20=x87_copy(r(m,current+32)),c24=x87_copy(r(m,current+36)),c70=r(m,current+112),c74=r(m,current+116),c78=r(m,current+120);
                vw(m,o,0x3006ec,current,v(m,last));vw(m,o,0x3006f5,current+16,v(m,last+16));w(m,o,0x3006fa,frame+184,4,c78);
                w(m,o,0x300705,current+32,4,r(m,last+32));w(m,o,0x300709,frame+176,4,c70);const auto c7c=r(m,current+124,1);
                w(m,o,0x300714,frame+180,4,c74);const auto c7d=r(m,current+125,1);w(m,o,0x30071f,frame+188,1,c7c);w(m,o,0x300726,frame+189,1,c7d);
                w(m,o,0x300735,frame+60,4,last+32);w(m,o,0x30073d,frame+36,4,last+36);w(m,o,0x300743,current+36,4,r(m,last+36));
                vw(m,o,0x30074c,current+48,v(m,last+48));vw(m,o,0x300756,current+64,v(m,last+64));vw(m,o,0x300760,current+80,v(m,last+80));vw(m,o,0x30076a,current+96,v(m,last+96));
                w(m,o,0x300773,frame+52,4,last+48);w(m,o,0x30077b,frame+24,4,last+64);w(m,o,0x300787,frame+28,4,last+112);w(m,o,0x30078d,current+112,4,r(m,last+112));
                w(m,o,0x300795,frame+32,4,last+116);w(m,o,0x30079b,current+116,4,r(m,last+116));w(m,o,0x3007a3,frame+40,4,last+120);w(m,o,0x3007a9,current+120,4,r(m,last+120));
                w(m,o,0x3007b1,frame+48,4,last+124);w(m,o,0x3007b7,current+124,1,r(m,last+124,1));w(m,o,0x3007bf,frame+56,4,last+125);w(m,o,0x3007c5,current+125,1,r(m,last+125,1));
                vw(m,o,0x3007cd,last+16,c10);w(m,o,0x3007d4,last+32,4,c20);vw(m,o,0x3007da,last+48,c30);w(m,o,0x3007e1,last+36,4,c24);vw(m,o,0x3007e7,last+64,c40);
                vw(m,o,0x3007f1,last,c0);vw(m,o,0x3007f6,last+80,c50);vw(m,o,0x3007f9,last+96,c60);w(m,o,0x300804,last+112,4,r(m,frame+176));w(m,o,0x300811,last+116,4,r(m,frame+180));w(m,o,0x30081e,last+120,4,r(m,frame+184));
                w(m,o,0x30082b,last+124,1,r(m,frame+188,1));w(m,o,0x300835,last+125,1,r(m,frame+189,1));--total;w(m,o,0x300837,esp+16,4,total);continue;
            }
        }
        ++index;w(m,o,0x300847,frame+20,4,index);
    }
    return total;
}
void source_retain_physics_contact_packet_1_1(M&m,const std::array<std::uint32_t,7>&a,std::uint32_t esp,FP fp,const S&s,const O&o) {
    FpScope scope(fp);const auto frame=((esp-4u)&~15u)-0xb0u,packet=a[3],bank=a[5],count=r(m,a[6]);
    auto best=square_extended(f(r(m,r(m,a[2]+12)+4)));std::int32_t selected=-1;
    for(std::int32_t k=0;k<si(count);++k) {
        const auto row=bank+std::uint32_t(k)*48u;const auto d=_mm_sub_ps(v(m,row),v(m,packet+16));const auto distance=dot(d,d);
        w(m,o,0x309273,frame+24,4,bits(distance));
        if(!(static_cast<long double>(distance)<best))continue;
        const auto nd=_mm_sub_ps(v(m,row+32),v(m,packet+64));const auto normal=dot(nd,nd);w(m,o,0x3092ac,frame+20,4,bits(normal));
        if(!(normal<f(r(m,0x4654c0))))continue;selected=k;best=distance;
    }
    if(selected>=0) {
        const auto row=bank+std::uint32_t(selected)*48u,key=x87_copy(r(m,row+28));
        vw(m,o,0x3092ea,row,v(m,packet+16));vw(m,o,0x3092f1,row+16,v(m,packet+48));vw(m,o,0x3092f9,row+32,v(m,packet+64));
        w(m,o,0x309300,row+44,4,r(m,packet+32));w(m,o,0x309306,row+28,4,key);w(m,o,0x309309,row+12,4,r(m,packet+36));return;
    }
    const auto row=si(count)<4?bank+count*48u:frame+128;
    vw(m,o,0x30932a,row,v(m,packet+16));vw(m,o,0x309331,row+16,v(m,packet+48));vw(m,o,0x309339,row+32,v(m,packet+64));
    w(m,o,0x309340,row+12,4,r(m,packet+36));w(m,o,0x309349,row+44,4,r(m,packet+32));
    source_transform_physics_contact_point_1_1(m,frame+64,r(m,a[0]+8)+32,row,fp,o);
    source_transform_physics_contact_point_1_1(m,frame+48,r(m,a[1]+8)+32,row+16,fp,o);
    const auto weight=r(m,packet+36),distance=r(m,row+44);w(m,o,0x309389,frame+20,4,weight);
    const auto wgt=_mm_set1_ps(f(weight)),normal=v(m,row+32);
    const auto point=_mm_add_ps(_mm_mul_ps(_mm_sub_ps(v(m,0x418a60),wgt),v(m,frame+48)),_mm_mul_ps(wgt,v(m,frame+64)));
    vw(m,o,0x3093b1,frame+80,point);w(m,o,0x3093b6,frame+92,4,distance);vw(m,o,0x3093ca,frame+96,normal);
    const auto manager=a[4],index=required(m,s,0x3093cf,r(m,r(m,manager)+4),manager,frame-20u,{a[0],a[1],a[2],frame+80},o);
    if((index&0xffffu)==0xffffu)return;
    w(m,o,0x3093e4,row+28,4,(index&0xffffu)|0x3f000000);const auto newcount=r(m,a[6])+1u;w(m,o,0x3093ef,a[6],4,newcount);
    if(si(newcount)>4) {
        w(m,o,0x3093fc,frame+28,4,bank);w(m,o,0x309405,frame+40,4,bank+144);w(m,o,0x30940d,frame+32,4,bank+48);w(m,o,0x309415,frame+36,4,bank+96);w(m,o,0x30941f,frame+44,4,row);
        const auto n=required(m,s,0x309423,0x308fb0,std::nullopt,frame-20u,{frame+28,a[0],a[1],manager},o);w(m,o,0x30942b,a[6],4,n);
    }
}
void source_emit_retained_physics_contacts_1_1(M&m,const std::array<std::uint32_t,8>&a,std::uint32_t esp,FP fp,const S&s,const O&o) {
    FpScope scope(fp);const auto frame=((esp-4u)&~15u)-0x30u;
    w(m,o,0x309473,frame+40,4,bits(square_spill(f(r(m,r(m,a[2]+12)+8)))));
    const auto descriptor=r(m,a[6]),old=r(m,descriptor+4),wanted=old+r(m,a[5]);auto cap=r(m,descriptor+8)&0x7fffffffu;
    w(m,o,0x309488,frame+28,4,0);w(m,o,0x309490,frame+24,4,descriptor);
    if(si(cap)<si(wanted))required(m,s,0x3094a2,0x2df710,std::nullopt,frame-16u,{descriptor,si(wanted)>=si(cap*2u)?wanted:cap*2u,48},o);
    auto out=r(m,descriptor)+old*48u;w(m,o,0x3094b6,descriptor+4,4,wanted);auto row=a[4];
    while(si(r(m,frame+28))<si(r(m,a[5]))) {
        const auto pa=transform(m,r(m,a[0]+8)+32,v(m,row)),pb=transform(m,r(m,a[1]+8)+32,v(m,row+16)),normal=v(m,row+32);
        const auto projection=dot(_mm_sub_ps(pa,pb),normal);w(m,o,0x309582,frame+24,4,bits(projection));w(m,o,0x309594,frame+32,4,r(m,frame+24));
        const auto tangent=_mm_sub_ps(pa,_mm_add_ps(pb,_mm_mul_ps(_mm_set1_ps(projection),normal)));const auto d2=dot(tangent,tangent);w(m,o,0x3095d9,frame+36,4,bits(d2));
        if(projection<f(r(m,a[2]+8))&&projection>f(a[3])&&d2<f(r(m,frame+40))) {
            const auto weight=r(m,row+12);w(m,o,0x309603,frame+44,4,weight);const auto wt=_mm_set1_ps(f(weight));
            const auto point=_mm_add_ps(_mm_mul_ps(_mm_sub_ps(v(m,0x418a60),wt),pb),_mm_mul_ps(wt,pa));
            vw(m,o,0x309627,out,point);vw(m,o,0x30962e,out+16,v(m,row+32));w(m,o,0x309632,out+12,4,r(m,frame+24));w(m,o,0x309639,out+32,2,r(m,row+28,2));
            w(m,o,0x309645,frame+28,4,r(m,frame+28)+1u);row+=48;out+=48;
        }else {
            const auto manager=a[7];required(m,s,0x30965a,r(m,r(m,manager)+8),manager,frame-8u,{r(m,row+28,2)},o);
            const auto n=r(m,a[5])-1u;w(m,o,0x309660,a[5],4,n);const auto last=a[4]+n*48u;
            vw(m,o,0x309673,row,v(m,last));vw(m,o,0x30967b,row+16,v(m,last+16));vw(m,o,0x309685,row+32,v(m,last+32));
            const auto d=r(m,a[6]);w(m,o,0x30968b,d+4,4,r(m,d+4)-1u);
        }
    }
}
std::uint16_t source_create_physics_managed_contact_1_1(M&m,std::uint32_t manager,const std::array<std::uint32_t,4>&a,std::uint32_t esp,FP fp,const S&s,const O&o) {
    FpScope scope(fp);const auto frame=((esp-4u)&~15u)-0x80u;
    w(m,o,0x31f1ca,frame+40,4,manager);
    if(si(r(m,manager+0xa4))>250)return 0xffff;
    const auto property_a=r(m,root(m,a[0])+32),property_b=r(m,root(m,a[1])+32);
    w(m,o,0x31f1f4,frame+44,4,property_a);w(m,o,0x31f20e,frame+28,4,property_b);
    if(!r(m,manager+0xa4))required(m,s,0x31f21b,0x313800,r(m,manager+8),frame-8u,{manager+16},o);
    const auto index=required(m,s,0x31f22d,0x322410,manager+16,frame-12u,{frame+36,frame+24},o);
    const auto motion_a=r(m,property_a+0x3c),motion_b=r(m,property_b+0x3c);
    for(unsigned k=0;k<2;++k) {
        const auto motion=k?motion_b:motion_a,entry=r(m,r(m,motion)+0x58),out=frame+(k?64u:48u);
        if(entry==0x31feb0)source_physics_contact_point_velocity_1_1(m,motion,a[3],out,fp,o);
        else required(m,s,k?0x31f254:0x31f242,entry,motion,frame-12u,{a[3],out},o);
    }
    const auto velocity=dot(_mm_sub_ps(v(m,frame+64),v(m,frame+48)),v(m,a[3]+16));w(m,o,0x31f28b,frame+32,4,bits(velocity));
    const auto scalar=r(m,frame+24),point=r(m,frame+36);
    w(m,o,0x31f2c9,frame+92,2,index&0xffffu);w(m,o,0x31f2ce,frame+104,4,a[1]);w(m,o,0x31f2d4,frame+124,4,0);
    const float mat_a=f(r(m,property_a+76)),mat_b=f(r(m,property_b+76)),rest_a=f(r(m,property_a+80)),rest_b=f(r(m,property_b+80));float friction,restitution;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("flds %1\n\tfmuls %2\n\tfsqrt\n\tfstps %0":"=m"(friction):"m"(mat_b),"m"(mat_a):"st");
    asm volatile("flds %1\n\tfmuls %2\n\tfsqrt\n\tfstps %0":"=m"(restitution):"m"(rest_b),"m"(rest_a):"st");
#else
    missing("Original managed contact x87 material owner unavailable");
#endif
    w(m,o,0x31f2dc,scalar+8,4,bits(friction));w(m,o,0x31f2ec,frame+100,4,a[0]);w(m,o,0x31f2f2,scalar+12,4,bits(restitution));
    vw(m,o,0x31f2ff,point,v(m,a[3]));vw(m,o,0x31f306,point+16,v(m,a[3]+16));w(m,o,0x31f30a,point+12,4,x87_copy(r(m,a[3]+12)));
    w(m,o,0x31f319,frame+112,4,point);w(m,o,0x31f321,frame+116,4,scalar);w(m,o,0x31f329,frame+120,4,r(m,frame+32));w(m,o,0x31f332,frame+96,4,manager);
    required(m,s,0x31f336,0x31e8b0,std::nullopt,frame-12u,{r(m,manager+8),frame+92},o);
    if(r(m,property_a+100))required(m,s,0x31f34f,0x31c2e0,std::nullopt,frame-12u,{property_a,frame+92},o);
    if(r(m,property_b+100))required(m,s,0x31f368,0x31c2e0,std::nullopt,frame-12u,{property_b,frame+92},o);
    if(r(m,frame+124)==1) {
        required(m,s,0x31f382,r(m,r(m,manager)+8),manager,frame-8u,{index},o);return 0xffff;
    }
    const auto restitution_now=f(r(m,scalar+12)),scale=f(r(m,a[2]+48)),relative=f(r(m,frame+32)),tuning=f(r(m,0x4652bc)),distance=f(r(m,a[3]+12));float bias,limit,impulse;
    const auto linear_a=f(r(m,motion_a+44)),linear_b=f(r(m,motion_b+44)),epsilon=f(r(m,0x41b6a8)),one=f(r(m,0x465030)),factor=f(r(m,0x4650cc));
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("flds %2\n\tfmuls %3\n\tfmuls %4\n\tfmuls %5\n\tfsts %0\n\tfadds %6\n\tfstps %1":"=m"(bias),"=m"(limit):"m"(restitution_now),"m"(scale),"m"(relative),"m"(tuning),"m"(distance):"st");
    asm volatile("flds %1\n\tfadds %2\n\tfadds %3\n\tfdivrs %4\n\tflds %5\n\tfadds %4\n\tfmulp\n\tfmuls %6\n\tfmuls %7\n\tfstps %0":"=m"(impulse):"m"(linear_a),"m"(linear_b),"m"(epsilon),"m"(one),"m"(restitution_now),"m"(relative),"m"(factor):"st");
#else
    missing("Original managed contact x87 bias owner unavailable");
#endif
    w(m,o,0x31f3a9,scalar+4,4,bits(bias));w(m,o,0x31f3b3,scalar+20,4,bits(limit));w(m,o,0x31f3e9,scalar,4,bits(impulse));return std::uint16_t(index);
}
} // namespace halo2
