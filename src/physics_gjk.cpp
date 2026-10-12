#include "halo2/physics_gjk.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M=SourcePhysicsContactMemory;using O=SourcePhysicsContactRecordObserver;
using S=SourcePhysicsGjkServices;using FP=SourcePhysicsFactoryFloatingPoint;
std::uint32_t r(const M&m,std::uint32_t p,unsigned n=4) {
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t x=0;
    for(unsigned k=0;k<n;++k)x|=std::to_integer<std::uint32_t>(b[k])<<(k*8u);
    return x;
}

void w(M&m,const O&o,std::uint32_t site,std::uint32_t p,unsigned n,std::uint64_t x) {
    SourcePhysicsContactRecordStore a{site,p,{},{}};a.before.resize(n);a.written.resize(n);m.read(p,a.before);
    for(unsigned k=0;k<n;++k)a.written[k]=std::byte((x>>(k*8u))&255u);
    m.write(p,a.written);if(o)o(a);
}
[[noreturn]]void missing(const char*s){throw std::runtime_error(s);}
std::uint32_t required(M&m,const S&s,std::uint32_t site,std::uint32_t entry,
    std::uint32_t ecx,std::uint32_t esp,std::vector<std::uint32_t>args,const O&o) {
    if(!s.required_owner)missing("Required original GJK owner unavailable");
    return s.required_owner(m,{site,entry,ecx,esp,std::nullopt,std::nullopt,std::nullopt,std::nullopt,std::move(args),std::nullopt},o);
}
struct FpScope {
    unsigned mx;std::fenv_t env;
    explicit FpScope(FP p):mx(_mm_getcsr()) {
        if((p.x87_control_word&0x3fu)!=0x3fu||(p.mxcsr&0x1f80u)!=0x1f80u)
            missing("Unmasked original GJK FP effects unavailable");
        if(std::fegetenv(&env)!=0)missing("Original GJK FP environment unavailable");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("fldcw %0" : :"m"(p.x87_control_word):"memory");
#else
        missing("Original GJK x87 backend unavailable");
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
__m128 inverse(const M&m,std::uint32_t matrix,__m128 a) {
    const auto x=v(m,matrix),y=v(m,matrix+16),z=v(m,matrix+32);
    const auto xy=_mm_shuffle_ps(x,y,0x44),xy2=_mm_shuffle_ps(x,y,0xee);
    const auto zz=_mm_shuffle_ps(z,z,0x44),zz2=_mm_shuffle_ps(z,z,0xee);
    const auto tx=_mm_shuffle_ps(xy,zz,0x88),ty=_mm_shuffle_ps(xy,zz,0xdd),tz=_mm_shuffle_ps(xy2,zz2,0x88);
    return _mm_add_ps(_mm_add_ps(_mm_mul_ps(tx,_mm_shuffle_ps(a,a,0)),
        _mm_mul_ps(ty,_mm_shuffle_ps(a,a,0x55))),_mm_mul_ps(tz,_mm_shuffle_ps(a,a,0xaa)));
}
std::int32_t si(std::uint32_t x){return std::bit_cast<std::int32_t>(x);}
std::uint32_t low(__m128 x){return bits(_mm_cvtss_f32(x));}
__m128 scalar(std::uint32_t x){return _mm_set_ss(f(x));}
__m128 dot_vector(__m128 a,__m128 b) {
    const auto x=_mm_mul_ps(a,b);
    return _mm_add_ss(_mm_shuffle_ps(x,x,0xaa),_mm_add_ss(_mm_shuffle_ps(x,x,0x55),x));
}
__m128 point_vector(const M&m,std::uint32_t matrix,__m128 a) {
    return _mm_add_ps(_mm_add_ps(_mm_mul_ps(v(m,matrix),_mm_shuffle_ps(a,a,0)),
        _mm_mul_ps(v(m,matrix+16),_mm_shuffle_ps(a,a,0x55))),
        _mm_add_ps(_mm_mul_ps(v(m,matrix+32),_mm_shuffle_ps(a,a,0xaa)),v(m,matrix+48)));
}
long double xmul(long double x,long double y){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double z;asm volatile("fldt %1\n\tfldt %2\n\tfmulp\n\tfstpt %0":"=m"(z):"m"(x),"m"(y):"st");return z;
#else
    (void)x;(void)y;missing("Original GJK x87 multiply backend unavailable");
#endif
}
long double xadd(long double x,long double y){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double z;asm volatile("fldt %1\n\tfldt %2\n\tfaddp\n\tfstpt %0":"=m"(z):"m"(x),"m"(y):"st");return z;
#else
    (void)x;(void)y;missing("Original GJK x87 addition backend unavailable");
#endif
}
long double xsub(long double x,long double y){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double z;asm volatile("fldt %1\n\tfldt %2\n\tfsubrp\n\tfstpt %0":"=m"(z):"m"(x),"m"(y):"st");return z;
#else
    (void)x;(void)y;missing("Original GJK x87 subtraction backend unavailable");
#endif
}
long double xdiv(long double x,long double y){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double z;asm volatile("fldt %1\n\tfldt %2\n\tfdivrp\n\tfstpt %0":"=m"(z):"m"(x),"m"(y):"st");return z;
#else
    (void)x;(void)y;missing("Original GJK x87 division backend unavailable");
#endif
}
float xspill(long double x){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    float z;asm volatile("fldt %1\n\tfstps %0":"=m"(z):"m"(x):"st");return z;
#else
    (void)x;missing("Original GJK x87 spill backend unavailable");
#endif
}
bool negative(__m128 x){return (_mm_movemask_ps(x)&1)!=0;}
void support(M&m,const S&s,std::uint32_t shape,std::uint32_t dir,std::uint32_t out,
    std::uint32_t site,std::uint32_t esp,const O&o) {
    const auto entry=r(m,r(m,shape)+0x30),frame=((esp-4u)&~15u)-16u;
    if(entry==0x2e22b0) {
        support(m,s,r(m,shape+0x30),dir,out,0x2e22c9,frame-12u,o);
        const auto shift=v(m,shape+16);const auto d=dot_vector(shift,v(m,dir));
        w(m,o,0x2e22f3,frame+12,4,low(d));
        if(_mm_cvtss_f32(d)>std::bit_cast<float>(r(m,0x465048))) {
            const auto old_w=r(m,out+12);vw(m,o,0x2e2311,out,_mm_add_ps(v(m,out),shift));
            w(m,o,0x2e2314,out+12,4,old_w);
        }
        return;
    }
    if(entry==0x1f1d20) {
        auto best=scalar(r(m,0x447020));std::uint32_t index=0;const auto count=r(m,shape+0x14);
        if(si(count)>0) {
            const auto direction=v(m,dir);
            for(std::uint32_t k=0;si(k)<si(count);++k) {
                const auto d=dot_vector(v(m,shape+32+k*16u),direction);
                w(m,o,0x1f1d66,frame+12,4,low(d));
                if(_mm_cvtss_f32(d)>_mm_cvtss_f32(best)){best=d;index=k;}
            }
        }
        vw(m,o,0x1f1d91,out,v(m,shape+32+index*16u));return;
    }
    if(entry==0x2e42a0) {
        const auto f=((esp-4u)&~15u)-16u;const auto first=v(m,shape+16),second=v(m,shape+32);
        const auto d=dot_vector(_mm_sub_ps(second,first),v(m,dir));w(m,o,0x2e42da,f+12,4,low(d));
        // FCOMP; TEST AH,5; JP selects the second endpoint for greater, equal
        // and unordered. Only ordered less selects the first endpoint.
        const bool less=_mm_cvtss_f32(d)<std::bit_cast<float>(r(m,0x465048));
        vw(m,o,less?0x2e42f9:0x2e430f,out,less?first:second);
        w(m,o,less?0x2e42fc:0x2e4312,out+12,4,less?0x3f000000u:0x3f000010u);return;
    }
    if(entry==0x3037f0) {
        const auto f=((esp-4u)&~15u)-32u,bank=r(m,shape+0x30),count=r(m,shape+0x34)*4u;
        auto maximum=_mm_set1_ps(std::bit_cast<float>(0xff7fffffu));std::uint32_t selected=0;
        w(m,o,0x303802,f+12,4,0xff7fffffu);
        if(si(count)>0) {
            w(m,o,0x303828,f+12,4,r(m,dir));const auto dx=_mm_set1_ps(std::bit_cast<float>(r(m,dir)));
            w(m,o,0x303838,f+12,4,r(m,dir+4));const auto dy=_mm_set1_ps(std::bit_cast<float>(r(m,dir+4)));
            w(m,o,0x303842,f+12,4,r(m,dir+8));const auto dz=_mm_set1_ps(std::bit_cast<float>(r(m,dir+8)));
            for(std::uint32_t k=0;si(k)<si(count);k+=4) {
                const auto p=bank+(k>>2)*48u;
                const auto dots=_mm_add_ps(_mm_add_ps(_mm_mul_ps(dx,v(m,p)),_mm_mul_ps(dy,v(m,p+16))),_mm_mul_ps(dz,v(m,p+32)));
                const auto mask=std::uint32_t(_mm_movemask_ps(_mm_cmplt_ps(maximum,dots)));
                vw(m,o,0x303892,f+16,dots);
                if(mask) {
                    // Literal original jump-table entry and strict x87 compares.
                    const auto target=mask>14?0x3038b0u:r(m,0x303988+r(m,0x3039a4+mask,1)*4u);
                    std::array<float,4>d{};_mm_storeu_ps(d.data(),dots);unsigned lane;
                    switch(target) {
                    case 0x3038dd:lane=0;break;
                    case 0x303904:lane=1;break;
                    case 0x30391c:lane=2;break;
                    case 0x303925:lane=3;break;
                    case 0x3038bf:lane=d[0]>d[2]?(d[0]>d[3]?0:3):(d[2]>d[3]?2:3);break;
                    case 0x3038e6:lane=d[1]>d[2]?(d[1]>d[3]?1:3):(d[2]>d[3]?2:3);break;
                    case 0x3038b0:lane=d[0]>d[1]?(d[0]>d[2]?(d[0]>d[3]?0:3):(d[2]>d[3]?2:3)):
                        (d[1]>d[2]?(d[1]>d[3]?1:3):(d[2]>d[3]?2:3));break;
                    default:missing("Original convex support jump owner unavailable");
                    }
                    selected=k+lane;w(m,o,0x30392b,f+12,4,std::bit_cast<std::uint32_t>(d[lane]));maximum=_mm_set1_ps(d[lane]);
                }
            }
        }
        const auto p=bank+(selected>>2)*48u+(selected&3u)*4u;
        w(m,o,0x303965,out,4,r(m,p));w(m,o,0x30396b,out+4,4,r(m,p+16));
        w(m,o,0x303972,out+8,4,r(m,p+32));w(m,o,0x30397b,out+12,4,selected|0x3f000000u);return;
    }
    required(m,s,site,entry,shape,esp,{dir,out},o);
}
}

void source_retrieve_physics_support_points_1_1(M&m,std::uint32_t shape,const std::array<std::uint32_t,3>&a,std::uint32_t esp,FP fp,const S&s,const O&o,std::uint32_t call_instruction) {
    FpScope scope(fp);const auto method=r(m,r(m,shape)+0x34),keys=a[0],count=a[1],output=a[2];
    if(method==0x2e2320) {
        const auto frame=((esp-4u)&~15u)-16u;
        source_retrieve_physics_support_points_1_1(m,r(m,shape+0x30),a,frame-16u,fp,s,o,0x2e233d);
        for(std::uint32_t k=0;std::bit_cast<std::int32_t>(k)<std::bit_cast<std::int32_t>(count);++k) {
            const auto p=output+k*16u,encoded=r(m,p+12);vw(m,o,0x2e235d,p,_mm_add_ps(v(m,p),v(m,shape+16)));
            w(m,o,0x2e2360,p+12,4,encoded);
        }
        return;
    }
    if(method==0x1f1da0) {
        for(auto k=count;std::bit_cast<std::int32_t>(k)>0;) {
            --k;vw(m,o,0x1f1dce,output+k*16u,v(m,shape+(r(m,keys+k*2u,2)+2u)*16u));
        }
        return;
    }
    if(method==0x2e4320) {
        for(std::uint32_t k=0;std::bit_cast<std::int32_t>(k)<std::bit_cast<std::int32_t>(count);++k) {
            const auto key=r(m,keys+k*2u,2);vw(m,o,0x2e433c,output+k*16u,v(m,shape+16+key));
            w(m,o,0x2e4348,output+k*16u+12,4,r(m,keys+k*2u,2)|0x3f000000u);
        }
        return;
    }
    if(method==0x3039c0) {
        w(m,o,0x3039d8,esp-4u,4,shape);auto keyp=keys,out=output,remainder=count-1u;
        if(std::bit_cast<std::int32_t>(count)>=4) {
            auto groups=count>>2;remainder=count-1u-groups*4u;
            w(m,o,0x3039e5,esp+12,4,groups);w(m,o,0x3039f1,esp+8,4,remainder);
            const std::array<std::array<std::uint32_t,4>,4>sites{{{0x303a10,0x303a17,0x303a1e,0x303a2b},
                {0x303a4a,0x303a51,0x303a58,0x303a64},{0x303a83,0x303a8a,0x303a95,0x303a9e},
                {0x303abd,0x303ac4,0x303ad5,0x303ad8}}};
            do {
                for(unsigned k=0;k<4;++k) {
                    const auto key=r(m,keyp+k*2u,2),bank=r(m,shape+48)+(key>>2)*48u+(key&3u)*4u;
                    w(m,o,sites[k][0],out+k*16u,4,r(m,bank));w(m,o,sites[k][1],out+k*16u+4,4,r(m,bank+16));
                    w(m,o,sites[k][2],out+k*16u+8,4,r(m,bank+32));w(m,o,sites[k][3],out+k*16u+12,4,key|0x3f000000u);
                }
                keyp+=8;out+=64;--groups;w(m,o,0x303ae5,esp+12,4,groups);
            }while(groups);
        }
        if(std::bit_cast<std::int32_t>(remainder)>=0) {
            auto left=remainder+1u;w(m,o,0x303afc,esp+12,4,left);
            do {
                const auto key=r(m,keyp,2),bank=r(m,shape+48)+(key>>2)*48u+(key&3u)*4u;
                w(m,o,0x303b1b,out,4,r(m,bank));w(m,o,0x303b22,out+4,4,r(m,bank+16));
                w(m,o,0x303b29,out+8,4,r(m,bank+32));w(m,o,0x303b36,out+12,4,key|0x3f000000u);
                keyp+=2;out+=16;--left;w(m,o,0x303b40,esp+12,4,left);
            }while(left);
        }
        return;
    }
    if(!call_instruction)missing("Required original retrieve caller instruction unavailable");
    required(m,s,call_instruction,method,shape,esp,{a.begin(),a.end()},o);
}
std::uint32_t source_physics_gjk_segment_pair_1_1(M&m,const std::array<std::uint32_t,5>&a,std::uint32_t esp,FP fp,const O&o) {
    FpScope scope(fp);const auto frame=((esp-4u)&~15u)-48u;
    const auto da=v(m,a[1]),pa=v(m,a[0]),delta=_mm_sub_ps(v(m,a[2]),pa),db=v(m,a[3]);
    const auto ab=dot(da,db);w(m,o,0x30eb76,frame+20,4,bits(ab));
    const auto ad=dot(da,delta),bd=dot(db,delta);w(m,o,0x30ebb6,frame+40,4,bits(bd));
    const auto aa=dot(da,da),bb=dot(db,db);w(m,o,0x30ebea,frame+44,4,bits(ad));
    w(m,o,0x30ebfe,frame+24,4,bits(bb));w(m,o,0x30ec0a,frame+28,4,bits(aa));
    // Each operation executes source x87 arithmetic under the actual control
    // word. Float spills do not prematurely replace retained extended values.
    const auto mul=xmul,add=xadd,sub=xsub,div=xdiv;
    const auto spill=xspill;
    const auto product=spill(mul(bb,aa));w(m,o,0x30ec12,frame+36,4,bits(product));
    const auto square=spill(mul(ab,ab));w(m,o,0x30ec1e,frame+32,4,bits(square));
    const auto determinant=spill(std::fabs(sub(product,square)));w(m,o,0x30ec2c,frame+12,4,bits(determinant));
    const auto numerator=sub(mul(bb,ad),mul(bd,ab));const auto numerator_word=spill(numerator);
    w(m,o,0x30ec42,frame+16,4,bits(numerator_word));
    long double t;std::uint32_t mask;
    const auto left=mul(determinant,determinant),right=mul(numerator,determinant);const auto zero=f(r(m,0x465048));
    if(left<=right) {t=f(r(m,0x465030));mask=1;}
    else if(!(numerator_word>zero)&&!std::isnan(numerator_word)) {t=zero;mask=2;}
    else {
        const auto threshold=mul(add(square,product),f(r(m,0x4651bc)));
        if(!(threshold<determinant)){t=f(r(m,0x465030));mask=1;}
        else {t=div(numerator_word,determinant);mask=0;}
    }
    const auto u_value=sub(mul(ab,t),bd);auto u=spill(u_value);w(m,o,0x30ecb3,frame+12,4,bits(u));
    bool clamp=false;
    if(!(u_value<bb)&&!std::isnan(u_value)) {u=1.0f;mask=4;w(m,o,0x30ecc2,frame+12,4,0x3f800000);clamp=true;}
    else if(!(u>zero)&&!std::isnan(u)) {u=0.0f;mask=8;w(m,o,0x30ece2,frame+12,4,0);clamp=true;}
    else {u=spill(div(u,bb));w(m,o,0x30ed45,frame+12,4,bits(u));}
    if(clamp) {
        const auto candidate=add(mul(ab,u),ad);const auto word=spill(candidate);w(m,o,0x30ecfd,frame+16,4,bits(word));
        if(!(candidate>zero)&&!std::isnan(candidate)){t=zero;mask|=2;}
        else if(!(word<aa)&&!std::isnan(word)){t=f(r(m,0x465030));mask|=1;}
        else t=div(word,aa);
    }
    const auto parameter=spill(t);w(m,o,0x30ed4c,frame+44,4,bits(parameter));
    const auto point=_mm_add_ps(pa,_mm_mul_ps(_mm_set1_ps(parameter),da));w(m,o,0x30ed67,frame+44,4,bits(u));
    vw(m,o,0x30ed71,a[4],point);
    const auto difference=_mm_sub_ps(point,_mm_add_ps(v(m,a[2]),_mm_mul_ps(_mm_set1_ps(u),v(m,a[3]))));
    vw(m,o,0x30ed8a,a[4]+16,difference);const auto distance=dot(v(m,a[4]+16),v(m,a[4]+16));
    w(m,o,0x30edaf,frame+44,4,bits(distance));w(m,o,0x30edb8,a[4]+32,4,r(m,frame+44));return mask;
}
std::uint32_t source_physics_gjk_triangle_side_1_1(M&m,std::uint32_t workspace,const std::array<std::uint32_t,3>&a,std::uint32_t esp,FP fp,const O&o) {
    FpScope scope(fp);const auto frame=((esp-4u)&~15u)-32u;
    auto x3=v(m,a[1]),x7=v(m,a[1]+16),x2=v(m,a[1]+32);
    auto x1=_mm_sub_ps(x3,x2),x5=_mm_shuffle_ps(x1,x1,0xc9),x4=_mm_shuffle_ps(x1,x1,0xd2);
    auto x0=_mm_sub_ps(x2,x7);x2=_mm_sub_ps(x7,x3);x3=_mm_shuffle_ps(x0,x0,0xd2);
    auto x6=_mm_shuffle_ps(x0,x0,0xc9);x0=_mm_mul_ps(x3,x5);x1=x0;x0=x6;
    vw(m,o,0x309955,frame+16,x4);x0=_mm_mul_ps(x0,x4);x4=v(m,a[0]);x0=_mm_sub_ps(x0,x1);
    x1=_mm_sub_ps(x4,x7);x7=_mm_mul_ps(_mm_shuffle_ps(x1,x1,0xd2),x6);
    x6=_mm_shuffle_ps(x1,x1,0xc9);x1=_mm_mul_ps(x6,x3);x6=v(m,a[1]+32);x3=_mm_sub_ps(x4,x6);
    x6=_mm_mul_ps(_mm_shuffle_ps(x3,x3,0xd2),x5);x5=_mm_mul_ps(_mm_shuffle_ps(x3,x3,0xc9),v(m,frame+16));
    x3=v(m,a[1]);x4=_mm_sub_ps(x4,x3);x5=_mm_sub_ps(x5,x6);x3=_mm_shuffle_ps(x2,x2,0xc9);
    x6=_mm_mul_ps(_mm_shuffle_ps(x4,x4,0xd2),x3);x3=_mm_shuffle_ps(x2,x2,0xd2);x1=_mm_sub_ps(x1,x7);
    x2=_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(x4,x4,0xc9),x3),x6);
    x3=x1;x1=_mm_shuffle_ps(x1,x5,0xee);x7=_mm_shuffle_ps(x2,x2,0xee);x1=_mm_shuffle_ps(x1,x7,0x88);
    x3=_mm_shuffle_ps(x3,x5,0x44);x4=_mm_shuffle_ps(x2,x2,0x44);x6=_mm_shuffle_ps(x0,x0,0xaa);
    x1=_mm_mul_ps(x1,x6);x2=x1;x5=_mm_shuffle_ps(x3,x4,0xdd);x1=_mm_shuffle_ps(x0,x0,0x55);
    x5=_mm_mul_ps(x5,x1);x1=_mm_shuffle_ps(x0,x0,0);x3=_mm_mul_ps(_mm_shuffle_ps(x3,x4,0x88),x1);
    x1=_mm_add_ps(_mm_add_ps(x3,x5),x2);const auto mask=std::uint32_t(_mm_movemask_ps(x1))&7u;
    vw(m,o,0x309a2e,workspace+0x160,x1);if(!a[2])return mask;
    vw(m,o,0x309a3e,workspace+0x120,x0);if(mask!=7u)return mask;
    x1=v(m,workspace+32);x2=_mm_sub_ps(v(m,workspace+0xa0),x1);x1=_mm_mul_ps(x0,x2);
    x2=_mm_shuffle_ps(x1,x1,0x55);x3=_mm_shuffle_ps(x1,x1,0xaa);x2=_mm_add_ss(x2,x1);x3=_mm_add_ss(x3,x2);
    if((std::uint32_t(_mm_movemask_ps(x3))&1u)==0u)return mask;
    vw(m,o,0x309a88,workspace+0x120,_mm_xor_ps(x0,v(m,0x418ae0)));
    x0=v(m,a[1]);x1=v(m,a[1]+16);vw(m,o,0x309a96,a[1],x1);vw(m,o,0x309a99,a[1]+16,x0);
    x0=v(m,a[1]+64);x1=v(m,a[1]+80);vw(m,o,0x309aa5,a[1]+64,x1);vw(m,o,0x309aa9,a[1]+80,x0);
    const auto old=r(m,workspace+0x160);w(m,o,0x309ab9,workspace+0x160,4,r(m,workspace+0x164));
    w(m,o,0x309abf,workspace+0x164,4,old);return mask;
}
std::uint32_t source_physics_gjk_1_1(M&m,std::uint32_t d,const std::array<std::uint32_t,5>&a,std::uint32_t esp,FP fp,
    const SourcePhysicsReciprocalSqrtEstimate&estimate,const S&s,const O&o) {
    FpScope scope(fp);const auto frame=((esp-4u)&~15u)-128u;
    const auto signed_byte=[&](std::uint32_t p){return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(r(m,p,1)))));};
    const auto cross=[](__m128 x,__m128 y){return _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(x,x,0xc9),_mm_shuffle_ps(y,y,0xd2)),
        _mm_mul_ps(_mm_shuffle_ps(x,x,0xd2),_mm_shuffle_ps(y,y,0xc9)));};
    const auto field=[&](unsigned offset){return r(m,d+offset);};
    const auto store=[&](std::uint32_t site,unsigned offset,std::uint32_t value){w(m,o,site,d+offset,4,value);};
    const auto stack=[&](std::uint32_t site,unsigned offset,std::uint32_t value){w(m,o,site,frame+offset,4,value);};
    //30AE54..30AE95: cache-owned encoded keys and signed counts select actual
    // shape retrieve virtuals. Original entry frames are retained for aliasing.
    store(0x30ae54,0,signed_byte(a[3]+8));store(0x30ae64,4,signed_byte(a[3]+9));
    stack(0x30ae6d,28,0);stack(0x30ae75,20,0x2edbe6ff);
    source_retrieve_physics_support_points_1_1(m,a[0],{a[3],field(0),d+32},frame-16u,fp,s,o,0x30ae7d);
    source_retrieve_physics_support_points_1_1(m,a[1],{a[3]+field(0)*2u,field(4),d+0xe0},frame-16u,fp,s,o,0x30ae95);
    for(std::uint32_t k=0;si(k)<si(field(4));++k)vw(m,o,0x30aef9,d+0xa0+k*16u,point_vector(m,a[2],v(m,d+0xe0+k*16u)));
    stack(0x30af03,32,0);
    std::uint32_t target=0,mask=0,axis=0;
    // Source tail routines use the current literal ECX/EBX/EAX register ABI.
    const auto fallback=[&](std::uint32_t site,std::uint32_t entry,std::uint32_t ecx,std::uint32_t e,
        std::vector<std::uint32_t>args,std::optional<std::uint32_t>eax={},std::optional<std::uint32_t>ebx={}) {
        if(!s.required_owner)missing("Required original GJK simplex owner unavailable");
        return s.required_owner(m,{site,entry,ecx,e,eax,std::nullopt,std::nullopt,std::nullopt,std::move(args),ebx},o);
    };
iteration:
    stack(0x30af23,20,bits(xspill(static_cast<long double>(f(r(m,frame+20)))*2.0L)));
    stack(0x30af2e,28,r(m,frame+28)+1u);store(0x30af32,16,0);
dispatch:
    {const auto key=((field(0)<<3)|field(4))-9u;
    if(key>24u){axis=1;goto direction;}
    target=r(m,0x30bbf0+r(m,0x30bc1c+key,1)*4u);}
    switch(target) {
    case 0x30af4e:case 0x30af6c: {
        const bool b=target==0x30af4e;
        w(m,o,b?0x30af5f:0x30af7b,frame-8u,1,field(8)==2u);
        mask=fallback(0x30af8b,0x309d40,d,frame-28u,
            b?std::vector<std::uint32_t>{d+32,d+0xa0,d,d+4,field(8)==2u,1u}:
              std::vector<std::uint32_t>{d+0xa0,d+32,d+4,d,field(8)==2u,0xffffffffu});
        if(mask==1u){axis=1;goto direction;}if(mask!=2u)goto ready;
        [[fallthrough]];
    }
    case 0x30af9e: {
        vw(m,o,0x30afc6,frame+96,_mm_sub_ps(v(m,d+48),v(m,d+32)));
        vw(m,o,0x30afd7,frame+80,_mm_sub_ps(v(m,d+0xb0),v(m,d+0xa0)));
        mask=source_physics_gjk_segment_pair_1_1(m,{d+32,frame+96,d+0xa0,frame+80,d+0x130},frame-24u,fp,o);
        if(!mask) {
            auto n=cross(v(m,frame+80),v(m,frame+96));
            const bool sign=negative(dot_vector(n,_mm_sub_ps(v(m,d+32),v(m,d+0xa0))));
            vw(m,o,0x30b518,d+0x120,n);if(sign)goto ready;
            vw(m,o,0x30b52f,d+0x120,_mm_xor_ps(n,v(m,0x418ae0)));store(0x30b538,8,field(4));axis=0;goto direction;
        }
        if(mask&1u)vw(m,o,0x30aff4,d+32,v(m,d+48));
        if(mask&3u)store(0x30affd,0,1);
        if(mask&4u) {
            vw(m,o,0x30b00e,d+0xa0,v(m,d+0xb0));vw(m,o,0x30b01c,d+0xe0,v(m,d+0xf0));store(0x30b023,4,1);
        }else if(mask&8u)store(0x30b036,4,1);
        goto dispatch;
    }
    case 0x30b041: {
        mask=fallback(0x30b04f,0x309ae0,3u,frame-8u,{0xbf800000u},d+0xa0,d+32);
        if(si(mask)<0){axis=1;goto direction;}
        store(0x30b06f,4,field(4)-1u);vw(m,o,0x30b074,d+(mask+10u)*16u,v(m,d+0xd0));
        vw(m,o,0x30b082,d+(mask+14u)*16u,v(m,d+0x110));
        [[fallthrough]];
    }
    case 0x30b086: {
        mask=source_physics_gjk_triangle_side_1_1(m,d,{d+32,d+0xa0,1},frame-16u,fp,o);
        if(mask==7u)goto ready;
        const auto selected=signed_byte(0x419ce8+mask);
        if(si(selected)<0) {
            auto index=selected+8u;if(si(index)>3)index=1;
            store(0x30b55f,4,field(4)-1u);vw(m,o,0x30b564,d+(index+10u)*16u,v(m,d+0xc0));
            vw(m,o,0x30b572,d+(index+14u)*16u,v(m,d+0x100));goto b_line;
        }
        const auto dx=signed_byte(0x419cf0+selected);auto cx=signed_byte(0x419cf2+selected);
        const auto root=v(m,d+(selected+10u)*16u),p=_mm_sub_ps(v(m,d+32),root);
        const auto dx_edge=_mm_sub_ps(v(m,d+(dx+10u)*16u),root),cx_edge=_mm_sub_ps(v(m,d+(cx+10u)*16u),root);
        stack(0x30b0d9,16,d+(cx+10u)*16u);store(0x30b11b,4,2);
        if(negative(dot_vector(p,dx_edge))) {
            vw(m,o,0x30b123,d+(dx+10u)*16u,v(m,d+0xc0));vw(m,o,0x30b136,d+(dx+14u)*16u,v(m,d+0x100));
            if(negative(dot_vector(p,cx_edge))) {
                if(cx==field(4))cx=dx;
                store(0x30b164,4,field(4)-1u);vw(m,o,0x30b176,d+(cx+10u)*16u,v(m,d+(field(4)+10u)*16u));
                vw(m,o,0x30b18c,d+(cx+14u)*16u,v(m,d+(field(4)+14u)*16u));
            }
        }else {vw(m,o,0x30b19c,r(m,frame+16),v(m,d+0xc0));vw(m,o,0x30b1a9,d+(cx+14u)*16u,v(m,d+0x100));}
        goto dispatch;
    }
    case 0x30b1b2: {
        mask=fallback(0x30b1c0,0x309ae0,9u,frame-8u,{0x3f800000u},d+32,d+0xa0);
        if(si(mask)<0){axis=1;goto direction;}
        store(0x30b1dd,0,field(0)-1u);vw(m,o,0x30b1df,d+(mask+2u)*16u,v(m,d+80));
        [[fallthrough]];
    }
    case 0x30b1e3: {
        mask=source_physics_gjk_triangle_side_1_1(m,d,{d+0xa0,d+32,0xffffffffu},frame-16u,fp,o);
        if(mask==7u)goto ready;
        const auto selected=signed_byte(0x419ce8+mask);
        if(si(selected)<0) {
            auto index=selected+8u;if(si(index)>3)index=1;
            store(0x30b67f,0,field(0)-1u);vw(m,o,0x30b681,d+(index+2u)*16u,v(m,d+64));goto a_line;
        }
        const auto saved=signed_byte(0x419cf0+selected);auto cx=signed_byte(0x419cf2+selected);
        stack(0x30b21c,16,saved);
        const auto root=v(m,d+(selected+2u)*16u),p=_mm_sub_ps(v(m,d+0xa0),root);
        const auto dx_edge=_mm_sub_ps(v(m,d+(saved+2u)*16u),root),cx_edge=_mm_sub_ps(v(m,d+(cx+2u)*16u),root);
        store(0x30b279,0,2);
        if(negative(dot_vector(p,dx_edge))) {
            vw(m,o,0x30b284,d+(saved+2u)*16u,v(m,d+64));
            if(negative(dot_vector(p,cx_edge))) {
                if(cx==field(0))cx=r(m,frame+16);
                store(0x30b2b0,0,field(0)-1u);vw(m,o,0x30b2c2,d+(cx+2u)*16u,v(m,d+(field(0)+2u)*16u));
            }
        }else vw(m,o,0x30b2c8,d+(cx+2u)*16u,v(m,d+64));
        goto dispatch;
    }
    case 0x30b576:goto b_line;
    case 0x30b685:goto a_line;
    case 0x30b763:goto points;
    case 0x30b2e0:axis=1;goto direction;
    default:missing("Original GJK jump-table owner unavailable");
    }
b_line: {
    const auto second=v(m,d+0xb0),first=v(m,d+0xa0),p=v(m,d+32),edge=_mm_sub_ps(second,first);
    const auto x4=_mm_sub_ps(first,p),x3=_mm_sub_ps(second,p);
    const bool sign=negative(dot_vector(x3,edge));
    if(sign!=negative(dot_vector(x4,edge))) {
        vw(m,o,0x30b632,d+0x120,cross(cross(x3,x4),edge));store(0x30b63b,8,field(4));axis=0;goto direction;
    }
    if(sign){vw(m,o,0x30b650,d+0xa0,second);vw(m,o,0x30b657,d+0xe0,v(m,d+0xf0));}
    store(0x30b65e,4,field(4)-1u);goto points;
}
a_line: {
    const auto second=v(m,d+48),first=v(m,d+32),p=v(m,d+0xa0),edge=_mm_sub_ps(second,first);
    const auto x4=_mm_sub_ps(first,p),x3=_mm_sub_ps(second,p);
    const bool sign=negative(dot_vector(x3,edge));
    if(sign!=negative(dot_vector(x4,edge))) {
        vw(m,o,0x30b73e,d+0x120,cross(edge,cross(x3,x4)));store(0x30b747,8,field(4));axis=0;goto direction;
    }
    if(sign){vw(m,o,0x30b759,d+32,second);vw(m,o,0x30b75d,d+96,v(m,d+112));}
    store(0x30b761,0,field(0)-1u);
}
points:
    vw(m,o,0x30b771,d+0x120,_mm_sub_ps(v(m,d+0xa0),v(m,d+32)));
ready:
    store(0x30b77a,8,field(4));axis=0;
direction: {
    const auto n=v(m,d+0x120);stack(0x30b318,36,bits(dot(n,n)));
    stack(0x30b346,24,bits(dot(_mm_sub_ps(v(m,d+0xa0),v(m,d+32)),n)));
    if(axis)goto degenerate;
    const auto dot_word=f(r(m,frame+24)),length=f(r(m,frame+36)),epsilon=f(r(m,frame+20));
    if(!(xmul(xmul(epsilon,f(r(m,0x419bdc))),length)<xmul(xmul(dot_word,dot_word),dot_word)))goto degenerate;
    stack(0x30b386,76,r(m,frame+28)&1u);stack(0x30b38a,16,bits(xspill(xmul(length,epsilon))));
    if(!(r(m,frame+28)&1u)||field(0)==signed_byte(a[3]+10)||field(16)==1u)goto support_b;
    support(m,s,a[0],d+0x120,d+(field(0)+2u)*16u,0x30b3be,frame-12u,o);
    {const auto count=field(0);const auto point=v(m,d+(count+2u)*16u),normal=v(m,d+0x120);
    const auto projection=dot(_mm_sub_ps(point,v(m,d+32)),normal);stack(0x30b3fa,40,bits(projection));
    const auto limit=f(r(m,frame+16));if(!(xmul(std::fabs(static_cast<long double>(projection)),projection)>limit))goto support_b;
    if(count==3u) {const auto q=dot(_mm_sub_ps(point,v(m,d+64)),normal);stack(0x30b44f,44,bits(q));if(xmul(std::fabs(static_cast<long double>(q)),q)<limit)goto support_b;}
    if(count==2u||count==3u) {const auto q=dot(_mm_sub_ps(point,v(m,d+48)),normal);stack(0x30b490,48,bits(q));if(xmul(std::fabs(static_cast<long double>(q)),q)<limit)goto support_b;}
    store(0x30b4ae,0,count+1u);goto iteration;}
}
support_b: {
    if(field(4)==signed_byte(a[3]+11)||field(16)==0xffffffffu)goto last_a;
    const auto direction_b=_mm_xor_ps(inverse(m,a[2],v(m,d+0x120)),v(m,0x418ae0));
    vw(m,o,0x30b81d,frame+112,direction_b);
    support(m,s,a[1],frame+112,d+(field(4)+14u)*16u,0x30b822,frame-12u,o);
    const auto count=field(4);const auto point=v(m,d+(count+14u)*16u),normal=v(m,frame+112);
    const auto q=dot(_mm_sub_ps(point,v(m,d+0xe0)),normal);stack(0x30b866,52,bits(q));
    const auto limit=f(r(m,frame+16));if(!(xmul(std::fabs(static_cast<long double>(q)),q)>limit))goto last_a;
    if(count==3u) {const auto x=dot(_mm_sub_ps(point,v(m,d+0x100)),normal);stack(0x30b8be,56,bits(x));if(xmul(std::fabs(static_cast<long double>(x)),x)<limit)goto last_a;}
    if(count==2u||count==3u) {const auto x=dot(_mm_sub_ps(point,v(m,d+0xf0)),normal);stack(0x30b902,60,bits(x));if(xmul(std::fabs(static_cast<long double>(x)),x)<limit)goto last_a;}
    vw(m,o,0x30b95d,d+(count+10u)*16u,point_vector(m,a[2],v(m,d+(count+14u)*16u)));store(0x30b961,4,count+1u);goto iteration;
}
last_a: {
    if(r(m,frame+76)||field(0)==signed_byte(a[3]+10)||field(16)==1u)goto finish;
    support(m,s,a[0],d+0x120,d+(field(0)+2u)*16u,0x30b99e,frame-12u,o);
    const auto count=field(0);const auto point=v(m,d+(count+2u)*16u),normal=v(m,d+0x120);
    const auto q=dot(_mm_sub_ps(point,v(m,d+32)),normal);stack(0x30b9da,64,bits(q));
    const auto limit=f(r(m,frame+16));if(!(xmul(std::fabs(static_cast<long double>(q)),q)>limit))goto finish;
    if(count==3u) {const auto x=dot(_mm_sub_ps(point,v(m,d+64)),normal);stack(0x30ba2f,68,bits(x));if(xmul(std::fabs(static_cast<long double>(x)),x)<limit)goto finish;}
    if(count==2u||count==3u) {const auto x=dot(_mm_sub_ps(point,v(m,d+48)),normal);stack(0x30ba70,72,bits(x));if(xmul(std::fabs(static_cast<long double>(x)),x)<limit)goto finish;}
    store(0x30ba8a,0,count+1u);goto iteration;
}
degenerate:
    if(!r(m,d+12,1))stack(0x30baaf,32,fallback(0x30baaa,0x30a370,d,frame-20u,{a[0],a[1],a[2],a[3]}));
    else {
        if(si(field(0)+field(4))>4) {
            if(si(field(0))>si(field(4))){store(0x30bac5,0,3);store(0x30bacb,4,1);}
            else {store(0x30bad3,0,1);store(0x30bad9,4,3);}
        }
        stack(0x30badf,32,3);
    }
finish: {
    const auto difference=_mm_sub_ps(v(m,d+0xa0),v(m,d+32)),normal=v(m,d+0x120);
    auto length=dot_vector(normal,normal);vw(m,o,0x30bb1d,frame+112,length);
    if(!estimate)missing("Required original30BB22 RSQRTSS estimate unavailable");
    const auto approximation=estimate(low(length));w(m,o,0x30bb26,frame+112,4,approximation);
    const auto estimate_vector=v(m,frame+112);
    length=_mm_mul_ss(_mm_mul_ss(length,estimate_vector),estimate_vector);
    stack(0x30bb39,76,0x40400000);auto correction=_mm_sub_ss(scalar(r(m,frame+76)),length);
    stack(0x30bb4b,76,0x3f000000);auto reciprocal=_mm_mul_ss(_mm_mul_ss(scalar(r(m,frame+76)),estimate_vector),correction);
    vw(m,o,0x30bb6e,a[4],_mm_mul_ps(_mm_shuffle_ps(reciprocal,reciprocal,0),normal));
    const auto distance=_mm_mul_ss(dot_vector(difference,normal),reciprocal);stack(0x30bb8f,76,low(distance));
    w(m,o,0x30bb9a,a[4]+12,4,r(m,frame+76));w(m,o,0x30bb9f,a[3]+8,1,field(0));w(m,o,0x30bba4,a[3]+9,1,field(4));
    std::uint32_t count=0;
    do {w(m,o,0x30bbb3,a[3]+count*2u,2,r(m,d+44+count*16u,2));++count;}while(si(count)<si(field(0)));
    const auto bkeys=a[3]+count*2u;count=0;
    do {w(m,o,0x30bbd3,bkeys+count*2u,2,r(m,d+0xec+count*16u,2));++count;}while(si(count)<si(field(4)));
    return r(m,frame+32);
}
}

} // namespace halo2
