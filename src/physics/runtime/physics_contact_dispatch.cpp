#include "halo2/physics_contact_dispatch.hpp"
#include <algorithm>
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
std::uint32_t x87_copy(std::uint32_t x) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    float y;const float a=f(x);asm volatile("flds %1\n\tfstps %0":"=m"(y):"m"(a):"st");return bits(y);
#else
    (void)x;missing("Original contact batch x87 copy unavailable");
#endif
}
}
void source_dispatch_physics_contacts_1_1(M&m,std::uint32_t parent,const std::array<std::uint32_t,4>&a,std::uint32_t esp,FP fp,SourcePhysicsReciprocalSqrtEstimate estimate,const SourcePhysicsContactDispatchServices&s,const O&o) {
    FpScope scope(fp);const auto frame=((esp-4u)&~15u)-0x330u;
    const auto profile=[&](bool end) {
        const auto cursor=r(m,0x487674);if(cursor>=r(m,0x487678))return;
        if(!s.timestamp_counter)missing("Required original contact-dispatch profiling RDTSC owner unavailable");
        w(m,o,end?0x300f62:0x3008ad,cursor,4,end?0x4188f4:0x4195dc);
        const auto clock=std::uint32_t(s.timestamp_counter()),slot=frame+(end?36u:32u);
        w(m,o,end?0x300f6a:0x3008b5,slot,4,clock);w(m,o,end?0x300f72:0x3008bd,cursor+4,4,r(m,slot));w(m,o,end?0x300f78:0x3008c3,0x487674,4,cursor+12u);
    };
    w(m,o,0x3008a7,frame+28,4,parent);profile(false);
    w(m,o,0x3008d1,frame+56,4,r(m,a[1]+8));w(m,o,0x3008d5,frame+60,4,a[1]);
    w(m,o,0x3008e4,frame+48,4,r(m,r(m,a[1])+12));w(m,o,0x3008f0,frame+52,4,r(m,a[1]+4));
    source_update_physics_child_cache_1_1(m,parent,{a[0],frame+48,a[2],0x2ef680},frame-20u,fp,s.shape_and_child,o);
    const auto descriptor=frame+0x110;
    w(m,o,0x300909,descriptor,4,frame+0x120);w(m,o,0x300910,descriptor+4,4,0);w(m,o,0x300917,descriptor+8,4,0x80000004);
    source_construct_physics_contact_scratch_1_1(m,frame+108,r(m,parent+16),frame-8u,s.contact,o);
    w(m,o,0x30094a,frame+112,4,r(m,parent+16));w(m,o,0x30094e,frame+76,4,a[1]);w(m,o,0x300952,frame+72,4,r(m,a[1]+8));
    w(m,o,0x300956,frame+36,4,r(m,frame+48));w(m,o,0x30095a,frame+40,4,0);
    const auto shape_type=[&](std::uint32_t shape,std::uint32_t site,std::uint32_t stack) {
        const auto entry=r(m,r(m,shape)+20);if(entry==0xa0120)return 0x17u;if(entry==0x9ddc0)return 0x18u;
        if(!s.shape_and_child.required_owner)missing("Required original dispatcher shape type unavailable");
        return s.shape_and_child.required_owner(m,{site,entry,shape,stack,std::nullopt,std::nullopt,std::nullopt,std::nullopt,{}},o);
    };
    SourcePhysicsNarrowphaseServices narrow;
    narrow.required_owner=[&](M&memory,const SourcePhysicsBspOwnerCall&q,const O&obs)->std::uint32_t {
        if(q.entry==0x30ae40){std::array<std::uint32_t,5>args{};if(q.arguments.size()!=5)missing("Original dispatcher GJK ABI unavailable");std::copy_n(q.arguments.begin(),5,args.begin());return source_physics_gjk_1_1(memory,q.ecx,args,q.entry_esp,fp,estimate,s.gjk,obs);}
        if(!s.shape_and_child.required_owner)missing("Required original dispatcher narrowphase owner unavailable");
        return s.shape_and_child.required_owner(memory,q,obs);
    };
    const auto inverse=[&](std::uint32_t out,std::uint32_t matrix,std::uint32_t p) {
        auto x=v(m,matrix),y=v(m,matrix+16),z=v(m,matrix+32),value=v(m,p);
        const auto xy=_mm_shuffle_ps(x,y,0x44),xy2=_mm_shuffle_ps(x,y,0xee),zz=_mm_shuffle_ps(z,z,0x44),zz2=_mm_shuffle_ps(z,z,0xee);
        const auto tx=_mm_shuffle_ps(xy,zz,0x88),ty=_mm_shuffle_ps(xy,zz,0xdd),tz=_mm_shuffle_ps(xy2,zz2,0x88);
        vw(m,o,0x2e0d26,out,_mm_add_ps(_mm_add_ps(_mm_mul_ps(tx,_mm_shuffle_ps(value,value,0)),_mm_mul_ps(ty,_mm_shuffle_ps(value,value,0x55))),_mm_mul_ps(tz,_mm_shuffle_ps(value,value,0xaa))));
    };
    if(si(r(m,parent+16))>0) {
        w(m,o,0x300964,frame+20,4,0);w(m,o,0x300968,frame+32,4,0);
        do {
            const auto child=r(m,parent+12)+r(m,frame+32);
            if(r(m,child+8)!=source_null_physics_child_agent_1_1()) {
                const auto container=r(m,r(m,frame+36)+12),entry=r(m,r(m,container)+52),output=r(m,frame+108)+r(m,frame+20),key=r(m,child);
                std::uint32_t shape;
                if(entry==0x1f3320)shape=source_physics_bsp_child_shape_1_1(m,container,key,output,frame-12u,fp,s.shape_and_child,o);
                else {if(!s.shape_and_child.required_owner)missing("Required original dispatcher child shape unavailable");shape=s.shape_and_child.required_owner(m,{0x30099a,entry,container,frame-12u,std::nullopt,std::nullopt,std::nullopt,std::nullopt,{key,output}},o);}
                const auto ashape=r(m,a[0]);w(m,o,0x3009a6,frame+64,4,shape);w(m,o,0x3009aa,frame+68,4,key);
                const auto bt=shape_type(shape,0x3009b2,frame-4u),at=shape_type(ashape,0x3009ba,frame-8u),admission=r(m,0x4693ac);
                const std::array<std::uint32_t,4>gargs{frame+47,r(m,a[2]),at,bt};std::uint32_t gate;
                if(admission==0x2e1430)gate=source_admit_physics_narrowphase_1_1(m,gargs,frame-20u,narrow,o);
                else gate=required(m,s.contact,0x3009c9,admission,std::nullopt,frame-20u,{gargs.begin(),gargs.end()},o);
                const auto agent=r(m,child+8),method=r(m,r(m,agent)+(r(m,gate,1)?32u:16u));
                if(r(m,gate,1)) {
                    const std::array<std::uint32_t,5>args{frame+24,a[0],frame+64,a[2],frame+144};
                    if(method==0x2e14e0)source_update_physics_motion_narrowphase_1_1(m,agent,args,frame-24u,fp,narrow,o);
                    else required(m,s.contact,0x3009fd,method,agent,frame-24u,{args.begin(),args.end()},o);
                    if(!s.packet_frame_owner)missing("Required original300400 retained guest packet-frame owner unavailable");
                    const auto append_esp=frame-4u;
                    s.packet_frame_owner(m,{0x300a07,0x300400,append_esp,descriptor,
                        ((append_esp-4u)&~15u)-0x80u,128u},o);
                    source_append_physics_contact_packet_1_1(m,descriptor,frame-4u,s.contact,o);
                    const auto packet=[&](){return r(m,descriptor)+(r(m,descriptor+4)-1u)*128u;};
                    w(m,o,0x300a1d,packet()+112,4,agent);w(m,o,0x300a34,packet()+116,4,shape);w(m,o,0x300a49,packet()+120,4,key);w(m,o,0x300a62,packet()+125,1,r(m,frame+24,1));
                    if(r(m,frame+24,1)) {
                        const auto p=packet();vw(m,o,0x300a8f,p,v(m,frame+144));vw(m,o,0x300a9a,p+16,v(m,frame+160));w(m,o,0x300aa5,p+32,4,r(m,frame+176));w(m,o,0x300aaf,p+36,4,r(m,frame+180));
                        vw(m,o,0x300aba,p+48,v(m,frame+192));vw(m,o,0x300ac6,p+64,v(m,frame+208));inverse(frame+128,r(m,frame+72)+32,frame+208);
                        const auto distance=x87_copy(r(m,frame+176));vw(m,o,0x300b0a,packet()+80,v(m,frame+192));vw(m,o,0x300b15,packet()+96,v(m,frame+128));w(m,o,0x300b19,packet()+108,4,distance);
                        const auto type=r(m,agent+21,1);w(m,o,(type==1||type==2)?0x300b55:0x300b3c,packet()+124,1,(type==1||type==2)?1:0);
                    }
                }else required(m,s.contact,0x300b70,method,agent,frame-20u,{a[0],ashape,frame+64,a[3]},o);
            }
            w(m,o,0x300b92,frame+40,4,r(m,frame+40)+1u);w(m,o,0x300b96,frame+32,4,r(m,frame+32)+12u);w(m,o,0x300b9a,frame+20,4,r(m,frame+20)+256u);
        }while(si(r(m,frame+40))<si(r(m,parent+16)));
    }
    auto total=r(m,descriptor+4);w(m,o,0x300bb1,frame+20,4,0);
    if(si(total)>0) {
        w(m,o,0x300bbb,frame+24,4,0);
        do {
            const auto index=r(m,frame+20),current=r(m,descriptor)+r(m,frame+24);auto chosen=index;auto distance=f(r(m,current+32));
            for(auto j=index;j<total;++j){const auto candidate=f(r(m,r(m,descriptor)+j*128u+32));if(distance>candidate){chosen=j;distance=candidate;}}
            const auto selected=r(m,descriptor)+chosen*128u;
            const auto a0=v(m,selected),a10=v(m,selected+16),a30=v(m,selected+48),a40=v(m,selected+64),a50=v(m,selected+80),a60=v(m,selected+96);
            const auto s20=x87_copy(r(m,selected+32)),s24=x87_copy(r(m,selected+36)),s70=r(m,selected+112),s74=r(m,selected+116),s78=r(m,selected+120);
            vw(m,o,0x300cbc,selected,v(m,current));vw(m,o,0x300cc3,selected+16,v(m,current+16));w(m,o,0x300cc7,frame+264,4,s78);
            w(m,o,0x300cd1,frame+268,1,r(m,selected+124,1));w(m,o,0x300cdb,frame+269,1,r(m,selected+125,1));
            w(m,o,0x300ce5,selected+32,4,r(m,current+32));w(m,o,0x300ceb,selected+36,4,r(m,current+36));
            vw(m,o,0x300cf2,selected+48,v(m,current+48));vw(m,o,0x300cfa,selected+64,v(m,current+64));vw(m,o,0x300d02,selected+80,v(m,current+80));vw(m,o,0x300d0a,selected+96,v(m,current+96));
            w(m,o,0x300d11,selected+112,4,r(m,current+112));w(m,o,0x300d17,selected+116,4,r(m,current+116));w(m,o,0x300d1d,selected+120,4,r(m,current+120));w(m,o,0x300d23,selected+124,1,r(m,current+124,1));w(m,o,0x300d29,selected+125,1,r(m,current+125,1));
            w(m,o,0x300d2c,current+32,4,s20);w(m,o,0x300d3d,current+36,4,s24);vw(m,o,0x300d40,current,a0);vw(m,o,0x300d43,current+16,a10);vw(m,o,0x300d47,current+48,a30);vw(m,o,0x300d4b,current+64,a40);vw(m,o,0x300d4f,current+80,a50);vw(m,o,0x300d53,current+96,a60);
            w(m,o,0x300d57,current+120,4,r(m,frame+264));w(m,o,0x300d61,current+112,4,s70);w(m,o,0x300d68,current+116,4,s74);w(m,o,0x300d6b,current+124,1,r(m,frame+268,1));w(m,o,0x300d6e,current+125,1,r(m,frame+269,1));
            if(r(m,current+125,1))total=source_prune_physics_contact_packets_1_1(m,parent,{current+80,descriptor,index+1u,total,r(m,a[2]+12)},frame-24u,fp,s.contact,o);
            w(m,o,0x300db3,frame+20,4,index+1u);w(m,o,0x300db7,frame+24,4,r(m,frame+24)+128u);
        }while(si(r(m,frame+20))<si(total));
    }
    const auto count=r(m,descriptor+4);w(m,o,0x300dca,frame+40,4,count);
    if(si(count)>0) {
        w(m,o,0x300dd4,frame+24,4,0);w(m,o,0x300ddc,frame+32,4,count);
        do {
            const auto agent=r(m,r(m,descriptor)+r(m,frame+24)+112);auto n=r(m,agent+24),row=agent+64;
            while(si(n)>0) {
                const auto scalar=x87_copy(r(m,row+28));vw(m,o,0x300e13,frame+64,v(m,row));w(m,o,0x300e18,frame+76,4,scalar);inverse(frame+128,r(m,a[1]+8)+32,row+16);
                vw(m,o,0x300e4c,frame+80,v(m,frame+128));total=source_prune_physics_contact_packets_1_1(m,r(m,frame+28),{frame+64,descriptor,0,total,r(m,a[2]+12)},frame-24u,fp,s.contact,o);row+=48;--n;
            }
            w(m,o,0x300e6d,frame+24,4,r(m,frame+24)+128u);w(m,o,0x300e71,frame+32,4,r(m,frame+32)-1u);
        }while(r(m,frame+32));
        w(m,o,0x300e89,frame+20,4,0);std::uint32_t index=0;
        do {
            w(m,o,0x300e9a,frame+76,4,a[1]);w(m,o,0x300ea9,frame+72,4,r(m,a[1]+8));
            const auto packet=r(m,descriptor)+r(m,frame+20),agent=r(m,packet+112);
            w(m,o,0x300eb3,frame+64,4,r(m,packet+116));w(m,o,0x300eb7,frame+68,4,r(m,packet+120));
            if(si(index)<si(total)&&r(m,packet+125,1)) {
                SourcePhysicsContactBatchServices contact;
                contact.required_owner=[&](M&memory,const SourcePhysicsContactBatchOwnerCall&q,const O&obs)->std::uint32_t {
                    if(q.entry==0x31f1b0){std::array<std::uint32_t,4>args{};std::copy_n(q.arguments.begin(),4,args.begin());return source_create_physics_managed_contact_1_1(memory,*q.ecx,args,q.entry_esp,fp,s.contact,obs);}
                    if(!s.contact.required_owner)missing("Required original dispatcher retained-contact owner unavailable");return s.contact.required_owner(memory,q,obs);
                };
                source_retain_physics_contact_packet_1_1(m,{a[0],frame+64,a[2],packet,r(m,parent+8),agent+48,agent+24},frame-32u,fp,contact,o);
            }
            const float d=f(r(m,packet+32)),margin=f(r(m,0x4650e0));float minimum;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
            asm volatile("flds %1\n\tfsubs %2\n\tfstps %0":"=m"(minimum):"m"(d),"m"(margin):"st");
#else
            missing("Original dispatcher x87 minimum owner unavailable");
#endif
            w(m,o,0x300f0e,frame+36,4,bits(minimum));source_emit_retained_physics_contacts_1_1(m,{a[0],frame+64,a[2],bits(minimum),agent+48,agent+24,a[3],r(m,parent+8)},frame-36u,fp,s.contact,o);
            ++index;w(m,o,0x300f45,frame+20,4,r(m,frame+20)+128u);
        }while(si(index)<si(r(m,frame+40)));
    }
    profile(true);source_dispose_physics_contact_scratch_1_1(m,frame+108,frame-4u,s.contact,o);
    const auto cap=r(m,descriptor+8);if(si(cap)>=0)required(m,s.contact,0x300fc0,r(m,r(m,r(m,0x487638))+20),r(m,0x487638),frame-16u,{r(m,descriptor),(cap&0x7fffffff)*128u,0x12},o);
}
} // namespace halo2
