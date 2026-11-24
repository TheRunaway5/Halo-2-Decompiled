// Native TU1.5 pixel scales (1BE90) against the unmodified translated body
// of the shadow frame unit, extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). There are no children. Fixtures author view
// bounds of every width (zero and negative too), constants random, zero
// and NaN, MXCSR rounding modes (by policy), and states NONE or with the
// byte set or clear; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/shadows/native_tu15_pixel_scales.hpp"
using namespace frame_misc_harness;
namespace scales=halo2::native_pixel_scales_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t states=0x10000000u;
std::uint32_t random_float(std::mt19937_64 &rng) {
    switch(rng()%6) {
    case 0:return 0u;
    case 1:return 0x7fc00000u;
    default:return std::bit_cast<std::uint32_t>(float(std::int32_t(rng()%4001)-2000)/7.0f);
    }
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    std::mt19937_64 floats(rng());
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4c65f0u,std::uint32_t(rng()),2);m.put(0x4c65f4u,pick(4)==0?std::uint32_t(rng()):std::uint32_t(rng()%4000),2);
    if(pick(6)==0)m.put(0x4c65f4u,r(m,0x4c65f0u)&0xffffu,2);
    m.put(0x46a11cu,random_float(floats));m.put(0x46a120u,random_float(floats));m.put(0x46a0d0u,pick(6)?0x3f800000u:random_float(floats));
    m.put(0x51df50u,pick(4)==0?0u:states+8u*pick(4));
    for(std::uint32_t at=states;at<states+0x40u;at+=4)m.put(at,pick(2)?0u:std::uint32_t(rng()));
    for(std::uint32_t at=0x492340u;at<0x492370u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1b0000u+pick(0x100));
    random_abi(rng,c,stack);
    if(pick(3)==0)c.mxcsr=(c.mxcsr&~0x6000u)|(pick(4)<<13);
    if(shape==15)m.drop(pick(2)?states:0x492000u);
    return c;
}
bool native(WorldState &s){return scales::bridge(s,[](auto &v){original::native_frame_misc_reference_1be90(v);});}
void reference(WorldState &s){original::run_1be90(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PIXEL_SCALES");
    std::mt19937 rng(0x1be90u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,scales::coverage().load(),unsigned(scales::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
