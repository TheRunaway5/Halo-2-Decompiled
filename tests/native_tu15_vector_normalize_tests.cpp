// Native TU1.5 vector normalize (1E1010) against the unmodified translated
// body of the scene device constructor unit (and its variant in map world
// support), extracted by tools/validate_native_frame_misc_tu15.py into
// body.inc, plus the verification reference the production binder generates
// (reference.inc). Fixtures author 16-byte vectors with regular, subnormal,
// infinite, signed-zero and NaN components across all MXCSR rounding/FTZ/DAZ
// modes, aligned and unaligned entry ESPs, live RSQRTSS service (and missing
// RSQRTSS service to test fault parity), and dropped pages.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/renderer/native_tu15_vector_normalize.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_vector_normalize_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
std::uint32_t source_estimate(WorldState &s,std::uint32_t input) {
    if(!s.services.reciprocal_sqrt)throw std::runtime_error("Required original contact RSQRTSS owner unavailable");
    return s.services.reciprocal_sqrt(input);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t vec_mem=0x10000000u;
std::uint32_t fbits(float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;}
std::uint32_t hw_rsqrtss(std::uint32_t bits) {
    auto value=_mm_castsi128_ps(_mm_set_epi32(0,0,0,std::bit_cast<std::int32_t>(bits)));
    asm volatile("rsqrtss %1,%0":"=x"(value):"x"(value):"memory");
    return std::bit_cast<std::array<std::uint32_t,4>>(value)[0];
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    std::mt19937_64 frng(rng());
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x46a0e4u,fbits(3.0f));
    m.put(0x46a0ccu,fbits(0.5f));
    for(std::uint32_t k=0;k<4;++k) {
        const std::uint32_t v=chance(4)?special_float(frng):fbits((float(int(pick(201))-100))*0.25f);
        m.put(vec_mem+4u*k,v);
    }
    for(std::uint32_t at=stack-0x60u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1e1000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[1]=vec_mem;
    if(shape!=14)c.reciprocal_sqrt=hw_rsqrtss;
    if(shape==15) {
        static constexpr std::uint32_t areas[]{vec_mem,0x46a0e4u};
        m.drop(areas[pick(2)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1e1010(v);});}
void reference(WorldState &s){original::run_1e1010(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_VECTOR_NORMALIZE");
    std::mt19937 rng(0x1e1010u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
