// Native TU1.5 tolerance check AA200 against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author values around the tolerance
// (differences inside and outside it, special floats, NaN payloads and
// infinities), the tolerance and one constants, every MXCSR mode and up to
// six live x87 registers (seven are refused); one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_tolerance_checks.hpp"
using namespace frame_misc_harness;
namespace checks=halo2::native_tolerance_checks_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t vectors=0x10000000u;
std::uint32_t value(std::mt19937 &rng,unsigned shape) {
    static constexpr std::uint32_t special[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7f800000u,0xff800000u,0x00000001u,
        0x80000000u,0x00400000u};
    if((shape==3 || shape==4) && rng()%3==0)return special[rng()%8];
    // Huge finite values (exponent 254) whose squares or differences stay finite or overflow.
    static constexpr std::uint32_t huge[]{0x5f000000u,0x5f3504f3u,0x7f000000u,0x7f7fffffu,0xff000000u};
    if(rng()%7==0)return huge[rng()%5];
    static constexpr float near_one[]{1.0f,0.9999f,1.0001f,0.999f,1.001f,0.5f,2.0f,0.0f,-1.0f,0.57735f,0.577f};
    return std::bit_cast<std::uint32_t>(near_one[rng()%11]);
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=vectors;at<vectors+0x100u;at+=4)m.put(at,value(rng,shape));
    m.put(0x46a0d0u,std::bit_cast<std::uint32_t>(1.0f));
    m.put(0x46a180u,pick(4)?std::bit_cast<std::uint32_t>(0.001f):value(rng,3));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xa0000u+pick(0x100));m.put(stack+4u,value(rng,shape));
    random_abi(rng,c,stack);
    if(c.abi.x87_depth>6u)c.abi.x87_depth=6u;
    if(shape==14)c.abi.x87_depth=7u;
    c.abi.gp[0]=vectors+4u*pick(60);
    c.abi.xmm[0][0]=value(rng,shape);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{vectors,0x46a000u};
        m.drop(regions[pick(2)]);
    }
    return c;
}
bool native(WorldState &s){return checks::bridge_unit_length(s,[](auto &v){original::native_frame_misc_reference_aa200(v);});}
void reference(WorldState &s){original::run_aa200(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_TOLERANCE_CHECKS");
    std::mt19937 rng(0xaa200u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t,c.abi.x87_depth>6u))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,checks::length_coverage().load(),unsigned(checks::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
