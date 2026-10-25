#include "halo2/source_loop_detail_control_tu15.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>
namespace halo2 {
#if (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__))
namespace {
struct FloatingPoint {
    std::uint16_t word{};std::uint32_t simd{};
    FloatingPoint(){
        asm volatile("fnstcw %0":"=m"(word));asm volatile("stmxcsr %0":"=m"(simd));
        const std::uint16_t source=0x027f;const std::uint32_t mxcsr=0x1f80;
        asm volatile("fldcw %0"::"m"(source));asm volatile("ldmxcsr %0"::"m"(mxcsr));
    }
    ~FloatingPoint(){asm volatile("fldcw %0"::"m"(word));asm volatile("ldmxcsr %0"::"m"(simd));}
};
std::uint16_t advance(std::uint32_t& seed){seed=seed*0x19660du+0x3c6ef35fu;return std::uint16_t(seed>>16);}
long double interpolate(std::array<float,2> bounds,std::uint32_t& seed){
    const auto draw=advance(seed);const auto scale=std::bit_cast<float>(0x37800080u);
    const long double fraction=static_cast<long double>(draw)*scale;
    const long double difference=static_cast<long double>(bounds[1])-bounds[0];
    const long double product=difference*fraction;
    return product+bounds[0];
}
long double cosine(long double x){long double result;asm volatile("fcos":"=t"(result):"0"(x));return result;}
long double sine(long double x){long double result;asm volatile("fsin":"=t"(result):"0"(x));return result;}
std::int32_t truncate(float value){std::int32_t result;asm volatile("cvttss2si %1,%0":"=r"(result):"x"(value));return result;}
}
Vec3 source_loop_detail_offset_tu15(const SourceLoopDetailBoundsTu15& bounds,std::uint32_t& seed){
    FloatingPoint scope;
    const auto distance=interpolate(bounds.distance,seed);
    if(distance==0)return {};
    const auto pitch=interpolate(bounds.pitch,seed),yaw=interpolate(bounds.yaw,seed);
    const auto pitch_cosine=cosine(pitch),yaw_cosine=cosine(yaw);
    const auto x=(yaw_cosine*pitch_cosine)*distance;
    const auto y=(sine(yaw)*pitch_cosine)*distance;
    const auto z=sine(pitch)*distance;
    return {float(x),float(y),float(z)};
}
std::int32_t source_loop_detail_deadline_tu15(std::array<float,2> period,
    std::uint32_t flags,std::int32_t clock,std::uint32_t& seed,bool initial){
    FloatingPoint scope;
    if(initial){
        const float seconds=(flags&4u)?0.f:float(interpolate(period,seed));
        const float duration=seconds*1000.f,now=float(clock),deadline=duration+now;
        return truncate(deadline);
    }
    const auto seconds=interpolate(period,seed),duration=seconds*1000.f;
    const auto deadline=duration+static_cast<long double>(clock);
    if(!std::isfinite(deadline) || deadline<static_cast<long double>(INT64_MIN) ||
        deadline>=-static_cast<long double>(INT64_MIN))
        throw std::invalid_argument("Detail deadline escapes the recovered finite int64 domain");
    return std::bit_cast<std::int32_t>(std::uint32_t(std::int64_t(deadline)));
}
SourceLoopDetailSpatialTu15 source_loop_detail_spatial_tu15(
    std::span<const std::byte,68> parent,const SourceLoopDetailBoundsTu15& bounds,std::uint32_t& seed){
    const auto scalar=[&](unsigned offset){std::uint32_t bits{};
        for(unsigned n=0;n<4;++n)bits|=std::to_integer<std::uint32_t>(parent[offset+n])<<(n*8);
        return std::bit_cast<float>(bits);};
    if((std::to_integer<unsigned>(parent[3])&15u)!=1u)
        throw std::invalid_argument("Detail position requires its actual spatial parent");
    FloatingPoint scope;
    const auto offset=source_loop_detail_offset_tu15(bounds,seed);
    return {offset,{scalar(0xc)+offset.x,scalar(0x10)+offset.y,scalar(0x14)+offset.z},scalar(4),scalar(8)};
}
#else
Vec3 source_loop_detail_offset_tu15(const SourceLoopDetailBoundsTu15&,std::uint32_t&){
    throw std::runtime_error("Detail offsets require the qualified original x87 owner");
}
std::int32_t source_loop_detail_deadline_tu15(std::array<float,2>,std::uint32_t,std::int32_t,std::uint32_t&,bool){
    throw std::runtime_error("Detail deadlines require the qualified original x87 owner");
}
SourceLoopDetailSpatialTu15 source_loop_detail_spatial_tu15(
    std::span<const std::byte,68>,const SourceLoopDetailBoundsTu15&,std::uint32_t&){
    throw std::runtime_error("Detail positions require the qualified original x87 owner");
}
#endif
}
