#include "halo2/native_tu15_draw_units.hpp"
#include "halo2/native_tu15_sphere_rectangle.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <bit>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <emmintrin.h>
#endif

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory& m,std::uint32_t at,unsigned size=4) {
    std::array<std::byte,4> bytes{};m.read(at,std::span(bytes).first(size));std::uint32_t value{};
    for(unsigned i=0;i<size;++i)value|=std::to_integer<unsigned>(bytes[i])<<(8*i);return value;
}
void put(SourcePhysicsContactMemory& m,std::uint32_t at,std::uint32_t value,unsigned size=4) {
    std::array<std::byte,4> bytes{};for(unsigned i=0;i<size;++i)bytes[i]=std::byte(value>>(8*i));
    m.write(at,std::span(bytes).first(size));
}
class Stores {
    SourcePhysicsContactMemory& memory_;
    const SourcePhysicsContactRecordObserver& observer_;
public:
    Stores(SourcePhysicsContactMemory& memory,const SourcePhysicsContactRecordObserver& observer):memory_(memory),observer_(observer){}
    void operator()(std::uint32_t pc,std::uint32_t at,std::uint32_t value,unsigned size=4)const {
        if(!observer_){put(memory_,at,value,size);return;}
        SourcePhysicsContactRecordStore event{pc,at,std::vector<std::byte>(size),std::vector<std::byte>(size)};
        memory_.read(at,event.before);
        for(unsigned i=0;i<size;++i)event.written[i]=std::byte(value>>(8*i));
        memory_.write(at,event.written);observer_(event);
    }
};
std::int32_t signed_word(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
std::int32_t signed_half(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
std::uint32_t selector(std::uint32_t flags){return (flags>>12)&0xffffff01u;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using Scalar=__m128;
Scalar scalar(std::uint32_t v){return _mm_castsi128_ps(_mm_cvtsi32_si128(signed_word(v)));}
Scalar multiply(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
std::int32_t truncate(Scalar v){std::int32_t out;asm volatile("cvttss2si %1,%0":"=r"(out):"x"(v):"memory");return out;}
std::int32_t floor_word(const SourcePhysicsContactMemory& m,Scalar value) {
    const auto integral=truncate(value);auto rounded=scalar(0);
    asm volatile("cvtsi2ss %1,%0":"+x"(rounded):"r"(integral):"memory");
    asm volatile("cmpneqss %1,%0":"+x"(rounded):"x"(value):"memory");
    const auto zero=scalar(read(m,0x46a0e8u));
    asm volatile("cmpltss %1,%0":"+x"(value):"x"(zero):"memory");
    unsigned mask;asm volatile("andps %2,%1\n\tmovmskps %1,%0":"=r"(mask),"+x"(value):"x"(rounded):"memory");
    return signed_word(std::bit_cast<std::uint32_t>(integral)-mask);
}
std::uint32_t coverage(const SourcePhysicsContactMemory& m,std::uint32_t value,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15DrawUnitServices& services) {
    const auto factor=read(m,0x46a130u);std::uint32_t scaled;
    {
        detail::NativeTu15ArithmeticEnvironment environment(fp);
        scaled=std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(multiply(scalar(value),scalar(factor)))));
    }
    if(services.argument_write) {
        services.argument_write(0x2cddbu,12,scaled);services.argument_write(0x2cde1u,6,scaled);
    }
    if(native_tu15_floor_16ac0(m,scaled,fp)<0)return 0;
    if(services.argument_write)services.argument_write(0x2ce13u,6,scaled);
    if(native_tu15_floor_16ac0(m,scaled,fp)>255)return 255;
    return std::uint32_t(native_tu15_floor_16ac0(m,scaled,fp));
}
struct Fade {std::uint32_t value;bool negative;};
Fade fade(const SourcePhysicsContactMemory& m,std::uint32_t value,NativeTu15PhysicsFrameFp& fp) {
    const auto factor=read(m,0x46a26cu);detail::NativeTu15ArithmeticEnvironment environment(fp);
    const auto integer=truncate(multiply(scalar(value),scalar(factor)));
    return {integer<0?0u:integer>255?255u:std::uint32_t(integer),integer<0};
}
#endif
}
void native_tu15_draw_units_begin_2cb10(SourcePhysicsContactMemory& m) {
    const auto context=read(m,0x4cd3a0u),mode=read(m,context+0x2a78u);
    if(mode){put(m,0x4cd398u,read(m,0x4cd39cu));return;}
    put(m,0x4cd39cu,0);put(m,0x4cd398u,0);put(m,0x4cdd48u,0);
    for(unsigned i=0;i<9;++i)put(m,0x4cdd4cu+4u*i,0);
    for(unsigned i=0;i<279;++i)put(m,0x4cdd70u+4u*i,read(m,0x4cdd4cu+4u*i));
    put(m,0x4ce1ccu,0);
}
NativeTu15MaskAllocation native_tu15_mask_arena_135dc0(SourcePhysicsContactMemory& m,
    std::uint32_t count,std::uint32_t context,const SourcePhysicsContactRecordObserver& observe) {
    // The original owner receives the full ECX. Its callers zero-extend a
    // section word; keeping wrapping32-bit addition also preserves other inputs.
    if(signed_word(count)<=0)return {};
    const auto words=std::uint32_t(signed_word(count+31u)>>5);
    const auto used=read(m,context+0xa5cu,2),next=used+words,base=context+0x25cu;
    if(signed_word(next)>=512)return {base,words};
    Stores(m,observe)(0x135df5u,context+0xa5cu,next,2);return {base+4u*used,words};
}
std::int32_t native_tu15_floor_16ac0(const SourcePhysicsContactMemory& m,std::uint32_t value,NativeTu15PhysicsFrameFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    detail::NativeTu15ArithmeticEnvironment environment(fp);return floor_word(m,scalar(value));
#else
    (void)m;(void)value;(void)fp;throw std::runtime_error("Exact native16AC0 arithmetic backend unavailable");
#endif
}
std::int16_t native_tu15_draw_unit_2cbd0(SourcePhysicsContactMemory& m,
    const NativeTu15DrawUnitCall& call,NativeTu15PhysicsFrameFp& fp,const NativeTu15DrawUnitServices& services,
    const SourcePhysicsContactRecordObserver& observe,NativeTu15DrawUnitDetails* details) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    constexpr auto none=0xffffffffu;const auto& a=call.stack;
    NativeTu15DrawUnitDetails state;const Stores store(m,observe);
    const auto ordinal=read(m,0x4cd398u);store(0x2cbe7u,0x4cd398u,ordinal+1u);
    const auto refused=[&](bool early) {
        state.early_refusal=early;state.geometry_refusal=!early;
        store(early?0x2cfc5u:0x2cfafu,0x4cd398u,read(m,0x4cd398u)-1u);
        if(details)*details=state;return std::int16_t(-1);
    };
    if(signed_word(ordinal)>=850 || call.geometry==none)return refused(true);
    const auto unit=0x4c6958u+32u*std::uint32_t(signed_half(ordinal));
    store(0x2cc0cu,unit+4u,a[1]);store(0x2cc13u,unit+8u,a[2]);
    const auto old_flags=read(m,unit);
    auto flags=(old_flags&0xe0000000u)|(a[0]&0x1fffffu)|((call.distance_byte&255u)<<21);
    store(0x2cc40u,unit+0x1au,a[6],1);
    auto packed=((((call.instance&2047u)<<9)|(call.geometry&511u))<<9)|((a[10]&31u)<<4)|(a[3]&15u);
    const auto old_packed=read(m,unit+0xcu);
    store(0x2cc62u,unit,flags);store(0x2cc68u,unit+0x10u,a[4]);store(0x2cc7au,unit+0x1cu,a[7]);
    const auto old_region=read(m,unit+0x1bu,1),old_region_again=read(m,unit+0x1bu,1);
    packed|=old_packed&0xe0000000u;store(0x2cc85u,unit+0xcu,packed);
    store(0x2cc96u,unit+0x1bu,old_region_again^((old_region^a[18])&15u),1);
    auto section=(packed>>4)&31u;
    if(section!=31u) {
        if(read(m,0x4ce1ccu)<=section+1u)store(0x2cca6u,0x4ce1ccu,section+1u);
        section=(read(m,unit+0xcu)>>4)&31u;store(0x2ccbbu,0x4cdd6cu+36u*section,a[11]);
    }
    section=(read(m,unit+0xcu)>>4)&31u;
    if(section!=31u && a[8]!=none) {
        const auto target=0x4cdd4cu+36u*section;store(0x2ccf3u,target,a[17]);
        for(unsigned i=0;i<3;++i)store(std::array{0x2ccfeu,0x2cd03u,0x2cd09u}[i],target+4u+4u*i,read(m,a[19]+4u*i));
        store(0x2cd10u,target+0x20u,a[11]);
        if((a[12]&255u)||read(m,0x515ef0u,1)) {
            const auto lower=read(m,0x44aac8u);store(0x2cd74u,target+0x10u,lower);store(0x2cd79u,target+0x18u,lower);
            const auto upper=read(m,0x45695cu);store(0x2cd86u,target+0x14u,upper);store(0x2cd8bu,target+0x1cu,upper);
        } else if((a[13]&255u)||read(m,0x515ef1u,1)) {
            const auto upper=read(m,0x45695cu);store(0x2cd58u,target+0x10u,upper);store(0x2cd5du,target+0x18u,upper);
            const auto lower=read(m,0x44aac8u);store(0x2cd86u,target+0x14u,lower);store(0x2cd8bu,target+0x1cu,lower);
        } else {
            state.rectangle=true;
            if(services.rectangle)services.rectangle(a[19],target+0x10u,a[20],fp);
            else native_tu15_sphere_rectangle_30270(m,a[19],0x4c6634u,0x4c65c0u,target+0x10u,a[20],fp,observe);
        }
    }
    store(0x2cd9cu,unit+0x18u,a[5],2);store(0x2cda0u,0x4cd6f6u+ordinal,255,1);
    if(a[8]!=none) {
        const auto slot=read(m,0x4cdd48u);
        if(signed_word(slot)<192) {
            store(0x2cdb5u,0x4cd6f6u+ordinal,slot,1);
            const auto current_slot=read(m,0x4cdd48u);store(0x2cdc0u,0x4cda48u+4u*current_slot,a[8]);
            store(0x2cdc7u,0x4cdd48u,read(m,0x4cdd48u)+1u);
        }
    }
    store(0x2ce55u,0x4cd3a4u+ordinal,coverage(m,a[9],fp,services),1);
    const auto object_context=read(m,unit+0x10u);
    if(object_context && a[8]!=none) {
        const bool primary=(a[14]&255u)!=0;
        const auto opacity=fade(m,a[primary?15u:16u],fp);
        store(primary?(opacity.negative?0x2ce87u:0x2ce98u):0x2cec3u,object_context+(primary?0x76u:0x77u),opacity.value,1);
    }
    const auto metadata=native_tu15_render_section_44930(m,ordinal,std::uint8_t(selector(a[0])));
    const auto classification=read(m,metadata+0x14u,2),current_packed=read(m,unit+0xcu);
    packed=(current_packed&0x1fffffffu)|(classification<<29);
    const auto context=read(m,unit+0x10u);store(0x2cef1u,unit+0xcu,packed);
    const auto owner=context?read(m,context):none,model=read(m,unit+4u);
    const auto lighting=native_tu15_render_lighting_2cfe0(m,owner,model,(packed>>18)&2047u,fp,services.lighting);
    const auto current_flags=read(m,unit),current_model=read(m,unit+4u);
    flags=(current_flags&0x1fffffffu)|(lighting<<29);store(0x2cf33u,unit,flags);
    const auto geometry=native_tu15_render_geometry_449d0(m,ordinal,std::uint8_t(current_model==none),std::uint8_t(selector(flags)),fp,services.lighting.request);
    if(!geometry)return refused(false);
    const auto current_context=read(m,0x4cd3a0u),mode=read(m,current_context+0x2a78u);
    if(!mode)store(0x2cf4eu,0x4cd39cu,read(m,0x4cd39cu)+1u);
    flags=read(m,unit);
    state.return_word=(flags&0xffff0000u)|(ordinal&65535u);
    if(flags&0x40u) {
        state.mask_requested=true;
        const auto current=native_tu15_render_section_44930(m,ordinal,std::uint8_t(selector(flags)));
        store(0x2cf6au,unit+0x14u,0);const auto count=read(m,current+0x24u,2);
        if(count)store(0x2cf84u,unit+0x14u,native_tu15_mask_arena_135dc0(m,count,current_context,observe).address);
        const auto mask=read(m,unit+0x14u);state.return_word=(mask&0xffff0000u)|(ordinal&65535u);
        if(!mask)store(0x2cf99u,unit,(read(m,unit)&~0x40u)|1u);
    }
    if(details)*details=state;
    return std::bit_cast<std::int16_t>(std::uint16_t(ordinal));
#else
    (void)m;(void)call;(void)fp;(void)services;(void)observe;(void)details;throw std::runtime_error("Exact native2CBD0 arithmetic backend unavailable");
#endif
}
}
