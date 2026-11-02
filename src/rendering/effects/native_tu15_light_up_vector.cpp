#include "halo2/native_tu15_light_up_vector.hpp"
#include "halo2/native_campaign_ai_vector_math_tu15.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory& memory, std::uint32_t address) {
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using Scalar=__m128;
Scalar scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(Scalar value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
Scalar divide(Scalar a,Scalar b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar multiply(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar subtract(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
#endif
}
NativeTu15LightUpResult native_tu15_light_orthonormal_up(const SourcePhysicsContactMemory& constants,
    std::array<std::uint32_t,3> up,const std::array<std::uint32_t,3>& forward,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15LightUpStore& publish) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto epsilon=read(constants,0x46a0ecu);
    {
        detail::NativeTu15ArithmeticEnvironment environment(fp);
        std::array<std::uint32_t,3> cross{};std::uint32_t magnitude;
        // Preserve the unspilled 53-bit cz in ST0 across its REAL spill:
        // the norm begins with cz_extended * cz_REAL, not cz_REAL squared.
        asm volatile("flds %1\n\tfmuls %2\n\tflds %3\n\tfmuls %4\n\tfsubrp %%st,%%st(1)\n\tfstps %0"
            :"=m"(cross[0]):"m"(up[1]),"m"(forward[2]),"m"(forward[1]),"m"(up[2])
            :"memory","st","st(1)");
        asm volatile("flds %3\n\tfmuls %4\n\tflds %1\n\tfmuls %5\n\tfsubrp %%st,%%st(1)\n\tfstps %0"
            :"=m"(cross[1]):"m"(up[0]),"m"(up[1]),"m"(forward[0]),"m"(up[2]),"m"(forward[2])
            :"memory","st","st(1)");
        unsigned char too_small{};
        // Keep live x87 quantities in one asm block, with no virtual call or
        // undeclared x87 value crossing the compiler's instruction boundary.
        asm volatile("flds %5\n\tfmuls %3\n\tflds %4\n\tfmuls %6\n\tfsubrp %%st,%%st(1)\n\tfsts %0\n\t"
            "fmuls %0\n\tflds %7\n\tfmuls %7\n\tfaddp %%st,%%st(1)\n\tflds %8\n\tfmuls %8\n\tfaddp %%st,%%st(1)\n\tfsqrt\n\tfsts %1\n\tfabs\n\t"
            "flds %9\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %2"
            :"=&m"(cross[2]),"=&m"(magnitude),"=&qm"(too_small)
            :"m"(up[0]),"m"(up[1]),"m"(forward[1]),"m"(forward[0]),"m"(cross[1]),"m"(cross[0]),"m"(epsilon)
            :"memory","cc","st","st(1)");
        auto nx=scalar(cross[0]),ny=scalar(cross[1]),nz=scalar(cross[2]);
        if(!too_small) {
            const auto inverse=divide(scalar(read(constants,0x46a0d0u)),scalar(magnitude));
            nx=multiply(inverse,nx);ny=multiply(ny,inverse);nz=multiply(nz,inverse);
        }
        const auto by_nx=multiply(scalar(forward[1]),nx);
        const auto nx_bz=multiply(nx,scalar(forward[2]));
        const auto bx_ny=multiply(scalar(forward[0]),ny);
        const auto ny_bz=multiply(ny,scalar(forward[2]));
        const auto z=subtract(bx_ny,by_nx);
        const auto y=subtract(nx_bz,multiply(scalar(forward[0]),nz));
        const auto x=subtract(multiply(scalar(forward[1]),nz),ny_bz);
        up={bits(x),bits(y),bits(z)};
    }
    if(publish)for(unsigned axis=0;axis<3;++axis)publish(NativeTu15LightUpStage::perpendicular,axis,up[axis]);
    const auto magnitude=native_campaign_ai_normalize_vector_value_tu15(constants,up,fp,
        [&](unsigned axis,std::uint32_t value){if(publish)publish(NativeTu15LightUpStage::normalized,axis,value);});
    return {up,magnitude};
#else
    (void)constants;(void)up;(void)forward;(void)fp;(void)publish;
    throw std::runtime_error("Exact native TU1.5 light vector backend unavailable");
#endif
}
}
