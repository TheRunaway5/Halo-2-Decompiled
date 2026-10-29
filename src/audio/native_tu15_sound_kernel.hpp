#pragma once
// Shared support for native TU1.5 sound kernels: typed reads of the original
// game state, stores published with their original instruction address, the
// caller's floating-point environment, and the kernel registry each kernel
// group contributes to (see native_tu15_sound_manager.hpp).
#include "halo2/cache.hpp"
#include "halo2/native_tu15_sound_manager.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
#include <cmath>
#include <span>
#include <immintrin.h>

namespace halo2::native_tu15_sound {
using namespace physics_contact_runtime_detail;
using M=SourcePhysicsContactMemory;
using O=SourcePhysicsContactRecordObserver;
using Abi=SourcePhysicsContactRuntimeAbi;
using FP=SourcePhysicsFactoryFloatingPoint;

// Retail globals read by the sound manager.
inline constexpr std::uint32_t tag_instances=0x4f0414u;   // 16-byte rows; data pointer at +8
inline constexpr std::uint32_t sound_gestalt=0x52d664u;   // +4 playback rows (0x38), +0x1C parameters,
                                                          // +0x24 pitch ranges, +0x2C permutations,
                                                          // +0x3C runtime used masks
inline constexpr std::uint32_t random_globals=0x4f3cd8u;  // +4: game random seed
inline constexpr std::uint32_t sound_globals=0x4ecc1cu;   // +0xC0 count, +0xC4 sound-class tag reference
// Retail .rdata 0x46A160: 0x37800080, 1/65535 as a float.
inline const float random_word_scale=std::bit_cast<float>(0x37800080u);

inline std::uint32_t u32(const M &m,std::uint32_t p){std::array<std::byte,4> b;m.read(p,b);return std::bit_cast<std::uint32_t>(b);}
inline std::uint32_t u16(const M &m,std::uint32_t p){std::array<std::byte,2> b;m.read(p,b);return std::bit_cast<std::uint16_t>(b);}
inline std::uint32_t s16(const M &m,std::uint32_t p){std::array<std::byte,2> b;m.read(p,b);
    return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(b)));}
inline std::uint8_t u8(const M &m,std::uint32_t p){std::array<std::byte,1> b;m.read(p,b);return std::to_integer<std::uint8_t>(b[0]);}
inline std::uint32_t s8(const M &m,std::uint32_t p){return std::uint32_t(std::int32_t(std::int8_t(u8(m,p))));}
inline float f32(const M &m,std::uint32_t p){return std::bit_cast<float>(u32(m,p));}
// Stores published with their original instruction address.
inline void store_bytes(M &m,const O &o,std::uint32_t site,std::uint32_t p,std::uint32_t value,unsigned bytes) {
    SourcePhysicsContactRecordStore x{site,p,std::vector<std::byte>(bytes),std::vector<std::byte>(bytes)};m.read(p,x.before);
    for(unsigned i=0;i<bytes;++i)x.written[i]=std::byte((value>>(i*8))&255u);
    m.write(p,x.written);if(o)o(x);
}
inline void store(M &m,const O &o,std::uint32_t site,std::uint32_t p,std::uint32_t value){store_bytes(m,o,site,p,value,4);}
inline void store16(M &m,const O &o,std::uint32_t site,std::uint32_t p,std::uint32_t value){store_bytes(m,o,site,p,value,2);}
inline void store8(M &m,const O &o,std::uint32_t site,std::uint32_t p,std::uint32_t value){store_bytes(m,o,site,p,value,1);}
// RET n: pops the return address and n bytes of stack arguments.
inline std::uint32_t pop_return(const M &m,Abi &a,std::uint32_t arguments=0) {
    const auto at=u32(m,a.gp[4]);a.gp[4]+=4u+arguments;return at;
}
inline std::uint32_t argument(const M &m,const Abi &a,unsigned n){return u32(m,a.gp[4]+4u+n*4u);}
// Halo 2's linear congruential generator.
inline std::uint32_t next_seed(std::uint32_t seed){return seed*0x19660du+0x3c6ef35fu;}
inline __m128 xmm(const Abi &a,unsigned n) {
    return _mm_castsi128_ps(_mm_loadu_si128(reinterpret_cast<const __m128i*>(a.xmm[n].data())));
}
inline void set_xmm(Abi &a,unsigned n,__m128 value) {
    _mm_storeu_si128(reinterpret_cast<__m128i*>(a.xmm[n].data()),_mm_castps_si128(value));
}
inline __m128 int_scalar(std::uint32_t value){return _mm_cvtsi32_ss(_mm_setzero_ps(),std::int32_t(value));}
// UCOMISS equality as compiled tests use it (LAHF; TEST AH,44h; JNP): true
// only for ordered, equal operands.
inline bool ordered_equal(__m128 x,__m128 y) {
    const bool equal=_mm_ucomieq_ss(x,y);
    return equal && !std::isnan(_mm_cvtss_f32(x)) && !std::isnan(_mm_cvtss_f32(y));
}

// The caller's floating-point environment for the duration of a kernel: its
// control word, MXCSR and sticky x87 exception flags. On exit the state
// returns to the ABI the same way the original instructions leave it.
struct Floating {
    Abi &a;FpScope scope;
    Floating(Abi &abi,FP fp):a(abi),scope(fp) {
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
    Floating(const Floating&)=delete;
    // An x87 load (FLD/FILD): TOP decrements and C1 clears.
    void push(long double x) {
        if(a.x87_depth>=8)throw FormatError("Native sound kernel x87 stack overflow");
        std::move_backward(a.x87.begin(),a.x87.begin()+a.x87_depth,a.x87.begin()+a.x87_depth+1);
        a.x87[0]=x;++a.x87_depth;
        const auto top=((a.x87_status>>11u)+7u)&7u;
        a.x87_status=std::uint16_t((a.x87_status&~0x3a00u)|(top<<11u));
    }
    long double pop() {
        if(!a.x87_depth)throw FormatError("Native sound kernel x87 stack underflow");
        const auto x=a.x87[0];
        std::move(a.x87.begin()+1,a.x87.begin()+a.x87_depth,a.x87.begin());--a.x87_depth;
        const auto top=((a.x87_status>>11u)+1u)&7u;
        a.x87_status=std::uint16_t((a.x87_status&~0x3800u)|(top<<11u));
        return x;
    }
    void clear_c1(){a.x87_status&=std::uint16_t(~0x200u);}
    // FSTP m32: the rounded single and the C1 (round-up) bit it reports.
    float store_single(long double x) {
        float result;std::uint16_t status;
        asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1":"=m"(result),"=m"(status):"m"(x):"st");
        a.x87_status=std::uint16_t((a.x87_status&~0x200u)|(status&0x200u));
        return result;
    }
    ~Floating() {
        a.x87_status=std::uint16_t((a.x87_status&~0x3fu)|source_x87_exception_flags());
        std::uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));
        a.floating_point=FP{cw,_mm_getcsr()};
    }
};

inline std::uint32_t tag_data(const M &m,std::uint32_t datum) {
    return u32(m,((datum&0xffffu)<<4)+u32(m,tag_instances)+8u);
}
inline std::uint32_t playback_row(const M &m,std::uint32_t definition) {
    return s16(m,definition+6u)*0x38u+u32(m,u32(m,sound_gestalt)+4u);
}

// What a kernel owes its compiled callers: return registers at their width,
// every register the original preserves, x87/SSE state and stores. Scratch
// registers the original clobbers (and EFLAGS) are not part of it.
struct Contract {
    std::uint8_t gp_full=0xff,gp_low16=0,xmm=0xff;
    std::uint8_t gp_low8=0;
};
using Services=SourcePhysicsContactRuntimeServices;
struct Kernel {
    std::uint32_t entry;
    void (*run)(M &,Abi &,FP,const O &,const Services &);
    Contract contract{};
    // Calls a routine that is not native (through Services::required_owner);
    // the live oracle cannot predict such a kernel over a copy-on-write view.
    bool calls_out=false;
};
// Calls a not-yet-native original routine the way the original CALL does:
// pushes the stack arguments (last first) and the return address, and lets
// the live world run the callee. A cdecl caller then pops its arguments.
inline void call_out(M &m,Abi &a,const Services &services,const O &o,std::uint32_t site,std::uint32_t target,
    std::uint32_t return_address,std::span<const std::uint32_t> arguments,bool callee_pops) {
    if(!services.required_owner)throw FormatError("Native sound kernel callee has no live owner");
    for(auto n=arguments.size();n-->0;){a.gp[4]-=4u;store(m,{},site,a.gp[4],arguments[n]);}
    a.gp[4]-=4u;store(m,{},site,a.gp[4],return_address);
    services.required_owner(m,{site,target,false},a,o);
    if(!callee_pops)a.gp[4]+=std::uint32_t(arguments.size()*4u);
}
// Kernel groups; native_tu15_sound_manager.cpp aggregates them.
std::span<const Kernel> sound_manager_kernels();
std::span<const Kernel> gain_kernels();             // native_tu15_sound_kernels_gain.cpp
std::span<const Kernel> properties_kernels();       // native_tu15_sound_kernels_properties.cpp
std::span<const Kernel> listener_kernels();         // native_tu15_sound_kernels_listener.cpp
std::span<const Kernel> resource_kernels();         // native_tu15_sound_kernels_resources.cpp
std::span<const Kernel> startup_kernels();          // native_tu15_sound_kernels_startup.cpp
std::span<const Kernel> object_loop_kernels();      // native_tu15_sound_kernels_object_loop.cpp
std::span<const Kernel> campaign_audio_kernels();   // native_tu15_sound_kernels_campaign_audio.cpp
std::span<const Kernel> unit_speech_kernels();      // native_tu15_sound_kernels_unit_speech.cpp
} // namespace halo2::native_tu15_sound
