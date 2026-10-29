#include "halo2/native_campaign_ai_path_candidates_tu15.hpp"
#include <array>
#include <bit>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory, std::uint32_t address,
                   unsigned width = 4) {
    std::array<std::byte,4> bytes{};
    memory.read(address,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void publish(SourcePhysicsContactMemory &memory, const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site, std::uint32_t address, std::uint32_t value) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(4)};
    if(observe){event.before.resize(4);memory.read(address,event.before);}
    for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observe)observe(event);
}
void copy_point(SourcePhysicsContactMemory &memory, std::uint32_t source,
    std::uint32_t destination, const std::array<std::uint32_t,3> &sites,
    const SourcePhysicsContactRecordObserver &observe) {
    for(unsigned axis=0;axis<3;++axis)
        publish(memory,observe,sites[axis],destination+axis*4u,read(memory,source+axis*4u));
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class SimdEnvironment {
    NativeTu15PhysicsFrameFp &fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),previous_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native path candidate requires masked SIMD exceptions");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
using Scalar=__m128;
Scalar scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(Scalar value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
Scalar subtract(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar multiply(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar add(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar divide(Scalar a,Scalar b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool nonpositive_or_unordered(Scalar value) {
    unsigned char result{};const auto zero=_mm_setzero_ps();
    asm volatile("comiss %2,%1\n\tsetbe %0":"=qm"(result):"x"(value),"x"(zero):"cc","memory");return result;
}
bool greater(Scalar a,Scalar b) {
    unsigned char result{};
    asm volatile("comiss %2,%1\n\tseta %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result;
}
#endif
}
std::int16_t native_campaign_ai_path_find_node_tu15(const SourcePhysicsContactMemory &memory,
    std::uint32_t workspace, std::uint32_t key) {
    auto slot=(key&0x1ffu)<<3u;
    for(;;) {
        const auto node=std::bit_cast<std::int16_t>(std::uint16_t(read(memory,workspace+0x120b6u+slot*2u,2)));
        slot=(slot+1u)&0xfffu;
        if(node==-1)return node;
        const auto record=workspace+0xb8u+std::uint32_t(std::int32_t(node))*0x44u;
        if(read(memory,record)==key)return node;
    }
}
void native_campaign_ai_path_segment_candidate_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t start, std::uint32_t end, std::uint32_t query,
    std::uint32_t destination, NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observe) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    SimdEnvironment environment(fp);
    const auto end_x=scalar(read(memory,end));
    auto dx=subtract(end_x,scalar(read(memory,start)));
    const auto end_y=scalar(read(memory,end+4u));
    auto dy=subtract(end_y,scalar(read(memory,start+4u)));
    const auto end_z=scalar(read(memory,end+8u));
    auto dz=subtract(end_z,scalar(read(memory,start+8u)));
    const auto x_squared=multiply(dx,dx),y_squared=multiply(dy,dy);
    auto norm=add(x_squared,y_squared);
    norm=add(norm,multiply(dz,dz));
    if(nonpositive_or_unordered(norm)) {
        copy_point(memory,start,destination,{0x27c50f,0x27c514,0x27c51a},observe);return;
    }
    auto y=scalar(read(memory,start+4u));y=subtract(y,scalar(read(memory,query+4u)));
    auto z=scalar(read(memory,start+8u));
    auto x=scalar(read(memory,start));
    z=subtract(z,scalar(read(memory,query+8u)));
    x=subtract(x,scalar(read(memory,query)));
    y=multiply(y,dy);z=multiply(z,dz);
    auto numerator=add(y,z);x=multiply(x,dx);numerator=add(numerator,x);
    const auto fraction=divide(numerator,norm);
    if(greater(_mm_setzero_ps(),fraction)) {
        copy_point(memory,end,destination,{0x27c4fe,0x27c503,0x27c509},observe);return;
    }
    if(greater(fraction,scalar(read(memory,0x46a0d0u)))) {
        copy_point(memory,end,destination,{0x27c4fe,0x27c503,0x27c509},observe);return;
    }
    dx=multiply(dx,fraction);dx=add(dx,scalar(read(memory,start)));
    publish(memory,observe,0x27c4db,destination,bits(dx));
    dy=multiply(dy,fraction);dy=add(dy,scalar(read(memory,start+4u)));
    publish(memory,observe,0x27c4e8,destination+4u,bits(dy));
    dz=multiply(dz,fraction);dz=add(dz,scalar(read(memory,start+8u)));
    publish(memory,observe,0x27c4f6,destination+8u,bits(dz));
#else
    (void)memory;(void)start;(void)end;(void)query;(void)destination;(void)fp;(void)observe;
    throw std::runtime_error("Exact native TU1.5 path candidate arithmetic unavailable");
#endif
}
}
