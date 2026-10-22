#include "halo2/physics_current_broadphase.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M=SourcePhysicsContactMemory;using O=SourcePhysicsContactRecordObserver;
using FP=SourcePhysicsFactoryFloatingPoint;using S=SourcePhysicsCurrentBroadphaseServices;
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
std::int32_t sign(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
[[noreturn]]void missing(const char*s){throw std::runtime_error(s);}
struct FpScope {
    unsigned mx;std::fenv_t env;
    explicit FpScope(FP p):mx(_mm_getcsr()) {
        if((p.x87_control_word&0x3fu)!=0x3fu||(p.mxcsr&0x1f80u)!=0x1f80u)
            missing("Unmasked original current broadphase FP effects unavailable");
        if(std::fegetenv(&env)!=0)missing("Original current broadphase FP environment unavailable");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("fldcw %0" : :"m"(p.x87_control_word):"memory");
#else
        missing("Original current broadphase x87 backend unavailable");
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
std::uint32_t x87_sum(std::uint32_t a,std::uint32_t b,bool multiply) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto x=std::bit_cast<float>(a),y=std::bit_cast<float>(b);float z;
    if(multiply)asm volatile("flds %1\n\tfmuls %2\n\tfstps %0":"=m"(z):"m"(x),"m"(y):"st");
    else asm volatile("flds %1\n\tfadds %2\n\tfstps %0":"=m"(z):"m"(x),"m"(y):"st");
    return std::bit_cast<std::uint32_t>(z);
#else
    (void)a;(void)b;(void)multiply;missing("Original current bounds x87 spill unavailable");
#endif
}
// Each row is one complete original endpoint crossing loop, in source order:
// minimum-left, maximum-right, maximum-left, minimum-right. Sites are literal
// original owning stores; X's packed overlap and special bucket differ fromYZ.
struct Sites {
    std::uint32_t own,copy,id,min_index,max_index,pair_count,pair_a,pair_b,
        bucket_count,bucket_word,own_id,own_index,final_value;
};
constexpr std::array<std::array<Sites,4>,3> sites{{
    {{{0,0x2e6a1f,0x2e6a22,0x2e6adf,0x2e6a2c,0x2e6a8b,0x2e6a95,0x2e6a9a,0x2e6ada,0x2e6ad2,0x2e6aec,0x2e6af4,0x2e6b10},
      {0x2e6b34,0x2e6b49,0x2e6b51,0x2e6b5b,0x2e6c01,0x2e6bb2,0x2e6bb8,0x2e6bc0,0x2e6bfc,0x2e6bf1,0,0,0},
      {0x2e6c39,0x2e6c4a,0x2e6c52,0x2e6c5c,0x2e6d04,0x2e6cb1,0x2e6cb7,0x2e6cbc,0x2e6cef,0x2e6cf9,0,0,0x2e6d1b},
      {0x2e6d47,0x2e6d58,0x2e6d60,0x2e6e12,0x2e6d6a,0x2e6dbf,0x2e6dc5,0x2e6dca,0x2e6dfd,0x2e6e07,0,0,0x2e6e2d}}},
    {{{0,0x2e6e68,0x2e6e5f,0x2e6edc,0x2e6e6d,0x2e6ecc,0x2e6ed2,0x2e6ed7,0,0,0x2e6eec,0x2e6ef8,0x2e6f07},
      {0x2e6f30,0x2e6f45,0x2e6f4d,0x2e6f53,0x2e6fc1,0x2e6fb1,0x2e6fb7,0x2e6fbc,0,0,0,0,0},
      {0x2e7007,0x2e7018,0x2e7020,0x2e7026,0x2e7094,0x2e7084,0x2e708a,0x2e708f,0,0,0,0,0x2e70ac},
      {0x2e70d7,0x2e70e7,0x2e70ef,0x2e7164,0x2e70f5,0x2e7154,0x2e715a,0x2e715f,0,0,0,0,0x2e7177}}},
    {{{0,0x2e71b8,0x2e71af,0x2e722a,0x2e71bd,0x2e721a,0x2e7220,0x2e7225,0,0,0x2e723b,0x2e7247,0x2e7263},
      {0x2e7283,0x2e7298,0x2e72a0,0x2e72a6,0x2e7313,0x2e7303,0x2e7309,0x2e730e,0,0,0,0,0},
      {0x2e7357,0x2e7368,0x2e7370,0x2e7376,0x2e73e3,0x2e73d3,0x2e73d9,0x2e73de,0,0,0,0,0x2e73fb},
      {0x2e7427,0x2e7438,0x2e7440,0x2e74b3,0x2e7446,0x2e74a3,0x2e74a9,0x2e74ae,0,0,0,0,0x2e74ce}}}
}};
constexpr std::array<unsigned,3> minimum_offset{8,0,2},maximum_offset{10,4,6};
bool overlap(const M&m,std::uint32_t own,std::uint32_t other,unsigned axis) {
    if(axis==0)return (((r(m,other+4)-r(m,own))|(r(m,own+4)-r(m,other)))&0x80008000u)==0;
    std::uint32_t bits=0;
    for(unsigned a=0;a<3;++a)if(a!=axis) {
        bits|=std::uint16_t(r(m,other+maximum_offset[a],2)-r(m,own+minimum_offset[a],2));
        bits|=std::uint16_t(r(m,own+maximum_offset[a],2)-r(m,other+minimum_offset[a],2));
    }
    return (bits&0x8000u)==0;
}
void pair_delta(M&m,const O&o,std::uint32_t backend,std::uint32_t own,
    std::uint32_t other,std::uint32_t index,std::uint32_t descriptor,
    unsigned axis,unsigned operation,const Sites&s) {
    if(!overlap(m,own,other,axis))return;
    const auto code=r(m,other+12);
    if(axis!=0||(code&1u)==0) {
        if(r(m,descriptor+4)==(r(m,descriptor+8)&0x7fffffffu))source_double_physics_body_bank_1_1(m,descriptor,8,o);
        const auto dst=r(m,descriptor)+r(m,descriptor+4)*8u;
        w(m,o,s.pair_count,descriptor+4,4,r(m,descriptor+4)+1u);
        w(m,o,s.pair_a,dst,4,r(m,own+12));w(m,o,s.pair_b,dst+4,4,code);
    }else {
        const auto d=r(m,backend+0x78)+(code&~1u)+4u;
        if(operation<2) {
            if(r(m,d+4)==(r(m,d+8)&0x7fffffffu))source_double_physics_body_bank_1_1(m,d,2,o);
            w(m,o,s.bucket_word,r(m,d)+r(m,d+4)*2u,2,index);
            w(m,o,s.bucket_count,d+4,4,r(m,d+4)+1u);
        }else {
            auto k=0u;const auto n=r(m,d+4),p=r(m,d);
            while(sign(k)<sign(n)&&r(m,p+k*2u,2)!=std::uint16_t(index))++k;
            if(sign(k)>=sign(n))k=0xffffffffu;
            w(m,o,s.bucket_count,d+4,4,n-1u);
            w(m,o,s.bucket_word,p+k*2u,2,r(m,p+(n-1u)*2u,2));
        }
    }
}
std::uint32_t required(M&m,const S&s,std::uint32_t site,std::uint32_t target,
    std::uint32_t receiver,std::uint32_t esp,std::vector<std::uint32_t>args,const O&o) {
    if(!s.required_owner)missing("Required original current broadphase lower owner unavailable");
    return s.required_owner(m,{site,target,receiver,esp,std::move(args),false},o);
}
void profile(M&m,const S&s,std::uint32_t name,std::uint32_t site,std::uint32_t spill,
    std::uint32_t spill_site,std::uint32_t output_site,std::uint32_t global_site,const O&o) {
    const auto p=r(m,0x487674);if(p>=r(m,0x487678))return;
    w(m,o,site,p,4,name);
    if(!s.timestamp_counter)missing("Required original current broadphase RDTSC provider unavailable");
    const auto counter=std::uint32_t(s.timestamp_counter());w(m,o,spill_site,spill,4,counter);
    w(m,o,output_site,p+4,4,counter);w(m,o,global_site,0x487674,4,p+12u);
}
}
void source_update_physics_broadphase_bodies_1_1(M&m,std::uint32_t backend,
    const std::array<std::uint32_t,5>&a,FP fp,const O&o) {
    FpScope scope(fp);auto handles=a[0],bounds=a[1];const auto end=handles+a[2]*4u;
    while(handles<end) {
        const auto quantize=[&](std::uint32_t input,std::uint32_t origin,bool maximum) {
            auto q=_mm_mul_ps(_mm_add_ps(v(m,input),v(m,origin)),v(m,backend+0x30));
            q=_mm_add_ps(_mm_max_ps(_mm_min_ps(q,v(m,0x418c20)),v(m,0x418c10)),v(m,0x418c00));
            std::array<float,4>f{};_mm_storeu_ps(f.data(),q);const auto bits=std::bit_cast<std::array<std::uint32_t,4>>(f);
            std::array<std::uint32_t,3> result{};
            for(unsigned n=0;n<3;++n)result[n]=maximum?((bits[n]>>7)&0xffffu)|1u:((bits[n]>>7)&0xfffeu);
            return result;
        };
        const auto lo=quantize(bounds,backend+0x10,false),hi=quantize(bounds+16,backend+0x20,true);
        const auto index=r(m,r(m,handles)),bank=r(m,backend+0x40),own=bank+index*16u;
        for(unsigned axis=0;axis<3;++axis) {
            const auto table=r(m,backend+0x4c+axis*12u),mn=minimum_offset[axis],mx=maximum_offset[axis];
            for(unsigned op=0;op<4;++op) {
                const auto&s=sites[axis][op];const bool left=op==0||op==2,is_min=op==0||op==3;
                const auto off=is_min?mn:mx,value=is_min?lo[axis]:hi[axis];
                auto at=r(m,own+off,2),p=table+at*4u;
                const auto crossed=[&]{return r(m,left?p-4u:p+4u,2);};
                while(left?value<crossed():crossed()<value) {
                    const auto edge=crossed(),other_id=r(m,left?p-2u:p+6u,2),other=bank+other_id*16u;
                    const bool other_max=(edge&1u)!=0;
                    if(op==0) {
                        if(axis==0){w(m,o,s.copy,p,2,edge);w(m,o,s.id,p+2,2,other_id);}
                        else {w(m,o,s.id,p+2,2,other_id);w(m,o,s.copy,p,2,edge);}
                        w(m,o,other_max?s.max_index:s.min_index,other+(other_max?mx:mn),2,at);
                        if(other_max)pair_delta(m,o,backend,own,other,index,a[3],axis,op,s);
                        --at;p-=4u;w(m,o,s.own_id,p+2,2,index);w(m,o,s.own_index,own+off,2,at);
                    }else {
                        at=left?at-1u:at+1u;
                        w(m,o,s.own,own+off,2,at);
                        const auto neighbor=left?p-4u:p+4u;
                        w(m,o,s.copy,p,4,r(m,neighbor));w(m,o,s.id,neighbor+2,2,index);
                        const auto other_off=other_max?mx:mn;
                        w(m,o,other_max?s.max_index:s.min_index,other+other_off,2,
                            left?r(m,other+other_off,2)+1u:r(m,other+other_off,2)-1u);
                        if(other_max==is_min)pair_delta(m,o,backend,own,other,index,op==1?a[3]:a[4],axis,op,s);
                        p=neighbor;
                    }
                }
                // The first maximum-right loop delays its value store until
                // maximum-left completes; these are the actual nine writes.
                if(s.final_value)w(m,o,s.final_value,p,2,value);
            }
        }
        handles+=4u;bounds+=32u;
    }
}
void source_physics_capsule_bounds_1_1(M&m,std::uint32_t shape,std::uint32_t transform,
    std::uint32_t margin,std::uint32_t output,FP fp,const O&o) {
    FpScope scope(fp);const auto radius=x87_sum(margin,r(m,shape+12),false);
    const auto rotation_x=v(m,transform),rotation_y=v(m,transform+16),rotation_z=v(m,transform+32),translation=v(m,transform+48);
    const auto point=[&](std::uint32_t p) {
        const auto x=v(m,p);const auto z=_mm_mul_ps(rotation_z,_mm_shuffle_ps(x,x,0xaa));
        const auto y=_mm_mul_ps(rotation_y,_mm_shuffle_ps(x,x,0x55));
        const auto xx=_mm_mul_ps(rotation_x,_mm_shuffle_ps(x,x,0));
        return _mm_add_ps(_mm_add_ps(xx,y),_mm_add_ps(z,translation));
    };
    const auto a=point(shape+16),b=point(shape+32),rad=_mm_set1_ps(std::bit_cast<float>(radius));
    vw(m,o,0x2e4707,output+16,_mm_max_ps(a,b));
    vw(m,o,0x2e470e,output,_mm_sub_ps(_mm_min_ps(a,b),rad));
    vw(m,o,0x2e4718,output+16,_mm_add_ps(v(m,output+16),rad));
}
void source_physics_current_wrapper_bounds_1_1(M&m,std::uint32_t wrapper,std::uint32_t transform,
    std::uint32_t margin,std::uint32_t output,FP fp,const O&o) {
    const auto shape=r(m,wrapper+0x30),target=r(m,r(m,shape)+0x18);
    if(target==0x303e30)source_physics_convex_bounds_1_1(m,shape,transform,margin,output,fp,o);
    else if(target==0x2e4670)source_physics_capsule_bounds_1_1(m,shape,transform,margin,output,fp,o);
    else missing("Required original current wrapper underlying shape bounds unavailable");
    FpScope scope(fp);auto expansion=_mm_setzero_ps();const auto body=r(m,wrapper+0x34);
    if(body)if(const auto world=r(m,body+8))expansion=_mm_mul_ps(_mm_set1_ps(std::bit_cast<float>(r(m,world+0x150))),v(m,r(m,body+0x3c)+0x40));
    const auto hi=v(m,output+16),lo=v(m,output);
    vw(m,o,0x2e23e8,output+16,_mm_max_ps(_mm_add_ps(hi,expansion),hi));
    vw(m,o,0x2e23f5,output,_mm_min_ps(_mm_add_ps(lo,expansion),lo));
}
void source_update_physics_island_broadphase_1_1(M&m,std::uint32_t island,std::uint32_t backend,
    std::uint32_t step,std::uint32_t entry,FP fp,const S&s,const O&o) {
    (void)step;FpScope scope(fp);const auto frame=entry-0x5cu,added=frame+0x28u,removed=frame+0x3cu;
    profile(m,s,0x41b650,0x31dba3,frame+0x18,0x31dbab,0x31dbb3,0x31dbb9,o);
    source_construct_physics_pair_bank_1_1(m,added,r(m,r(m,island+0x20)+0x238),s.scratch,o);
    source_construct_physics_pair_bank_1_1(m,removed,r(m,r(m,island+0x20)+0x238),s.scratch,o);
    const auto alloc=[&](std::uint32_t bytes,std::uint32_t bytes_slot,std::uint32_t bytes_site,
        std::uint32_t pointer_slot,std::uint32_t pointer_site,std::uint32_t call_site,
        std::uint32_t cursor_site,std::uint32_t available_site) {
        const auto adapter=r(m,0x487638),available=r(m,adapter+12);
        w(m,o,bytes_site,bytes_slot,4,bytes);std::uint32_t p;
        if(sign(bytes)>sign(available)) {
            if(!s.scratch.scratch_allocate)missing("Required original current bounds scratch allocator unavailable");
            p=s.scratch.scratch_allocate(m,adapter,r(m,r(m,adapter)+0x24),bytes,o);
            pointer_site=call_site==0x31dc14?0x31dc17:0x31dc50;
        }else {
            p=r(m,adapter+8);w(m,o,cursor_site,adapter+8,4,p+bytes);w(m,o,available_site,adapter+12,4,available-bytes);
        }
        w(m,o,pointer_site,pointer_slot,4,p);return p;
    };
    const auto bounds=alloc((r(m,island+0x40)*32u+15u)&~15u,frame+0x24,0x31dc0b,frame+0x14,0x31dc2b,0x31dc14,0x31dc25,0x31dc28);
    const auto handles=alloc((r(m,island+0x40)*4u+15u)&~15u,frame+0x20,0x31dc44,frame+0x10,0x31dc64,0x31dc4d,0x31dc5e,0x31dc61);
    const auto margin=x87_sum(r(m,r(m,r(m,island+0x20)+0xcc)+8),r(m,0x46502c),true);
    w(m,o,0x31dc8d,frame+0x1c,4,margin);
    profile(m,s,0x41b644,0x31dc93,frame+0x18,0x31dc9b,0x31dca3,0x31dca9,o);
    auto n=r(m,island+0x40),p=r(m,island+0x3c),out=bounds,handle=handles;
    if(sign(n-1u)>=0) {
        w(m,o,0x31dcc1,frame+0x18,4,n);
        do {
            const auto collidable=r(m,p)+12u;w(m,o,0x31dcd3,handle,4,collidable?collidable+16u:0u);
            const auto shape=r(m,collidable),transform=r(m,collidable+8)+0x20u,target=r(m,r(m,shape)+0x18);
            if(target==0x2e2380)source_physics_current_wrapper_bounds_1_1(m,shape,transform,margin,out,fp,o);
            else if(target==0x303e30)source_physics_convex_bounds_1_1(m,shape,transform,margin,out,fp,o);
            else if(target==0x2e4670)source_physics_capsule_bounds_1_1(m,shape,transform,margin,out,fp,o);
            else required(m,s,0x31dce5,target,shape,frame-16u,{transform,margin,out},o);
            out+=32u;p+=4u;handle+=4u;--n;w(m,o,0x31dcf6,frame+0x18,4,n);
        }while(n);
    }
    profile(m,s,0x41b634,0x31dd0f,frame+0x1c,0x31dd17,0x31dd1f,0x31dd25,o);
    const auto target=r(m,r(m,backend)+0x14);
    if(target==0x2e68d0)source_update_physics_broadphase_bodies_1_1(m,backend,{handles,bounds,r(m,island+0x40),added,removed},fp,o);
    else required(m,s,0x31dd49,target,backend,frame-24u,{handles,bounds,r(m,island+0x40),added,removed},o);
    profile(m,s,0x41b628,0x31dd5f,frame+0x60,0x31dd67,0x31dd6f,0x31dd75,o);
    if(sign(r(m,island+0x78))>0) {
        const auto old=r(m,added+4),count=old+r(m,island+0x78),capacity=r(m,added+8)&0x7fffffffu;
        if(sign(capacity)<sign(count))source_grow_physics_body_bank_1_1(m,added,sign(count)<sign(capacity*2u)?capacity*2u:count,8,o);
        w(m,o,0x31ddb7,added+4,4,count);
        for(std::uint32_t k=0;sign(k)<sign(r(m,island+0x78));++k) {
            const auto src=r(m,island+0x74)+k*8u,dst=r(m,added)+(old+k)*8u;
            w(m,o,0x31ddce,dst,4,r(m,src));w(m,o,0x31ddd9,dst+4,4,r(m,src+4));
        }
        w(m,o,0x31dde8,island+0x78,4,0);
    }
    const auto release=[&](std::uint32_t p,std::uint32_t bytes,std::uint32_t cursor_site,std::uint32_t available_site) {
        const auto adapter=r(m,0x487638),alternate=r(m,adapter+0x14);
        if(bytes&&(p==alternate||alternate==0)) {
            if(!s.scratch.scratch_free)missing("Required original current bounds scratch retirement unavailable");
            s.scratch.scratch_free(m,adapter,r(m,r(m,adapter)+0x28),p,bytes,o);
        }else {w(m,o,cursor_site,adapter+8,4,r(m,adapter+8)-bytes);w(m,o,available_site,adapter+12,4,r(m,adapter+12)+bytes);}
    };
    release(handles,r(m,frame+0x20),0x31de1f,0x31de22);
    release(bounds,r(m,frame+0x24),0x31de51,0x31de54);
    source_update_physics_island_pairs_1_1(m,island,{r(m,added),r(m,added+4),r(m,removed),r(m,removed+4)},frame-0x40u,s.pairs,o);
    w(m,o,0x31de72,island+0x30,1,1);
    profile(m,s,0x41903c,0x31de8f,entry+4u,0x31de97,0x31de9f,0x31dea5,o);
    source_destroy_physics_pair_bank_1_1(m,removed,s.scratch,o);source_destroy_physics_pair_bank_1_1(m,added,s.scratch,o);
}
} // namespace halo2
