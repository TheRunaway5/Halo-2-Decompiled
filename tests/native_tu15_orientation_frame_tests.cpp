// Native TU1.5 orientation frame (144210) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author forward and up vectors of ordinary
// floats and specials (zeros of both signs, subnormals, infinities, NaNs),
// frames apart from and over their inputs, and every MXCSR the harness
// draws; one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_orientation_frame.hpp"
using namespace frame_misc_harness;
namespace orientation=halo2::native_orientation_frame_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t vectors=0x10000000u,frames=0x10010000u;
std::uint32_t value(std::mt19937 &rng) {
    static constexpr std::uint32_t specials[]{0u,0x80000000u,0x00000001u,0x007fffffu,0x7f800000u,0xff800000u,0x7fc00000u,
        0x3f800000u,0xbf800000u,0x7f7fffffu};
    if(rng()%6==0)return specials[rng()%10];
    const float f=float(int(rng()%20001)-10000)/997.0f;
    return std::bit_cast<std::uint32_t>(f);
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x46a0d0u,0x3f800000u);
    for(std::uint32_t at=vectors;at<vectors+0x80u;at+=4)m.put(at,value(rng));
    for(std::uint32_t at=frames;at<frames+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x140000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[2]=vectors+4u*pick(8);c.abi.gp[0]=vectors+0x40u+4u*pick(8);c.abi.gp[1]=frames+4u*pick(8);
    if(shape==7)c.abi.gp[1]=vectors+4u*pick(16);                // the frame over its inputs
    if(shape==15)m.drop(pick(2)?vectors:frames);
    return c;
}
bool native(WorldState &s){return orientation::bridge(s,[](auto &v){original::native_frame_misc_reference_144210(v);});}
void reference(WorldState &s){original::run_144210(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ORIENTATION_FRAME");
    std::mt19937 rng(0x144210u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,orientation::coverage().load(),unsigned(orientation::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
