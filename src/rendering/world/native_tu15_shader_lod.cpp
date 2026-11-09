#include "halo2/native_tu15_shader_lod.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <bit>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <emmintrin.h>
#endif

namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory& m,std::uint32_t at,unsigned size=4) {
    std::array<std::byte,4> b{};m.read(at,std::span(b).first(size));std::uint32_t v{};
    for(unsigned i=0;i<size;++i)v|=std::to_integer<unsigned>(b[i])<<(8*i);return v;
}
std::int32_t signed_word(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
void ordinary(std::uint32_t group) {
    if(group==0x5052544du||group==0x70727433u)
        throw std::invalid_argument("Native1BF30 dynamic shader requires its original service owner");
}
std::uint32_t lod_block(const SourcePhysicsContactMemory& m,std::uint32_t tags,std::uint32_t shader) {
    const auto definition=word(m,tags+16u*(shader&65535u)+8u);
    const auto template_id=word(m,word(m,definition+0x24u));
    const auto template_definition=word(m,tags+16u*(template_id&65535u)+8u);
    return word(m,template_definition+0x5cu);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using Scalar=__m128;
Scalar scalar(std::uint32_t v){return _mm_castsi128_ps(_mm_cvtsi32_si128(signed_word(v)));}
Scalar mul(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool at_least(Scalar metric,Scalar threshold) {
    unsigned char result;
    asm volatile("comiss %2,%1\n\tsetae %0":"=qm"(result):"x"(metric),"x"(threshold):"cc","memory");
    return result;
}
#endif
}
NativeTu15ShaderLod1bf30 native_tu15_shader_lod_1bf30(const SourcePhysicsContactMemory& m,
    std::uint32_t bias,std::uint32_t shader,std::uint32_t metric_bits,NativeTu15PhysicsFrameFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto tags=word(m,0x4f0414u);
    const auto group_at=tags+16u*std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(shader))));
    ordinary(word(m,group_at));
    const auto count=word(m,lod_block(m,tags,shader));
    auto arithmetic=fp;std::uint32_t index{};
    {
        detail::NativeTu15ArithmeticEnvironment environment(arithmetic);
        auto metric=scalar(metric_bits);
        if(word(m,0x4c6839u,1))metric=mul(metric,scalar(word(m,0x46a11cu)));
        if(bias!=5u) {
            //1BFB8 loads the bias factor into the MULSS destination. Preserve
            // its operand order when both inputs are NaNs.
            metric=mul(scalar(word(m,0x492348u+4u*bias)),metric);
            while(signed_word(index)<signed_word(count)) {
                ordinary(word(m,group_at));
                const auto rows=word(m,lod_block(m,tags,shader)+4u);
                if(at_least(metric,scalar(word(m,rows+10u*index+6u))))break;
                ++index;
            }
        }
    }
    const auto upper=signed_word(count-1u),scan=signed_word(index);
    const auto final=scan>upper?upper:scan;
    arithmetic.control_word=fp.control_word;fp=arithmetic;
    return {final,final<signed_word(count),{scan,final}};
#else
    (void)m;(void)bias;(void)shader;(void)metric_bits;(void)fp;
    throw std::invalid_argument("Exact native1BF30 arithmetic backend unavailable");
#endif
}
}
