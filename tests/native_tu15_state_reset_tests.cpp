// Native TU1.5 state reset (C93D0) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author states anywhere in a page (their
// old contents random) and origins (special floats too), the origin
// overlapping the state; one region removed injects a fault; a set
// direction flag is refused. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "campaign/ai/native_tu15_state_reset.hpp"
using namespace frame_misc_harness;
namespace reset=halo2::native_state_reset_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t states=0x10000000u,points=0x10100000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=states;at<states+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=points;at<points+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto state=states+4u*pick(64);
    m.put(0x474d08u,shape==7?state+4u*pick(28):points+4u*pick(8));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc9000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape!=14)c.abi.eflags&=~0x400u;
    c.abi.gp[2]=state;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{states,points,0x474000u};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s){return reset::bridge(s,[](auto &v){original::native_frame_misc_reference_c93d0(v);});}
void reference(WorldState &s){original::run_c93d0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_STATE_RESET");
    std::mt19937 rng(0xc93d0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t,(c.abi.eflags&0x400u)!=0))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,reset::coverage().load(),unsigned(reset::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
