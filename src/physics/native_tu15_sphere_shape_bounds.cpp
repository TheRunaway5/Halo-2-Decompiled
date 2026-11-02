#include "halo2/native_tu15_sphere_shape_bounds.hpp"
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#include <emmintrin.h>
#endif
namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};
    std::array<std::byte,28>native_{};unsigned host_mxcsr_{};NativeTu15PhysicsFrameFp&fp_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
        if(depth>7||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
            (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native TU1.5 sphere bounds requires its unsupported x87/SSE fault owner");
        host_mxcsr_=_mm_getcsr();
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),28);const std::uint16_t empty=0xffff;
        std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);
        std::memcpy(native_.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){
        asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));
        fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        _mm_setcsr(host_mxcsr_);
    }
};
class Memory {
    SourcePhysicsContactMemory&memory_;
    const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o)
        :memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t address)const{
        std::array<std::byte,4>raw{};memory_.read(address,raw);std::uint32_t value{};
        for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(raw[n])<<(n*8u);
        return value;
    }
    __m128 vector(std::uint32_t address)const{
        if(address&15u)throw std::runtime_error("Native TU1.5 sphere bounds requires aligned vector storage");
        std::array<std::byte,16>raw{};memory_.read_vector(address,raw);__m128 value;
        std::memcpy(&value,raw.data(),16);return value;
    }
    void vector(std::uint32_t site,std::uint32_t address,__m128 value){
        if(address&15u)throw std::runtime_error("Native TU1.5 sphere bounds requires aligned vector storage");
        std::array<std::byte,16>raw{};std::memcpy(raw.data(),&value,16);
        SourcePhysicsContactRecordStore event{site,address,{},{}};
        if(observe_){event.before.resize(16);memory_.read(address,event.before);
            event.written.assign(raw.begin(),raw.end());}
        memory_.write(address,raw);if(observe_)observe_(event);
    }
};
__m128 subtract(__m128 center,__m128 radius){asm volatile("subps %1,%0":"+x"(center):"x"(radius):"memory");return center;}
__m128 add(__m128 center,__m128 radius){asm volatile("addps %1,%0":"+x"(center):"x"(radius):"memory");return center;}
#endif
}
void native_tu15_sphere_shape_bounds(SourcePhysicsContactMemory&memory,NativeTu15BoundsShape shape,
    NativeTu15BoundsTransform transform,std::uint32_t margin,NativeTu15BoundsOutput output,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15SphereBoundsStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp,depth);Memory m(memory,observe);if(stats)++stats->calls;
    asm volatile("flds %0"::"m"(margin):"memory","st");
    const auto radius=m.word(shape.identity+12u);
    asm volatile("fadds %0"::"m"(radius):"memory","st");
    const auto lower_center=m.vector(transform.identity+48u);
    std::uint32_t combined{};asm volatile("fstps %0":"=m"(combined)::"memory","st");
    __m128 expansion=_mm_castsi128_ps(_mm_cvtsi32_si128(std::int32_t(combined)));
    expansion=_mm_shuffle_ps(expansion,expansion,0);
    m.vector(0x2e8dbau,output.identity,subtract(lower_center,expansion));
    const auto upper_center=m.vector(transform.identity+48u);
    m.vector(0x2e8dc4u,output.identity+16u,add(upper_center,expansion));
#else
    (void)memory;(void)shape;(void)transform;(void)margin;(void)output;(void)fp;(void)depth;(void)observe;(void)stats;
    throw std::runtime_error("Exact native TU1.5 sphere bounds backend unavailable");
#endif
}
} // namespace halo2
