#include "halo2/native_campaign_screen_flash_sample_tu15.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <xmmintrin.h>
#include <emmintrin.h>

namespace halo2 {
NativeCampaignScreenFlashRequiredTu15::NativeCampaignScreenFlashRequiredTu15(
    std::uint32_t instruction,std::uint32_t target):
    std::runtime_error("Native screen flash reached an unqualified original CRT owner"),
    instruction_(instruction),target_(target){}
namespace {
class Environment {
    alignas(16) std::array<std::byte,108> host_{};
    std::array<std::byte,28> native_{};
    std::uint32_t host_mxcsr_{};
    NativeTu15PhysicsFrameFp &fp_;
public:
    explicit Environment(NativeTu15PhysicsFrameFp &fp):fp_(fp) {
        if((fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u) ||
            (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native screen flash requires masked retained FP exceptions");
        host_mxcsr_=_mm_getcsr();
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),native_.size());
        constexpr std::uint16_t empty=0xffffu;
        std::memcpy(native_.data(),&fp.control_word,2);
        std::memcpy(native_.data()+4,&fp.status,2);
        std::memcpy(native_.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(native_):"memory");
        _mm_setcsr(fp.mxcsr);
    }
    ~Environment() {
        asm volatile("fnstcw %0;fnstsw %1":"=m"(fp_.control_word),"=m"(fp_.status));
        fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        _mm_setcsr(host_mxcsr_);
    }
};
std::uint64_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,8> bytes{};memory.read(address,std::span(bytes).first(width));std::uint64_t result{};
    for(unsigned n=0;n<width;++n)result|=std::uint64_t(std::to_integer<unsigned>(bytes[n]))<<(n*8u);
    return result;
}
std::uint32_t word(const SourcePhysicsContactMemory &m,std::uint32_t at){return std::uint32_t(read(m,at));}
std::int32_t signed_word(std::uint32_t bits){return std::bit_cast<std::int16_t>(std::uint16_t(bits));}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observer,
    std::uint32_t site,std::uint32_t at,std::uint32_t value) {
    SourcePhysicsContactRecordStore event{site,at,{},std::vector<std::byte>(4)};
    if(observer){event.before.resize(4);memory.read(at,event.before);}
    for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(at,event.written);if(observer)observer(event);
}
using Scalar=__m128;
Scalar scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));}
std::uint32_t bits(Scalar value){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));}
Scalar add(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar sub(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar mul(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar divide(Scalar a,Scalar b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar integer(std::int32_t value){Scalar result{};asm volatile("cvtsi2ssl %1,%0":"+x"(result):"r"(value):"memory");return result;}
bool above(Scalar a,Scalar b){unsigned char result;asm volatile("comiss %2,%1;seta %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result!=0;}
void push_single(std::uint32_t value){asm volatile("flds %0"::"m"(value):"memory","st");}
void push_double(std::uint64_t value){asm volatile("fldl %0"::"m"(value):"memory","st");}
std::uint32_t pop_single(){std::uint32_t value;asm volatile("fstps %0":"=m"(value)::"memory","st");return value;}
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=m"(value));return value;}
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
void control(std::uint16_t value){asm volatile("fldcw %0"::"m"(value):"memory");}
void store_double(){std::uint64_t value;asm volatile("fstl %0":"=m"(value)::"memory");}
std::uint8_t classification(std::uint16_t sw) {
    auto high=std::uint8_t(sw>>8u);high=std::uint8_t(high<<1u);
    high=std::uint8_t(std::bit_cast<std::int8_t>(high)>>1);
    return std::uint8_t(std::rotl(high,1)&15u);
}
// Actual37DC0A/37D46C/331ECB, specialized only in the two genuine method
// bodies admitted below. Method selection still reads authored source data.
void remainder(SourcePhysicsContactMemory &memory) {
    const auto original_control=control();
    if(!word(memory,0x46ecd0u)) {
        asm volatile("fxch %%st(1)":::"memory","st","st(1)");store_double();
        asm volatile("fxch %%st(1)":::"memory","st","st(1)");store_double();
    }
    const auto kind=read(memory,0x48c8dau,1);
    const auto working_control=kind==5?std::uint16_t(((original_control|0x0200u)&0xfeffu&0xff00u)|0x003fu):std::uint16_t(0x133fu);
    control(working_control);
    asm volatile("fxam":::"memory");const auto denominator_class=classification(status());
    asm volatile("fxch %%st(1);fxam":::"memory","st","st(1)");const auto numerator_class=classification(status());
    asm volatile("fxch %%st(1)":::"memory","st","st(1)");
    const auto numerator=std::uint8_t(read(memory,0x46ecb4u+numerator_class,1));
    const auto denominator=std::uint8_t(read(memory,0x46ecb4u+denominator_class,1));
    const auto selector=std::bit_cast<std::int8_t>(std::uint8_t((numerator<<2u)|denominator));
    const auto method=word(memory,0x48c8dcu+std::uint32_t(std::int32_t(selector)));
    if(method==0x37dc14u) {
        asm volatile("fxch %%st(1)":::"memory","st","st(1)");
        do {asm volatile("fprem":::"memory","st","st(1)");}while(status()&0x0400u);
        asm volatile("fstp %%st(1)":::"memory","st","st(1)");
    } else if(method==0x331f68u) {
        asm volatile("fstp %%st(0);fstp %%st(0);fldz":::"memory","st","st(1)");
    } else throw NativeCampaignScreenFlashRequiredTu15(0x331f55u,method);
    if(!word(memory,0x48d4f4u)) {
        store_double();
        if(!(original_control&0x20u) && (status()&0x20u))
            throw NativeCampaignScreenFlashRequiredTu15(0x37d66bu,0x3334f0u);
    }
    control(original_control);
}
Scalar curve(SourcePhysicsContactMemory &memory,std::uint16_t kind,Scalar value) {
    const auto zero=_mm_setzero_ps();Scalar one{};
    if(above(zero,value))value=zero;
    else {one=scalar(word(memory,0x46a0d0u));if(above(value,one))value=one;}
    if(!kind)return value;
    const auto scaled=mul(value,scalar(word(memory,0x46a43cu)));
    push_single(bits(scaled));push_double(read(memory,0x46a1d0u,8));
    remainder(memory);
    const auto adjusted=sub(scaled,scalar(word(memory,0x46a178u)));
    const auto fraction=pop_single();
    push_single(bits(adjusted));std::uint32_t index;
    asm volatile("fistpl %0":"=m"(index)::"memory","st");
    one=scalar(word(memory,0x46a0d0u));
    const auto table=0x4782a8u+std::uint32_t(signed_word(kind))*1024u;
    Scalar result;
    if(std::uint16_t(index)==1023u) {
        result=integer(std::int32_t(read(memory,table+1023u,1)));
        result=mul(result,scalar(word(memory,0x46a0c4u)));
    } else {
        const auto scale=scalar(word(memory,0x46a0c4u));
        const auto at=table+std::uint32_t(signed_word(index));
        const auto first=std::int32_t(read(memory,at,1)),second=std::int32_t(read(memory,at+1u,1));
        auto left=mul(integer(first),scale);
        const auto complement=sub(one,scalar(fraction));left=mul(left,complement);
        auto right=mul(integer(second),scale);right=mul(right,scalar(fraction));result=add(left,right);
    }
    if(above(zero,result))return zero;
    return above(result,one)?one:result;
}
}
void native_campaign_screen_flash_sample_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t user,std::uint32_t destination,NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observer) {
    Environment environment(fp);const auto zero=_mm_setzero_ps();
    const auto bank=word(memory,0x51df5cu);
    publish(memory,observer,0x156e4a,destination+4u,0);
    if(signed_word(std::uint32_t(read(memory,bank+4u,2)))!=-1) {
        const auto policy=read(memory,0x51df60u,1);
        publish(memory,observer,0x156e61,destination,1);
        if(policy) {
            const auto game=word(memory,0x4ecc20u);
            const auto mode=std::bit_cast<std::int8_t>(std::uint8_t(read(memory,game+0x12u,1)));
            const auto vector=word(memory,mode<0?0x474c70u:0x474c78u);
            constexpr std::array<std::uint32_t,3> negative{0x156e80,0x156e85,0x156e8b},positive{0x156e9b,0x156ea0,0x156ea6};
            for(unsigned n=0;n<3;++n)publish(memory,observer,mode<0?negative[n]:positive[n],destination+12u+n*4u,word(memory,vector+n*4u));
        } else {
            const auto first=word(memory,0x4f54f8u),second=word(memory,0x4f54fcu);
            publish(memory,observer,0x156eb9,destination+12u,first);
            const auto third=word(memory,0x4f5500u);
            publish(memory,observer,0x156ec0,destination+16u,second);
            publish(memory,observer,0x156ec3,destination+20u,third);
        }
        auto one=scalar(word(memory,0x46a0d0u));
        publish(memory,observer,0x156ece,destination+8u,bits(one));
        const auto duration=signed_word(std::uint32_t(read(memory,bank+4u,2)));
        Scalar amount=one;
        if(duration>0) {
            const auto clock=word(memory,0x51df54u),current=word(memory,clock+8u),start=word(memory,bank);
            amount=divide(integer(std::bit_cast<std::int32_t>(current-start)),integer(duration));
            if(above(zero,amount))amount=zero;else if(above(amount,one))amount=one;
            amount=curve(memory,5,amount);
            one=scalar(word(memory,0x46a0d0u));
        }
        publish(memory,observer,0x156f24,destination+4u,bits(amount));
        if(!read(memory,bank+6u,1))publish(memory,observer,0x156f37,destination+4u,bits(sub(one,amount)));
        if(above(zero,scalar(word(memory,destination+4u)))) {
            publish(memory,observer,0x156f45,destination+4u,0);return;
        }
        amount=scalar(word(memory,destination+4u));
        publish(memory,observer,0x156f59,destination+4u,bits(above(amount,one)?one:amount));return;
    }
    if(user==0xffffffffu)return;
    const auto offset=user*0xa0u;
    const auto duration=signed_word(std::uint32_t(read(memory,bank+offset+0xaeu,2)));
    const auto slot=bank+offset+0x30u;
    if(duration<=0 && !(read(memory,slot+0x88u,1)&1u))return;
    const auto kind=signed_word(std::uint32_t(read(memory,slot+0x18u,2)));
    const auto type=signed_word(std::uint32_t(read(memory,0x47524cu+std::uint32_t(kind)*2u,2)));
    publish(memory,observer,0x156f93,destination,std::uint32_t(type));
    constexpr std::array<std::uint32_t,4> color_sites{0x156f9d,0x156fa2,0x156fa8,0x156fae};
    for(unsigned n=0;n<4;++n)publish(memory,observer,color_sites[n],destination+8u+n*4u,word(memory,slot+0x28u+n*4u));
    auto amount=scalar(word(memory,slot+0x1cu));
    if(above(amount,zero)) {
        const auto remaining=signed_word(std::uint32_t(read(memory,slot+0x7eu,2)));
        const auto response=std::uint16_t(read(memory,slot+0x20u,2));
        amount=divide(integer(remaining),scalar(word(memory,slot+0x1cu)));
        amount=curve(memory,response,amount);amount=mul(amount,scalar(word(memory,slot+0x24u)));
        publish(memory,observer,0x156fd9,destination+4u,bits(amount));
    } else publish(memory,observer,0x156fe3,destination+4u,word(memory,slot+0x24u));
    if(above(zero,scalar(word(memory,destination+4u)))) {
        publish(memory,observer,0x157001,destination+4u,0);return;
    }
    amount=scalar(word(memory,destination+4u));const auto one=scalar(word(memory,0x46a0d0u));
    publish(memory,observer,0x157001,destination+4u,bits(above(amount,one)?one:amount));
}
}
