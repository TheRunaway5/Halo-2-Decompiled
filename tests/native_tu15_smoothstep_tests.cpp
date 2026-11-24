// Native TU1.5 smoothstep (110FC0) against the unmodified translated body of the
// physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author the input (below 0, within, above
// 1, special floats, random bits; its upper lanes random) and the
// constants (their values, sometimes others), on random MXCSR modes; one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_smoothstep.hpp"
using namespace frame_misc_harness;
namespace curve=halo2::native_smoothstep_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t real(std::mt19937 &rng) {
    static constexpr std::uint32_t special[]{0u,0x80000000u,0x7fc00000u,0x7f800000u,0xff800000u,0x3f800000u,0x00000001u,
        0x7f7fffffu,0x3f7fffffu,0x3f800001u};
    switch(rng()%5) {
    case 0:return special[rng()%10];
    case 1:return std::uint32_t(rng());
    default:return std::bit_cast<std::uint32_t>(float(int(rng()%4001)-1000)/2000.0f);
    }
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x46a0d0u,pick(6)?0x3f800000u:real(rng));
    m.put(0x46a0e4u,pick(6)?0x40400000u:real(rng));m.put(0x46a11cu,pick(6)?0x40000000u:real(rng));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1e0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.xmm[2][0]=real(rng);
    if(shape==15)m.drop(0x46a000u);
    return c;
}
bool native(WorldState &s){return curve::bridge(s,[](auto &v){original::native_frame_misc_reference_110fc0(v);});}
void reference(WorldState &s){original::run_110fc0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SMOOTH_CURVES");
    std::mt19937 rng(0x110fc0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,curve::coverage().load(),unsigned(curve::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
