#include "halo2/native_campaign_ai_path_query_start_tu15.hpp"
#include "halo2/native_campaign_ai_path_heap_tu15.hpp"
#include "halo2/native_campaign_ai_point_geometry_tu15.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));
    std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width=4) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);memory.read(address,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observe)observe(event);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class SimdEnvironment {
    NativeTu15PhysicsFrameFp &fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),previous_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native path query requires its SIMD trap owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
// Preserve the host's real x87 values while the native distance/conversion
// algorithm owns its hardware arithmetic temporaries. No virtual stack exists.
class ArithmeticEnvironment {
    alignas(16) std::array<std::byte,108> host_{};
    std::array<std::byte,28> native_{};NativeTu15PhysicsFrameFp &fp_;
public:
    explicit ArithmeticEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp) {
        if((fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u))
            throw std::runtime_error("Native path query requires masked x87 exceptions");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),native_.size());const std::uint16_t empty=0xffffu;
        std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);
        std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");
    }
    ~ArithmeticEnvironment() {
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(fp_.control_word),"=m"(fp_.status));
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    }
};
using Scalar=__m128;
Scalar scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(Scalar value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
Scalar subtract(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar multiply(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar add(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool above(std::uint32_t a,std::uint32_t b,NativeTu15PhysicsFrameFp &fp) {
    SimdEnvironment environment(fp);const auto first=scalar(a),second=scalar(b);unsigned char result{};
    asm volatile("comiss %2,%1\n\tseta %0":"=qm"(result):"x"(first),"x"(second):"memory","cc");
    return result!=0;
}
std::uint32_t distance_squared(const SourcePhysicsContactMemory &memory,std::uint32_t first,
    std::uint32_t second,NativeTu15PhysicsFrameFp &fp) {
    const auto first_frame=read(memory,first+0xc,2),second_frame=read(memory,second+0xc,2);
    if(first_frame==second_frame) {
        SimdEnvironment environment(fp);
        auto x=scalar(read(memory,second));x=subtract(x,scalar(read(memory,first)));
        auto z=scalar(read(memory,second+8));z=subtract(z,scalar(read(memory,first+8)));
        auto y=scalar(read(memory,second+4));y=subtract(y,scalar(read(memory,first+4)));
        auto sum=multiply(x,x);const auto z_squared=multiply(z,z);sum=add(sum,z_squared);
        const auto y_squared=multiply(y,y);return bits(add(sum,y_squared));
    }
    const auto a=native_campaign_ai_project_point_value_tu15(memory,first,fp);
    const auto b=native_campaign_ai_project_point_value_tu15(memory,second,fp);
    SimdEnvironment environment(fp);
    const auto z=subtract(scalar(b[2]),scalar(a[2]));
    const auto y=subtract(scalar(b[1]),scalar(a[1]));
    const auto x=subtract(scalar(b[0]),scalar(a[0]));
    auto sum=multiply(z,z);const auto y_squared=multiply(y,y);sum=add(sum,y_squared);
    const auto x_squared=multiply(x,x);return bits(add(sum,x_squared));
}
struct Cost {std::uint32_t distance{};std::uint64_t quantized{};};
Cost quantize_distance(const SourcePhysicsContactMemory &memory,std::uint32_t squared,
    NativeTu15PhysicsFrameFp &fp) {
    ArithmeticEnvironment environment(fp);Cost result;
    // The float publication rounds the distance but scaling consumes the
    // retained extended sqrt. Reading the multiplier stays after that spill.
    asm volatile("flds %1\n\tfsqrt\n\tfsts %0":"=m"(result.distance):"m"(squared):"memory","st");
    const auto scale=read(memory,0x46a110u);
    std::uint32_t sign_image{},difference_image{};
    asm volatile("fmuls %2\n\tfld %%st(0)\n\tfsts %0\n\tfistpll %1\n\tfildll %1"
        :"=m"(sign_image),"=m"(result.quantized):"m"(scale):"memory","st","st(1)");
    // CRT conversion rounds under the actual CW first, then corrects one unit
    // toward zero from the actual float difference. Zero/indefinite outcomes
    // use the source's two-pop path without inventing a finite replacement.
    const auto low=std::uint32_t(result.quantized),high=std::uint32_t(result.quantized>>32u);
    if(low==0u && (high&0x7fffffffu)==0u) {
        asm volatile("fstps %0\n\tfstps %0":"=m"(difference_image)::"memory","st","st(1)");
    }else {
        asm volatile("fsubrp %%st,%%st(1)\n\tfstps %0":"=m"(difference_image)::"memory","st","st(1)");
        const bool negative=(sign_image&0x80000000u)!=0;
        const auto adjusted=negative ? difference_image^0x80000000u : difference_image;
        const bool correction=std::uint64_t(adjusted)+0x7fffffffull>0xffffffffull;
        if(correction)result.quantized=negative ? result.quantized+1u : result.quantized-1u;
    }
    return result;
}
#endif
}
bool native_campaign_ai_path_query_start_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t workspace,NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto navigation=read(memory,workspace+0x8cu);
    if(!navigation)return false;
    const auto initial_sector=std::int32_t(read(memory,workspace+0x24u));
    if(initial_sector<0 || initial_sector>=std::int32_t(read(memory,navigation)))return false;
    const auto height=read(memory,workspace+0x1cu),minimum_height=read(memory,0x46a460u);
    if(!above(height,minimum_height,fp))return false;
    Cost cost{};
    if(read(memory,workspace+0x54u,1)) {
        const auto squared=distance_squared(memory,workspace+0x14u,workspace+0x58u,fp);
        cost=quantize_distance(memory,squared,fp);
        if(std::bit_cast<std::int32_t>(std::uint32_t(cost.quantized))>=0x7fff)return false;
    }
    const auto current_navigation=read(memory,workspace+0x8cu);
    const auto sectors=read(memory,current_navigation+4u);
    const auto sector=sectors+read(memory,workspace+0x24u)*8u;
    const auto node_index=std::uint16_t(read(memory,workspace+0xaeu,2));
    const auto node=workspace+0xb0u+std::uint32_t(std::int32_t(std::int16_t(node_index)))*0x44u;
    const auto maximum=read(memory,0x45695cu);
    publish(memory,observe,0x27b1a5,workspace+0xae,std::uint16_t(node_index+1u),2);
    publish(memory,observe,0x27b1b6,node+2,0xffffu,2);
    publish(memory,observe,0x27b1ba,node+4,0xffffffffu);
    publish(memory,observe,0x27b1c0,node+8,read(memory,workspace+0x24u));
    constexpr std::array<std::uint32_t,4> pose_sites{0x27b1ce,0x27b1d3,0x27b1d9,0x27b1e3};
    for(unsigned axis=0;axis<4;++axis)publish(memory,observe,pose_sites[axis],node+0x18u+axis*4u,read(memory,workspace+0x14u+axis*4u));
    publish(memory,observe,0x27b1ec,node+0x28,0);
    publish(memory,observe,0x27b1f1,node+0x30,0);
    publish(memory,observe,0x27b1f6,node+0x2c,maximum);
    publish(memory,observe,0x27b1fb,node+0x34,0);
    publish(memory,observe,0x27b200,node+0x38,cost.distance);
    const auto priority=std::uint16_t(cost.quantized);
    publish(memory,observe,0x27b205,node+0x3c,priority,2);
    publish(memory,observe,0x27b209,node+0x3e,0,2);
    publish(memory,observe,0x27b219,node+0xc,(read(memory,sector,2)>>12u)&1u,1);
    const bool special=(read(memory,sector,2)&0x3c0u)!=0;
    publish(memory,observe,0x27b222,node+0xe,0,1);
    publish(memory,observe,0x27b229,node+0xd,special,1);
    publish(memory,observe,0x27b231,node+0x10,0xffffu,2);
    publish(memory,observe,0x27b235,node+0x14,0xffffffffu);
    if(read(memory,workspace+0x54u,1)) {
        publish(memory,observe,0x27b247,workspace+0x9c,read(memory,workspace+0x14u));
        publish(memory,observe,0x27b24c,workspace+0xa0,read(memory,workspace+0x18u));
        const auto z=read(memory,workspace+0x1cu),frame=read(memory,workspace+0x20u);
        publish(memory,observe,0x27b255,workspace+0xa4,z);
        publish(memory,observe,0x27b258,workspace+0x90,node_index,2);
        publish(memory,observe,0x27b25f,workspace+0x94,cost.distance);
        publish(memory,observe,0x27b267,workspace+0xa8,frame);
        publish(memory,observe,0x27b26a,workspace+0x98,cost.distance);
    }
    const auto hash=read(memory,node+8u)&0x1ffu;
    publish(memory,observe,0x27b282,workspace+0x120b6u+hash*16u,node_index,2);
    native_campaign_ai_path_heap_insert_tu15(memory,workspace,std::bit_cast<std::int16_t>(node_index),
        std::bit_cast<std::int16_t>(priority),observe);
    return true;
#else
    (void)memory;(void)workspace;(void)fp;(void)observe;
    throw std::runtime_error("Exact native TU1.5 path-query arithmetic backend unavailable");
#endif
}
}
