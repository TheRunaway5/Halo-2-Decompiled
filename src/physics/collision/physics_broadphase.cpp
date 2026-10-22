#include "halo2/physics_broadphase.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M=SourcePhysicsContactMemory;using O=SourcePhysicsContactRecordObserver;
using S=SourcePhysicsBroadphaseServices;using FP=SourcePhysicsFactoryFloatingPoint;
std::uint32_t r(const M&m,std::uint32_t p,unsigned n=4) {
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v=0;
    for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);
    return v;
}
void w(M&m,const O&o,std::uint32_t site,std::uint32_t p,unsigned n,std::uint64_t v) {
    SourcePhysicsContactRecordStore s{site,p,{},{}};s.before.resize(n);s.written.resize(n);
    m.read(p,s.before);for(unsigned i=0;i<n;++i)s.written[i]=std::byte((v>>(i*8))&255u);
    m.write(p,s.written);if(o)o(s);
}
std::int32_t i32(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
std::int16_t i16(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
[[noreturn]]void missing(const char*s){throw std::runtime_error(s);}
std::array<float,4> vector(const M&m,std::uint32_t p) {
    std::array<float,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::bit_cast<float>(r(m,p+i*4));return b;
}
void vector_store(M&m,const O&o,std::uint32_t site,std::uint32_t p,const std::array<float,4>&v) {
    const auto b=std::bit_cast<std::array<std::uint32_t,4>>(v);
    for(unsigned i=0;i<2;++i)w(m,o,site,p+i*8,8,std::uint64_t(b[i*2])|(std::uint64_t(b[i*2+1])<<32));
}
struct FpScope {
    unsigned mx;std::fenv_t env;
    explicit FpScope(FP p):mx(_mm_getcsr()) {
        if((p.x87_control_word&0x3fu)!=0x3fu||(p.mxcsr&0x1f80u)!=0x1f80u)
            missing("Unmasked original broadphase floating-point delivery unavailable");
        if(std::fegetenv(&env)!=0)missing("Original floating-point environment unavailable");
        _mm_setcsr(p.mxcsr);
    }
    ~FpScope(){std::fesetenv(&env);_mm_setcsr(mx);}
};
std::uint32_t scratch_ctor(M&m,std::uint32_t d,std::uint32_t count,unsigned stride,
    const S&s,const O&o,bool pairs) {
    const auto start=pairs?0x313cb0u:0x2e7dc0u;
    w(m,o,start,d,4,0);w(m,o,start+2,d+4,4,0);w(m,o,start+5,d+8,4,0x80000000);
    const auto adapter=r(m,0x487638),bytes=(count*stride+15u)&~15u,available=r(m,adapter+0xc);
    std::uint32_t p;
    if(i32(bytes)>i32(available)) {
        if(!s.scratch_allocate)missing("Required original scratch allocator virtual+24 owner unavailable");
        p=s.scratch_allocate(m,adapter,r(m,r(m,adapter)+0x24),bytes,o);
    }else {
        p=r(m,adapter+8);w(m,o,pairs?0x313cf0:0x2e7e00,adapter+8,4,p+bytes);
        w(m,o,pairs?0x313cf3:0x2e7e03,adapter+0xc,4,available-bytes);
    }
    w(m,o,pairs?0x313cf9:0x2e7e09,d,4,p);
    w(m,o,pairs?0x313d01:0x2e7e11,d+0xc,4,p);
    w(m,o,pairs?0x313d04:0x2e7e14,d+8,4,count|0x80000000u);
    w(m,o,pairs?0x313d0b:0x2e7e1b,d+0x10,4,count);return d;
}
void scratch_dtor(M&m,std::uint32_t d,unsigned stride,const S&s,const O&o,bool pairs) {
    const auto adapter=r(m,0x487638),p=r(m,d+0xc),bytes=(r(m,d+0x10)*stride+15u)&~15u;
    const auto alternate=r(m,adapter+0x14);
    if(bytes!=0&&(p==alternate||alternate==0)) {
        if(!s.scratch_free)missing("Required original scratch allocator virtual+28 owner unavailable");
        s.scratch_free(m,adapter,r(m,r(m,adapter)+0x28),p,bytes,o);
    }else {
        w(m,o,pairs?0x313eac:0x2e80ec,adapter+8,4,r(m,adapter+8)-bytes);
        w(m,o,pairs?0x313eaf:0x2e80ef,adapter+0xc,4,r(m,adapter+0xc)+bytes);
    }
    const auto capacity=r(m,d+8);
    if(i32(capacity)>=0) {
        const auto backend=r(m,0x480da0),block=r(m,d),n=(capacity&0x7fffffffu)*stride;
        if(r(m,r(m,adapter)+0x14)!=0x148a70u||
           (block&&(i32(block)<i32(backend+0x4340u)||i32(block)>i32(backend+0xf0340u)||
           r(m,r(m,backend)+0x14)!=0x22eb20u)))
            missing("Required original scratch grown-bank free owner unavailable");
        if(block)source_free_physics_body_small_block_1_1(m,backend,block,n,o);
    }
}
float half_margin(const M&m,std::uint32_t world,std::uint16_t cw) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const float a=std::bit_cast<float>(r(m,r(m,world+0xcc)+8)),b=std::bit_cast<float>(r(m,0x46502c));
    float out;std::uint16_t old;
    asm volatile("fnstcw %0" : "=m"(old));
    asm volatile("fldcw %1\n\tflds %2\n\tfmuls %3\n\tfstps %0" : "=m"(out) : "m"(cw),"m"(a),"m"(b) : "st");
    asm volatile("fldcw %0" : : "m"(old));return out;
#else
    (void)m;(void)world;(void)cw;missing("Original x87 margin producer unavailable");
#endif
}
}
std::uint32_t source_construct_physics_pair_bank_1_1(M&m,std::uint32_t d,std::uint32_t n,const S&s,const O&o){return scratch_ctor(m,d,n,8,s,o,true);}
void source_destroy_physics_pair_bank_1_1(M&m,std::uint32_t d,const S&s,const O&o){scratch_dtor(m,d,8,s,o,true);}
std::uint32_t source_construct_physics_overlap_bitset_1_1(M&m,std::uint32_t d,std::uint32_t n,const S&s,const O&o){return scratch_ctor(m,d,n,4,s,o,false);}
void source_destroy_physics_overlap_bitset_1_1(M&m,std::uint32_t d,const S&s,const O&o){scratch_dtor(m,d,4,s,o,false);}
std::uint32_t source_insert_physics_broadphase_axis_1_1(M&m,std::uint32_t axis,
    const std::array<std::uint32_t,6>&a,const O&o) {
    const auto count=r(m,axis+4)+2u,cap=r(m,axis+8)&0x7fffffffu;
    if(i32(cap)<i32(count)) {
        const auto doubled=cap*2u;
        source_grow_physics_body_bank_1_1(m,axis,i32(count)>=i32(doubled)?count:doubled,4,o);
    }
    auto p=r(m,axis)+count*4u-12u;
    w(m,o,0x2e7535,axis+4,4,count);
    const auto maximum=std::uint16_t(a[3]),minimum=std::uint16_t(a[2]),index=std::uint16_t(a[1]);
    while(maximum<=r(m,p,2)) {w(m,o,0x2e7542,p+8,4,r(m,p));p-=4;}
    w(m,o,0x2e7551,p+8,2,maximum);w(m,o,0x2e7555,p+10,2,index);
    w(m,o,0x2e7569,a[5],2,std::uint32_t(i32(p-r(m,axis))>>2)+2u);
    while(minimum<r(m,p,2)){w(m,o,0x2e7578,p+4,4,r(m,p));p-=4;}
    w(m,o,0x2e7583,p+6,2,index);w(m,o,0x2e758b,p+4,2,minimum);
    const auto result=std::uint32_t(i32(p-r(m,axis))>>2)+1u;
    w(m,o,0x2e7596,a[4],2,result);return result;
}
void source_repair_physics_broadphase_indices_1_1(M&m,std::uint32_t bank,
    std::uint32_t index,std::uint32_t fresh,const O&o) {
    const std::array<std::uint16_t,4> min_a{std::uint16_t(r(m,fresh,2)-1),std::uint16_t(r(m,fresh+2,2)-1),std::uint16_t(r(m,fresh,2)-1),std::uint16_t(r(m,fresh+2,2)-1)};
    const std::array<std::uint16_t,4> max_a{std::uint16_t(r(m,fresh+4,2)-2),std::uint16_t(r(m,fresh+6,2)-2),std::uint16_t(r(m,fresh+4,2)-2),std::uint16_t(r(m,fresh+6,2)-2)};
    const std::array<std::uint16_t,2> min_x{std::uint16_t(r(m,fresh+8,2)-1),std::uint16_t(r(m,fresh+8,2)-1)},max_x{std::uint16_t(r(m,fresh+10,2)-2),std::uint16_t(r(m,fresh+10,2)-2)};
    for(auto p=bank;p<bank+(index<<4);p+=16) {
        std::uint64_t lo=0,hi=0;
        for(unsigned n=0;n<4;++n) {
            const auto word=r(m,p+n*2,2);
            const auto delta=std::uint16_t((i16(word)>i16(max_a[n])?0xffffu:0u)+(i16(word)>i16(min_a[n])?0xffffu:0u));
            lo|=std::uint64_t(std::uint16_t(word-delta))<<(n*16);
        }
        for(unsigned n=0;n<4;++n) {
            const auto word=r(m,p+8+n*2,2);
            const auto delta=n<2?std::uint16_t((i16(word)>i16(max_x[n])?0xffffu:0u)+(i16(word)>i16(min_x[n])?0xffffu:0u)):std::uint16_t(0);
            hi|=std::uint64_t(std::uint16_t(word-delta))<<(n*16);
        }
        w(m,o,0x2e631c,p,8,lo);w(m,o,0x2e6326,p+8,8,hi);
    }
}
void source_physics_wrapper_bounds_1_1(M&m,std::uint32_t wrapper,std::uint32_t transform,
    std::uint32_t margin,std::uint32_t output,FP fp,const O&o) {
    const auto shape=r(m,wrapper+0x30);
    source_physics_shape_bounds_1_1(m,shape,transform,margin,output,fp,o);FpScope scope(fp);
    __m128 expansion=_mm_setzero_ps();const auto body=r(m,wrapper+0x34);
    if(body)if(const auto world=r(m,body+8)) {
        const auto velocity=vector(m,r(m,body+0x3c)+0x40);
        expansion=_mm_mul_ps(_mm_set1_ps(std::bit_cast<float>(r(m,world+0x150))),_mm_loadu_ps(velocity.data()));
    }
    const auto hi=vector(m,output+16),lo=vector(m,output);std::array<float,4> out{};
    _mm_storeu_ps(out.data(),_mm_max_ps(_mm_add_ps(_mm_loadu_ps(hi.data()),expansion),_mm_loadu_ps(hi.data())));
    vector_store(m,o,0x2e23e8,output+16,out);
    _mm_storeu_ps(out.data(),_mm_min_ps(_mm_add_ps(_mm_loadu_ps(lo.data()),expansion),_mm_loadu_ps(lo.data())));
    vector_store(m,o,0x2e23f5,output,out);
}
void source_insert_physics_broadphase_body_1_1(M&m,std::uint32_t backend,
    std::uint32_t handle,std::uint32_t bounds,std::uint32_t pairs,
    const SourcePhysicsBroadphaseScratch&scratch,FP fp,const S&s,const O&o) {
    FpScope scope(fp);
    const auto scale=vector(m,backend+0x30),bias=vector(m,0x418c00),upper=vector(m,0x418c20),lower=vector(m,0x418c10);
    const auto quantize=[&](std::uint32_t input,std::uint32_t origin,bool maximum) {
        const auto v=vector(m,input),offset=vector(m,origin);std::array<float,4> q{};
        auto x=_mm_mul_ps(_mm_add_ps(_mm_loadu_ps(v.data()),_mm_loadu_ps(offset.data())),_mm_loadu_ps(scale.data()));
        x=_mm_add_ps(_mm_max_ps(_mm_min_ps(x,_mm_loadu_ps(upper.data())),_mm_loadu_ps(lower.data())),_mm_loadu_ps(bias.data()));
        _mm_storeu_ps(q.data(),x);const auto words=std::bit_cast<std::array<std::uint32_t,4>>(q);
        std::array<std::uint32_t,3> result{};
        for(unsigned n=0;n<3;++n)result[n]=maximum?((words[n]>>7)&0xffffu)|1u:((words[n]>>7)&0xfffeu);
        return result;
    };
    const auto minimum=quantize(bounds,backend+0x10,false),maximum=quantize(bounds+16,backend+0x20,true);
    const auto index=r(m,backend+0x44);
    if(index==(r(m,backend+0x48)&0x7fffffffu))source_double_physics_body_bank_1_1(m,backend+0x40,16,o);
    const auto bank=r(m,backend+0x40),fresh=bank+r(m,backend+0x44)*16u;
    w(m,o,0x2e8b2e,backend+0x44,4,r(m,backend+0x44)+1u);
    source_insert_physics_broadphase_axis_1_1(m,backend+0x4c,{bank,index,minimum[0],maximum[0],fresh+8,fresh+10},o);
    source_insert_physics_broadphase_axis_1_1(m,backend+0x58,{bank,index,minimum[1],maximum[1],fresh,fresh+4},o);
    source_insert_physics_broadphase_axis_1_1(m,backend+0x64,{bank,index,minimum[2],maximum[2],fresh+2,fresh+6},o);
    source_repair_physics_broadphase_indices_1_1(m,bank,index,fresh,o);
    w(m,o,0x2e8bad,fresh+0xc,4,handle);w(m,o,0x2e8bb0,handle,4,index);
    const auto count=r(m,backend+0x44),descriptor=scratch.bitset_descriptor;
    source_construct_physics_overlap_bitset_1_1(m,descriptor,std::uint32_t(i32(count)>>5)+4u,s,o);
    const auto bitset=r(m,descriptor);
    for(std::uint32_t n=0;i32(n)<=i32(count)>>7;++n) {
        w(m,o,0x2e8bfc,bitset+n*16,8,0);w(m,o,0x2e8bfc,bitset+n*16+8,8,0);
    }
    const auto toggle=[&](std::uint32_t object,std::uint32_t site) {
        const auto p=bitset+(object>>5)*4u;w(m,o,site,p,4,r(m,p)^(1u<<(object&31u)));
    };
    auto table=r(m,backend+0x4c),scan=table+4u;
    if(r(m,backend+0x70)!=0) {
        const auto shift=(16u-r(m,backend+0x74))&31u;
        const auto bucket_index=std::uint32_t(i32(minimum[0])>>shift);
        if(i32(bucket_index)>0) {
            const auto bucket=r(m,backend+0x78)+bucket_index*16u-16u;
            const auto object=r(m,bucket,2);toggle(object,0x2e8c50);
            auto list=r(m,bucket+4);const auto n=r(m,bucket+8);
            for(std::uint32_t k=0;i32(k)<i32(n);++k,list+=2)toggle(r(m,list,2),0x2e8c86);
            const auto other=r(m,backend+0x40)+r(m,bucket,2)*16u;
            table=r(m,backend+0x4c);
            const auto stop=table+r(m,other+10,2)*4u;
            auto p=table+r(m,other+8,2)*4u+4u;
            while(p<stop) {
                if((r(m,p,1)&1u)==0) {
                    const auto candidate=r(m,p+2,2),at=bitset+(candidate>>5)*4u;
                    w(m,o,0x2e8cf8,at,4,r(m,at)&~(1u<<(candidate&31u)));
                }
                p+=4;
            }
            scan=table+r(m,other+8,2)*4u+4u;
        }
    }
    const auto min_edge=table+r(m,fresh+8,2)*4u;
    while(scan<min_edge) {toggle(r(m,scan+2,2),0x2e8d4e);scan+=4;}
    const auto max_edge=r(m,backend+0x4c)+r(m,fresh+10,2)*4u;
    scan+=4;
    while(scan<max_edge) {
        if((r(m,scan,1)&1u)==0)toggle(r(m,scan+2,2),0x2e8d90);
        scan+=4;
    }
    const auto body_bank=r(m,backend+0x40),words=std::uint32_t(i32(r(m,backend+0x44))>>5)+1u;
    for(std::uint32_t n=0;n<words;++n) {
        auto bits=r(m,bitset+n*4);auto other=body_bank+n*0x200u;
        while(bits) {
            if((bits&255u)==0){other+=0x80;bits>>=8;continue;}
            if(bits&1u) {
                const auto overlap=((r(m,fresh+4)-r(m,other))|(r(m,other+4)-r(m,fresh)))&0x80008000u;
                if(overlap==0) {
                    auto code=r(m,other+0xc);
                    if((code&1u)==0) {
                        if(r(m,pairs+4)==(r(m,pairs+8)&0x7fffffffu))source_double_physics_body_bank_1_1(m,pairs,8,o);
                        const auto p=r(m,pairs)+r(m,pairs+4)*8u;
                        w(m,o,0x2e8e45,p,4,handle);w(m,o,0x2e8e48,p+4,4,code);
                        w(m,o,0x2e8e4f,pairs+4,4,r(m,pairs+4)+1u);
                    }else {
                        const auto d=r(m,backend+0x78)+(code&~1u)+4u;
                        if(r(m,d+4)==(r(m,d+8)&0x7fffffffu))source_double_physics_body_bank_1_1(m,d,2,o);
                        w(m,o,0x2e8e84,r(m,d)+r(m,d+4)*2u,2,index);
                        w(m,o,0x2e8e88,d+4,4,r(m,d+4)+1u);
                    }
                }
            }
            other+=16;bits>>=1;
        }
    }
    source_destroy_physics_overlap_bitset_1_1(m,descriptor,s,o);
}
void source_insert_physics_world_broadphase_1_1(M&m,std::uint32_t world,std::uint32_t body,
    const SourcePhysicsWorldBroadphaseScratch&scratch,FP fp,const S&s,
    const SourcePhysicsWorldPairDeliveryServices&delivery,const O&o) {
    const auto shape=r(m,body+0xc);if(!shape)return;
    FpScope scope(fp);const auto frame=scratch.frame,pairs=frame+0xc,bounds=frame+0x20;
    source_construct_physics_pair_bank_1_1(m,pairs,r(m,world+0x234),s,o);
    const auto margin=std::bit_cast<std::uint32_t>(half_margin(m,world,fp.x87_control_word));
    const auto transform=r(m,body+0x14)+0x20,shape_method=r(m,r(m,shape)+0x18);
    if(shape_method==0x303e30)source_physics_convex_bounds_1_1(m,shape,transform,margin,bounds,fp,o);
    else if(shape_method==0x2e2380)source_physics_wrapper_bounds_1_1(m,shape,transform,margin,bounds,fp,o);
    else if(shape_method==0x2e4dd0||shape_method==0x2e52e0||shape_method==0x2e4670||shape_method==0x3049b0)
        source_physics_shape_bounds_1_1(m,shape,transform,margin,bounds,fp,o);
    else missing("Required original world current-shape bounds owner unavailable");
    const auto backend=r(m,world+0xc4);
    if(r(m,r(m,backend)+4)!=0x2e8a00)missing("Required original world broadphase backend virtual owner unavailable");
    source_insert_physics_broadphase_body_1_1(m,backend,body+0x1c,bounds,pairs,scratch.backend,fp,s,o);
    if(i32(r(m,pairs+4))>0) {
        if(r(m,body+0x40,1)) {
            const auto list=frame+0x4c;
            w(m,o,0x314579,list,4,0x419f30);w(m,o,0x314581,list+8,4,frame+0x60);
            w(m,o,0x314585,list+0xc,4,0);w(m,o,0x31458d,list+0x10,4,0x80000004);
            w(m,o,0x314595,list+4,4,body+0xc);
            const auto manager=r(m,world+0x138);w(m,o,0x3145a3,manager+0x24,4,list);
            const auto optional=r(m,world+0xd0);
            if(!delivery.static_pairs)missing("Required original378EB0 world pair manager owner unavailable");
            delivery.static_pairs(m,manager,{r(m,pairs),r(m,pairs+4),0,0,optional?optional+8u:0u},o);
            const auto retained_manager=r(m,world+0x138);
            w(m,o,0x3145e2,retained_manager+0x24,4,retained_manager+0x100);
            for(std::uint32_t n=0;i32(n)<i32(r(m,list+0xc));++n) {
                const auto entity=r(m,r(m,list+8)+n*4u),d=entity+0x80;
                if(!delivery.dynamic_pairs)missing("Required original31D980 island pair manager owner unavailable");
                delivery.dynamic_pairs(m,entity,{r(m,d),r(m,d+4),0,0},o);
                // Original AND7FFFFFFF clears SF, so JGE always skips growth.
                w(m,o,0x314635,d+4,4,0);
            }
            const auto capacity=r(m,list+0x10);
            if(i32(capacity)>=0) {
                const auto adapter=r(m,0x487638),backend_allocator=r(m,0x480da0),p=r(m,list+8);
                if(r(m,r(m,adapter)+0x14)!=0x148a70u||(p&&(i32(p)<i32(backend_allocator+0x4340u)||
                   i32(p)>i32(backend_allocator+0xf0340u)||r(m,r(m,backend_allocator)+0x14)!=0x22eb20u)))
                    missing("Required original static pair-list free owner unavailable");
                if(p)source_free_physics_body_small_block_1_1(m,backend_allocator,p,(capacity&0x7fffffffu)*4u,o);
            }
            w(m,o,0x31421a,list,4,0x419ebc);
        }else {
            if(!delivery.dynamic_pairs)missing("Required original31D980 island pair manager owner unavailable");
            delivery.dynamic_pairs(m,r(m,body+0x44),{r(m,pairs),r(m,pairs+4),0,0},o);
        }
    }
    source_destroy_physics_pair_bank_1_1(m,pairs,s,o);
}
} // namespace halo2
