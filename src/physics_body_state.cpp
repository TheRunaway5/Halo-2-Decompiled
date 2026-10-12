#include "halo2/physics_body_state.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using Memory=SourcePhysicsContactMemory;
using Observer=SourcePhysicsContactRecordObserver;
std::uint32_t read(const Memory &m,std::uint32_t a,unsigned n=4) {
    std::array<std::byte,4>b{};m.read(a,std::span(b).first(n));std::uint32_t v=0;
    for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);
    return v;
}
std::int32_t signed_word(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
std::uint32_t signed_byte(std::uint32_t v) {
    return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v))));
}
void write(Memory &m,const Observer &o,std::uint32_t site,std::uint32_t a,
           unsigned n,std::uint64_t v) {
    SourcePhysicsContactRecordStore s{site,a,{},{}};s.before.resize(n);s.written.resize(n);
    m.read(a,s.before);for(unsigned i=0;i<n;++i)s.written[i]=std::byte((v>>(i*8))&255u);
    m.write(a,s.written);if(o)o(s);
}
void zero_vector(Memory &m,const Observer&o,std::uint32_t site,std::uint32_t p) {
    write(m,o,site,p,8,0);write(m,o,site,p+8,8,0);
}
[[noreturn]]void unavailable(const char *s){throw std::runtime_error(s);}
void method(const Memory&m,std::uint32_t p,unsigned slot,std::uint32_t target) {
    if(read(m,read(m,p)+slot)!=target)unavailable("Required original allocator virtual owner unavailable");
}
std::uint32_t allocate(Memory &m,std::uint32_t bytes,const Observer&o) {
    method(m,read(m,0x487638),0x10,0x148a10);
    if(read(m,0x480d98)!=2)unavailable("Required original allocator mode unavailable");
    const auto backend=read(m,0x480da0);method(m,backend,0x10,0x22e950);
    const auto p=source_allocate_physics_body_small_block_1_1(m,backend,bytes,o);
    if(p==0)unavailable("Required original alternate allocator provider unavailable");
    return p;
}
void release(Memory&m,std::uint32_t p,std::uint32_t bytes,const Observer&o) {
    method(m,read(m,0x487638),0x14,0x148a70);
    if(p==0)return;
    const auto backend=read(m,0x480da0);
    // Original signed address comparisons in148A70; no range-only success.
    if(signed_word(p)<signed_word(backend+0x4340u) ||
       signed_word(p)>signed_word(backend+0xf0340u))
        unavailable("Required original alternate allocator free provider unavailable");
    method(m,backend,0x14,0x22eb20);
    source_free_physics_body_small_block_1_1(m,backend,p,bytes,o);
}
void copy(Memory&m,std::uint32_t dst,std::uint32_t src,std::uint32_t size,const Observer&o) {
    const auto words=size>>2;
    for(std::uint32_t i=0;i<words;++i)write(m,o,0x2df373,dst+i*4,4,read(m,src+i*4));
    for(std::uint32_t i=words*4;i<size;++i)write(m,o,0x2df37a,dst+i,1,read(m,src+i,1));
}
void grow(Memory&m,std::uint32_t d,std::uint32_t cap,std::uint32_t stride,
          bool doubled,const Observer&o) {
    const auto p=allocate(m,cap*stride,o);copy(m,p,read(m,d),read(m,d+4)*stride,o);
    const auto oldcap=read(m,d+8);
    if(signed_word(oldcap)>=0)release(m,read(m,d),(oldcap&0x7fffffffu)*stride,o);
    write(m,o,doubled?0x2df7e3:0x2df768,d,4,p);
    write(m,o,doubled?0x2df7e6:0x2df76b,d+8,4,cap);
}
void info_header(Memory&m,std::uint32_t p,const Observer&o) {
    write(m,o,0x31c114,p+0xc,4,0);write(m,o,0x31c117,p+0x10,4,0);
    write(m,o,0x31c11a,p+0x14,4,0x80000000);write(m,o,0x31c121,p,4,0);
    write(m,o,0x31c123,p+4,4,0);write(m,o,0x31c126,p+8,1,1);
    write(m,o,0x31c1a8,p+0x18,1,1);write(m,o,0x31c1ac,p+0x1a,2,0xffff);
}
struct FpScope {
    unsigned saved;std::fenv_t environment;
    explicit FpScope(std::uint16_t cw,std::uint32_t mxcsr):saved(_mm_getcsr()) {
        if((cw&0x3fu)!=0x3fu || (mxcsr&0x1f80u)!=0x1f80u)
            unavailable("Unmasked source floating-point exception delivery unavailable");
        if(std::fegetenv(&environment)!=0)unavailable("Floating-point environment unavailable");
        _mm_setcsr(mxcsr);
    }
    ~FpScope(){std::fesetenv(&environment);_mm_setcsr(saved);}
};
// X/Y retain the actual reciprocal on the x87 stack across both products.
// Z explicitly spills the reciprocal to binary32 before either product.
std::array<float,2> bounds_axis(float one,float scale,std::int32_t lo,
    std::int32_t hi,bool reciprocal_spill,std::uint16_t cw) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint16_t saved;std::array<float,2> out{};float reciprocal;
    asm volatile("fnstcw %0" : "=m"(saved));
    if(reciprocal_spill) {
        asm volatile("fldcw %1\n\tflds %2\n\tfdivs %3\n\tfstps %0"
            : "=m"(reciprocal) : "m"(cw),"m"(one),"m"(scale) : "memory");
        asm volatile("fildl %2\n\tfmuls %4\n\tfstps %0\n\tfildl %3\n\tfmuls %4\n\tfstps %1\n\tfldcw %5"
            : "=m"(out[0]),"=m"(out[1]) : "m"(lo),"m"(hi),"m"(reciprocal),"m"(saved) : "memory");
    } else {
        asm volatile("fldcw %2\n\tflds %3\n\tfdivs %4\n\tfildl %5\n\tfmul %%st(1),%%st\n\tfstps %0\n\tfildl %6\n\tfmulp\n\tfstps %1\n\tfldcw %7"
            : "=m"(out[0]),"=m"(out[1]) : "m"(cw),"m"(one),"m"(scale),"m"(lo),"m"(hi),"m"(saved) : "memory");
    }
    return out;
#else
    (void)one;(void)scale;(void)lo;(void)hi;(void)reciprocal_spill;(void)cw;
    unavailable("Original x87 current-bounds backend unavailable");
#endif
}
}
std::uint32_t source_initialize_physics_body_info_1_1(Memory&m,std::uint32_t p,const Observer&o) {
    info_header(m,p,o);
    zero_vector(m,o,0x31789b,p+0x20);zero_vector(m,o,0x31789f,p+0x30);
    write(m,o,0x3178a8,p+0x3c,4,0x3f800000);
    for(const auto [site,offset]:std::array<std::array<std::uint32_t,2>,5>{{
        {0x3178ab,0x40},{0x3178af,0x50},{0x3178b3,0x60},{0x3178b7,0x70},{0x3178bb,0x80}}})
        zero_vector(m,o,site,p+offset);
    write(m,o,0x3178c2,p+0x60,4,0x3f800000);write(m,o,0x3178c5,p+0x74,4,0x3f800000);
    write(m,o,0x3178c8,p+0x88,4,0x3f800000);write(m,o,0x3178ce,p+0xa0,4,0x3f800000);
    zero_vector(m,o,0x3178d4,p+0x90);
    write(m,o,0x3178db,p+0xa4,4,0);write(m,o,0x3178e5,p+0xa8,4,0x3d4ccccd);
    write(m,o,0x3178ef,p+0xac,4,0x3f000000);write(m,o,0x3178f9,p+0xb0,4,0x3ecccccd);
    write(m,o,0x317903,p+0xb4,1,1);write(m,o,0x31790a,p+0xb5,1,2);return p;
}
std::uint32_t source_allocate_physics_body_small_block_1_1(Memory&m,std::uint32_t a,
    std::uint32_t bytes,const Observer&o) {
    if(signed_word(bytes)<0)unavailable("Required original negative allocation-size owner unavailable");
    if(signed_word(bytes)>0x2000)return 0;
    const auto c=bytes<=0x200?signed_byte(read(m,a+0xac+bytes,1)):
      signed_word(read(m,a+0x2b0+((bytes-1)>>10)*4));
    const auto head_at=a+0x24+c*4u,count_at=a+0x2e8+c*4u;
    const auto head=read(m,head_at);
    write(m,o,0x22e98f,count_at,4,read(m,count_at)+1u);
    if(head) {
        write(m,o,0x22e99a,head_at,4,read(m,head));
        const auto free_at=a+0xf0388+c*4u;
        write(m,o,0x22e9a7,free_at,4,read(m,free_at)-1u);return head;
    }
    const auto stride=read(m,a+0x68+c*4u);
    if(bytes>0x200) {
        const auto pages=read(m,a+0xf0340);
        if(signed_word(pages)>=0x76)return 0;
        const auto count=read(m,a+0x2d0);
        write(m,o,0x22eaaa,a+0xf0340,4,pages+1u);
        write(m,o,0x22eab0,a+0x2d0,4,count+1u);
        const auto block=a+0x4340+(pages<<13),saved_count=read(m,count_at);
        auto next=block+stride;
        if(signed_word(stride)<0x2000) {
            auto total=stride;
            do {
                const auto created_at=a+0xf0344+c*4u,free_at=a+0xf0388+c*4u;
                write(m,o,0x22ead1,created_at,4,read(m,created_at)+1u);
                write(m,o,0x22ead8,free_at,4,read(m,free_at)+1u);
                write(m,o,0x22eadf,count_at,4,read(m,count_at)-1u);
                write(m,o,0x22eaea,next,4,read(m,head_at));
                write(m,o,0x22eaec,head_at,4,next);
                total+=stride;next+=stride;
            }while(signed_word(total)<0x2000);
        }
        write(m,o,0x22ea6d,count_at,4,saved_count);
        const auto created_at=a+0xf0344+c*4u;
        write(m,o,0x22ea7e,created_at,4,read(m,created_at)+1u);
        return block;
    }
    if(read(m,a+0x20)+stride>read(m,a+0x1c)) {
        const auto pages=read(m,a+0xf0340);
        if(signed_word(pages)>=0x76)return 0;
        const auto page=a+0x4340+(pages<<13);
        write(m,o,0x22e9ed,a+0x18,4,page);write(m,o,0x22e9f0,a+0x20,4,page);
        write(m,o,0x22e9f8,a+0x1c,4,page+0x1fc0);
        write(m,o,0x22ea03,a+0xf0340,4,pages+1u);
        write(m,o,0x22ea09,a+0x2d0,4,read(m,a+0x2d0)+1u);
    }
    const auto block=read(m,a+0x20);auto next=block+stride;
    write(m,o,0x22ea1b,a+0x20,4,next);
    const auto saved_count=read(m,count_at);auto total=stride;
    if(signed_word(stride)<0x100) {
        do {
            if(next+stride>=read(m,a+0x1c))break;
            const auto created_at=a+0xf0344+c*4u,free_at=a+0xf0388+c*4u;
            write(m,o,0x22ea38,created_at,4,read(m,created_at)+1u);
            write(m,o,0x22ea3f,free_at,4,read(m,free_at)+1u);
            write(m,o,0x22ea46,count_at,4,read(m,count_at)-1u);
            write(m,o,0x22ea51,next,4,read(m,head_at));write(m,o,0x22ea53,head_at,4,next);
            next=read(m,a+0x20)+stride;total+=stride;
            write(m,o,0x22ea64,a+0x20,4,next);
        }while(signed_word(total)<0x100);
    }
    write(m,o,0x22ea6d,count_at,4,saved_count);
    const auto created_at=a+0xf0344+c*4u;
    write(m,o,0x22ea7e,created_at,4,read(m,created_at)+1u);return block;
}
void source_free_physics_body_small_block_1_1(Memory&m,std::uint32_t a,
    std::uint32_t block,std::uint32_t bytes,const Observer&o) {
    if(signed_word(bytes)<0 || signed_word(bytes)>0x2000)
        unavailable("Required original OS allocator free owner unavailable");
    const auto c=bytes<=0x200?signed_byte(read(m,a+0xac+bytes,1)):
      signed_word(read(m,a+0x2b0+((bytes-1)>>10)*4));
    const auto free=a+0xf0388+c*4u,count=a+0x2e8+c*4u;
    write(m,o,0x22eb58,free,4,read(m,free)+1u);
    const auto head=read(m,a+0x24+c*4u);
    write(m,o,0x22eb63,count,4,read(m,count)-1u);
    write(m,o,0x22eb6e,block,4,head);write(m,o,0x22eb70,a+0x24+c*4u,4,block);
}
void source_grow_physics_body_bank_1_1(Memory&m,std::uint32_t d,
    std::uint32_t cap,std::uint32_t stride,const Observer&o){grow(m,d,cap,stride,false,o);}
void source_double_physics_body_bank_1_1(Memory&m,std::uint32_t d,
    std::uint32_t stride,const Observer&o) {
    const auto n=read(m,d+4);grow(m,d,n?n*2u:1u,stride,true,o);
}
void source_copy_physics_body_bank_bytes_1_1(Memory&m,std::uint32_t dst,
    std::uint32_t src,std::uint32_t bytes,const Observer&o){copy(m,dst,src,bytes,o);}
void source_import_physics_entity_properties_1_1(Memory&m,std::uint32_t entity,
    std::uint32_t info,const Observer&o) {
    const auto word=read(m,info);
    if(read(m,entity+0x28)!=word)write(m,o,0x318a30,entity+0x28,4,word);
    const auto count=read(m,info+0x10),capacity=read(m,entity+0x38)&0x7fffffffu;
    if(signed_word(capacity)<signed_word(count)) {
        const auto doubled=capacity*2u;
        source_grow_physics_body_bank_1_1(m,entity+0x30,
            signed_word(count)>=signed_word(doubled)?count:doubled,8,o);
    }
    write(m,o,0x318a59,entity+0x34,4,count);
    for(std::uint32_t n=0;signed_word(n)<signed_word(read(m,info+0x10));++n) {
        const auto source=read(m,info+0xc)+n*8u;
        const auto key=read(m,source),value=read(m,source+4),dst=read(m,entity+0x30)+n*8u;
        write(m,o,0x318a73,dst,4,key);write(m,o,0x318a76,dst+4,4,value);
    }
}
std::uint32_t source_construct_physics_entity_base_1_1(Memory&m,std::uint32_t p,
    std::uint32_t info,const Observer&o) {
    write(m,o,0x318b0e,p+6,2,1);write(m,o,0x318b18,p,4,0x419dd0);
    write(m,o,0x318b20,p+8,4,0);
    const auto flag=signed_byte(read(m,info+8,1)),shape=read(m,info+4);
    write(m,o,0x318b2a,p+0xc,4,shape);write(m,o,0x318b2d,p+0x14,4,0);
    write(m,o,0x318b30,p+0x18,4,0);write(m,o,0x318b33,p+0x10,4,0xffffffff);
    write(m,o,0x318b3a,p+0x1c,4,0);write(m,o,0x318b3d,p+0x24,4,flag);
    write(m,o,0x318b40,p+0x20,4,0);write(m,o,0x318b43,p+0x28,4,0);
    write(m,o,0x318b46,p+0x2c,4,0);write(m,o,0x318b4d,p+0x30,4,0);
    write(m,o,0x318b50,p+0x34,4,0);write(m,o,0x318b53,p+0x38,4,0x80000000);
    write(m,o,0x318b64,p+0x2c,4,p);
    if(shape)write(m,o,0x318b69,shape+6,2,read(m,shape+6,2)+1u);
    source_import_physics_entity_properties_1_1(m,p,info,o);return p;
}
std::uint32_t source_construct_physics_entity_1_1(Memory&m,std::uint32_t p,
    std::uint32_t info,const Observer&o) {
    source_construct_physics_entity_base_1_1(m,p,info,o);
    write(m,o,0x31241e,p,4,0x419ea4);write(m,o,0x312424,p+0x4c,4,0x3f000000);
    write(m,o,0x31242b,p+0x50,4,0x3ecccccd);write(m,o,0x312432,p+0x68,4,0x80000001);
    write(m,o,0x31243c,p+0x60,4,p+0x6c);write(m,o,0x312441,p+0x64,4,0);
    write(m,o,0x312444,p+0x70,4,0);write(m,o,0x312447,p+0x74,4,0);
    for(const auto [site,offset,value]:std::array<std::array<std::uint32_t,3>,10>{{
        {0x31244f,0x78,0x80000000},{0x312452,0x7c,0},{0x312455,0x80,0},
        {0x31245b,0x84,0x80000000},{0x312461,0x88,0},{0x312467,0x8c,0},
        {0x31246d,0x90,0x80000000},{0x312473,0x94,0},{0x312479,0x98,0},
        {0x31247f,0x9c,0x80000000}}})write(m,o,site,p+offset,4,value);
    write(m,o,0x312485,p+0x2c,4,p);const auto flag=read(m,info+0x18,1);
    write(m,o,0x31248b,p+0x44,4,0);write(m,o,0x31248e,p+0x40,1,0);
    write(m,o,0x312491,p+0x48,1,flag);const auto material=read(m,info+0x1a,2);
    write(m,o,0x312498,p+0x54,4,0);write(m,o,0x31249c,p+0x5c,2,material);
    write(m,o,0x3124a0,p+0x58,4,0xffffffff);return p;
}
void source_add_physics_entity_property_1_1(Memory&m,std::uint32_t p,
    std::uint32_t key,std::uint32_t value,const Observer&o) {
    const auto count=read(m,p+0x34),bank=read(m,p+0x30);
    for(std::uint32_t n=0;signed_word(n)<signed_word(count);++n)
        if(read(m,bank+n*8u)==key)return; // Existing key is not overwritten.
    if(read(m,p+0x34)==(read(m,p+0x38)&0x7fffffffu))
        source_double_physics_body_bank_1_1(m,p+0x30,8,o);
    const auto n=read(m,p+0x34),dst=read(m,p+0x30)+n*8u;
    write(m,o,0x318ad5,dst,4,key);write(m,o,0x318ad8,dst+4,4,value);
    write(m,o,0x318adc,p+0x34,4,read(m,p+0x34)+1u);
}
std::uint32_t source_decode_physics_current_bounds_1_1(Memory&m,
    const SourcePhysicsQuantizedBounds &in,const Observer&o) {
    FpScope fp(in.x87_control_word,in.mxcsr);
    const auto one=std::bit_cast<float>(read(m,0x465030));
    const auto b=in.backend,p=in.packed_bounds;
    const auto table=std::array{read(m,b+0x4c),read(m,b+0x58),read(m,b+0x64)};
    const auto offsets=std::array<std::array<unsigned,2>,3>{{{8,10},{0,4},{2,6}}};
    std::array<float,4> lo{},hi{},origin{};
    for(unsigned n=0;n<4;++n)origin[n]=std::bit_cast<float>(read(m,b+0x10+n*4));
    for(unsigned n=0;n<3;++n) {
        const auto min=std::int32_t(read(m,table[n]+read(m,p+offsets[n][0],2)*4u,2));
        const auto max=std::int32_t(read(m,table[n]+read(m,p+offsets[n][1],2)*4u,2));
        const auto a=bounds_axis(one,std::bit_cast<float>(read(m,b+0x30+n*4)),min,max,n==2,in.x87_control_word);
        lo[n]=a[0];hi[n]=a[1];
    }
    const auto sub=[&](const auto &v) {
        std::array<float,4> out{};
        _mm_storeu_ps(out.data(),_mm_sub_ps(_mm_loadu_ps(v.data()),_mm_loadu_ps(origin.data())));
        return std::bit_cast<std::array<std::uint32_t,4>>(out);
    };
    const auto lower=sub(lo),upper=sub(hi);
    for(unsigned n=0;n<2;++n)write(m,o,0x2e6661,in.output+n*8,8,std::uint64_t(lower[n*2])|(std::uint64_t(lower[n*2+1])<<32));
    for(unsigned n=0;n<2;++n)write(m,o,0x2e668f,in.output+0x10+n*8,8,std::uint64_t(upper[n*2])|(std::uint64_t(upper[n*2+1])<<32));
    return in.output;
}
std::uint32_t source_physics_current_bounds_1_1(Memory&m,std::uint32_t backend,
    std::uint32_t handle,std::uint32_t out,std::uint16_t cw,std::uint32_t mxcsr,const Observer&o) {
    return source_decode_physics_current_bounds_1_1(m,{backend,read(m,backend+0x40)+read(m,handle)*16u,out,cw,mxcsr},o);
}
void source_deliver_physics_contact_manager_1_1(Memory&m,
    const SourcePhysicsContactManagerDelivery &in,const Observer&o,
    const SourcePhysicsContactDispatchObserver &dispatch,const SourcePhysicsContactMathObserver &math,
    const SourcePhysicsContactRefreshObserver &refresh) {
    const auto s=in.context,component=in.stack_arguments[0];std::uint32_t row_offset=0;
    for(std::uint32_t n=0;signed_word(n)<signed_word(read(m,read(m,read(m,s+0xc))+4));++n,row_offset+=0x30) {
        const auto manager_row=read(m,read(m,read(m,s+0xc)))+row_offset;
        for(std::uint32_t j=0;signed_word(j)<signed_word(read(m,component+0x8c));++j) {
            const auto row=read(m,component+0x88)+j*72u;
            if(read(m,row,2)!=read(m,manager_row+0x20,2) || read(m,row+0x14)!=in.stack_arguments[2] ||
               signed_byte(read(m,row+0x45,1))!=in.stack_arguments[3] || signed_byte(read(m,row+0x46,1))!=in.stack_arguments[4])continue;
            const auto flags=read(m,s+4);
            if(read(m,flags+0x18)!=1 || !read(m,flags+0x20))continue;
            const auto flip=read(m,flags+0x20)!=read(m,s+0x10);
            // Preserve the actual source read even though its result is a dead
            // stack-local store in this measured nonexception call.
            (void)read(m,flip?flags:read(m,s+8));
            SourcePhysicsContactRefresh r{read(m,s),row,
                {0,(in.flip_scratch_word&0xffffff00u)|std::uint32_t(flip),in.stack_arguments[1],in.stack_arguments[3]},
                in.x87_control_word,in.mxcsr};
            if(refresh)refresh(r); // Literal EAX/ECX/four stack arguments, not a success service.
            source_refresh_physics_contact_1_1(m,r,o,dispatch,math);
        }
    }
}
} // namespace halo2
