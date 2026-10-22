#include "halo2/physics_narrowphase_owners.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M=SourcePhysicsContactMemory;using O=SourcePhysicsContactRecordObserver;
using S=SourcePhysicsNarrowphaseServices;using FP=SourcePhysicsFactoryFloatingPoint;
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
    if(!s.required_owner)missing("Required original narrowphase owner unavailable");
    return s.required_owner(m,{site,entry,ecx,esp,std::nullopt,std::nullopt,std::nullopt,std::nullopt,std::move(args)},o);
}
struct FpScope {
    unsigned mx;std::fenv_t env;
    explicit FpScope(FP p):mx(_mm_getcsr()) {
        if((p.x87_control_word&0x3fu)!=0x3fu||(p.mxcsr&0x1f80u)!=0x1f80u)
            missing("Unmasked original narrowphase FP effects unavailable");
        if(std::fegetenv(&env)!=0)missing("Original narrowphase FP environment unavailable");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("fldcw %0" : :"m"(p.x87_control_word):"memory");
#else
        missing("Original narrowphase x87 backend unavailable");
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
std::uint32_t shape_type(M&m,const S&s,std::uint32_t shape,std::uint32_t site,std::uint32_t esp,const O&o) {
    const auto entry=r(m,r(m,shape)+0x14);
    if(entry==0xa0120)return 0x17;
    if(entry==0x9ddc0)return 0x18;
    return required(m,s,site,entry,shape,esp,{},o);
}
void profiling(const M&m) {
    // Actual measured storage has no profiling capacity. RDTSC is a platform
    // producer, not a chosen timestamp or a silently skipped successful write.
    if(r(m,0x487674)<r(m,0x487678))missing("Unmeasured original profiling RDTSC owner unavailable");
}
}
std::uint32_t source_admit_physics_narrowphase_1_1(M&m,const std::array<std::uint32_t,4>&a,std::uint32_t esp,const S&s,const O&o) {
    if(r(m,a[1]+0x18c+((a[2]<<5)+a[3])*4u)==0x2e13b0)w(m,o,0x2e1455,a[0],1,1);
    else required(m,s,0x2e1462,0x2ffe20,a[3],esp-24u,{a.begin(),a.end()},o);
    return a[0];
}
void source_rotate_physics_contact_vector_1_1(M&m,std::uint32_t output,std::uint32_t matrix,std::uint32_t vector,FP fp,const O&o) {
    FpScope scope(fp);const auto a=v(m,vector);
    const auto z=_mm_mul_ps(v(m,matrix+32),_mm_shuffle_ps(a,a,0xaa));
    const auto y=_mm_mul_ps(v(m,matrix+16),_mm_shuffle_ps(a,a,0x55));
    const auto x=_mm_mul_ps(v(m,matrix),_mm_shuffle_ps(a,a,0));
    vw(m,o,0x2e0cae,output,_mm_add_ps(_mm_add_ps(x,y),z));
}
std::uint32_t source_emit_physics_narrowphase_result_1_1(M&m,std::uint32_t workspace,const std::array<std::uint32_t,3>&a,std::uint32_t esp,FP fp,const O&o) {
    FpScope scope(fp);const auto frame=((esp-4u)&~15u)-16u,output=a[2],vector=a[1];
    w(m,o,0x309873,output+32,4,r(m,vector+12));
    const auto normal=_mm_xor_ps(v(m,vector),v(m,0x418ae0));vw(m,o,0x309883,output,normal);
    if(r(m,workspace)==1u) {
        const auto point=v(m,workspace+32);w(m,o,0x30988f,output+36,4,0x3f800000);
        vw(m,o,0x309896,output+16,point);
    }else if(r(m,workspace+4)==1u) {
        const auto scalar=r(m,vector+12);w(m,o,0x3098aa,frame+12,4,scalar);
        const auto point=_mm_add_ps(v(m,workspace+0xa0),_mm_mul_ps(_mm_set1_ps(f(scalar)),normal));
        w(m,o,0x3098c8,output+36,4,0);vw(m,o,0x3098cf,output+16,point);
    }else {
        const auto point=v(m,workspace+0x130);w(m,o,0x3098e1,output+36,4,0x3f000000);
        vw(m,o,0x3098e8,output+16,point);
    }
    return output;
}
std::uint32_t source_update_physics_separation_1_1(M&m,std::uint32_t agent,const std::array<std::uint32_t,5>&a,std::uint32_t esp,FP fp,const S&s,const O&o) {
    FpScope scope(fp);const auto frame=((esp-4u)&~15u)-0x1e0u;
    const auto ashape=r(m,a[1]),bshape=r(m,a[2]);w(m,o,0x2f4bf4,frame+24,4,ashape);w(m,o,0x2f4bf8,frame+28,4,bshape);
    const auto zero=f(r(m,0x465048));
    if(f(r(m,agent+44))>zero) {
        profiling(m);
        const auto at=r(m,a[1]+8),bt=r(m,a[2]+8);
        const auto delta=_mm_sub_ps(_mm_sub_ps(v(m,bt+16),v(m,bt)),_mm_sub_ps(v(m,at+16),v(m,at)));
        const auto d=dot(delta,v(m,agent+32));w(m,o,0x2f4c78,frame+20,4,bits(d));
        const auto old=f(r(m,agent+44)),ar=f(r(m,at+12)),br=f(r(m,bt+12));float cached;unsigned short status;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("flds %2\n\tfadds %3\n\tfadds %4\n\tfsubrs %5\n\tfsts %0\n\tfcomps %6\n\tfnstsw %%ax":"=m"(cached),"=a"(status):"m"(br),"m"(ar),"m"(d),"m"(old),"m"(zero):"st");
#else
        missing("Original cached separation x87 owner unavailable");
#endif
        w(m,o,0x2f4c89,agent+44,4,bits(cached));
        if((status&0x4100u)==0){w(m,o,0x2f4e5d,a[0],1,0);return a[0];}
    }
    profiling(m);
    source_physics_relative_transform_1_1(m,frame+48,r(m,a[1]+8)+32,r(m,a[2]+8)+32,fp,o);
    w(m,o,0x2f4cfd,frame+0x7c,1,0);
    const auto result=required(m,s,0x2f4d05,0x30ae40,frame+0x70,frame-24u,{ashape,bshape,frame+48,agent+12,frame+32},o);
    return source_finish_physics_separation_1_1(m,agent,a,esp,result,fp,o);
}
std::uint32_t source_finish_physics_separation_1_1(M&m,std::uint32_t agent,const std::array<std::uint32_t,5>&a,std::uint32_t esp,std::uint32_t result,FP fp,const O&o) {
    FpScope scope(fp);const auto frame=((esp-4u)&~15u)-0x1e0u;
    const auto ashape=r(m,frame+24),bshape=r(m,frame+28);
    if(result){w(m,o,0x2f4e5d,a[0],1,0);return a[0];}
    const auto output=a[4];source_emit_physics_narrowphase_result_1_1(m,frame+0x70,{agent+12,frame+32,output},frame-16u,fp,o);
    w(m,o,0x2f4d33,frame+20,4,output+64);
    source_rotate_physics_contact_vector_1_1(m,output+64,r(m,a[1]+8)+32,output,fp,o);
    const auto initial=f(r(m,output+32)),ar=f(r(m,ashape+12)),br=f(r(m,bshape+12));float distance;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("flds %1\n\tfsubs %2\n\tfsubs %3\n\tfstps %0":"=m"(distance):"m"(initial),"m"(ar),"m"(br):"st");
#else
    missing("Original narrowphase distance x87 owner unavailable");
#endif
    w(m,o,0x2f4d50,frame+24,4,bits(distance));w(m,o,0x2f4d5c,output+32,4,r(m,frame+24));
    if(distance<f(r(m,a[3]+8))) {
        w(m,o,0x2f4d7f,frame+20,4,bits(-ar));
        const auto translated=_mm_add_ps(v(m,output+16),_mm_mul_ps(_mm_set1_ps(-ar),v(m,output)));
        w(m,o,0x2f4d95,frame+20,4,bits(-distance));
        vw(m,o,0x2f4dac,output+16,translated);
        const auto local=_mm_sub_ps(_mm_add_ps(v(m,output+16),_mm_mul_ps(_mm_set1_ps(-distance),v(m,output))),v(m,frame+96));
        vw(m,o,0x2f4e1e,output+48,inverse(m,frame+48,local));w(m,o,0x2f4e22,a[0],1,1);return a[0];
    }
    if(r(m,agent+24)==0u&&r(m,r(m,a[3]+12),1)!=0u) {
        vw(m,o,0x2f4e44,agent+32,v(m,r(m,frame+20)));
        const auto threshold=f(r(m,a[3]+8)),scale=f(r(m,0x4650f0));float cached;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("flds %1\n\tfmuls %2\n\tfsubrs %3\n\tfstps %0":"=m"(cached):"m"(threshold),"m"(scale),"m"(distance):"st");
#else
        missing("Original separation cache x87 owner unavailable");
#endif
        w(m,o,0x2f4e55,agent+44,4,bits(cached));
    }
    w(m,o,0x2f4e5d,a[0],1,0);return a[0];
}
std::uint32_t source_update_physics_motion_narrowphase_1_1(M&m,std::uint32_t agent,const std::array<std::uint32_t,5>&a,std::uint32_t esp,FP fp,const S&s,const O&o) {
    FpScope scope(fp);const auto frame=((esp-4u)&~15u)-0x70u;
    w(m,o,0x2e14f0,frame+28,4,agent);const auto saved=r(m,r(m,a[3]+12),1);
    w(m,o,0x2e14ff,frame+36,1,0);w(m,o,0x2e1504,frame+26,1,saved);w(m,o,0x2e1508,frame+32,4,0);
    for(unsigned k=0;k<2;++k) {
        const auto col=a[k+1],shape=r(m,col);
        if(shape_type(m,s,shape,k?0x2e162c:0x2e1510,frame-4u,o)!=0x17u)continue;
        auto root=col;while(r(m,root+12))root=r(m,root+12);
        const auto property=r(m,root+32),motion=r(m,property+0x3c);const auto speed=dot(v(m,motion+64),v(m,motion+64));
        w(m,o,k?0x2e1676:0x2e155a,frame+40,4,bits(speed));
        if(k)w(m,o,0x2e1681,frame+44,4,property);
        if(!(speed>f(r(m,shape+32)))||!std::uint8_t(required(m,s,k?0x2e1691:0x2e1571,0x27f920,k?property:frame+40,frame-8u,{property},o))) {
            vw(m,o,k?0x2e1748:0x2e1621,shape+16,_mm_setzero_ps());continue;
        }
        vw(m,o,k?0x2e1710:0x2e15ec,shape+16,inverse(m,r(m,col+8)+32,v(m,r(m,property+0x3c)+64)));
        const auto scalar=r(m,a[3]+16);w(m,o,k?0x2e171b:0x2e15f7,frame+(k?44u:40u),4,scalar);
        vw(m,o,k?0x2e1736:0x2e160b,shape+16,_mm_mul_ps(_mm_set1_ps(f(scalar)),v(m,shape+16)));
        w(m,o,k?0x2e173a:0x2e160f,frame+32,4,k?r(m,frame+32)+2u:1u);
        w(m,o,k?0x2e173e:0x2e1617,frame+36,1,1);
    }
    if(r(m,frame+32)!=r(m,agent+0xf0)||r(m,frame+36,1)) {
        if(r(m,frame+32)!=r(m,agent+0xf0))w(m,o,0x2e1762,r(m,a[3]+12),1,0);
        const auto as=r(m,a[1]),bs=r(m,a[2]);w(m,o,0x2e1780,frame+36,4,as);
        source_physics_relative_transform_1_1(m,frame+48,r(m,a[1]+8)+32,r(m,a[2]+8)+32,fp,o);
        if(shape_type(m,s,bs,0x2e1792,frame-4u,o)==5u)required(m,s,0x2e17ac,0x305650,agent+12,frame-16u,{as,bs,frame+48},o);
        else source_initialize_physics_support_state_1_1(m,agent+12,{as,bs,frame+48},frame-16u,fp,s,o);
        w(m,o,0x2e17ce,agent+44,4,0xbf800000);
    }
    source_update_physics_separation_1_1(m,agent,{frame+27,a[1],a[2],a[3],a[4]},frame-24u,fp,s,o);
    return source_finish_physics_motion_narrowphase_1_1(m,agent,a,esp,o);
}
std::uint32_t source_finish_physics_motion_narrowphase_1_1(M&m,std::uint32_t agent,const std::array<std::uint32_t,5>&a,std::uint32_t esp,const O&o) {
    const auto frame=((esp-4u)&~15u)-0x70u;
    w(m,o,0x2e17f4,agent+0xf0,4,r(m,frame+32));w(m,o,0x2e1805,r(m,a[3]+12),1,r(m,frame+26,1));
    // POP EDI at2E1804 has advanced ESP by4 before the source+17 read.
    w(m,o,0x2e180c,a[0],1,r(m,frame+27,1));return a[0];
}
} // namespace halo2
