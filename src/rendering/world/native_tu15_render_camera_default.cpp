#include "halo2/native_tu15_render_camera_default.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <limits>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
NativeTu15RenderCameraDefaultX87AbiRequired::NativeTu15RenderCameraDefaultX87AbiRequired(const NativeTu15RenderCameraX87State &state):
    std::runtime_error("Native render camera result requires a sparse x87 ABI owner"),state_(state){}
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> raw{};memory.read(address,std::span(raw).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<std::uint32_t>(raw[n])<<(8u*n);return value;
}
std::int32_t signed_word(std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width=4,bool *committed=nullptr) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);memory.read(address,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(8u*n));
    memory.write(address,event.written);if(committed)*committed=true;if(observe)observe(event);
}
using X87Image=std::array<std::byte,108>;
unsigned x87_tag(const std::array<std::byte,10> &value) {
    std::uint64_t significand{};for(unsigned n=0;n<8;++n)significand|=std::uint64_t(std::to_integer<unsigned>(value[n]))<<(8u*n);
    const auto exponent=(std::to_integer<unsigned>(value[8])|(std::to_integer<unsigned>(value[9])<<8u))&0x7fffu;
    if(!exponent&&!significand)return 1; // Signed zero.
    return !exponent || exponent==0x7fffu || !(significand>>63u)?2u:0u;
}
class X87Environment {
    alignas(16) X87Image host_{},native_{};
    NativeTu15PhysicsFrameFp &fp_;NativeTu15RenderCameraX87State &state_;
public:
    X87Environment(NativeTu15PhysicsFrameFp &fp,NativeTu15RenderCameraX87State &state):fp_(fp),state_(state) {
        if((fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u))
            throw std::invalid_argument("Native default camera requires masked retained x87 exceptions");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        native_=host_;std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);
        std::uint16_t tags=0xffffu;const auto top=(fp.status>>11u)&7u;
        for(unsigned n=0;n<8;++n) {
            if(state.occupied&(1u<<n)) {
                const auto shift=((top+n)&7u)*2u;
                tags=std::uint16_t((tags&~(3u<<shift))|(x87_tag(state.values[n])<<shift));
            }
            std::memcpy(native_.data()+28u+n*10u,state.values[n].data(),10);
        }
        std::memcpy(native_.data()+8,&tags,2);restore(native_);
    }
    ~X87Environment() {
        asm volatile("fnsave %0":"=m"(native_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(&fp_.control_word,native_.data(),2);std::memcpy(&fp_.status,native_.data()+4,2);
        std::uint16_t tags{};std::memcpy(&tags,native_.data()+8,2);const auto top=(fp_.status>>11u)&7u;
        state_.occupied=0;
        for(unsigned n=0;n<8;++n) {
            if(((tags>>(((top+n)&7u)*2u))&3u)!=3u)state_.occupied|=1u<<n;
            std::memcpy(state_.values[n].data(),native_.data()+28u+n*10u,10);
        }
        restore(host_);
    }
    static void restore(const X87Image &image) {
        asm volatile("frstor %0"::"m"(image):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    }
    static X87Image snapshot() {
        X87Image image;
        asm volatile("fnsave %0":"=m"(image)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        restore(image);return image;
    }
};
class SimdEnvironment {
    NativeTu15PhysicsFrameFp &fp_;unsigned host_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),host_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))throw std::invalid_argument("Native render camera requires masked SIMD exceptions");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(host_);}
};
void default_camera(SourcePhysicsContactMemory &memory,std::uint32_t destination,
    NativeTu15PhysicsFrameFp &fp,NativeTu15RenderCameraX87State &state,
    const SourcePhysicsContactRecordObserver &observe) {
    X87Environment environment(fp,state);
    const auto fov=read(memory,0x5320acu);asm volatile("flds %0"::"m"(fov):"memory","st");
    const auto position=read(memory,0x474ce8u);
    const auto half=read(memory,0x46a0ccu);asm volatile("fmuls %0"::"m"(half):"memory","st");
    const auto first=read(memory,position),one=read(memory,0x46a0d0u);
    asm volatile("fptan":::"memory","st","st(1)");
    publish(memory,observe,0x2f8c4,destination,first);
    const auto second=read(memory,position+4u);publish(memory,observe,0x2f8c9,destination+4u,second);
    const auto third=read(memory,position+8u);publish(memory,observe,0x2f8cf,destination+8u,third);
    const auto forward=read(memory,0x474d08u);
    constexpr std::array<std::uint32_t,3> forward_sites{0x2f8dc,0x2f8e1,0x2f8e7};
    for(unsigned n=0;n<3;++n){const auto value=read(memory,forward+n*4u);publish(memory,observe,forward_sites[n],destination+0xcu+n*4u,value);}
    const auto up=read(memory,0x474d10u);
    constexpr std::array<std::uint32_t,3> up_sites{0x2f8f4,0x2f8f9,0x2f8ff};
    for(unsigned n=0;n<3;++n){const auto value=read(memory,up+n*4u);publish(memory,observe,up_sites[n],destination+0x18u+n*4u,value);}
    publish(memory,observe,0x2f902,destination+0x2cu,one);
    asm volatile("fstp %%st(0)":::"memory","st");
    const auto aspect=read(memory,0x46a13cu);asm volatile("fmuls %0"::"m"(aspect):"memory","st");
    std::uint64_t denominator{};std::array<std::byte,8> raw{};memory.read(0x46a1d0u,raw);
    for(unsigned n=0;n<8;++n)denominator|=std::uint64_t(std::to_integer<unsigned>(raw[n]))<<(8u*n);
    asm volatile("fldl %0;fpatan;fadd %%st(0),%%st(0)"::"m"(denominator):"memory","st","st(1)");
    // FSTPS is one atomic source publication: a rejected memory write must
    // preserve its pre-round/pop hardware state; a committed observer fault
    // retains the rounded result and pop, exactly like the original store.
    const auto before=X87Environment::snapshot();std::uint32_t angle{};
    asm volatile("fstps %0":"=m"(angle)::"memory","st");bool committed=false;
    try{publish(memory,observe,0x2f919,destination+0x28u,angle,4,&committed);}
    catch(...){if(!committed)X87Environment::restore(before);throw;}
}
}
void native_tu15_render_camera_default_initialize(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRecordObserver &observe) {
    const auto value=read(memory,0x46ab88u);publish(memory,observe,0x380458,0x5320acu,value);
}
void native_tu15_render_camera_default_packet(SourcePhysicsContactMemory &memory,
    std::uint32_t destination,const NativeTu15RenderCameraGridArgument &argument,
    NativeTu15PhysicsFrameFp &fp,NativeTu15RenderCameraX87State &state,
    const SourcePhysicsContactRecordObserver &observe) {
    (void)argument.grid();
    SimdEnvironment environment(fp);
    default_camera(memory,destination,fp,state,observe);
    publish(memory,observe,0x2f91c,destination+0x24u,0,1);
    const auto near_distance=read(memory,0x4922f4u);publish(memory,observe,0x2f928,destination+0x40u,near_distance);
    const auto far_distance=read(memory,0x4922f8u);publish(memory,observe,0x2f935,destination+0x44u,far_distance);
    publish(memory,observe,0x2f93a,destination+0x58u,0,1);
}
}
