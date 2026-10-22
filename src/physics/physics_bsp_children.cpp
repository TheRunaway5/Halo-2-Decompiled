#include "halo2/physics_bsp_children.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M=SourcePhysicsContactMemory;using O=SourcePhysicsContactRecordObserver;
using FP=SourcePhysicsFactoryFloatingPoint;using S=SourcePhysicsBspChildServices;
std::uint32_t r(const M&m,std::uint32_t p,unsigned n=4) {
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v=0;
    for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);
    return v;
}
void w(M&m,const O&o,std::uint32_t site,std::uint32_t p,unsigned n,std::uint64_t v) {
    SourcePhysicsContactRecordStore x{site,p,{},{}};x.before.resize(n);x.written.resize(n);m.read(p,x.before);
    for(unsigned i=0;i<n;++i)x.written[i]=std::byte((v>>(i*8))&255u);
    m.write(p,x.written);if(o)o(x);
}
std::int32_t si(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
std::uint32_t sar(std::uint32_t v,std::uint32_t shift){return std::bit_cast<std::uint32_t>(si(v)>>(shift&31u));}
std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v))));}
[[noreturn]]void missing(const char*s){throw std::runtime_error(s);}
struct FpScope {
    unsigned mx;std::fenv_t env;
    explicit FpScope(FP p):mx(_mm_getcsr()) {
        if((p.x87_control_word&0x3fu)!=0x3fu||(p.mxcsr&0x1f80u)!=0x1f80u)
            missing("Unmasked original BSP FP effects unavailable");
        if(std::fegetenv(&env)!=0)missing("Original BSP FP environment unavailable");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("fldcw %0" : :"m"(p.x87_control_word):"memory");
#else
        missing("Original BSP x87 backend unavailable");
#endif
        _mm_setcsr(p.mxcsr);
    }
    ~FpScope(){std::fesetenv(&env);_mm_setcsr(mx);}
};
__m128 v(const M&m,std::uint32_t p) {
    std::array<float,4>x{};for(unsigned i=0;i<4;++i)x[i]=std::bit_cast<float>(r(m,p+i*4u));
    return _mm_loadu_ps(x.data());
}
void vw(M&m,const O&o,std::uint32_t site,std::uint32_t p,__m128 x) {
    std::array<float,4>f{};_mm_storeu_ps(f.data(),x);const auto b=std::bit_cast<std::array<std::uint32_t,4>>(f);
    for(unsigned i=0;i<2;++i)w(m,o,site,p+i*8u,8,std::uint64_t(b[i*2])|(std::uint64_t(b[i*2+1])<<32));
}
__m128 scalar(std::uint32_t a){return _mm_set_ss(std::bit_cast<float>(a));}
std::uint32_t low(__m128 a){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(a));}
std::uint32_t required(M&m,const S&s,const SourcePhysicsBspOwnerCall&c,const O&o) {
    if(!s.required_owner)missing("Required original resident BSP child owner unavailable");
    return s.required_owner(m,c,o);
}
struct Transpose {__m128 x,y,z;};
Transpose transpose(const M&m,std::uint32_t p) {
    auto x=v(m,p),y=v(m,p+16),z=v(m,p+32);
    auto a=_mm_shuffle_ps(x,y,0x44),b=_mm_shuffle_ps(x,y,0xee);
    auto c=_mm_shuffle_ps(z,z,0x44),d=_mm_shuffle_ps(z,z,0xee);
    return {_mm_shuffle_ps(a,c,0x88),_mm_shuffle_ps(a,c,0xdd),_mm_shuffle_ps(b,d,0x88)};
}
__m128 transformed(const Transpose&t,__m128 a) {
    const auto x=_mm_mul_ps(t.x,_mm_shuffle_ps(a,a,0));
    const auto y=_mm_mul_ps(t.y,_mm_shuffle_ps(a,a,0x55));
    const auto z=_mm_mul_ps(t.z,_mm_shuffle_ps(a,a,0xaa));
    return _mm_add_ps(_mm_add_ps(x,y),z);
}
}
void source_physics_relative_transform_1_1(M&m,std::uint32_t output,std::uint32_t left,
    std::uint32_t right,FP fp,const O&o) {
    FpScope scope(fp);const auto t=transpose(m,left);
    for(unsigned i=0;i<4;++i)vw(m,o,0x2e0ae0,output+i*16u,transformed(t,v(m,right+i*16u)));
    // Original reloads left after all four stores, preserving possible aliasing.
    const auto t2=transpose(m,left);const auto delta=transformed(t2,v(m,left+48));
    vw(m,o,0x2e0b4d,output+48,_mm_sub_ps(v(m,output+48),delta));
}
void source_query_physics_mopp_1_1(M&m,std::uint32_t ctx,std::uint32_t p,
    std::uint32_t pc,std::uint32_t esp,const S&s,const O&o) {
    const auto local=esp-0x38u;
    const auto q=[&](unsigned n){return r(m,p+n*4u);};
    const auto be=[&](std::uint32_t at,unsigned n){std::uint32_t a=0;for(unsigned i=0;i<n;++i)a=(a<<8)|r(m,at+i,1);return a;};
    const auto clone=[&](std::uint32_t site){
        if(p!=local){for(unsigned i=0;i<14;++i)w(m,o,site,local+i*4u,4,r(m,p+i*4u));p=local;}
    };
    const auto recur=[&](std::uint32_t target){source_query_physics_mopp_1_1(m,ctx,p,target,esp-0x54u,s,o);};
    for(;;) {
        const auto op=r(m,pc,1);
        if(op==0)return;
        if(op>=1&&op<=4) {
            const auto x=(r(m,pc+1,1)+q(8))<<(op&31u);
            const auto y=(r(m,pc+2,1)+q(9))<<(op&31u);
            const auto z=(r(m,pc+3,1)+q(10))<<(op&31u);
            w(m,o,0x3061c6,local+32,4,x);w(m,o,0x3061d3,local+36,4,y);w(m,o,0x3061e0,local+40,4,z);
            const auto shift=q(12)+op;w(m,o,0x3061e9,local+48,4,shift);
            w(m,o,0x3061fb,local+16,4,sar(r(m,ctx+32),16u-shift)-x);
            w(m,o,0x306206,local+20,4,sar(r(m,ctx+36),16u-shift)-y);
            w(m,o,0x306213,local+24,4,sar(r(m,ctx+40),16u-shift)-z);
            w(m,o,0x306224,local,4,sar(r(m,ctx+16),16u-shift)+1u-x);
            w(m,o,0x30622b,local+4,4,sar(r(m,ctx+20),16u-shift)+1u-y);
            w(m,o,0x30623b,local+8,4,sar(r(m,ctx+24),16u-shift)+1u-z);
            const auto tail=q(13);w(m,o,0x306242,local+52,4,tail);
            const auto key=q(11);w(m,o,0x306249,local+44,4,key);p=local;pc+=4;continue;
        }
        if(op>=5&&op<=7){const auto n=op-4;pc+=be(pc+1,n)+n+1u;continue;}
        if(op>=9&&op<=11) {
            const auto value=be(pc+1,op==9?1:op==10?2:4);
            clone(op==9?0x30626c:op==10?0x306298:0x3062d6);
            w(m,o,op==9?0x306270:op==10?0x30629c:0x3062da,local+44,4,op==11?value:r(m,local+44)+value);
            pc+=op==9?2:op==10?3:5;continue;
        }
        if(op>=0x10&&op<=0x12) {
            const auto axis=op-0x10,lo=r(m,pc+1,1),hi=r(m,pc+2,1);pc+=4;
            if(si(q(axis))>si(hi)){const auto jump=r(m,pc-1,1);if(si(q(axis+4))<si(lo))recur(pc);pc+=jump;}
            else if(si(q(axis+4))>=si(lo))return;
            continue;
        }
        if(op>=0x13&&op<=0x1c) {
            std::uint32_t lo=0,hi=0,mult=2,bias=0;
            switch(op) {
            case 0x13:lo=q(2)+q(1);hi=q(6)+q(5);break;
            case 0x14:lo=q(1)-q(6);hi=q(5)-q(2);bias=0xff;break;
            case 0x15:lo=q(2)+q(0);hi=q(6)+q(4);break;
            case 0x16:lo=q(0)-q(6);hi=q(4)-q(2);bias=0xff;break;
            case 0x17:lo=q(1)+q(0);hi=q(5)+q(4);break;
            case 0x18:lo=q(0)-q(5);hi=q(4)-q(1);bias=0xff;break;
            case 0x19:lo=q(2)+q(1)+q(0);hi=q(6)+q(5)+q(4);mult=3;break;
            case 0x1a:lo=q(1)-q(6)+q(0);hi=q(5)-q(2)+q(4);mult=3;bias=0xff;break;
            case 0x1b:lo=q(2)-q(5)+q(0);hi=q(4)+q(6)-q(1);mult=3;bias=0xff;break;
            case 0x1c:lo=q(0)-q(6)-q(5);hi=q(4)-q(2)-q(1);mult=3;bias=0x1fe;break;
            }
            const auto a=r(m,pc+1,1)*mult-bias,b=r(m,pc+2,1)*mult-bias;pc+=4;
            if(si(lo)>si(b)){const auto jump=r(m,pc-1,1);if(si(hi)<si(a))recur(pc);pc+=jump;}
            else if(si(hi)>=si(a))return;
            continue;
        }
        if(op>=0x20&&op<=0x22) {
            const auto axis=op-0x20,a=r(m,pc+1,1);pc+=3;
            if(si(q(axis))>si(a)){const auto jump=r(m,pc-1,1);if(si(q(axis+4))<=si(a))recur(pc);pc+=jump;}
            continue;
        }
        if(op>=0x23&&op<=0x25) {
            const auto axis=op-0x23,a=r(m,pc+1,1),b=r(m,pc+2,1),first=be(pc+3,2);pc+=7;
            if(si(q(axis))>si(b)){const auto second=be(pc-2,2);if(si(q(axis+4))<si(a))recur(pc+first);pc+=second;}
            else{if(si(q(axis+4))>=si(a))return;pc+=first;}
            continue;
        }
        if(op>=0x26&&op<=0x28) {
            const auto axis=op-0x26;
            if(si(q(axis))<si(r(m,pc+1,1))||si(q(axis+4))>=si(r(m,pc+2,1)))return;
            pc+=3;continue;
        }
        if(op>=0x29&&op<=0x2b) {
            const auto axis=op-0x29;
            if(si(r(m,ctx+16+axis*4u))<si(be(pc+1,3))||si(r(m,ctx+32+axis*4u))>si(be(pc+4,3)))return;
            pc+=7;continue;
        }
        if(op>=0x30&&op<=0x53) {
            const auto value=op<=0x4f?op-0x30:be(pc+1,op-0x4f),key=q(11)+value,d=r(m,ctx);
            if(r(m,d+4)==(r(m,d+8)&0x7fffffffu))source_double_physics_body_bank_1_1(m,d,4,o);
            w(m,o,0x30641b,r(m,d)+r(m,d+4)*4u,4,key);
            w(m,o,0x306424,d+4,4,r(m,d+4)+1u);return;
        }
        if(op>=0x60&&op<=0x6b) {
            const auto group=(op-0x60)/4u,index=(op-0x60)%4u;
            const auto site=group==0?0x3062ea:group==1?0x306300:0x30632b;
            w(m,o,site,local+52+index*4u,4,be(pc+1,group==0?1:group==1?2:4));
            const auto saved=r(m,local+52);clone(0x30634a);w(m,o,0x306350,local+52,4,saved);
            pc+=group==0?2:group==1?3:5;continue;
        }
        // Reserved/unknown bytecodes call the real diagnostic virtual first.
        const auto owner=r(m,0x48769c),target=r(m,r(m,owner)+4);
        required(m,s,{0x30636f,target,owner,esp-0x54u,owner,r(m,owner),p,std::nullopt,{0x418a34,op}},o);
        missing("Original fatal MOPP diagnostic continuation unavailable");
    }
}
void source_initialize_physics_mopp_query_1_1(M&m,std::uint32_t ctx,std::uint32_t mopp,
    std::uint32_t bounds,std::uint32_t output,std::uint32_t esp,FP fp,const S&s,const O&o) {
    FpScope scope(fp);const auto frame=((esp-4u)&~15u)-0x50u;
    w(m,o,0x3066ef,ctx,4,output);w(m,o,0x3066f4,ctx+48,4,mopp);
    const auto quant=[&](unsigned offset,std::uint32_t spill,std::uint32_t is,std::uint32_t ss,std::uint32_t fs) {
        const auto a=std::bit_cast<float>(r(m,bounds+offset)),b=std::bit_cast<float>(r(m,mopp+(offset&15u))),c=std::bit_cast<float>(r(m,mopp+12));
        float f;std::int32_t n;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("flds %2\n\tfsubs %3\n\tfmuls %4\n\tfstps %0\n\tflds %0\n\tfistpl %1":"=m"(f),"=m"(n):"m"(a),"m"(b),"m"(c):"st");
#else
        missing("Original BSP FISTP backend unavailable");
#endif
        w(m,o,ss,frame+spill,4,std::bit_cast<std::uint32_t>(f));
        w(m,o,fs,frame+(spill==8?12:8),4,std::bit_cast<std::uint32_t>(n));
        if(is)w(m,o,is,ctx+(offset>=16?16:32)+(offset&15u),4,std::bit_cast<std::uint32_t>(n)-1u);
        return std::bit_cast<std::uint32_t>(n)-1u;
    };
    quant(0,8,0x306710,0x3066ff,0x306707);quant(16,12,0x30672f,0x30671e,0x306726);
    quant(4,12,0x30674f,0x30673e,0x306746);quant(20,12,0x30676f,0x30675e,0x306766);
    quant(8,12,0x30678f,0x30677e,0x306786);const auto max_z=quant(24,12,0,0x30679e,0x3067a6);
    const auto p=frame+16;
    w(m,o,0x3067b2,p+16,4,sx16(r(m,ctx+34,2)));
    w(m,o,0x3067bc,p,4,sx16(r(m,ctx+18,2))+1u);
    w(m,o,0x3067c4,p+20,4,sx16(r(m,ctx+38,2)));
    w(m,o,0x3067cd,ctx+24,4,max_z);
    w(m,o,0x3067d3,p+4,4,sx16(r(m,ctx+22,2))+1u);
    w(m,o,0x3067dc,p+8,4,sar(max_z,16)+1u);
    w(m,o,0x3067e0,p+24,4,sx16(r(m,ctx+42,2)));
    for(unsigned i=0;i<6;++i)w(m,o,0x3067ef+i*4u,p+32+i*4u,4,0);
    source_query_physics_mopp_1_1(m,ctx,p,mopp+48,frame-12,s,o);
}
void source_query_physics_bsp_mopp_1_1(M&m,std::uint32_t shape,std::uint32_t bounds,
    std::uint32_t output,std::uint32_t esp,FP fp,const S&s,const O&o) {
    const auto ctx=((esp-4u)&~15u)-0x40u;
    source_initialize_physics_mopp_query_1_1(m,ctx,r(m,shape+16),bounds,output,ctx-16u,fp,s,o);
}
bool source_physics_bsp_child_enabled_1_1(M&m,std::uint32_t key,std::uint32_t esp,const S&s,const O&o) {
    const auto kind=key>>29,index=key&0xffffu,part=(key>>16)&0x1fffu;
    const auto target=r(m,0x1f2ebc+(kind-1u)*4u);
    if(target==0x1f2e21) {
        const auto face=r(m,r(m,0x4e7910)+44)+index*8u;
        if(!(r(m,face+4,1)&8u))return true;
        const auto bit=r(m,face+5,1);if(bit==0xffffffffu)return true;
        const auto row=sx16(r(m,0x46fb64,2))*8u+(bit>>5);
        return (r(m,r(m,0x4f4850)+row*4u+1)&(1u<<(bit&31u)))!=0;
    }
    if(target==0x1f2dd1) {
        const auto bsp=r(m,0x4e7918),inst=r(m,bsp+0x144)+index*0x58u;
        const auto def=r(m,bsp+0x13c)+sx16(r(m,inst+0x34,2))*0xc8u;
        const auto face=r(m,def+0x9c)+part*8u;
        if(!(r(m,face+4,1)&8u))return true;
        const auto a=r(m,face+5,1);
        return (required(m,s,{0x1f2e0d,0x185aa0,index,esp-12u,a,r(m,def+0x9c),sx16(r(m,inst+0x34,2))*0xc8u,std::nullopt,{}},o)&255u)!=0;
    }
    if(target==0x1f2e6b) {
        const auto datum=r(m,r(m,0x52625c)+index*4u);if(datum==0xffffffffu)return false;
        const auto header=r(m,r(m,0x4e78d0)+0x44)+(datum&0xffffu)*12u;
        if(r(m,header+2,1)&16u)return false;
        const auto body=r(m,header+8),map=body+sx16(r(m,body+0x11a,2));
        const auto b=r(m,map+(part&31u),1);
        return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(b))))==((part>>5)&255u);
    }
    if(target==0x1f2eb7)return true;
    missing("Original BSP child switch target unavailable");
}
void source_filter_physics_bsp_children_1_1(M&m,std::uint32_t shape,std::uint32_t d,
    std::uint32_t esp,const S&s,const O&o) {
    if(si(r(m,d+4))<=0)return;
    w(m,o,0x1f26c1,esp+4,4,shape+20);
    for(std::uint32_t i=0;si(i)<si(r(m,d+4));++i) {
        if(source_physics_bsp_child_enabled_1_1(m,r(m,r(m,d)+i*4u),esp-8u,s,o))continue;
        const auto n=r(m,d+4)-1u;w(m,o,0x1f26db,d+4,4,n);
        for(auto j=i;si(j)<si(n);++j)w(m,o,0x1f26ea,r(m,d)+j*4u,4,r(m,r(m,d)+(j+1u)*4u));
        --i;
    }
}
std::uint32_t source_construct_physics_bsp_triangle_1_1(M&m,std::uint32_t output,
    std::uint32_t transform,const std::array<std::uint32_t,5>&a,std::uint32_t esp,FP fp,const O&o) {
    FpScope scope(fp);const auto radius=r(m,0x486570),one=r(m,0x465030);
    w(m,o,0x1f170c,output+8,4,0);w(m,o,0x1f170f,output+20,4,0);
    w(m,o,0x1f1716,output+27,1,a[1]);w(m,o,0x1f1723,output+28,4,a[2]);
    w(m,o,0x1f1726,output+6,2,1);w(m,o,0x1f172c,output+12,4,radius);
    w(m,o,0x1f1731,output,4,0x45d238);w(m,o,0x1f1737,output+16,4,a[3]);
    const auto face=r(m,a[0]+44)+a[3]*8u,first=r(m,face+2,2);auto edge=first;
    // Source-owned local pointers preserve non-null transform/output aliases.
    w(m,o,0x1f1740,esp+4,4,face);w(m,o,0x1f1749,esp+8,4,first);
    do {
        const auto row=r(m,a[0]+52)+edge*12u;
        const auto side=sx16(r(m,row+10,2))==a[3]?1u:0u;
        auto point=r(m,a[0]+60)+r(m,row+side*2u,2)*16u;
        if(transform) {
            auto x=scalar(r(m,point)),y=scalar(r(m,point+4)),z=scalar(r(m,point+8));
            const auto scale=scalar(r(m,transform));
            // JNP on LAHF(AH&44) skips only ordered equality. Unordered
            //has two set bits and enters the original scaling branch.
            const auto sv=std::bit_cast<float>(r(m,transform)),ov=std::bit_cast<float>(one);
            if(sv!=ov){x=_mm_mul_ss(scale,x);y=_mm_mul_ss(scale,y);z=_mm_mul_ss(scale,z);}
            for(unsigned i=0;i<3;++i) {
                auto t=_mm_add_ss(_mm_mul_ss(scalar(r(m,transform+28+i*4u)),z),_mm_mul_ss(scalar(r(m,transform+16+i*4u)),y));
                t=_mm_add_ss(t,_mm_mul_ss(scalar(r(m,transform+4+i*4u)),x));t=_mm_add_ss(t,scalar(r(m,transform+40+i*4u)));
                w(m,o,0x1f17e1+(i==1?0x29u:i==2?0x57u:0u),esp-12u+i*4u,4,low(t));
            }
            point=esp-12u;
        }
        const auto index=r(m,output+20),dst=output+32+index*16u;
        const auto z=r(m,point+8),y=r(m,point+4),x=r(m,point);
        w(m,o,0x1f1857,dst,4,x);w(m,o,0x1f185c,dst+4,4,y);w(m,o,0x1f1861,dst+8,4,z);
        w(m,o,0x1f1866,dst+12,4,0);w(m,o,0x1f1879,output+44+r(m,output+20)*16u,4,r(m,output+20)|0x3f000000u);
        w(m,o,0x1f1885,output+20,4,r(m,output+20)+1u);edge=r(m,row+4+side*2u,2);
    }while(edge!=first);
    w(m,o,0x1f189d,output+24,2,r(m,face+6,2));w(m,o,0x1f18aa,output+26,1,r(m,face+4,1));
    w(m,o,0x1f18ad,output+8,4,a[4]);return output;
}
std::uint32_t source_physics_bsp_child_shape_1_1(M&m,std::uint32_t container,std::uint32_t key,
    std::uint32_t output,std::uint32_t esp,FP fp,const S&s,const O&o) {
    if(!source_physics_bsp_child_enabled_1_1(m,key,esp-12u,s,o)) {
        if(output){w(m,o,0x1f346d,output+6,2,1);w(m,o,0x1f3473,output+8,4,0);w(m,o,0x1f347a,output,4,0x45d2b4);}return output;
    }
    if((key>>29)==1)return output?source_construct_physics_bsp_triangle_1_1(m,output,0,{r(m,0x4e7910),1,0xffffffffu,key&0xffffu,key},esp-36u,fp,o):0u;
    // Other full original constructors have distinct graph/refcount/instanced
    // owners. Their entire owning function is required rather than supplying
    // a child or pretending the terrain branch represents those classes.
    (void)container;missing("Original1F3320 nonterrain branch at1F337E unavailable");
}
void source_query_physics_bsp_children_1_1(M&m,std::uint32_t shape,std::uint32_t bounds,
    std::uint32_t d,std::uint32_t esp,FP fp,const S&s,const O&o) {
    const auto frame=((esp-4u)&~15u)-0x100u;
    source_query_physics_bsp_mopp_1_1(m,shape,bounds,d,frame-12u,fp,s,o);
    source_filter_physics_bsp_children_1_1(m,shape,d,frame-8u,s,o);
    for(std::uint32_t i=0;si(i)<si(r(m,d+4));++i) {
        const auto key=r(m,r(m,d)+i*4u);
        if((key&0xe0000000u)==0x40000000u) {
            missing("Original1F2420 instanced branch at1F2482 unavailable");
        }
    }
}
} // namespace halo2
