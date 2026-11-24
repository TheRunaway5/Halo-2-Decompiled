// Native TU1.5 polygon inside (243510) against the unmodified translated body
// of the light portal unit (the validator repeats it with the physics live
// support unit's), extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). There are no children. Fixtures author convex
// and other polygons of zero to eight vertices (negative counts too),
// points inside, on and outside them, tolerances of either sign (NaN too),
// and MXCSR rounding modes by policy; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_polygon_inside.hpp"
using namespace frame_misc_harness;
namespace polygon=halo2::native_polygon_inside_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t points=0x10000000u,vertices=0x10010000u;
std::uint32_t f(float v){return std::bit_cast<std::uint32_t>(v);}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    const auto count=pick(9);
    const float radius=1.0f+float(pick(8));
    for(std::uint32_t k=0;k<8;++k) {
        const float angle=6.2831853f*float(k)/float(count?count:1u);
        const bool regular=pick(4)!=0;
        m.put(vertices+8u*k,f(regular?radius*std::cos(angle):float(int(pick(41))-20)/4.0f));
        m.put(vertices+8u*k+4u,f(regular?radius*std::sin(angle):float(int(pick(41))-20)/4.0f));
    }
    m.put(points,f(float(int(pick(41))-20)/4.0f));m.put(points+4u,pick(9)==0?0x7fc00000u:f(float(int(pick(41))-20)/4.0f));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x240000u+pick(0x100));
    m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|(pick(8)==0?0x8000u+pick(4):count));
    m.put(stack+8u,pick(6)==0?0x7fc00000u:f(float(int(pick(9))-2)/8.0f));
    random_abi(rng,c,stack);
    c.abi.gp[3]=points;c.abi.gp[2]=vertices;
    if(pick(3)==0)c.mxcsr=(c.mxcsr&~0x6000u)|(pick(4)<<13);
    if(shape==15)m.drop(pick(2)?points:vertices);
    return c;
}
bool native(WorldState &s){return polygon::bridge(s,[](auto &v){original::native_frame_misc_reference_243510(v);});}
void reference(WorldState &s){original::run_243510(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_POLYGON_INSIDE");
    std::mt19937 rng(0x243510u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,polygon::coverage().load(),unsigned(polygon::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
