#include "halo2/native_campaign_camera_commands_tu15.hpp"
#include <array>
#include <bit>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#include <emmintrin.h>
#endif

namespace halo2 {
namespace {
class Storage {
    SourcePhysicsContactMemory &memory_;
    const SourcePhysicsContactRecordObserver &observe_;
public:
    Storage(SourcePhysicsContactMemory &m,const SourcePhysicsContactRecordObserver &o):memory_(m),observe_(o){}
    std::uint32_t read(std::uint32_t at,unsigned width=4)const {
        std::array<std::byte,4> bytes{};memory_.read(at,std::span(bytes).first(width));
        std::uint32_t value{};for(unsigned i=0;i<width;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8u*i);
        return value;
    }
    void publish(std::uint32_t site,std::uint32_t at,std::uint32_t value,unsigned width=4) {
        SourcePhysicsContactRecordStore event{site,at,{},std::vector<std::byte>(width)};
        if(observe_){event.before.resize(width);memory_.read(at,event.before);}
        for(unsigned i=0;i<width;++i)event.written[i]=std::byte(value>>(8u*i));
        memory_.write(at,event.written);if(observe_)observe_(event);
    }
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class SimdEnvironment {
    unsigned host_;
    NativeCampaignCameraCommandFpTu15 &fp_;
public:
    explicit SimdEnvironment(NativeCampaignCameraCommandFpTu15 &fp):host_(_mm_getcsr()),fp_(fp) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native camera FOV requires masked SIMD exceptions");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(host_);}
};
using Scalar=__m128;
Scalar scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(Scalar value){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));}
Scalar zero(){return _mm_setzero_ps();}
Scalar add(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar sub(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar mul(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar divide(Scalar a,Scalar b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool ordered_zero(Scalar a) {
    unsigned char equal,ordered;
    asm volatile("ucomiss %3,%2;sete %0;setnp %1":"=qm"(equal),"=qm"(ordered):"x"(a),"x"(zero()):"cc","memory");
    return equal&&ordered;
}
bool above(Scalar a,Scalar b){unsigned char result;asm volatile("comiss %2,%1;seta %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result;}
bool at_least(Scalar a,Scalar b){unsigned char result;asm volatile("comiss %2,%1;setae %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result;}
bool below_or_unordered(Scalar a,Scalar b){unsigned char result;asm volatile("comiss %2,%1;setb %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result;}
#endif
}

void native_campaign_cinematic_reset_tu15(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRecordObserver &observe) {
    Storage data(memory,observe);const auto state=data.read(0x51df50u);
    constexpr std::array<std::uint32_t,9> clear_sites{0x13ea49,0x13ea4b,0x13ea4e,0x13ea51,0x13ea54,0x13ea57,0x13ea5a,0x13ea5d,0x13ea60};
    for(unsigned n=0;n<clear_sites.size();++n)data.publish(clear_sites[n],state+n*4u,0);
    constexpr std::array<std::uint32_t,4> none_sites{0x13ea69,0x13ea6b,0x13ea6e,0x13ea71};
    for(unsigned n=0;n<none_sites.size();++n)data.publish(none_sites[n],state+8u+n*4u,0xffffffffu);
    const auto game=data.read(0x4f3218u);
    if(data.read(game+8u)==1u) {
        const auto opacity=data.read(0x46a0d0u);
        data.publish(0x13ea90,state+4u,1,1);data.publish(0x13ea93,state,opacity);
    }
}

void native_campaign_cinematic_stop_tu15(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRecordObserver &observe) {
    Storage data(memory,observe);const auto simulation=data.read(0x4eabc4u),users=data.read(0x4f54f0u);
    data.publish(0x13f56d,simulation+0x81u,0,1);data.publish(0x13f573,simulation+0x80u,0,1);
    const auto game=data.read(0x4f3218u);data.publish(0x13f57e,game+0x11f8u,0,1);
    const auto state=data.read(0x51df50u);data.publish(0x13f589,state+4u,0,1);data.publish(0x13f58c,users+6u,0,1);
    const auto recording=data.read(0x5020a8u),screen=data.read(0x515ec0u);
    data.publish(0x13f59e,recording+0x20u,1,1);data.publish(0x13f5a2,state+5u,0,1);
    if(screen) {
        // The default is loaded before the reset, including when its source
        // aliases the destination bank. Each publication remains observable.
        const auto one=data.read(0x46a0d0u);
        for(unsigned n=0;n<0xffu;++n)data.publish(0x13f5b9,screen+n*4u,0);
        for(const auto [site,offset]:std::array<std::pair<std::uint32_t,unsigned>,5>{{{0x13f5bb,0x180},{0x13f5c3,0x184},{0x13f5cb,0x188},{0x13f5d3,0x18c},{0x13f5db,0x3f8}}})data.publish(site,screen+offset,one);
        data.publish(0x13f5e6,screen+0x190u,0);
    }
    const auto enabled=data.read(0x515e2cu);data.publish(0x13f5f4,enabled,0,1);
    const auto director=data.read(0x51df6cu);
    constexpr std::array<std::uint32_t,4> sites{0x13f600,0x13f602,0x13f605,0x13f608};
    for(unsigned n=0;n<sites.size();++n)data.publish(sites[n],director+4u+n*4u,0);
    data.publish(0x13f60b,0x515ef5u,0,1);
}

void native_campaign_camera_fov_tu15(SourcePhysicsContactMemory &memory,
    const NativeCampaignCameraFovArgumentsTu15 &arguments,NativeCampaignCameraCommandFpTu15 &fp,
    const SourcePhysicsContactRecordObserver &observe) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    SimdEnvironment environment(fp);Storage data(memory,observe);const auto director=data.read(0x51df6cu);
    const bool uninitialized=ordered_zero(scalar(data.read(director+0xcu)));
    auto current=scalar(data.read(0x55d6e4u));
    if(uninitialized)data.publish(0x16f5bf,director+0xcu,bits(current));
    const auto state=data.read(0x51df50u);
    if(state && data.read(state+5u,1)) {
        auto target=scalar(data.read(director+0x10u));
        if(!ordered_zero(target)) {
            const auto elapsed=scalar(data.read(director+4u));
            if(at_least(elapsed,scalar(data.read(director+8u))))current=target;
            else {
                const auto elapsed_again=scalar(data.read(director+4u));
                const auto duration_again=scalar(data.read(director+8u));
                auto fraction=divide(elapsed_again,duration_again);
                if(above(zero(),fraction))fraction=zero();
                else {const auto one=scalar(data.read(0x46a0d0u));if(above(fraction,one))fraction=one;}
                const auto start=scalar(data.read(director+0xcu));
                target=scalar(data.read(director+0x10u));
                current=add(mul(sub(target,start),fraction),start);
            }
        }
    }
    const auto ticks=arguments.ticks();data.publish(0x16f631,director+0xcu,bits(current));
    const auto duration=mul(_mm_cvtsi32_ss(zero(),ticks),scalar(data.read(0x46a17cu)));
    data.publish(0x16f642,director+8u,bits(duration));
    const auto degrees=scalar(arguments.degrees_bits());
    const auto target=mul(degrees,scalar(data.read(0x46a164u)));
    data.publish(0x16f658,director+0x10u,bits(target));
    data.publish(0x16f673,director+4u,0);
    const auto target_bits=bits(target);
    bool valid=(target_bits&0x7f800000u)!=0x7f800000u;
    if(valid)valid=!below_or_unordered(target,scalar(data.read(0x55d6e8u)));
    if(valid)valid=at_least(scalar(data.read(0x55d6ecu)),target);
    if(!valid) {
        constexpr std::array<std::uint32_t,4> sites{0x16f691,0x16f693,0x16f696,0x16f699};
        for(unsigned n=0;n<sites.size();++n)data.publish(sites[n],director+4u+n*4u,0);
    }
#else
    throw std::runtime_error("Native cinematic FOV requires its hardware SIMD backend");
#endif
}
}
