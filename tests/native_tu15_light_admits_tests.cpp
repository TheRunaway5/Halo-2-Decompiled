// Native TU1.5 light admits (C5170) against the unmodified translated body of
// the light portal unit, extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). There are no children. Fixtures author lights
// with and without the portal flag, intensities about the epsilon and
// vectors about the threshold (NaN too); one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "pstencil/native_tu15_light_admits.hpp"
using namespace frame_misc_harness;
namespace admits=halo2::native_light_admits_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t header=0x10000000u,lights=0x10010000u;
std::uint32_t random_float(std::mt19937_64 &rng) {
    switch(rng()%7) {
    case 0:return 0u;
    case 1:return 0x7fc00000u;
    case 2:return std::bit_cast<std::uint32_t>(0.01f);
    default:return std::bit_cast<std::uint32_t>(float(std::int32_t(rng()%41)-20)/10.0f);
    }
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    std::mt19937_64 floats(rng());
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbdcu,header);
    for(std::uint32_t at=header;at<header+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(header+0x44u,lights);
    for(std::uint32_t at=lights;at<lights+4u*0x110u;at+=4)m.put(at,random_float(floats));
    for(std::uint32_t k=0;k<4;++k)m.put(lights+0x110u*k+2u,pick(4)==0?std::uint32_t(rng())&~2u:std::uint32_t(rng())|2u,1);
    m.put(0x46a0ecu,pick(4)?std::bit_cast<std::uint32_t>(0.0001f):random_float(floats));
    m.put(0x46a12cu,pick(4)?std::bit_cast<std::uint32_t>(1.0f):random_float(floats));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc5000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15)m.drop(pick(2)?header:lights);
    return c;
}
bool native(WorldState &s){return admits::bridge(s,[](auto &v){original::native_frame_misc_reference_c5170(v);});}
void reference(WorldState &s){original::run_c5170(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_LIGHT_ADMITS");
    std::mt19937 rng(0xc5170u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,admits::coverage().load(),unsigned(admits::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
