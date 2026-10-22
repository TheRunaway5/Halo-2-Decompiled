#include "halo2/physics_child_agents.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M=SourcePhysicsContactMemory;using O=SourcePhysicsContactRecordObserver;
using S=SourcePhysicsChildAgentServices;using FP=SourcePhysicsFactoryFloatingPoint;
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
std::int32_t si(std::uint32_t x){return std::bit_cast<std::int32_t>(x);}
[[noreturn]]void missing(const char*s){throw std::runtime_error(s);}
std::uint32_t required(M&m,const S&s,std::uint32_t site,std::uint32_t entry,
    std::uint32_t ecx,std::uint32_t esp,std::vector<std::uint32_t>args,const O&o) {
    if(!s.required_owner)missing("Required original child-agent owner unavailable");
    return s.required_owner(m,{site,entry,ecx,esp,std::nullopt,std::nullopt,std::nullopt,std::nullopt,std::move(args)},o);
}
struct FpScope {
    unsigned mx;std::fenv_t env;
    explicit FpScope(FP p):mx(_mm_getcsr()) {
        if((p.x87_control_word&0x3fu)!=0x3fu||(p.mxcsr&0x1f80u)!=0x1f80u)
            missing("Unmasked original child-agent FP effects unavailable");
        if(std::fegetenv(&env)!=0)missing("Original child-agent FP environment unavailable");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("fldcw %0" : :"m"(p.x87_control_word):"memory");
#else
        missing("Original child-agent x87 backend unavailable");
#endif
        _mm_setcsr(p.mxcsr);
    }
    ~FpScope(){std::fesetenv(&env);_mm_setcsr(mx);}
};
__m128 v(const M&m,std::uint32_t p) {
    std::array<float,4>x{};for(unsigned k=0;k<4;++k)x[k]=std::bit_cast<float>(r(m,p+k*4u));return _mm_loadu_ps(x.data());
}
void vw(M&m,const O&o,std::uint32_t site,std::uint32_t p,__m128 x) {
    std::array<float,4>a{};_mm_storeu_ps(a.data(),x);const auto b=std::bit_cast<std::array<std::uint32_t,4>>(a);
    for(unsigned k=0;k<2;++k)w(m,o,site,p+k*8u,8,std::uint64_t(b[k*2])|(std::uint64_t(b[k*2+1])<<32));
}
std::uint32_t low(__m128 x){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(x));}
__m128 scalar(std::uint32_t x){return _mm_set_ss(std::bit_cast<float>(x));}
__m128 dot(__m128 a,__m128 b) {
    const auto x=_mm_mul_ps(a,b);
    return _mm_add_ss(_mm_shuffle_ps(x,x,0xaa),_mm_add_ss(_mm_shuffle_ps(x,x,0x55),x));
}
void point(M&m,std::uint32_t out,std::uint32_t matrix,std::uint32_t p,const O&o) {
    const auto a=v(m,p);
    const auto z=_mm_add_ps(_mm_mul_ps(v(m,matrix+32),_mm_shuffle_ps(a,a,0xaa)),v(m,matrix+48));
    const auto y=_mm_mul_ps(v(m,matrix+16),_mm_shuffle_ps(a,a,0x55));
    const auto x=_mm_mul_ps(v(m,matrix),_mm_shuffle_ps(a,a,0));
    vw(m,o,0x2e0be5,out,_mm_add_ps(_mm_add_ps(x,y),z));
}
void inverse_direction(M&m,std::uint32_t out,std::uint32_t matrix,std::uint32_t p,const O&o) {
    auto x=v(m,matrix),y=v(m,matrix+16),z=v(m,matrix+32),a=v(m,p);
    const auto xy=_mm_shuffle_ps(x,y,0x44),xy2=_mm_shuffle_ps(x,y,0xee);
    const auto zz=_mm_shuffle_ps(z,z,0x44),zz2=_mm_shuffle_ps(z,z,0xee);
    const auto tx=_mm_shuffle_ps(xy,zz,0x88),ty=_mm_shuffle_ps(xy,zz,0xdd),tz=_mm_shuffle_ps(xy2,zz2,0x88);
    vw(m,o,0x2e0d26,out,_mm_add_ps(_mm_add_ps(_mm_mul_ps(tx,_mm_shuffle_ps(a,a,0)),
        _mm_mul_ps(ty,_mm_shuffle_ps(a,a,0x55))),_mm_mul_ps(tz,_mm_shuffle_ps(a,a,0xaa))));
}
std::uint32_t shape_type(M&m,const S&s,std::uint32_t shape,std::uint32_t site,std::uint32_t esp,const O&o) {
    const auto entry=r(m,r(m,shape)+0x14);
    if(entry==0xa0120)return 0x17;
    if(entry==0x9ddc0)return 0x18;
    return required(m,s,site,entry,shape,esp,{},o);
}
void center(M&m,const S&s,std::uint32_t shape,std::uint32_t out,std::uint32_t site,std::uint32_t esp,const O&o) {
    const auto entry=r(m,r(m,shape)+0x38);
    if(entry==0x2e22a0){center(m,s,r(m,shape+0x30),out,0x2e22a5,esp,o);return;}
    if(entry==0x2e4360){vw(m,o,0x2e4368,out,v(m,shape+0x20));return;}
    if(entry==0x3037c0) {
        w(m,o,0x3037c9,out,4,r(m,r(m,shape+0x30)));
        w(m,o,0x3037d1,out+4,4,r(m,r(m,shape+0x30)+16));
        w(m,o,0x3037da,out+8,4,r(m,r(m,shape+0x30)+32));
        w(m,o,0x3037dd,out+12,4,0);return;
    }
    required(m,s,site,entry,shape,esp,{out},o);
}
void support(M&m,const S&s,std::uint32_t shape,std::uint32_t dir,std::uint32_t out,
    std::uint32_t site,std::uint32_t esp,const O&o) {
    const auto entry=r(m,r(m,shape)+0x30),frame=((esp-4u)&~15u)-16u;
    if(entry==0x2e22b0) {
        support(m,s,r(m,shape+0x30),dir,out,0x2e22c9,frame-12u,o);
        const auto shift=v(m,shape+16);const auto d=dot(shift,v(m,dir));
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
                const auto d=dot(v(m,shape+32+k*16u),direction);
                w(m,o,0x1f1d66,frame+12,4,low(d));
                if(_mm_cvtss_f32(d)>_mm_cvtss_f32(best)){best=d;index=k;}
            }
        }
        vw(m,o,0x1f1d91,out,v(m,shape+32+index*16u));return;
    }
    if(entry==0x2e42a0) {
        const auto f=((esp-4u)&~15u)-16u;const auto first=v(m,shape+16),second=v(m,shape+32);
        const auto d=dot(_mm_sub_ps(second,first),v(m,dir));w(m,o,0x2e42da,f+12,4,low(d));
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
std::uint32_t allocate(M&m,const S&s,std::uint32_t bytes,std::uint32_t category,std::uint32_t site,std::uint32_t esp,const O&o) {
    const auto adapter=r(m,0x487638),entry=r(m,r(m,adapter)+0x10),backend=r(m,0x480da0);
    if(entry==0x148a10&&r(m,0x480d98)==2u&&r(m,r(m,backend)+0x10)==0x22e950) {
        const auto p=source_allocate_physics_body_small_block_1_1(m,backend,bytes,o);
        if(!p)missing("Required original child-agent alternate allocator unavailable");
        return p;
    }
    return required(m,s,site,entry,adapter,esp,{bytes,category},o);
}
void free(M&m,const S&s,std::uint32_t block,std::uint32_t bytes,std::uint32_t site,std::uint32_t esp,const O&o) {
    const auto adapter=r(m,0x487638),entry=r(m,r(m,adapter)+0x14),backend=r(m,0x480da0);
    if(entry==0x148a70&&(!block||(si(block)>=si(backend+0x4340)&&si(block)<=si(backend+0xf0340)&&r(m,r(m,backend)+0x14)==0x22eb20))) {
        if(block)source_free_physics_body_small_block_1_1(m,backend,block,bytes,o);
        return;
    }
    required(m,s,site,entry,adapter,esp,{block,bytes,0x12},o);
}
}
std::uint32_t source_null_physics_child_agent_1_1(){return 0x487ac4;}
void source_sort_physics_child_keys_1_1(M&m,std::uint32_t keys,std::uint32_t left,std::uint32_t right,std::uint32_t esp,const O&o) {
    for(;;) {
        const auto pivot=r(m,keys+std::uint32_t(si(left+right)>>1)*4u);auto i=left,j=right;
        for(;;) {
            while(r(m,keys+i*4u)<pivot)++i;
            while(pivot<r(m,keys+j*4u))--j;
            if(si(j)<si(i))break;
            if(i!=j){const auto a=r(m,keys+i*4u),b=r(m,keys+j*4u);w(m,o,0x2ef932,keys+j*4u,4,a);w(m,o,0x2ef938,keys+i*4u,4,b);}
            --j;++i;if(si(i)>si(j))break;
        }
        if(si(left)<si(j))source_sort_physics_child_keys_1_1(m,keys,left,j,((esp-4u)&~15u)-36u,o);
        if(si(i)>=si(right))return;
        left=i;w(m,o,0x2ef95d,esp+8,4,left);
    }
}
std::uint32_t source_construct_physics_child_bank_1_1(M&m,std::uint32_t d,std::uint32_t count,std::uint32_t esp,const S&s,const O&o) {
    w(m,o,0x2ef990,d,4,0);w(m,o,0x2ef992,d+4,4,0);w(m,o,0x2ef995,d+8,4,0x80000000);
    const auto adapter=r(m,0x487638),bytes=(count*12u+15u)&~15u,available=r(m,adapter+12);std::uint32_t block;
    if(si(bytes)>si(available))block=required(m,s,0x2ef9c1,r(m,r(m,adapter)+0x24),adapter,esp-32u,{bytes},o);
    else {
        block=r(m,adapter+8);w(m,o,0x2ef9ce,adapter+8,4,block+bytes);w(m,o,0x2ef9d6,adapter+12,4,available-bytes);
    }
    w(m,o,0x2ef9e2,d+8,4,count|0x80000000u);w(m,o,0x2ef9e9,d+16,4,count);
    w(m,o,0x2ef9ec,d,4,block);w(m,o,0x2ef9ee,d+12,4,block);return d;
}
void source_destroy_physics_child_bank_1_1(M&m,std::uint32_t d,std::uint32_t esp,const S&s,const O&o) {
    const auto adapter=r(m,0x487638),block=r(m,d+12),bytes=(r(m,d+16)*12u+15u)&~15u,alternate=r(m,adapter+20);
    if(bytes&&(block==alternate||!alternate))required(m,s,0x2efa60,r(m,r(m,adapter)+0x28),adapter,esp-36u,{block,bytes},o);
    else {w(m,o,0x2efa6f,adapter+8,4,r(m,adapter+8)-bytes);w(m,o,0x2efa72,adapter+12,4,r(m,adapter+12)+bytes);}
    const auto capacity=r(m,d+8);if(si(capacity)>=0)free(m,s,r(m,d),(capacity&0x7fffffffu)*12u,0x2efa9d,esp-40u,o);
}
std::uint32_t source_filter_physics_child_layers_1_1(M&m,std::uint32_t filter,std::uint32_t out,std::uint32_t a,std::uint32_t b,const O&o) {
    if(((a^b)&0xffff0000u)==0&&(a&0xffff0000u)!=0)w(m,o,0x2e5e42,out,1,0);
    else w(m,o,0x2e5e63,out,1,(r(m,filter+0x1c+(a&31u)*4u)&(1u<<(b&31u)))!=0);
    return out;
}
std::uint32_t source_filter_physics_child_1_1(M&m,std::uint32_t filter,const std::array<std::uint32_t,6>&a,std::uint32_t esp,const S&s,const O&o) {
    const auto frame=((esp-4u)&~15u)-0x120u;
    w(m,o,0x1c6c47,frame+0x1c,4,filter);
    const auto root=[&](std::uint32_t p){while(r(m,p+12))p=r(m,p+12);return p;};
    auto ar=root(a[2]);
    if(r(m,0x486550)==r(m,ar+32)) {
        std::uint32_t shape;
        if(a[5]!=0xffffffffu)shape=required(m,s,0x1c6c7d,r(m,r(m,a[4])+0x34),a[4],frame-12u,{a[5],frame+32},o);
        else shape=r(m,a[3]);
        if(shape_type(m,s,shape,0x1c6c8a,frame-4u,o)==0x15)shape=r(m,shape+12);
        if(shape_type(m,s,shape,0x1c6c99,frame-4u,o)==0x17)shape=r(m,shape+0x30);
        if(shape_type(m,s,shape,0x1c6ca8,frame-4u,o)!=0x18&&
            (required(m,s,0x1c6cb2,0x183c00,shape,frame-4u,{},o)&255u)!=0) {
            const auto key=r(m,shape+8);
            const auto clamped=si(key)<1?1u:si(key)>16?16u:key;
            if(key&&clamped!=key) {
                const auto flag=r(m,key+0x1f,1)&1u;w(m,o,0x1c6ce4,frame+0x1b,1,flag);
                ar=root(a[2]);
                if(flag&&r(m,ar+24)==1u&&r(m,ar+32)) {
                    const auto property=r(m,r(m,ar+32)+0x3c);
                    if(required(m,s,0x1c6d10,r(m,r(m,property)+24),property,frame-4u,{},o)==7u){w(m,o,0x1c6d1b,a[0],1,0);return a[0];}
                }
            }
        }
    }
    const auto br=root(a[3]);ar=root(a[2]);
    return source_filter_physics_child_layers_1_1(m,filter-12,a[0],r(m,ar+28),r(m,br+28),o);
}
void source_initialize_physics_support_state_1_1(M&m,std::uint32_t state,const std::array<std::uint32_t,3>&a,std::uint32_t esp,FP fp,const S&s,const O&o) {
    FpScope scope(fp);const auto frame=((esp-4u)&~15u)-0x80u;
    center(m,s,a[0],frame+0x60,0x30557a,frame-8u,o);center(m,s,a[1],frame+0x50,0x305589,frame-8u,o);
    point(m,frame+0x40,a[2],frame+0x50,o);
    vw(m,o,0x3055b9,frame+16,_mm_sub_ps(v(m,frame+0x40),v(m,frame+0x60)));
    support(m,s,a[0],frame+16,frame+0x30,0x3055be,frame-12u,o);
    const auto index=r(m,frame+0x3c,2);const auto delta=_mm_sub_ps(v(m,frame+0x40),v(m,frame+0x30));
    w(m,o,0x3055e0,state,2,index);vw(m,o,0x3055e3,frame+16,delta);
    inverse_direction(m,frame+32,a[2],frame+16,o);
    vw(m,o,0x30560a,frame+32,_mm_xor_ps(v(m,frame+32),v(m,0x418ae0)));
    support(m,s,a[1],frame+32,frame+0x70,0x30560f,frame-12u,o);
    w(m,o,0x305617,state+2,2,r(m,frame+0x7c,2));w(m,o,0x305620,state+4,2,0xffff);w(m,o,0x305624,state+6,2,0xffff);
    w(m,o,0x30562a,state+8,1,1);w(m,o,0x30562d,state+9,1,1);
    for(unsigned k=0;k<2;++k) {
        const auto entry=r(m,r(m,a[k])+0x3c),site=k?0x30563eu:0x305634u;
        const auto value=entry==0x253ee0?0xffffffffu:entry==0x9d5a0?2u:required(m,s,site,entry,a[k],frame-4u,{},o);
        w(m,o,k?0x305642:0x305637,state+10+k,1,value);
    }
}
std::uint32_t source_construct_physics_child_agent_1_1(M&m,std::uint32_t p,const std::array<std::uint32_t,3>&a,std::uint32_t esp,FP fp,const S&s,const O&o) {
    const auto frame=((esp-4u)&~15u)-0x60u;
    w(m,o,0x2f48c6,frame+12,4,p);w(m,o,0x2f48ca,p+8,4,a[2]);w(m,o,0x2f48d3,p,4,0x419288);
    const auto left=r(m,a[0]+8)+32,right=r(m,a[1]+8)+32,as=r(m,a[0]),bs=r(m,a[1]);
    source_physics_relative_transform_1_1(m,frame+16,left,right,fp,o);
    const std::array<std::uint32_t,3>args{as,bs,frame+16};
    if(shape_type(m,s,bs,0x2f4900,frame-4u,o)==5u)required(m,s,0x2f4912,0x305650,p+12,frame-16u,{args.begin(),args.end()},o);
    else source_initialize_physics_support_state_1_1(m,p+12,args,frame-16u,fp,s,o);
    w(m,o,0x2f492c,p+24,4,0);w(m,o,0x2f4934,p+44,4,0xbf800000);return p;
}
std::uint32_t source_allocate_physics_child_agent_1_1(M&m,const std::array<std::uint32_t,4>&a,std::uint32_t esp,FP fp,const S&s,const O&o) {
    const auto p=allocate(m,s,0x100,0x1a,0x2e13d6,esp-32u,o);
    w(m,o,0x2e13db,p+4,2,0x100);w(m,o,0x2e13e1,esp-16u,4,p);
    source_construct_physics_child_agent_1_1(m,p,{a[0],a[1],a[3]},esp-36u,fp,s,o);
    w(m,o,0x2e1407,p,4,0x4188d0);w(m,o,0x2e140d,p+0xf0,4,0);return p;
}
std::uint32_t source_dispatch_physics_child_agent_1_1(M&m,const std::array<std::uint32_t,4>&a,std::uint32_t esp,FP fp,const S&s,const O&o) {
    const auto at=shape_type(m,s,r(m,a[0]),0x2ef693,esp-20u,o),bt=shape_type(m,s,r(m,a[1]),0x2ef6a0,esp-20u,o);
    const auto entry=r(m,r(m,a[2])+0x18c+((at<<5)+bt)*4u);
    if(entry==0x2e13b0)return source_allocate_physics_child_agent_1_1(m,a,esp-36u,fp,s,o);
    return required(m,s,0x2ef6bb,entry,a[1],esp-36u,{a.begin(),a.end()},o);
}
void source_update_physics_child_cache_1_1(M&m,std::uint32_t agent,const std::array<std::uint32_t,4>&a,std::uint32_t esp,FP fp,const S&s,const O&o) {
    FpScope scope(fp);const auto frame=((esp-4u)&~15u)-0x3e0u;
    w(m,o,0x2efc71,frame+0x28,4,agent);
    source_physics_relative_transform_1_1(m,frame+0x390,r(m,a[1]+8)+32,r(m,a[0]+8)+32,fp,o);
    const auto margin=std::bit_cast<float>(r(m,a[2]+8)),half=std::bit_cast<float>(r(m,0x46502c));float half_margin,expanded;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("flds %2\n\tfmuls %3\n\tfstps %0\n\tflds %0\n\tfadds %2\n\tfstps %1":"=m"(half_margin),"=m"(expanded):"m"(margin),"m"(half):"st");
#else
    missing("Original child-cache x87 margin owner unavailable");
#endif
    const auto half_bits=std::bit_cast<std::uint32_t>(half_margin),expanded_bits=std::bit_cast<std::uint32_t>(expanded);
    w(m,o,0x2efc8f,frame+0x18,4,half_bits);w(m,o,0x2efca2,frame-8u,4,expanded_bits);
    const auto shape=r(m,a[0]),bounds_method=r(m,r(m,shape)+0x18);
    if(bounds_method==0x2e2380)source_physics_current_wrapper_bounds_1_1(m,shape,frame+0x390,expanded_bits,frame+0x40,fp,o);
    else if(bounds_method==0x2e4670)source_physics_capsule_bounds_1_1(m,shape,frame+0x390,expanded_bits,frame+0x40,fp,o);
    else required(m,s,0x2efca6,bounds_method,shape,frame-16u,{frame+0x390,expanded_bits,frame+0x40},o);
    const auto minimum=v(m,frame+0x40),maximum=v(m,frame+0x50);
    if(r(m,0x4693a8,1)) {
        w(m,o,0x2efccb,frame+0x20,4,half_bits);
        const auto mask=v(m,0x45cac0),limit=_mm_set1_ps(half_margin);
        auto exceeded=std::uint32_t(_mm_movemask_ps(_mm_cmplt_ps(limit,_mm_and_ps(_mm_sub_ps(v(m,agent+0x30),maximum),mask))))&7u;
        w(m,o,0x2efcf1,frame+0x24,4,exceeded);
        if(!exceeded) {
            exceeded=std::uint32_t(_mm_movemask_ps(_mm_cmplt_ps(limit,_mm_and_ps(_mm_sub_ps(v(m,agent+0x20),minimum),mask))))&7u;
            w(m,o,0x2efd0b,frame+0x24,4,exceeded);if(!exceeded)return;
        }
    }
    vw(m,o,0x2efd23,agent+32,minimum);vw(m,o,0x2efd27,agent+48,maximum);
    const auto descriptor=frame+0x180;
    w(m,o,0x2efd2d,descriptor,4,frame+0x18c);w(m,o,0x2efd34,descriptor+4,4,0);w(m,o,0x2efd3b,descriptor+8,4,0x80000080);
    const auto bsp=r(m,a[1]),query_method=r(m,r(m,bsp)+0x30);
    if(query_method==0x1f2420)source_query_physics_bsp_children_1_1(m,bsp,frame+0x40,descriptor,frame-12u,fp,s,o);
    else required(m,s,0x2efd5e,query_method,bsp,frame-12u,{frame+0x40,descriptor},o);
    const auto container=r(m,bsp+12),child=frame+0x64;
    w(m,o,0x2efd6e,frame+0x70,4,a[1]);w(m,o,0x2efd72,frame+0x6c,4,r(m,a[1]+8));w(m,o,0x2efd76,frame+0x60,4,container);
    if(r(m,0x4878b8,1))missing("Unmeasured original incremental child-cache mode2EFD80 unavailable");
    auto count=r(m,descriptor+4);w(m,o,0x2f0021,frame+0x24,1,0);
    if(si(count)>1)source_sort_physics_child_keys_1_1(m,r(m,descriptor),0,count-1u,frame-20u,o);
    const auto manager=r(m,agent+8),old=r(m,agent+12),old_end=old+r(m,agent+16)*12u,keys=r(m,descriptor),key_end=keys+count*4u;
    w(m,o,0x2f005b,frame+0x20,4,manager);w(m,o,0x2f0062,frame+0x18,4,old_end);w(m,o,0x2f006e,frame+0x14,4,key_end);
    const auto temporary=frame+0x2c;
    source_construct_physics_child_bank_1_1(m,temporary,count,frame-8u,s,o);
    auto capacity=r(m,temporary+8)&0x7fffffffu;
    if(si(capacity)<si(count))source_grow_physics_body_bank_1_1(m,temporary,si(count)<si(capacity*2u)?capacity*2u:count,12,o);
    w(m,o,0x2f00b3,temporary+4,4,count);
    auto row=old,key=keys,dst=r(m,temporary);
    const auto release=[&](std::uint32_t p,std::uint32_t site) {
        const auto target=r(m,r(m,p)+0x14);required(m,s,site,target,p,frame-4u,{},o);
    };
    const auto create=[&](std::uint32_t k,bool merge) {
        const auto shape_method=r(m,r(m,container)+0x34);
        const auto result=shape_method==0x1f3320?
            source_physics_bsp_child_shape_1_1(m,container,k,frame+0x80,frame-12u,fp,s,o):
            required(m,s,merge?0x2f0112:0x2f01d8,shape_method,container,frame-12u,{k,frame+0x80},o);
        w(m,o,merge?0x2f011c:0x2f01db,frame+0x68,4,k);w(m,o,merge?0x2f0123:0x2f01df,child,4,result);
        const auto filter=r(m,a[2]+4),method=r(m,r(m,filter));
        const std::array<std::uint32_t,6> filter_args{frame+0x1f,a[2],a[0],child,result,k};
        const auto gate=method==0x1c6c30?source_filter_physics_child_1_1(m,filter,filter_args,frame-28u,s,o):
            required(m,s,merge?0x2f0143:0x2f0202,method,filter,frame-28u,{filter_args.begin(),filter_args.end()},o);
        std::uint32_t child_agent;const bool admitted=r(m,gate,1)!=0;
        if(admitted) {
            const std::array<std::uint32_t,4> args{a[0],child,a[2],manager};
            child_agent=a[3]==0x2ef680?source_dispatch_physics_child_agent_1_1(m,args,frame-20u,fp,s,o):
                required(m,s,merge?0x2f015c:0x2f021a,a[3],a[0],frame-20u,{args.begin(),args.end()},o);
        }else child_agent=source_null_physics_child_agent_1_1();
        w(m,o,merge?(admitted?0x2f015f:0x2f0176):0x2f0227,dst+8,4,child_agent);
        w(m,o,merge?(admitted?0x2f0164:0x2f017b):0x2f0230,dst,4,r(m,key));
        dst+=12;key+=4;
    };
    while(row!=old_end) {
        if(key==key_end)break;
        const auto k=r(m,key),previous=r(m,row);w(m,o,0x2f00d1,frame+16,4,k);
        if(k==previous) {
            w(m,o,0x2f00dd,dst,4,r(m,row));w(m,o,0x2f00e2,dst+4,4,r(m,row+4));w(m,o,0x2f00ee,dst+8,4,r(m,row+8));
            dst+=12;row+=12;key+=4;
        }else if(k<previous)create(k,true);
        else {if(const auto p=r(m,row+8))release(p,0x2f0190);row+=12;}
    }
    while(row!=old_end){if(const auto p=r(m,row+8))release(p,0x2f01b3);row+=12;}
    while(key!=key_end)create(r(m,key),false);
    capacity=r(m,agent+20);count=r(m,temporary+4);
    if(si(capacity&0x7fffffffu)<si(count)) {
        if(si(capacity)>=0)free(m,s,r(m,agent+12),(capacity&0x7fffffffu)*12u,0x2f026b,frame-16u,o);
        const auto block=allocate(m,s,count*12u,0x12,0x2f0283,frame-12u,o);
        w(m,o,0x2f028a,agent+12,4,block);w(m,o,0x2f028d,agent+20,4,count);
    }
    w(m,o,0x2f0295,agent+16,4,count);
    if(si(count)>0) {
        auto src=r(m,temporary),out=r(m,agent+12);
        do {
            w(m,o,0x2f02a6,out,4,r(m,src));w(m,o,0x2f02ab,out+4,4,r(m,src+4));w(m,o,0x2f02b8,out+8,4,r(m,src+8));
            src+=12;out+=12;--count;
        }while(count);
    }
    source_destroy_physics_child_bank_1_1(m,temporary,frame-4u,s,o);
    const auto key_capacity=r(m,descriptor+8);
    if(si(key_capacity)>=0)free(m,s,r(m,descriptor),(key_capacity&0x7fffffffu)*4u,0x2f02ff,frame-16u,o);
}
} // namespace halo2
