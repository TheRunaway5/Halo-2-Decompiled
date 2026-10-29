#include "halo2/native_campaign_object_world_point_tu15.hpp"
#include <array>
#include <bit>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t at,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(at,std::span(bytes).first(width));
    std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t at,std::uint32_t value) {
    SourcePhysicsContactRecordStore event{site,at,{},std::vector<std::byte>(4)};
    if(observe){event.before.resize(4);memory.read(at,event.before);}
    for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(at,event.written);if(observe)observe(event);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class SimdEnvironment {
    NativeTu15PhysicsFrameFp &fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),previous_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native object point requires its SIMD trap owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
__m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));}
std::uint32_t word(__m128 value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
__m128 multiply(__m128 left,__m128 right){asm volatile("mulss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
bool ordered_equal(__m128 left,__m128 right) {
    unsigned char equal{},ordered{};
    asm volatile("ucomiss %3,%2\n\tsete %0\n\tsetnp %1":"=qm"(equal),"=qm"(ordered):"x"(left),"x"(right):"cc","memory");
    return equal && ordered;
}
#endif
template<class Sink> void object_point(const SourcePhysicsContactMemory &memory,
    std::uint32_t object,NativeTu15PhysicsFrameFp &fp,const Sink &sink) {
    const auto objects=read(memory,0x4ecbd0u);
    const auto rows=read(memory,objects+0x44u);
    const auto record=read(memory,rows+(object&0xffffu)*12u+8u);
    const auto parent=read(memory,record+0x14u);
    if(parent==0xffffffffu) {
        sink(0,read(memory,record+0x64u),0xbbe24u);
        sink(1,read(memory,record+0x68u),0xbbe29u);
        sink(2,read(memory,record+0x6cu),0xbbe2fu);
        return;
    }
    // The child's three coordinates precede all parent/node/scale accesses.
    // Parent resolution uses the captured header rows and performs one level.
    const auto local_x=read(memory,record+0x64u);
    const auto local_y=read(memory,record+0x68u);
    const auto local_z=read(memory,record+0x6cu);
    const auto node=std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(read(memory,record+0x18u,1))));
    const auto parent_record=read(memory,rows+(parent&0xffffu)*12u+8u);
    const auto offset=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(memory,parent_record+0x116u,2))));
    const auto matrix=parent_record+std::uint32_t(node)*0x34u+std::uint32_t(offset);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto scale_bits=read(memory,matrix);
    const auto one_bits=read(memory,0x46a0d0u);
    SimdEnvironment environment(fp);
    auto x=scalar(local_x),y=scalar(local_y),z=scalar(local_z),scale=scalar(scale_bits);
    if(!ordered_equal(scale,scalar(one_bits))) {
        x=multiply(scale,x);
        y=multiply(scalar(read(memory,matrix)),y);
        z=multiply(scalar(read(memory,matrix)),z);
    }
    auto z_term=scalar(read(memory,matrix+0x1cu));
    auto y_term=scalar(read(memory,matrix+0x10u));
    z_term=multiply(z_term,z);y_term=multiply(y_term,y);
    auto sum=add(z_term,y_term);
    sum=add(sum,multiply(scalar(read(memory,matrix+4u)),x));
    sum=add(sum,scalar(read(memory,matrix+0x28u)));sink(0,word(sum),0xbbebcu);
    z_term=scalar(read(memory,matrix+0x20u));y_term=scalar(read(memory,matrix+0x14u));
    z_term=multiply(z_term,z);y_term=multiply(y_term,y);sum=add(z_term,y_term);
    sum=add(sum,multiply(scalar(read(memory,matrix+8u)),x));
    sum=add(sum,scalar(read(memory,matrix+0x2cu)));sink(1,word(sum),0xbbee8u);
    z_term=multiply(scalar(read(memory,matrix+0x24u)),z);
    y_term=multiply(scalar(read(memory,matrix+0x18u)),y);
    const auto x_basis=scalar(read(memory,matrix+0xcu));
    sum=add(z_term,y_term);sum=add(sum,multiply(x_basis,x));
    sum=add(sum,scalar(read(memory,matrix+0x30u)));sink(2,word(sum),0xbbf15u);
#else
    (void)local_x;(void)local_y;(void)local_z;(void)matrix;(void)fp;
    throw std::runtime_error("Exact native object point arithmetic backend unavailable");
#endif
}
}
void native_campaign_object_world_point_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t object,std::uint32_t destination,NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observe) {
    object_point(memory,object,fp,[&](unsigned axis,std::uint32_t bits,std::uint32_t site){publish(memory,observe,site,destination+axis*4u,bits);});
}
std::array<std::uint32_t,3> native_campaign_object_world_point_value_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t object,NativeTu15PhysicsFrameFp &fp) {
    std::array<std::uint32_t,3> point{};
    object_point(memory,object,fp,[&](unsigned axis,std::uint32_t bits,std::uint32_t){point[axis]=bits;});
    return point;
}
}
