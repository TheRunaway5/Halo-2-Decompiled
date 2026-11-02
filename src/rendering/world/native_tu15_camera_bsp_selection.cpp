#include "halo2/native_tu15_camera_bsp_selection.hpp"
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
std::int32_t signed_word(std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
std::int32_t signed_value(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(4)};
    if(observe){event.before.resize(4);memory.read(address,event.before);}
    for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(8u*n));
    memory.write(address,event.written);if(observe)observe(event);
}
class SimdEnvironment {
    NativeTu15PhysicsFrameFp &fp_;unsigned host_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),host_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native camera BSP requires masked SIMD exceptions");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(host_);}
};
__m128 scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(signed_value(value)));}
__m128 product(__m128 left,__m128 right){asm volatile("mulss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 sum(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 difference(__m128 left,__m128 right){asm volatile("subss %1,%0":"+x"(left):"x"(right):"memory");return left;}
bool nonnegative(__m128 value) {
    unsigned char result;const auto zero=_mm_setzero_ps();
    // COMISS (including its qNaN invalid status), rather than a C++ comparison
    // or UCOMISS. An unordered distance takes the negative-side child.
    asm volatile("comiss %2,%1; setae %0":"=qm"(result):"x"(value),"x"(zero):"cc","memory");
    return result!=0;
}
std::int32_t leaf(const SourcePhysicsContactMemory &memory,std::uint32_t tree,
    std::uint32_t point,std::uint32_t node) {
    const auto nodes=read(memory,tree+4u),planes=read(memory,tree+0xcu);
    for(;;) {
        const auto record=nodes+(node&0x7fffffu)*8u;
        const auto plane_index=signed_word(read(memory,record,2));
        const auto plane=planes+std::uint32_t(plane_index)*16u;
        const auto nz=scalar(read(memory,plane+8u));const auto z=scalar(read(memory,point+8u));
        auto distance=product(nz,z);
        const auto ny=scalar(read(memory,plane+4u));const auto y=scalar(read(memory,point+4u));
        distance=sum(distance,product(ny,y));
        const auto nx=scalar(read(memory,plane));const auto x=scalar(read(memory,point));
        distance=sum(distance,product(nx,x));
        const auto offset=scalar(read(memory,plane+0xcu));distance=difference(distance,offset);
        const auto side=nonnegative(distance)?1u:0u;
        // The source consumes a DWORD one byte before the selected packed
        // child. Keep that exact width/read boundary and signed24-bit decode.
        const auto child=signed_value(read(memory,record+1u+side*3u))>>8;
        if(std::uint32_t(child)&0x800000u)
            return child==-1?-1:std::int32_t(std::uint32_t(child)&0x7fffffu);
        node=std::uint32_t(child);
    }
}
}
std::int32_t native_tu15_camera_bsp_leaf(const SourcePhysicsContactMemory &memory,
    std::uint32_t tree,std::uint32_t point,std::uint32_t node,NativeTu15PhysicsFrameFp &fp) {
    SimdEnvironment environment(fp);return leaf(memory,tree,point,node);
}
bool native_tu15_camera_bsp_select(SourcePhysicsContactMemory &memory,std::uint32_t point,
    std::uint32_t leaf_output,const NativeTu15CameraBspSelectionArgument &argument,
    NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
    const auto cluster_output=argument.cluster_output();
    const auto bsp=read(memory,0x4ecc18u);const auto tree=read(memory,bsp+0x18u);
    SimdEnvironment environment(fp);const auto selected=leaf(memory,tree,point,0);
    if(selected!=-1) {
        publish(memory,observe,0x2b71eu,leaf_output,std::uint32_t(selected));
        const auto leaves=read(memory,bsp+0x30u);
        const auto cluster=signed_word(read(memory,leaves+std::uint32_t(selected)*8u,2));
        publish(memory,observe,0x2b728u,cluster_output,std::uint32_t(cluster));return true;
    }
    const auto old_leaf=signed_value(read(memory,leaf_output));
    bool valid=old_leaf>=0 && old_leaf<signed_value(read(memory,bsp+0x2cu));
    if(valid) {
        const auto old_cluster=signed_value(read(memory,cluster_output));
        valid=old_cluster>=0 && old_cluster<signed_value(read(memory,bsp+0x9cu));
    }
    if(!valid) {
        publish(memory,observe,0x2b74cu,leaf_output,0xffffffffu);
        publish(memory,observe,0x2b752u,cluster_output,0xffffffffu);
    }
    return false;
}
}
