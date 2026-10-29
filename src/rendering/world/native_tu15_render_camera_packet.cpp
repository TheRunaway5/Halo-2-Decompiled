#include "halo2/native_tu15_render_camera_packet.hpp"
#include <array>
#include <bit>
#include <limits>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
NativeTu15RenderCameraDivideError::NativeTu15RenderCameraDivideError(std::int32_t dividend,std::int32_t divisor):
    std::runtime_error("Native render camera reached the original signed divide trap"),dividend_(dividend),divisor_(divisor){}
NativeTu15RenderCameraDefaultRequired::NativeTu15RenderCameraDefaultRequired():
    std::runtime_error("Native2F7E0 null-observer default projection remains unqualified"){}
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> raw{};memory.read(address,std::span(raw).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<std::uint32_t>(raw[n])<<(8u*n);return value;
}
std::int32_t signed_word(std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width=4) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);memory.read(address,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(8u*n));
    memory.write(address,event.written);if(observe)observe(event);
}
class SimdEnvironment {
    NativeTu15PhysicsFrameFp &fp_;unsigned host_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),host_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))throw std::invalid_argument("Native render camera requires masked SIMD exceptions");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(host_);}
};
__m128 scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t scaled(std::uint32_t value,std::uint32_t factor) {
    auto left=scalar(value);const auto right=scalar(factor);
    asm volatile("mulss %1,%0":"+x"(left):"x"(right):"memory");
    return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(left));
}
}
void native_tu15_render_camera_packet(SourcePhysicsContactMemory &memory,std::uint32_t observer,
    std::uint32_t destination,std::uint32_t spans,const NativeTu15RenderCameraGridArgument &argument,
    NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
    const auto grid=argument.grid();
    if(!observer)throw NativeTu15RenderCameraDefaultRequired{};
    SimdEnvironment environment(fp);
    constexpr std::array<std::uint32_t,3> position_sites{0x2f7f8,0x2f7fd,0x2f803};
    for(unsigned n=0;n<3;++n){const auto value=read(memory,observer+n*4u);publish(memory,observe,position_sites[n],destination+n*4u,value);}
    auto factor=read(memory,0x45c55cu);
    constexpr std::array<std::uint32_t,3> forward_sites{0x2f816,0x2f81b,0x2f821},up_sites{0x2f82c,0x2f831,0x2f837};
    for(unsigned n=0;n<3;++n){const auto value=read(memory,observer+0x20u+n*4u);publish(memory,observe,forward_sites[n],destination+0xcu+n*4u,value);}
    for(unsigned n=0;n<3;++n){const auto value=read(memory,observer+0x2cu+n*4u);publish(memory,observe,up_sites[n],destination+0x18u+n*4u,value);}
    const auto vertical=read(memory,observer+0x4cu);publish(memory,observe,0x2f83d,destination+0x28u,vertical);
    const auto scale=read(memory,observer+0x50u);publish(memory,observe,0x2f843,destination+0x2cu,scale);
    if(spans && grid) {
        const auto span_x=signed_word(read(memory,spans,2)),grid_y=signed_word(read(memory,grid+2u,2));
        auto numerator=std::uint32_t(grid_y)*std::uint32_t(span_x);
        const auto span_y=signed_word(read(memory,spans+2u,2));
        numerator*=100u;
        const auto grid_x=signed_word(read(memory,grid,2));
        const auto denominator=std::bit_cast<std::int32_t>(std::uint32_t(span_y)*std::uint32_t(grid_x));
        const auto dividend=std::bit_cast<std::int32_t>(numerator);
        if(!denominator || (dividend==std::numeric_limits<std::int32_t>::min() && denominator==-1))
            throw NativeTu15RenderCameraDivideError(dividend,denominator);
        const auto ratio=dividend/denominator;
        if(ratio==200)factor=read(memory,0x46a0ccu);
        else if(ratio==50)factor=read(memory,0x456910u);
    }
    const auto angle=read(memory,destination+0x28u);
    publish(memory,observe,0x2f88e,destination+0x28u,scaled(angle,factor));
    const auto extent=read(memory,destination+0x2cu);
    publish(memory,observe,0x2f89d,destination+0x2cu,scaled(extent,factor));
    publish(memory,observe,0x2f91c,destination+0x24u,0,1);
    const auto near_distance=read(memory,0x4922f4u);publish(memory,observe,0x2f928,destination+0x40u,near_distance);
    const auto far_distance=read(memory,0x4922f8u);publish(memory,observe,0x2f935,destination+0x44u,far_distance);
    publish(memory,observe,0x2f93a,destination+0x58u,0,1);
}
}
