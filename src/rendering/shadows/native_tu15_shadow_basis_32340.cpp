#include "halo2/native_tu15_shadow_basis_32340.hpp"
#include "halo2/native_campaign_ai_vector_math_tu15.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <bit>
#include <stdexcept>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <emmintrin.h>
#endif

namespace halo2 {
namespace {
struct Diagnostics {
    bool enabled=[] {const auto* v=std::getenv("HALO2_NATIVE_SHADOW_BASIS_STATS");return v&&*v&&*v!='0';}();
    std::atomic<std::uint64_t> gates{},admitted{},basis{},tail{};
    ~Diagnostics(){if(enabled)std::fprintf(stderr,"Native32340 numeric stats: gates=%llu admitted=%llu basis=%llu tail=%llu\n",
        static_cast<unsigned long long>(gates.load()),static_cast<unsigned long long>(admitted.load()),
        static_cast<unsigned long long>(basis.load()),static_cast<unsigned long long>(tail.load()));}
};
Diagnostics& diagnostics(){static Diagnostics stats;return stats;}
std::uint32_t read(const SourcePhysicsContactMemory& memory,std::uint32_t address) {
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
    return value;
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using Scalar=__m128;
Scalar scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(Scalar value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
Scalar multiply(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar add(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar subtract(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool below(Scalar a,Scalar b){unsigned char v;asm volatile("comiss %2,%1; setb %0":"=qm"(v):"x"(a),"x"(b):"cc","memory");return v;}
bool above(Scalar a,Scalar b){unsigned char v;asm volatile("comiss %2,%1; seta %0":"=qm"(v):"x"(a),"x"(b):"cc","memory");return v;}
Scalar source_abs(Scalar value){const auto zero=scalar(0);return below(value,zero)?subtract(zero,value):value;}
#endif
}
NativeTu15ShadowGateResult native_tu15_shadow_gates_32340(const SourcePhysicsContactMemory& memory,
    const NativeTu15ShadowGateInput& input,NativeTu15PhysicsFrameFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    auto& stats=diagnostics();if(stats.enabled)++stats.gates;
    const auto minimum=read(memory,0x46a45c),zero=read(memory,0x46a0e8),cutoff=read(memory,0x46a4c0);
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    NativeTu15ShadowGateResult out;
    out.fade=bits(multiply(scalar(input.owner_fade),scalar(input.history_fade)));
    out.minimum_radius=above(scalar(input.radius),scalar(minimum))?input.radius:minimum;
    out.opacity=input.cached_opacity;
    out.cached_positive=above(scalar(input.cached_opacity),scalar(zero));
    if(out.cached_positive) {
        out.opacity=bits(multiply(scalar(input.cached_opacity),scalar(out.fade)));
        out.admitted=above(scalar(out.opacity),scalar(cutoff));
    }
    if(stats.enabled&&out.admitted)++stats.admitted;
    return out;
#else
    (void)memory;(void)input;(void)fp;
    throw std::runtime_error("Exact native TU1.5 shadow admission backend unavailable");
#endif
}
NativeTu15ShadowBasisResult native_tu15_shadow_basis_32340(const SourcePhysicsContactMemory& memory,
    const NativeTu15ShadowBasisInput& input,NativeTu15PhysicsFrameFp& fp,const NativeTu15ShadowBasisStore& publish) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    auto& stats=diagnostics();if(stats.enabled)++stats.basis;
    NativeTu15ShadowBasisResult out;out.direction=input.direction;
    const auto store=[&](std::uint32_t site,std::uint32_t address,std::uint32_t value){if(publish)publish(site,address,value);};
    store(0x3245b,0x48c53c,input.center[0]);store(0x32463,0x48c540,input.center[1]);
    store(0x3246c,0x48c530,input.center[0]);store(0x32471,0x48c544,input.center[2]);
    store(0x32477,0x48c534,input.center[1]);store(0x3247d,0x48c538,input.center[2]);
    for(unsigned n=0;n<3;++n)store(std::array{0x32485u,0x3248eu,0x3249bu}[n],0x48c558u+4*n,input.direction[n]);
    const auto normalize=[&](auto& value,std::uint32_t address,bool& did_normalize) {
        native_campaign_ai_normalize_vector_value_tu15(memory,value,fp,[&](unsigned n,std::uint32_t v){
            did_normalize=true;store(std::array{0x30c1bu,0x30c2cu,0x30c31u}[n],address+4*n,v);
        });
    };
    normalize(out.direction,0x48c558,out.direction_normalized);
    const auto two=read(memory,0x46a11c),threshold=read(memory,0x46a194);
    const auto first=read(memory,0x474d0c),second=read(memory,0x474d14);
    std::array<std::uint32_t,3> b1{},b2{};
    for(unsigned n=0;n<3;++n){b1[n]=read(memory,first+4*n);b2[n]=read(memory,second+4*n);}
    {
        detail::NativeTu15ArithmeticEnvironment environment(fp);
        const auto x=scalar(out.direction[0]),y=scalar(out.direction[1]),z=scalar(out.direction[2]);
        // COMISS/JB includes unordered; the strict ordered max chooses Y on
        // equality/unordered, then the original recomputes its absolute value.
        const auto ax=source_abs(x),ay=source_abs(y);
        const auto horizontal=source_abs(above(ax,ay)?x:y);
        const auto axial=scalar(input.axial);
        const auto end=add(multiply(axial,scalar(two)),horizontal);
        out.near_offset=bits(subtract(end,axial));
        out.far_offset=bits(subtract(add(end,axial),axial));
        out.width=bits(multiply(scalar(input.transverse),scalar(two)));
        unsigned char second_basis;
        asm volatile("flds %1\n\tfmuls %2\n\tflds %3\n\tfmuls %4\n\tfaddp %%st,%%st(1)\n\t"
            "flds %5\n\tfmuls %6\n\tfaddp %%st,%%st(1)\n\tfabs\n\tflds %7\n\t"
            "fcomip %%st(1),%%st\n\tfstp %%st(0)\n\tsetbe %0"
            :"=qm"(second_basis):"m"(out.direction[2]),"m"(b1[2]),"m"(out.direction[0]),"m"(b1[0]),
            "m"(out.direction[1]),"m"(b1[1]),"m"(threshold):"cc","memory","st","st(1)");
        out.second_basis=second_basis;const auto& b=out.second_basis?b2:b1;
        const auto bx=scalar(b[0]),by=scalar(b[1]),bz=scalar(b[2]);
        // Both branches have distinct source operand order for NaN payloads.
        if(!out.second_basis) {
            out.up[2]=bits(subtract(multiply(by,x),multiply(bx,y)));
            out.up[1]=bits(subtract(multiply(z,bx),multiply(bz,x)));
            out.up[0]=bits(subtract(multiply(bz,y),multiply(z,by)));
        } else {
            out.up[2]=bits(subtract(multiply(by,x),multiply(bx,y)));
            out.up[1]=bits(subtract(multiply(bx,z),multiply(bz,x)));
            out.up[0]=bits(subtract(multiply(bz,y),multiply(by,z)));
        }
    }
    store(0x32529,0x48c5e4,out.near_offset);store(0x32547,0x48c5e8,out.far_offset);store(0x32553,0x48c5d8,out.width);
    store(out.second_basis?0x32613:0x325bc,0x48c568,out.up[1]);
    store(out.second_basis?0x3261b:0x325c4,0x48c56c,out.up[2]);store(0x32628,0x48c564,out.up[0]);
    normalize(out.up,0x48c564,out.up_normalized);return out;
#else
    (void)memory;(void)input;(void)fp;(void)publish;
    throw std::runtime_error("Exact native TU1.5 shadow basis backend unavailable");
#endif
}
NativeTu15ShadowProjectionTail native_tu15_shadow_projection_tail_32340(const SourcePhysicsContactMemory& memory,
    const std::array<std::uint32_t,3>& center,const NativeTu15ShadowBasisResult& basis,
    const std::array<std::uint32_t,31>& projection,NativeTu15PhysicsFrameFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    auto& stats=diagnostics();if(stats.enabled)++stats.tail;
    const auto half=read(memory,0x46a0cc);
    NativeTu15ShadowProjectionTail out;out.virtual_distance=projection[10];
    for(unsigned n=0;n<3;++n)out.virtual_eye[n]=projection[11+n];
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    const auto h=scalar(half);const auto& d=basis.direction;const auto& u=basis.up;
    out.half_extent={bits(multiply(scalar(basis.far_offset),h)),bits(multiply(scalar(projection[22]),h)),bits(multiply(scalar(projection[23]),h))};
    out.left[0]=bits(subtract(multiply(scalar(u[1]),scalar(d[2])),multiply(scalar(u[2]),scalar(d[1]))));
    out.left[1]=bits(subtract(multiply(scalar(u[2]),scalar(d[0])),multiply(scalar(d[2]),scalar(u[0]))));
    out.left[2]=bits(subtract(multiply(scalar(u[0]),scalar(d[1])),multiply(scalar(u[1]),scalar(d[0]))));
    const auto negative_half=subtract(scalar(0),scalar(out.half_extent[0]));
    for(unsigned n=0;n<3;++n) {
        const auto c=scalar(center[n]);
        const auto residual=subtract(c,add(multiply(scalar(d[n]),negative_half),c));
        out.volume_center[n]=bits(n==1?add(c,residual):add(residual,c));
    }
    asm volatile("flds %1\n\tfmuls %1\n\tflds %2\n\tfmuls %2\n\tfaddp %%st,%%st(1)\n\t"
        "flds %3\n\tfmuls %3\n\tfaddp %%st,%%st(1)\n\tfsqrt\n\tfstps %0"
        :"=m"(out.query_radius):"m"(out.half_extent[2]),"m"(out.half_extent[1]),"m"(out.half_extent[0])
        :"memory","st","st(1)");
    return out;
#else
    (void)memory;(void)center;(void)basis;(void)projection;(void)fp;
    throw std::runtime_error("Exact native TU1.5 shadow projection tail backend unavailable");
#endif
}
}
