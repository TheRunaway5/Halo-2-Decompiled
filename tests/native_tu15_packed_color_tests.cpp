// Native TU1.5 packed colour (18BBC0) against the unmodified translated body
// of the effect particle frame unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author colours below, within and above
// the range (special floats, infinities and NaN payloads), the range ends
// and scales, every MXCSR mode and x87 control word the harness sets, and
// up to seven live x87 registers (eight are refused); one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_packed_color.hpp"
using namespace frame_misc_harness;
namespace pack=halo2::native_packed_color_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t colours=0x10000000u;
std::uint32_t channel(std::mt19937 &rng,unsigned shape) {
    static constexpr std::uint32_t special[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7f800000u,0xff800000u,0x00000001u,
        0x80000000u,0x00400000u};
    if((shape==3 || shape==4) && rng()%3==0)return special[rng()%8];
    static constexpr float values[]{0.0f,1.0f,0.5f,0.25f,0.999f,-0.5f,1.5f,0.0001f,0.75f,2.0f};
    return std::bit_cast<std::uint32_t>(values[rng()%10]);
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=colours;at<colours+0x100u;at+=4)m.put(at,channel(rng,shape));
    m.put(0x46a0dcu,pick(8)?0u:channel(rng,3));m.put(0x46a0d0u,pick(8)?std::bit_cast<std::uint32_t>(1.0f):channel(rng,3));
    m.put(0x46a338u,std::bit_cast<std::uint32_t>(pick(4)?1023.0f:2047.0f));m.put(0x46a33cu,std::bit_cast<std::uint32_t>(pick(4)?2047.0f:1023.5f));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x18b000u+pick(0x100));
    random_abi(rng,c,stack);
    if(c.abi.x87_depth>7u)c.abi.x87_depth=7u;
    if(shape==14)c.abi.x87_depth=8u;
    c.abi.gp[0]=colours+4u*pick(60);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{colours,0x46a000u};
        m.drop(regions[pick(2)]);
    }
    return c;
}
bool native(WorldState &s){return pack::bridge(s,[](auto &v){original::native_frame_misc_reference_18bbc0(v);});}
void reference(WorldState &s){original::run_18bbc0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PACKED_COLOR");
    std::mt19937 rng(0x18bbc0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t,c.abi.x87_depth>7u))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,pack::coverage().load(),unsigned(pack::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
