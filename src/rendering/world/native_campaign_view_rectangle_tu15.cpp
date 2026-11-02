#include "halo2/native_campaign_view_rectangle_tu15.hpp"
#include <array>
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> raw{};memory.read(address,std::span(raw).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<std::uint32_t>(raw[n])<<(8u*n);return value;
}
std::int32_t coordinate(const SourcePhysicsContactMemory &memory,std::uint32_t at) {
    return std::bit_cast<std::int16_t>(std::uint16_t(read(memory,at,2)));
}
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
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native view rectangle requires masked SIMD exceptions");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(host_);}
};
__m128 scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
__m128 integer(std::int32_t value){return _mm_cvtsi32_ss(_mm_setzero_ps(),value);}
std::uint32_t bits(__m128 value){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));}
// Explicit scalar operands preserve original NaN selection and rounding.
__m128 add(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 sub(__m128 left,__m128 right){asm volatile("subss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 mul(__m128 left,__m128 right){asm volatile("mulss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 div(__m128 left,__m128 right){asm volatile("divss %1,%0":"+x"(left):"x"(right):"memory");return left;}
class RectangleStorage {
    SourcePhysicsContactMemory &memory_;const SourcePhysicsContactRecordObserver &observe_;
    std::optional<std::uint32_t> address_;
public:
    NativeCampaignViewRectangleTu15 value{};
    RectangleStorage(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
        std::optional<std::uint32_t> address):memory_(memory),observe_(observe),address_(address){}
    std::uint32_t word(unsigned offset)const{return address_?read(memory_,*address_+offset):value.bounds.at(offset/4u);}
    void store(std::uint32_t site,unsigned offset,std::uint32_t bits) {
        if(address_)publish(memory_,observe_,site,*address_+offset,bits);
        else value.bounds.at(offset/4u)=bits;
    }
};
bool calculate(SourcePhysicsContactMemory &memory,std::uint32_t camera,
    RectangleStorage &rectangle,NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observe) {
    SimdEnvironment environment(fp);
    const auto safe_left=coordinate(memory,camera+0x3au),safe_right=coordinate(memory,camera+0x3eu);
    auto vertical_scale=scalar(read(memory,0x46a11cu));
    const auto safe_top=coordinate(memory,camera+0x38u);
    const auto half=scalar(read(memory,0x46a0ccu));
    auto horizontal_center=integer(safe_right+safe_left);
    const auto safe_bottom=coordinate(memory,camera+0x3cu);
    auto vertical_center=integer(safe_top+safe_bottom);
    const auto safe_height=integer(safe_bottom-safe_top);
    const auto outer_bottom=coordinate(memory,camera+0x34u),outer_top=coordinate(memory,camera+0x30u);
    const auto outer_height=integer(outer_bottom-outer_top);
    const auto outer_left=coordinate(memory,camera+0x32u);
    vertical_scale=div(vertical_scale,safe_height);
    const auto outer_right=coordinate(memory,camera+0x36u);
    horizontal_center=mul(horizontal_center,half);
    auto horizontal_scale=div(outer_height,integer(outer_right-outer_left));
    auto left=sub(integer(outer_left),horizontal_center);
    horizontal_scale=mul(horizontal_scale,vertical_scale);
    left=mul(left,horizontal_scale);
    rectangle.store(0x2f9bcu,0,bits(left));
    auto right=sub(integer(coordinate(memory,camera+0x36u)),horizontal_center);
    right=mul(right,horizontal_scale);
    rectangle.store(0x2f9d0u,4u,bits(right));
    const auto fresh_bottom=coordinate(memory,camera+0x34u);
    vertical_center=mul(vertical_center,half);
    auto bottom=sub(vertical_center,integer(fresh_bottom));
    bottom=mul(bottom,vertical_scale);
    rectangle.store(0x2f9ecu,8u,bits(bottom));
    auto top=sub(vertical_center,integer(coordinate(memory,camera+0x30u)));
    top=mul(top,vertical_scale);
    rectangle.store(0x2fa01u,12u,bits(top));

    if(read(memory,camera+0x58u,1))publish(memory,observe,0x2fa1cu,0x4c6839u,1,1);
    else {
        const auto offset=read(memory,camera+0x68u,1);
        publish(memory,observe,0x2fa13u,0x4c6839u,0,1);
        if(offset)publish(memory,observe,0x2fa1cu,0x4c6839u,1,1);
    }
    const auto scenario=read(memory,0x4ecc20u);
    if(scenario) {
        const auto type=read(memory,scenario+0x10u,2);
        const auto modifier=read(memory,0x4c6839u,1);
        publish(memory,observe,0x2fa3du,0x4c6839u,modifier|std::uint32_t(type==2),1);
    }
    if(read(memory,camera+0x58u,1)) {
        auto center_x=scalar(read(memory,camera+0x5cu));
        auto width=scalar(read(memory,camera+0x64u));
        const auto one=scalar(read(memory,0x46a0d0u));
        auto extent_x=scalar(rectangle.word(4u));
        extent_x=sub(extent_x,scalar(rectangle.word(0)));
        auto height=scalar(read(memory,camera+0x64u));
        auto extent_y=scalar(rectangle.word(12u));
        extent_y=sub(extent_y,scalar(rectangle.word(8u)));
        width=mul(width,extent_x);
        center_x=add(center_x,one);center_x=mul(center_x,half);center_x=mul(center_x,extent_x);
        auto center_y=scalar(read(memory,camera+0x60u));
        center_x=add(center_x,scalar(rectangle.word(0)));
        center_y=add(center_y,one);height=mul(height,extent_y);
        center_y=mul(center_y,half);center_y=mul(center_y,extent_y);
        center_y=add(center_y,scalar(rectangle.word(8u)));
        width=mul(width,half);const auto minimum_x=sub(center_x,width);width=add(width,center_x);
        height=mul(height,half);const auto minimum_y=sub(center_y,height);height=add(height,center_y);
        rectangle.store(0x2fac4u,0,bits(minimum_x));
        rectangle.store(0x2fac8u,8u,bits(minimum_y));
        rectangle.store(0x2facdu,12u,bits(height));
        rectangle.store(0x2fad2u,4u,bits(width));
    } else if(read(memory,camera+0x68u,1)) {
        auto delta_x=scalar(rectangle.word(4u));
        delta_x=sub(delta_x,scalar(rectangle.word(0)));
        delta_x=mul(delta_x,scalar(read(memory,0x46a264u)));
        delta_x=mul(delta_x,scalar(read(memory,camera+0x6cu)));
        auto delta_y=scalar(rectangle.word(12u));
        delta_y=sub(delta_y,scalar(rectangle.word(8u)));
        auto minimum_x=scalar(rectangle.word(0));
        delta_y=mul(delta_y,scalar(read(memory,0x46a260u)));
        delta_y=mul(delta_y,scalar(read(memory,camera+0x70u)));
        minimum_x=add(minimum_x,delta_x);
        rectangle.store(0x2fb16u,0,bits(minimum_x));
        auto maximum_x=scalar(rectangle.word(4u));maximum_x=add(maximum_x,delta_x);
        auto minimum_y=scalar(rectangle.word(8u));minimum_y=sub(minimum_y,delta_y);
        rectangle.store(0x2fb2cu,8u,bits(minimum_y));
        auto maximum_y=scalar(rectangle.word(12u));maximum_y=sub(maximum_y,delta_y);
        rectangle.store(0x2fb3au,12u,bits(maximum_y));
        rectangle.store(0x2fb3fu,4u,bits(maximum_x));
    }
    return true;
}
 }
bool native_campaign_view_rectangle_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t camera,std::uint32_t destination,NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observe) {
    RectangleStorage rectangle(memory,observe,destination);
    return calculate(memory,camera,rectangle,fp,observe);
}
NativeCampaignViewRectangleTu15 native_campaign_view_rectangle_value_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t camera,NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
    RectangleStorage rectangle(memory,observe,std::nullopt);
    calculate(memory,camera,rectangle,fp,observe);return rectangle.value;
}
}
