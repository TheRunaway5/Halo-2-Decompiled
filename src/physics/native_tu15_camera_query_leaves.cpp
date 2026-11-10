#include "halo2/native_tu15_segment_sphere_value.hpp"
#include "halo2/native_tu15_camera_query_leaves.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
    std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};m_.read(p,b);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o_){e.before.resize(4);m_.read(p,e.before);e.written.assign(b.begin(),b.end());}m_.write(p,b);if(o_)o_(e);}
};
std::uint32_t advance(Memory&m,std::uint32_t node,std::uint32_t iterator,std::uint32_t payload,
    std::uint32_t payload_pc,std::uint32_t iterator_pc){
    const auto pool=m.word(0x4eabb4u),rows=m.word(pool+0x44u),stride=m.word(pool+0x24u);
    const auto row=rows+(node&0xffffu)*stride,next=m.word(row+8u);
    m.word(payload_pc,payload,row+12u);
    if(next!=0xffffffffu){
        // The source reloads both fields before a nonfaulting prefetch. Keep
        // those architectural DATA reads; no semantic read of the prefetched
        // address is invented and no native allocation/free is performed.
        const auto current_stride=m.word(pool+0x24u),current_rows=m.word(pool+0x44u);
        (void)(current_rows+(next&0xffffu)*current_stride);
    }
    m.word(iterator_pc,iterator,next);return m.word(row+4u);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class SseEnvironment {
    NativeTu15PhysicsFrameFp&fp_;unsigned host_;
public:
    explicit SseEnvironment(NativeTu15PhysicsFrameFp&fp):fp_(fp),host_(_mm_getcsr()){
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native camera sphere query requires SSE exception owner");_mm_setcsr(fp.mxcsr);
    }
    ~SseEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(host_);}
};
using F=__m128;
F scalar(std::uint32_t n){F out{};std::memcpy(&out,&n,4);return out;}
F mul(F a,F b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F add(F a,F b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F sub(F a,F b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
struct Comparison{bool below,equal;bool above()const{return!below&&!equal;}};
Comparison compare(F a,F b){unsigned char below,equal;asm volatile("comiss %3,%2;setb %0;sete %1":"=qm"(below),"=qm"(equal):"x"(a),"x"(b):"cc","memory");return{below!=0,equal!=0};}
#endif
}
std::uint32_t native_tu15_camera_bsp_query_flags(std::uint32_t flags){return(flags>>23u)&0x3fu;}
std::uint32_t native_tu15_camera_object_query_flags(std::uint32_t flags){
    return 1u|((flags>>18u)&2u)|((flags>>16u)&4u)|((flags>>27u)&8u)|
        ((flags>>16u)&16u)|((flags>>17u)&32u)|((flags>>15u)&64u);
}
std::uint32_t native_tu15_begin_spatial_object_query(SourcePhysicsContactMemory&memory,
    std::uint16_t cluster,std::uint32_t iterator,std::uint32_t payload,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);const auto root=m.word(0x4eabb0u);
    const auto signed_cluster=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(cluster)));
    const auto node=m.word(root+signed_cluster*4u);m.word(0xba981u,iterator,node);
    if(node==0xffffffffu)return 0xffffffffu;
    return advance(m,node,iterator,payload,0xba9afu,0xba9c9u);
}
std::uint32_t native_tu15_next_spatial_object_query(SourcePhysicsContactMemory&memory,
    std::uint32_t iterator,std::uint32_t payload,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);const auto node=m.word(iterator);if(node==0xffffffffu)return 0xffffffffu;
    return advance(m,node,iterator,payload,0xbaa11u,0xbaa2bu);
}
bool native_tu15_segment_near_sphere(SourcePhysicsContactMemory&memory,
    std::uint32_t first,std::uint32_t center,std::uint32_t direction,std::uint32_t radius_bits,NativeTu15PhysicsFrameFp&fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    SseEnvironment environment(fp);const SourcePhysicsContactRecordObserver empty;Memory m(memory,empty);
    auto read=[&](std::uint32_t address){return m.word(address);};
    native_segment_sphere_detail::Values view{read,first,center,direction,radius_bits};
    native_segment_sphere_detail::Progress data;return native_segment_sphere_detail::intersects(view,data);
#else
    (void)memory;(void)first;(void)center;(void)direction;(void)radius_bits;(void)fp;
    throw std::runtime_error("Exact native camera sphere query backend unavailable");
#endif
}
} // namespace halo2
