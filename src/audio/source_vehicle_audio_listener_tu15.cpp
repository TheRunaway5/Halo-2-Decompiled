#include "halo2/source_vehicle_audio_listener_tu15.hpp"
#include "halo2/native_campaign_audio_listeners.hpp"
namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using namespace native_campaign_listeners_detail;
using Vector=std::array<std::uint32_t,3>;
std::uint32_t root(std::uint32_t value){
    std::uint32_t result;
    asm volatile("flds %1; fsqrt; fstps %0":"=m"(result):"m"(value):"memory","st");return result;
}
std::array<std::uint32_t,4> rotation(const Vector& from,const Vector& to){
    const auto x=scalar(from[0]),y=scalar(from[1]),z=scalar(from[2]);
    const auto a=scalar(to[0]),b=scalar(to[1]),c=scalar(to[2]);
    const auto zero=_mm_setzero_ps(),one=scalar(0x3f800000u),two=scalar(0x40000000u);
    auto sum=add(mul(y,b),mul(x,a));sum=add(sum,mul(c,z));sum=mul(add(sum,one),two);
    const auto length=root(above(sum,zero)?bits(sum):0u);
    if(std::bit_cast<float>(length)<std::bit_cast<float>(0x3a83126fu)){
        const auto ax=scalar(from[0]&0x7fffffffu),ay=scalar(from[1]&0x7fffffffu),az=scalar(from[2]&0x7fffffffu);
        if(!above(ax,ay) && !above(ax,az))return {0u,from[2],bits(sub(zero,y)),0u};
        if(!above(ay,az))return {bits(sub(zero,z)),0u,from[0],0u};
        return {from[1],bits(sub(zero,x)),0u,0u};
    }
    const auto inverse=div(one,scalar(length));
    return {bits(mul(sub(mul(y,c),mul(z,b)),inverse)),
        bits(mul(sub(mul(a,z),mul(c,x)),inverse)),
        bits(mul(sub(mul(x,b),mul(y,a)),inverse)),bits(mul(scalar(length),scalar(0x3f000000u)))};
}
std::array<std::uint32_t,4> blend(std::array<std::uint32_t,4> q,std::uint32_t factor){
    const auto zero=_mm_setzero_ps(),one=scalar(0x3f800000u);auto weight=scalar(factor);
    auto dot=add(mul(scalar(q[2]),zero),mul(scalar(q[1]),zero));
    dot=add(dot,mul(one,scalar(q[3])));dot=add(dot,mul(zero,scalar(q[0])));
    const auto other=sub(one,weight);if(above(zero,dot))weight=sub(zero,weight);
    q={bits(add(mul(zero,other),mul(scalar(q[0]),weight))),
        bits(add(mul(scalar(q[1]),weight),mul(other,zero))),
        bits(add(mul(scalar(q[2]),weight),mul(other,zero))),
        bits(add(mul(one,other),mul(weight,scalar(q[3]))))};
    auto sum=add(mul(scalar(q[0]),scalar(q[0])),mul(scalar(q[1]),scalar(q[1])));
    sum=add(sum,mul(scalar(q[2]),scalar(q[2])));sum=add(sum,mul(scalar(q[3]),scalar(q[3])));
    if(!above(sum,zero))return {0,0,0,0x3f800000u};
    const auto squared=bits(sum);const std::uint32_t one_bits=0x3f800000u;
    //11FFD0 retains one extended reciprocal across its four m32 products.
    asm volatile("flds %4; fsqrt; fdivrs %5; fld %%st(0); fmuls %0; fstps %0; "
        "fld %%st(0); fmuls %1; fstps %1; fld %%st(0); fmuls %2; fstps %2; "
        "fmuls %3; fstps %3":"+m"(q[0]),"+m"(q[1]),"+m"(q[2]),"+m"(q[3])
        :"m"(squared),"m"(one_bits):"memory","st","st(1)");
    return q;
}
Vector rotate(const Vector& value,const std::array<std::uint32_t,4>& q,bool up){
    const auto x=scalar(value[0]),y=scalar(value[1]),z=scalar(value[2]);
    const auto a=scalar(q[0]),b=scalar(q[1]),c=scalar(q[2]),w=scalar(q[3]),two=scalar(0x40000000u);
    const auto scale=sub(mul(mul(w,w),two),scalar(0x3f800000u));const auto twice_w=mul(w,two);
    auto dot=add(mul(y,b),mul(z,c));dot=mul(add(dot,mul(a,x)),two);
    const auto rx=add(add(mul(sub(mul(z,b),mul(y,c)),twice_w),mul(a,dot)),up?mul(x,scale):mul(scale,x));
    const auto ry=add(add(mul(sub(mul(c,x),mul(z,a)),twice_w),mul(b,dot)),mul(y,scale));
    const auto rz=add(add(mul(sub(mul(y,a),mul(b,x)),twice_w),mul(c,dot)),mul(z,scale));
    return {bits(rx),bits(ry),bits(rz)};
}
#endif
}
SourceVehicleAudioListenerTu15 source_vehicle_audio_listener_tu15(
    const SourceVehicleAudioListenerInputTu15& input,const SoundDriverFloatingPointContext& controls){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    NativeCampaignAudioFp fp{controls.x87_control_word,0,controls.mxcsr};
    native_campaign_ducking_detail::Environment environment(fp);
    SourceVehicleAudioListenerTu15 result{};
    for(unsigned i=0;i<3;++i){const auto camera=scalar(input.position[i]);
        result.position[i]=bits(add(mul(sub(scalar(input.unit_point[i]),camera),scalar(input.factor)),camera));}
    result.rotation=blend(rotation(input.forward,input.unit_aim),input.factor);
    result.forward=rotate(input.forward,result.rotation,false);result.up=rotate(input.up,result.rotation,true);
    return result;
#else
    (void)input;(void)controls;throw std::runtime_error("Native vehicle-listener FP backend unavailable");
#endif
}
}
