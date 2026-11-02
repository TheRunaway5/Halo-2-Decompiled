#include "halo2/native_player_effect_tick_tu15.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <stdexcept>
#include <xmmintrin.h>
namespace halo2 {
namespace {
class Environment {
    alignas(16) std::array<std::byte,108> host_{};
    std::array<std::byte,28> native_{};
    std::uint32_t host_mxcsr_{};NativeTu15PhysicsFrameFp& fp_;
public:
    explicit Environment(NativeTu15PhysicsFrameFp& fp):fp_(fp) {
        if((fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u) ||
            (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Player-effect tick requires masked retained FP exceptions");
        host_mxcsr_=_mm_getcsr();
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),native_.size());constexpr std::uint16_t empty=0xffffu;
        std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);
        std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment() {
        asm volatile("fnstcw %0;fnstsw %1":"=m"(fp_.control_word),"=m"(fp_.status));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);
    }
};
std::uint32_t read(const SourcePhysicsContactMemory& m,std::uint32_t at,unsigned width=4) {
    std::array<std::byte,4> b{};m.read(at,std::span(b).first(width));std::uint32_t v{};
    for(unsigned i=0;i<width;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[i]))<<(8*i);return v;
}
std::int16_t short_value(std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
void store(SourcePhysicsContactMemory& m,const SourcePhysicsContactRecordObserver& observe,
    std::uint32_t pc,std::uint32_t at,std::uint32_t value,unsigned width=4) {
    SourcePhysicsContactRecordStore event{pc,at,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);m.read(at,event.before);}
    for(unsigned i=0;i<width;++i)event.written[i]=std::byte(value>>(8*i));m.write(at,event.written);
    if(observe)observe(event);
}
}
void native_player_effect_tick_tu15(SourcePhysicsContactMemory& m,NativeTu15PhysicsFrameFp& fp,
    const SourcePhysicsContactRecordObserver& observe) {
    Environment environment(fp);
    const auto bank=read(m,0x51df5cu);const auto duration=short_value(read(m,bank+4u,2));
    if(duration!=-1) {
        const auto clock=read(m,0x51df54u),now=read(m,clock+8u),start=read(m,bank);
        if(std::bit_cast<std::int32_t>(now-start)>duration && !read(m,bank+6u,1))
            store(m,observe,0x1564b3u,bank+4u,0xffffu,2);
    }
    const auto flags=read(m,bank+0x28u);const auto feedback=read(m,0x50ebf8u);
    if(flags&1u) {
        const auto remaining=short_value(read(m,bank+0x24u,2));
        if(remaining>0)store(m,observe,0x1564d4u,bank+0x24u,std::uint16_t(remaining-1),2);
        else if(flags&2u) {
            store(m,observe,0x1564e2u,bank+0x28u,flags&~1u);
            store(m,observe,0x1564e5u,feedback+0x228u,0);
            store(m,observe,0x1564edu,feedback+0x220u,0);
            store(m,observe,0x1564f5u,feedback+0x224u,0);
        }
    }
    const auto factor=read(m,0x46a130u),one=read(m,0x46a0d0u);const auto users=read(m,0x4f54f0u);
    for(unsigned user=0;user<4;++user) {
        const auto slot=bank+0x30u+user*0xa0u,output=feedback+user*0x88u;
        if(read(m,users+0xcu+user*4u)==0xffffffffu) {
            for(unsigned n=0;n<0x28;++n)store(m,observe,0x156673u,slot+n*4u,0);
            for(unsigned n=0;n<0x22;++n)store(m,observe,0x156680u,output+n*4u,0);
            for(unsigned n=0;n<8;++n) {
                store(m,observe,0x156690u,output+n*12u,0xffffffffu);
                store(m,observe,0x156693u,output+n*12u+4u,0xffffffffu);
                store(m,observe,0x156696u,output+n*12u+8u,one);
            }
            continue;
        }
        const auto clock=read(m,0x51df54u);const int rate=short_value(read(m,clock+2u,2));
        __m128 value=_mm_setzero_ps();asm volatile("cvtsi2ssl %1,%0":"+x"(value):"r"(rate):"memory");
        const auto scale=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(factor)));
        asm volatile("mulss %1,%0":"+x"(value):"x"(scale):"memory");
        const float product=_mm_cvtss_f32(value);std::int32_t divisor{};
        asm volatile("flds %1;fistpl %0":"=m"(divisor):"m"(product):"memory","st");
        if(!divisor)throw std::runtime_error("Native156571 requires its original integer divide fault owner");
        const auto decrement=std::uint32_t(255/divisor);
        for(unsigned n=0;n<4;++n) {
            const auto old=read(m,slot+0x84u+n,1);
            if(old) {
                const auto reduced=old-decrement;
                store(m,observe,0x156596u,slot+0x84u+n,(reduced&0x80000000u)?0u:reduced,1);
            }
        }
        const auto vibration=read(m,slot+0x89u,1);
        if(vibration)store(m,observe,0x1565a8u,slot+0x89u,vibration-decrement,1);
        auto local_flags=read(m,slot+0x88u,1);
        const auto flash_ticks=short_value(read(m,slot+0x7eu,2));local_flags&=0xfeu;
        store(m,observe,0x1565bfu,slot+0x88u,local_flags,1);
        if(flash_ticks>0)store(m,observe,0x1565c8u,slot+0x7eu,std::uint16_t(flash_ticks-1),2);
        const auto translation_ticks=short_value(read(m,slot+0x80u,2));local_flags&=0xfdu;
        store(m,observe,0x1565dau,slot+0x88u,local_flags,1);
        if(translation_ticks>0)store(m,observe,0x1565e3u,slot+0x80u,std::uint16_t(translation_ticks-1),2);
        local_flags&=0xfbu;store(m,observe,0x1565ecu,slot+0x88u,local_flags,1);
        const auto rotation_ticks=short_value(read(m,slot+0x82u,2));
        if(rotation_ticks>0)store(m,observe,0x156601u,slot+0x82u,std::uint16_t(rotation_ticks-1),2);
        const auto recoil_ticks=short_value(read(m,slot+0x7cu,2));
        if(recoil_ticks>0) {
            store(m,observe,0x156617u,slot+0x7cu,std::uint16_t(recoil_ticks-1),2);
            if(recoil_ticks==1) {
                store(m,observe,0x15661du,output+0x80u,0);store(m,observe,0x156622u,output+0x84u,0);
                constexpr std::array<std::uint32_t,4> sites{0x15662bu,0x15662du,0x156630u,0x156633u};
                for(unsigned n=0;n<4;++n)store(m,observe,sites[n],slot+0x6cu+n*4u,0);
            }
        }
    }
}
}
