// Native TU1.5 channel reset (1D4E50) against the unmodified translated body
// of the physics live support unit (the validator repeats it with the AI
// actor unit's), extracted by tools/validate_native_frame_misc_tu15.py into
// body.inc, plus the verification reference the production binder generates
// (reference.inc). There are no children. Fixtures author channels at
// several alignments (over the constant too) and constants of every float;
// one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_channel_reset.hpp"
using namespace frame_misc_harness;
namespace channels=halo2::native_channel_reset_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t records=0x10000000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x46a0d0u,pick(3)?0x3f800000u:std::uint32_t(rng()));
    for(std::uint32_t at=records;at<records+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1d0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=records+pick(64)*2u;
    if(shape==7)c.abi.gp[0]=0x46a0d0u-0x18u-0x20u*pick(3);          // over the constant
    if(shape==15)m.drop(records+0x80u*pick(2));
    return c;
}
bool native(WorldState &s){return channels::bridge(s,[](auto &v){original::native_frame_misc_reference_1d4e50(v);});}
void reference(WorldState &s){original::run_1d4e50(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CHANNEL_RESET");
    std::mt19937 rng(0x1d4e50u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,channels::coverage().load(),unsigned(channels::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
