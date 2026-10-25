// Native kernels for TU1.5 sound manager routines (see the header). They
// read the original game state through the caller's memory and reproduce the
// retail functions' arithmetic under the caller's x87 control word and MXCSR.
#include "halo2/native_tu15_sound_manager.hpp"
#include "halo2/cache.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
#include <cmath>
#include <cstdlib>
#include <map>
#include <sstream>
#include <immintrin.h>

namespace halo2 {
namespace {
using namespace physics_contact_runtime_detail;
using M=SourcePhysicsContactMemory;
using O=SourcePhysicsContactRecordObserver;
using Abi=SourcePhysicsContactRuntimeAbi;

// Retail globals read by these routines.
constexpr std::uint32_t tag_instances=0x4f0414u;    // 16-byte rows; data pointer at +8
constexpr std::uint32_t sound_gestalt=0x52d664u;    // +4: 0x38-byte playback parameter rows
constexpr std::uint32_t random_globals=0x4f3cd8u;   // +4: game random seed
constexpr std::uint32_t sound_globals=0x4ecc1cu;    // +0xc0 count, +0xc4 sound-class tag reference
// Retail .rdata 0x46a160: 0x37800080, 1/65535 as a float.
const float random_word_scale=std::bit_cast<float>(0x37800080u);

std::uint32_t u32(const M &m,std::uint32_t p){std::array<std::byte,4> b;m.read(p,b);return std::bit_cast<std::uint32_t>(b);}
std::uint32_t s16(const M &m,std::uint32_t p){std::array<std::byte,2> b;m.read(p,b);
    return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(b)));}
std::uint32_t s8(const M &m,std::uint32_t p){std::array<std::byte,1> b;m.read(p,b);
    return std::uint32_t(std::int32_t(std::int8_t(std::to_integer<std::uint8_t>(b[0]))));}
std::uint8_t u8(const M &m,std::uint32_t p){std::array<std::byte,1> b;m.read(p,b);return std::to_integer<std::uint8_t>(b[0]);}
float f32(const M &m,std::uint32_t p){return std::bit_cast<float>(u32(m,p));}
// A store published with its original instruction address.
void store(M &m,const O &o,std::uint32_t site,std::uint32_t p,std::uint32_t value) {
    SourcePhysicsContactRecordStore x{site,p,std::vector<std::byte>(4),std::vector<std::byte>(4)};m.read(p,x.before);
    for(unsigned i=0;i<4;++i)x.written[i]=std::byte((value>>(i*8))&255u);
    m.write(p,x.written);if(o)o(x);
}
std::uint32_t pop_return(const M &m,Abi &a,std::uint32_t arguments=0) {
    const auto at=u32(m,a.gp[4]);a.gp[4]+=4u+arguments;return at;
}
// Halo 2's linear congruential generator (seed*0x19660d+0x3c6ef35f).
std::uint32_t next_seed(std::uint32_t seed){return seed*0x19660du+0x3c6ef35fu;}
void set_xmm_scalar(Abi &a,unsigned n,__m128 value) {
    _mm_storeu_si128(reinterpret_cast<__m128i*>(a.xmm[n].data()),_mm_castps_si128(value));
}

// Floating-point environment of the caller for the duration of a kernel:
// its control word, MXCSR and sticky x87 exception flags; results return to
// the ABI the same way the original instructions leave them.
struct Floating {
    Abi &a;FpScope scope;
    Floating(Abi &abi,SourcePhysicsFactoryFloatingPoint fp):a(abi),scope(fp) {
        if(a.active_mmx)throw FormatError("Native sound kernel entered with MMX owning the x87 file");
        if(a.floating_point) {
            const auto p=*a.floating_point;
            if((p.x87_control_word&0x3fu)!=0x3fu||(p.mxcsr&0x1f80u)!=0x1f80u)
                throw FormatError("Native sound kernel requires masked floating-point exceptions");
            asm volatile("fldcw %0"::"m"(p.x87_control_word):"memory");
            _mm_setcsr(p.mxcsr);
        }
        source_load_x87_exception_flags(a.x87_status);
    }
    void push(long double x) {
        if(a.x87_depth>=8)throw FormatError("Native sound kernel x87 stack overflow");
        std::move_backward(a.x87.begin(),a.x87.begin()+a.x87_depth,a.x87.begin()+a.x87_depth+1);
        a.x87[0]=x;++a.x87_depth;
        const auto top=((a.x87_status>>11u)+7u)&7u;
        a.x87_status=std::uint16_t((a.x87_status&~0x3a00u)|(top<<11u)); // a load clears C1
    }
    ~Floating() {
        a.x87_status=std::uint16_t((a.x87_status&~0x3fu)|source_x87_exception_flags());
        std::uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));
        a.floating_point=SourcePhysicsFactoryFloatingPoint{cw,_mm_getcsr()};
    }
};

std::uint32_t playback_row(const M &m,std::uint32_t definition) {
    return s16(m,definition+6u)*0x38u+u32(m,u32(m,sound_gestalt)+4u);
}
std::uint32_t tag_data(const M &m,std::uint32_t datum) {
    return u32(m,((datum&0xffffu)<<4)+u32(m,tag_instances)+8u);
}

// 127E00: a random value between a playback row's signed 16-bit bounds
// (+0x18 lower, +0x1A upper). EAX: sound definition; ECX: seed pointer.
// Returns the value on the x87 stack.
void random_bounds(M &m,Abi &a,SourcePhysicsFactoryFloatingPoint fp,const O &o) {
    const auto row=playback_row(m,a.gp[0]),seed_at=a.gp[1];
    const auto upper=s16(m,row+0x1au),lower=s16(m,row+0x18u);
    const auto seed=next_seed(u32(m,seed_at));
    store(m,o,0x127e3au,seed_at,seed);
    const auto word=seed>>16;
    Floating floating(a,fp);
    // fild word; fmul 1/65535; (upper-lower)*t + lower.
    volatile long double t=static_cast<long double>(std::int32_t(word));
    t=t*static_cast<long double>(random_word_scale);
    volatile long double span=static_cast<long double>(std::int32_t(upper))-static_cast<long double>(std::int32_t(lower));
    volatile long double scaled=t*span;
    volatile long double value=scaled+static_cast<long double>(std::int32_t(lower));
    floating.push(value);
    // ECX's stack slot held the random word when it was popped.
    a.gp[0]=word;a.gp[1]=word;a.gp[2]=upper;
    pop_return(m,a);
}

// 127D70: base + random(0, variance) from a playback row's gain (+0x10 base,
// +0x14 variance). EAX: sound definition. Returns the float in EAX and XMM0.
void random_gain(M &m,Abi &a,SourcePhysicsFactoryFloatingPoint fp,const O &o) {
    const auto row=playback_row(m,a.gp[0]);
    const auto globals=u32(m,random_globals);
    const auto seed=next_seed(u32(m,globals+4u));
    store(m,o,0x127d98u,globals+4u,seed);
    const auto word=seed>>16;
    const auto variance=u32(m,row+0x14u),base=u32(m,row+0x10u);
    Floating floating(a,fp);
    volatile long double product=static_cast<long double>(std::int32_t(word));
    product=product*static_cast<long double>(random_word_scale);
    float t;std::uint16_t status;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1":"=m"(t),"=m"(status):"m"(product):"st");
    a.x87_status=std::uint16_t((a.x87_status&~0x200u)|(status&0x200u));
    // real_random_range(0, variance) + base, in the original single-precision order.
    const auto zero=_mm_set_ss(0.f);
    auto value=_mm_sub_ss(_mm_set_ss(std::bit_cast<float>(variance)),zero);
    value=_mm_mul_ss(value,_mm_set_ss(t));
    value=_mm_add_ss(value,zero);
    value=_mm_add_ss(value,_mm_set_ss(std::bit_cast<float>(base)));
    set_xmm_scalar(a,0,value);set_xmm_scalar(a,1,zero);
    a.gp[0]=std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));a.gp[1]=word;a.gp[2]=variance;
    pop_return(m,a);
}

// 228220: the sound-class row for a class index (stack argument, low 16
// bits) in the globals' sound-class tag. Returns the row in EAX; ret 4.
void sound_class_row(M &m,Abi &a) {
    const auto globals=u32(m,sound_globals);
    const auto reference=u32(m,globals+0xc0u)?u32(m,globals+0xc4u):0u;
    const auto datum=u32(m,reference+4u);
    const auto data=tag_data(m,datum);
    const auto rows=u32(m,data+4u);
    a.gp[0]=s16(m,a.gp[4]+4u)*0x5cu+rows;a.gp[1]=rows;a.gp[2]=data;
    pop_return(m,a,4);
}

// 127B20: a playback scalar. ECX: request (flags byte, override at +0x38);
// EAX: sound tag datum. Sound-class driven tags read their class row +0x1C,
// others their playback row +4. Returns the float in XMM0.
void playback_scalar(M &m,Abi &a,SourcePhysicsFactoryFloatingPoint fp) {
    const auto request=a.gp[1];
    const auto flags=std::uint8_t(u8(m,request)>>3);
    a.gp[2]=(a.gp[2]&~0xffu)|flags;
    float value;
    if(flags&1u)value=f32(m,request+0x38u);
    else {
        const auto sound=tag_data(m,a.gp[0]);
        a.gp[1]=u32(m,tag_instances);
        if(u8(m,sound+1u)&8u) {
            // push edx (class index in DX); call 228220; the return address
            // and argument occupy dead scratch below the caller's frame.
            a.gp[2]=(a.gp[2]&0xffff0000u)|(s8(m,sound+2u)&0xffffu);
            const auto entry=a.gp[4];
            store(m,{},0x127b4du,entry-4u,a.gp[2]);store(m,{},0x127b4eu,entry-8u,0x127b53u);
            a.gp[4]=entry-8u;sound_class_row(m,a);
            value=f32(m,a.gp[0]+0x1cu);
        } else {
            a.gp[1]=u32(m,sound_gestalt);
            const auto rows=u32(m,a.gp[1]+4u);
            a.gp[0]=s16(m,sound+6u)*0x38u;a.gp[2]=rows;
            value=f32(m,a.gp[0]+rows+4u);
        }
    }
    Floating floating(a,fp);
    set_xmm_scalar(a,0,_mm_set_ss(value));
    pop_return(m,a);
}

__m128 xmm_scalar(const Abi &a,unsigned n) {
    return _mm_castsi128_ps(_mm_loadu_si128(reinterpret_cast<const __m128i*>(a.xmm[n].data())));
}
__m128 short_scalar(std::uint32_t value){return _mm_cvtsi32_ss(_mm_setzero_ps(),std::int32_t(value));}
// ucomiss equality as the original tests it (LAHF; TEST AH,44h; JNP):
// true only for ordered, equal operands.
bool ordered_equal(__m128 x,__m128 y) {
    const bool equal=_mm_ucomieq_ss(x,y);
    return equal && !std::isnan(_mm_cvtss_f32(x)) && !std::isnan(_mm_cvtss_f32(y));
}

// 21F960: pitch range selection. ECX: sound definition (+8 first range,
// +0xA range count); AX: previous range, -1 for none; XMM1: pitch. Keeps the
// previous range while the pitch stays inside its bounds and it has
// permutations; otherwise the first range containing the pitch, else the
// range whose bound is nearest. Returns the range in AX (ECX holds the
// search result slot the original pops into it).
void pitch_range(M &m,Abi &a,SourcePhysicsFactoryFloatingPoint fp) {
    Floating floating(a,fp);
    const auto definition=a.gp[1],gestalt=u32(m,sound_gestalt);
    const auto count=std::int32_t(s8(m,definition+0xau));
    const auto previous=std::int16_t(a.gp[0]);
    const auto pitch=xmm_scalar(a,1);
    std::uint32_t result=0xffffffffu;
    const auto finish=[&](std::uint32_t range) {
        a.gp[0]=(a.gp[0]&0xffff0000u)|(range&0xffffu);a.gp[1]=result;pop_return(m,a);
    };
    // Pitch range rows are 12 bytes (+2 parameter row, +0xA permutation
    // count); parameter rows are 10 bytes (+2 lower distance bound,
    // +4 natural pitch, +6/+8 pitch bounds).
    const auto range_row=[&](std::int32_t index){return u32(m,gestalt+0x24u)+std::uint32_t(index)*12u;};
    const auto parameter_row=[&](std::uint32_t range){return u32(m,gestalt+0x1cu)+s16(m,range+2u)*10u;};
    const auto clamp=[&](std::uint32_t parameters) {
        const auto lower=short_scalar(s16(m,parameters+6u));
        if(_mm_comigt_ss(lower,pitch))return lower;
        const auto upper=short_scalar(s16(m,parameters+8u));
        if(_mm_comigt_ss(pitch,upper))return upper;
        return pitch;
    };
    const auto first=std::int32_t(s16(m,definition+8u));
    if(previous!=-1 && previous<count) {
        const auto range=range_row(first+previous);
        if(ordered_equal(clamp(parameter_row(range)),pitch) && std::int16_t(s16(m,range+0xau))>0)
            return finish(std::uint16_t(previous));
    }
    // Retail .rdata 0x45695C: FLT_MAX.
    auto best=_mm_set_ss(std::bit_cast<float>(0x7f7fffffu));
    for(std::int32_t i=0;i<count;++i) {
        const auto range=range_row(first+i);
        if(std::int16_t(s16(m,range+0xau))<=0)continue;
        const auto parameters=parameter_row(range);
        if(ordered_equal(clamp(parameters),pitch))return finish(std::uint32_t(i));
        const auto natural=short_scalar(s16(m,parameters+4u));
        const auto distance=_mm_comigt_ss(pitch,natural)?_mm_sub_ss(pitch,natural):
            _mm_sub_ss(short_scalar(s16(m,parameters+2u)),pitch);
        if(_mm_comigt_ss(best,distance)){result=std::uint32_t(i);best=distance;}
    }
    finish(result);
}

std::uint32_t u16(const M &m,std::uint32_t p){std::array<std::byte,2> b;m.read(p,b);return std::bit_cast<std::uint16_t>(b);}
void store8(M &m,const O &o,std::uint32_t site,std::uint32_t p,std::uint8_t value) {
    SourcePhysicsContactRecordStore x{site,p,std::vector<std::byte>(1),std::vector<std::byte>{std::byte(value)}};
    m.read(p,x.before);m.write(p,x.written);if(o)o(x);
}
// Sound classes with flag bit 5 of +0xA keep no permutation history (the
// shared gate of 21FCC0/21FD20/21FD80/21FDE0/21FED0/21FF70).
bool keeps_history(const M &m,std::uint32_t definition) {
    const auto globals=u32(m,sound_globals);
    const auto reference=u32(m,globals+0xc0u)?u32(m,globals+0xc4u):0u;
    const auto data=tag_data(m,u32(m,reference+4u));
    const auto index=s8(m,definition+2u),rows=u32(m,data+4u);
    return !((u8(m,rows+index*0x5cu+0xau)>>5)&1u);
}

// 21FB20: draw a random unused permutation of a pitch range (12-byte row:
// +4 last, +5 forced, +6 used-mask bit offset, +8 first permutation,
// +0xA permutation count). Refills the
// used mask when exhausted (keeping the last permutation marked so it does
// not repeat), then rerolls permutations that lose their skip-fraction draw,
// up to twice the permutation count.
std::uint32_t random_permutation(M &m,const O &o,Abi &a,std::uint32_t definition,std::uint32_t range_index,
    std::uint32_t &used,std::uint32_t last,std::uint32_t seed_at,bool allow_skip) {
    const auto gestalt=u32(m,sound_gestalt);
    const auto range=u32(m,gestalt+0x24u)+
        std::uint32_t(std::int32_t(s16(m,definition+8u))+std::int32_t(std::int16_t(range_index)))*12u;
    const auto count=s16(m,range+0xau);
    const auto half=_mm_set_ss(0.5f),one=_mm_set_ss(1.f);   // retail .rdata 0x46A0CC, 0x46A0D0
    std::uint32_t attempts=0;
    for(;;) {
        const auto seed=next_seed(u32(m,seed_at));
        store(m,o,0x21fb81u,seed_at,seed);
        const auto candidate=((seed>>16)*count)>>16;
        const auto mask=(1u<<(u8(m,range+0xau)&31u))-1u;
        if(!(~used&mask)) {
            used=0;
            if(std::int16_t(s16(m,range+0xau))>1 && std::uint16_t(last)!=0xffffu)used=1u<<(last&31u);
            last=0xffffffffu;
        }
        const auto bit=1u<<(candidate&31u);
        if(used&bit)continue;
        used|=bit;
        const auto permutation=std::int32_t(s16(m,range+8u))+std::int32_t(std::int16_t(candidate));
        auto skip=_mm_mul_ss(_mm_cvtsi32_ss(_mm_setzero_ps(),std::int32_t(u16(m,u32(m,gestalt+0x2cu)+std::uint32_t(permutation)*16u+2u))),
            _mm_set_ss(random_word_scale));
        if(allow_skip && !_mm_comilt_ss(skip,half))skip=_mm_sub_ss(one,skip);
        const auto roll=next_seed(u32(m,seed_at));
        store(m,o,0x21fc49u,seed_at,roll);
        volatile long double draw=static_cast<long double>(std::int32_t(roll>>16));
        draw=draw*static_cast<long double>(random_word_scale);
        const volatile long double fraction=static_cast<long double>(_mm_cvtss_f32(skip));
        a.x87_status&=std::uint16_t(~0x200u);   // the draw's loads and pops leave C1 clear
        if(draw<fraction && ++attempts!=std::uint32_t(count+count))continue;
        return candidate;
    }
}

// 21FA80: permutation selection for one pitch range. EDI: sound definition;
// stack: pitch range index (ret 4). A forced next permutation (range +5)
// plays once; otherwise a random unused permutation, carrying the range's
// used mask in the gestalt runtime bank (+0x3C at bit offset range +6).
// The last permutation is kept at range +4. Returns AX.
void permutation(M &m,Abi &a,SourcePhysicsFactoryFloatingPoint fp,const O &o) {
    if(a.eflags&0x400u)throw FormatError("Permutation selection entered with the direction flag set");
    Floating floating(a,fp);
    const auto definition=a.gp[7],range_index=u32(m,a.gp[4]+4u);
    const auto gestalt=u32(m,sound_gestalt);
    const auto range=u32(m,gestalt+0x24u)+
        std::uint32_t(std::int32_t(s16(m,definition+8u))+std::int32_t(std::int16_t(range_index)))*12u;
    const bool history=keeps_history(m,definition);
    std::uint32_t chosen;
    if(history && std::int8_t(u8(m,range+5u))!=-1) {
        chosen=s8(m,range+5u);
        store8(m,o,0x21fd6fu,range+5u,0xffu);
        store8(m,o,0x21fd0fu,range+4u,std::uint8_t(chosen));
    } else {
        std::uint32_t used=0;
        const auto count=std::int16_t(s16(m,range+0xau));
        const auto bytes=std::uint32_t(std::int32_t(count)+7)>>3;
        const auto carried=history && count>1;
        const auto bank=[&]{return u32(m,u32(m,sound_gestalt)+0x3cu)+(u16(m,range+6u)>>3);};
        if(carried) {
            if(bytes>4)throw FormatError("Permutation used mask exceeds the original 32-bit slot");
            std::array<std::byte,4> bits{};m.read(bank(),std::span(bits).first(bytes));
            used=std::bit_cast<std::uint32_t>(bits);
        }
        const auto last=history?s8(m,range+4u):0xffffffffu;
        const auto seed_at=u32(m,random_globals)+4u;
        chosen=random_permutation(m,o,a,definition,range_index,used,last,seed_at,false);
        if(carried) {
            const auto at=bank();
            for(std::uint32_t i=0;i<(bytes>>2);++i)store(m,o,0x21ffe8u,at+i*4u,used);
            for(std::uint32_t i=bytes&~3u;i<bytes;++i)store8(m,o,0x21ffefu,at+i,std::uint8_t(used>>(i*8u)));
        }
        if(history)store8(m,o,0x21fd0fu,range+4u,std::uint8_t(chosen));
    }
    a.gp[0]=(a.gp[0]&0xffff0000u)|(chosen&0xffffu);
    pop_return(m,a,4);
}

// What each kernel owes its compiled callers: return registers at their
// width, every register the original preserves, x87/SSE state and stores.
// Scratch registers the original clobbers are not part of the contract.
struct Contract {
    std::uint8_t gp_full,gp_low16,xmm;
};
Contract contract(std::uint32_t entry) {
    switch(entry) {
    case 0x21f960u:return {0xfa,0x01,0xf2};   // AX result; EDX, XMM0/2/3 scratch
    case 0x21fa80u:return {0xf8,0x01,0xe0};   // AX result; ECX, EDX, XMM0-4 scratch
    default:return {0xff,0x00,0xff};
    }
}

// Copy-on-write view used to predict a kernel's result without touching the
// game state; the reference routine then runs on the real memory.
struct Overlay final : M {
    const M &base;std::map<std::uint32_t,std::byte> bytes;
    explicit Overlay(const M &b):base(b){}
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        base.read(p,out);
        for(std::size_t i=0;i<out.size();++i)if(auto f=bytes.find(std::uint32_t(p+i));f!=bytes.end())out[i]=f->second;
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        for(std::size_t i=0;i<in.size();++i)bytes[std::uint32_t(p+i)]=in[i];
    }
};
std::string hex(std::uint64_t v){std::ostringstream s;s<<std::hex<<v;return s.str();}
} // namespace

bool native_tu15_sound_manager_owns_entry(std::uint32_t entry) {
    switch(entry){case 0x127e00u:case 0x127d70u:case 0x127b20u:case 0x228220u:case 0x21f960u:case 0x21fa80u:return true;
    default:return false;}
}
void native_tu15_sound_manager_run(M &m,std::uint32_t entry,Abi &a,SourcePhysicsFactoryFloatingPoint fp,const O &o) {
    switch(entry) {
    case 0x127e00u:random_bounds(m,a,fp,o);return;
    case 0x127d70u:random_gain(m,a,fp,o);return;
    case 0x127b20u:playback_scalar(m,a,fp);return;
    case 0x228220u:sound_class_row(m,a);return;
    case 0x21f960u:pitch_range(m,a,fp);return;
    case 0x21fa80u:permutation(m,a,fp,o);return;
    default:throw FormatError("Sound manager entry has no native kernel");
    }
}

std::string native_tu15_sound_manager_compare(M &m,std::uint32_t entry,Abi &a,SourcePhysicsFactoryFloatingPoint fp,
    const O &o,const std::function<void(M &,Abi &,const O &)> &reference) {
    const auto entry_esp=a.gp[4];
    Overlay predicted(m);auto native=a;
    native_tu15_sound_manager_run(predicted,entry,native,fp,{});
    std::map<std::uint32_t,std::byte> all;
    const O capture=[&](const SourcePhysicsContactRecordStore &x) {
        for(std::size_t i=0;i<x.written.size();++i)all[std::uint32_t(x.address+i)]=x.written[i];
        if(o)o(x);
    };
    reference(m,a,capture);
    // Stores below the caller's stack pointer after the return (callee
    // scratch, its return address and popped arguments) are dead.
    const auto live=[&](std::uint32_t p){return !(p<a.gp[4] && p>=entry_esp-0x1000u);};
    std::map<std::uint32_t,std::byte> stored;
    for(const auto &[p,b]:all)if(live(p))stored[p]=b;
    std::string why;
    const auto differ=[&](const std::string &what,std::uint64_t n,std::uint64_t r) {
        if(why.empty() && n!=r)why=what+" native="+hex(n)+" reference="+hex(r);
    };
    const auto owed=contract(entry);
    for(unsigned i=0;i<8;++i) {
        if(owed.gp_full&(1u<<i))differ("gp"+std::to_string(i),native.gp[i],a.gp[i]);
        else if(owed.gp_low16&(1u<<i))differ("gp"+std::to_string(i)+" low16",native.gp[i]&0xffffu,a.gp[i]&0xffffu);
    }
    for(unsigned i=0;i<8;++i)if(owed.xmm&(1u<<i))
        for(unsigned l=0;l<4;++l)differ("xmm"+std::to_string(i),native.xmm[i][l],a.xmm[i][l]);
    differ("x87 depth",native.x87_depth,a.x87_depth);
    differ("x87 status",native.x87_status,a.x87_status);
    for(unsigned i=0;i<a.x87_depth && i<8;++i) {
        std::array<std::uint64_t,2> nb{},rb{};std::memcpy(nb.data(),&native.x87[i],10);std::memcpy(rb.data(),&a.x87[i],10);
        differ("st"+std::to_string(i)+" low",nb[0],rb[0]);differ("st"+std::to_string(i)+" high",nb[1]&0xffffu,rb[1]&0xffffu);
    }
    if(native.floating_point && a.floating_point) {
        differ("x87 control",native.floating_point->x87_control_word,a.floating_point->x87_control_word);
        differ("mxcsr",native.floating_point->mxcsr,a.floating_point->mxcsr);
    }
    std::map<std::uint32_t,std::byte> wrote;
    for(const auto &[p,b]:predicted.bytes)if(live(p))wrote[p]=b;
    if(why.empty() && wrote!=stored)why="stores differ: native "+std::to_string(wrote.size())+" bytes, reference "+std::to_string(stored.size());
    if(!why.empty())why="sound manager "+hex(entry)+": "+why;
    return why;
}

NativeTu15SoundManagerMode native_tu15_sound_manager_mode() {
    static const auto mode=[]{
        const auto set=[](const char *name){const auto *v=std::getenv(name);return v && *v && *v!='0';};
        if(set("HALO2_REFERENCE_SOUND_MANAGER"))return NativeTu15SoundManagerMode::reference;
        if(set("HALO2_SOUND_MANAGER_ORACLE"))return NativeTu15SoundManagerMode::oracle;
        return NativeTu15SoundManagerMode::native;
    }();
    return mode;
}
} // namespace halo2
