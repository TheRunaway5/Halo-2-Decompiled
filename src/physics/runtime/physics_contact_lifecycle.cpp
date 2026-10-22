#include "halo2/physics_contact_lifecycle.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using Memory = SourcePhysicsContactMemory;
using Observer = SourcePhysicsContactRecordObserver;
std::uint32_t read(const Memory &m, std::uint32_t a, unsigned size = 4) {
    std::array<std::byte, 4> b{}; m.read(a, std::span(b).first(size));
    std::uint32_t v = 0;
    for (unsigned n = 0; n < size; ++n) v |= std::to_integer<std::uint32_t>(b[n]) << (n*8);
    return v;
}
std::int32_t signed_word(std::uint32_t v) { return std::bit_cast<std::int32_t>(v); }
std::uint32_t signed_byte(std::uint32_t v) {
    return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v))));
}
void write(Memory &m, const Observer &o, std::uint32_t site,
           std::uint32_t a, unsigned size, std::uint32_t v) {
    SourcePhysicsContactRecordStore s{site,a,{},{}};
    s.before.resize(size); s.written.resize(size); m.read(a,s.before);
    for (unsigned n=0;n<size;++n) s.written[n]=std::byte((v>>(n*8))&255u);
    m.write(a,s.written); if(o)o(s);
}
struct Property { bool found; std::uint32_t word; };
Property property(const Memory &m, std::uint32_t body, std::uint32_t key) {
    const auto count=signed_word(read(m,body+0x34));
    if(count<=0)return {false,0};
    const auto pairs=read(m,body+0x30);
    for(std::int32_t n=0;n<count;++n) {
        const auto p=pairs+std::uint32_t(n)*8u;
        if(read(m,p)==key)return {true,read(m,p+4)};
    }
    return {false,0};
}
[[noreturn]] void unavailable(const char *owner) { throw std::runtime_error(owner); }
void method(const Memory &m,std::uint32_t p,unsigned slot,std::uint32_t target) {
    if(read(m,read(m,p)+slot)!=target)unavailable("Unmeasured original virtual body unavailable");
}
float scalar(std::uint32_t v) { return std::bit_cast<float>(v); }
std::uint32_t word(float v) { return std::bit_cast<std::uint32_t>(v); }
float divide(float a,float b) { return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(a),_mm_set_ss(b))); }
float multiply(float a,float b) { return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b))); }
float subtract(float a,float b) { return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b))); }
struct FpScope {
    unsigned saved;
    explicit FpScope(std::uint32_t mxcsr):saved(_mm_getcsr()) {
        if((mxcsr&0x1f80u)!=0x1f80u)unavailable("Unmasked original SSE exception ownership unavailable");
        _mm_setcsr(mxcsr);
    }
    ~FpScope(){_mm_setcsr(saved);}
};
std::int32_t fistp(float f,std::uint16_t cw) {
    if((cw&0x3fu)!=0x3fu)unavailable("Unmasked original x87 exception ownership unavailable");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::fenv_t environment;
    if(std::fegetenv(&environment)!=0)unavailable("Host floating-point environment capture unavailable");
    std::uint16_t saved;std::int32_t result;
    asm volatile("fnstcw %0" : "=m"(saved));
    asm volatile("fldcw %1\n\tflds %2\n\tfistpl %0\n\tfldcw %3"
        : "=m"(result) : "m"(cw),"m"(f),"m"(saved) : "memory");
    if(std::fesetenv(&environment)!=0)unavailable("Host floating-point environment restore unavailable");
    return result;
#else
    (void)f;unavailable("Original x87 FISTP backend unavailable");
#endif
}
}
std::uint32_t source_physics_contact_fast_allocate_1_1(Memory &m,
    std::uint32_t allocator,std::uint32_t bytes,const Observer &o) {
    if(signed_word(bytes)<0 || signed_word(bytes)>0x200)
        unavailable("Unmeasured original allocator request-size branch unavailable");
    const auto cls=signed_byte(read(m,allocator+0xac+bytes,1));
    const auto head_at=allocator+0x24+cls*4u;
    const auto head=read(m,head_at),count_at=allocator+0x2e8+cls*4u;
    write(m,o,0x22e98f,count_at,4,read(m,count_at)+1u);
    if(head==0)unavailable("Original allocator empty-list/growth owner unavailable");
    write(m,o,0x22e99a,head_at,4,read(m,head));
    const auto free_at=allocator+0xf0388+cls*4u;
    write(m,o,0x22e9a7,free_at,4,read(m,free_at)-1u);
    return head;
}
void source_grow_physics_contact_bank_1_1(Memory &m,std::uint32_t descriptor,
    std::uint32_t capacity,std::uint32_t stride,const Observer &o) {
    const auto adapter=read(m,0x487638);
    method(m,adapter,0x10,0x148a10);
    if(read(m,0x480d98)!=2)unavailable("Original allocator adapter mode unavailable");
    const auto backend=read(m,0x480da0);method(m,backend,0x10,0x22e950);
    const auto p=source_physics_contact_fast_allocate_1_1(m,backend,capacity*stride,o);
    if(read(m,descriptor+4)*stride!=0)
        unavailable("Original nonempty bank copy owner unavailable");
    if(signed_word(read(m,descriptor+8))>=0)
        unavailable("Original owned bank free dispatch unavailable");
    write(m,o,0x2df768,descriptor,4,p);
    write(m,o,0x2df76b,descriptor+8,4,capacity);
}
std::uint32_t source_append_physics_contact_1_1(Memory &m,
    const SourcePhysicsContactAppend &input,const Observer &o) {
    const auto &a=input.stack_arguments;
    const auto component=a[0],count=read(m,component+0x8c);
    if(signed_word(count)>=28)unavailable("Original full contact-bank eviction owner unavailable");
    const auto descriptor=component+0x88,required=count+1u;
    const auto capacity=read(m,descriptor+8)&0x7fffffffu;
    if(signed_word(capacity)<signed_word(required)) {
        const auto doubled=capacity*2u;
        source_grow_physics_contact_bank_1_1(m,descriptor,
            signed_word(required)>=signed_word(doubled)?required:doubled,72,o);
    }
    const auto row=read(m,descriptor)+count*72u;
    write(m,o,0x1d3f29,descriptor+4,4,required);
    if(row!=0) {
        SourcePhysicsContactConstruction c{row,a[1],a[4],
            {a[3],a[7],a[8],a[5],a[2],a[9],a[10],a[11],a[12]}};
        source_construct_physics_contact_record(m,c,SourcePhysicsContactImage::MapPack11,o);
    }
    if(read(m,row+0x44,1)&0x20u)
        unavailable("Original positive contact component1DC1A0 owner unavailable");
    return count;
}
std::uint32_t source_physics_contact_point_1_1(const Memory &m,
    std::uint32_t manager,std::uint32_t index) {
    const auto slot=read(m,read(m,manager+0x30)+(index&0xffffu),1);
    return read(m,manager+0x44)+slot*32u;
}
std::uint32_t source_physics_contact_scalar_1_1(const Memory &m,
    std::uint32_t manager,std::uint32_t index) {
    const auto slot=read(m,read(m,manager+0x30)+(index&0xffffu),1);
    return read(m,manager+0xa0)+slot*28u;
}
void source_refresh_physics_contact_1_1(Memory &m,const SourcePhysicsContactRefresh &in,
    const Observer &o,const SourcePhysicsContactDispatchObserver &dispatch,
    const SourcePhysicsContactMathObserver &math) {
    const auto row=in.row,index=read(m,row,2);
    method(m,in.manager,0x18,0x31f4b0);
    if(dispatch)dispatch({0x1d2f2a,0x31f4b0,in.manager,{index}});
    const auto point=source_physics_contact_point_1_1(m,in.manager,index);
    method(m,in.manager,0x14,0x31f4d0);
    if(dispatch)dispatch({0x1d2f39,0x31f4d0,in.manager,{index}});
    const auto coeff=source_physics_contact_scalar_1_1(m,in.manager,index);
    const auto pool=read(m,0x526248);
    const auto component=read(m,pool+0x44)+(in.stack_arguments[2]&0xffffu)*0xa0u;
    write(m,o,0x1d2f5d,row+0x34,4,read(m,point+0xc));
    const auto x=read(m,point),z=read(m,point+8),y=read(m,point+4);
    write(m,o,0x1d2f6c,row+0x1c,4,x);
    write(m,o,0x1d2f72,row+0x20,4,y);
    write(m,o,0x1d2f77,row+0x24,4,z);
    const auto nx=read(m,point+0x10),nz=read(m,point+0x18),ny=read(m,point+0x14);
    write(m,o,0x1d2f89,row+0x28,4,nx);
    write(m,o,0x1d2f8c,row+0x2c,4,ny);
    write(m,o,0x1d2f91,row+0x30,4,nz);
    const auto body=read(m,read(m,component+0x70)+in.stack_arguments[3]*0x60u+0x40);
    const auto motion=read(m,body+0x3c);method(m,motion,0x18,0x9d900);
    if(dispatch)dispatch({0x1d2fb0,0x9d900,motion,{}});
    // The actual complete9D900 leaf returns4; its return is ignored here.
    FpScope fp(in.mxcsr);
    const auto motion_scalar=scalar(read(m,motion+0x2c));
    const auto one=scalar(read(m,0x465030));
    float denominator=one;
    if(motion_scalar!=0 || std::isnan(motion_scalar)) {
        const auto inverse=divide(one,motion_scalar);
        if(!(one>inverse))denominator=divide(one,motion_scalar);
    }
    if(math)math({0x1d2ff8,{word(motion_scalar),word(denominator)}});
    auto numerator=scalar(read(m,coeff));
    if(!(numerator>=0))numerator=subtract(0,numerator);
    if(math)math({0x1d300e,{word(numerator),word(denominator)}});
    if((word(numerator)&0x7f800000u)==0x7f800000u)numerator=0;
    if(math)math({0x1d3029,{word(numerator),word(denominator)}});
    const auto force_word=word(divide(numerator,denominator));
    write(m,o,0x1d3032,row+0x38,4,force_word);
    if(math)math({0x1d3037,{force_word,word(denominator)}});
    if(in.stack_arguments[1]&255u)unavailable("Original flipped-normal contact branch unavailable");
    const auto headers=read(m,read(m,0x4e78d0)+0x44);
    const auto object_index=read(m,component+8)&0xffffu;
    const auto type=read(m,headers+object_index*12u+3,1);
    if((1u<<(type&31u))&2u)
        unavailable("Original character contact coefficient producer unavailable");
    write(m,o,0x1d332b,coeff+8,4,read(m,row+0x3c));
    write(m,o,0x1d3333,coeff+0xc,4,read(m,row+0x40));
    const auto force=scalar(read(m,row+0x38));
    bool wake=force>scalar(read(m,0x4651a0)) || (read(m,row+0x44,1)&8u);
    if(!wake) {
        const auto clock=read(m,0x518444);
        const auto rate=std::bit_cast<std::int16_t>(std::uint16_t(read(m,clock+2,2)));
        const auto delay_float=multiply(float(rate),scalar(read(m,0x465198)));
        if(math)math({0x1d30a8,{word(delay_float)}});
        const auto delay=fistp(delay_float,in.x87_control_word);
        if(math)math({0x1d30b6,{std::bit_cast<std::uint32_t>(delay)}});
        wake=signed_word(read(m,clock+8)-read(m,component+0x10))<delay;
    }
    if(wake)unavailable("Original positive object wakeup store branch unavailable");
    if(!(in.stack_arguments[0]&255u) && (read(m,row+0x44,1)&0x20u))
        unavailable("Original contact impulse producer unavailable");
}
void source_remove_physics_contact_row_1_1(Memory &m,std::uint32_t component,
    std::uint32_t index,const Observer &o) {
    const auto bank=read(m,component+0x88),row=bank+index*72u;
    const auto positive=read(m,row+0x44,1)&0x20u;
    const auto impact=read(m,row+8);
    if(impact!=0xffffffffu) {
        const auto pool=read(m,0x52648c);
        const auto record=read(m,pool+0x44)+(impact&0xffffu)*0xa0u;
        const auto refs=read(m,record+8,2);
        if(refs==1 && read(m,record+0x10,1)==1)
            unavailable("Original last-reference impact effect dispatch unavailable");
        write(m,o,0x229901,record+8,2,refs-1u);
        write(m,o,0x1d410c,row+8,4,0xffffffffu);
    }
    const auto count=read(m,component+0x8c)-1u;
    write(m,o,0x1d4125,component+0x8c,4,count);
    // REP MOVSD is forward and retains all18 stores, including self-copy.
    for(unsigned n=0;n<18;++n)
        write(m,o,0x1d4135,row+n*4u,4,read(m,bank+count*72u+n*4u));
    if(positive)unavailable("Original positive removed component1DC1A0 owner unavailable");
}
void source_remove_physics_contact_1_1(Memory &m,std::uint32_t other,
    std::uint32_t own,std::uint32_t component,std::uint32_t packet,const Observer &o) {
    const auto component_key=property(m,other,0x2001).word;
    const auto own_slot=property(m,own,0x2002).word;
    const auto other_slot=property(m,other,0x2002).word;
    const auto count=signed_word(read(m,component+0x8c));
    if(count<=0)return;
    const auto contact_index=read(m,packet,2),bank=read(m,component+0x88);
    for(std::int32_t n=0;n<count;++n) {
        const auto row=bank+std::uint32_t(n)*72u;
        if(read(m,row,2)==contact_index && read(m,row+0x14)==component_key &&
           signed_byte(read(m,row+0x45,1))==own_slot && signed_byte(read(m,row+0x46,1))==other_slot) {
            source_remove_physics_contact_row_1_1(m,component,std::uint32_t(n),o);return;
        }
    }
}
} // namespace halo2
