// Native TU1.5 input edges (1521B0) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author player handles (the high word
// random), players with random held buttons, trigger and latch bytes, and
// inputs with random buttons and trigger words; one region removed injects
// a fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/players/native_tu15_input_edges.hpp"
using namespace frame_misc_harness;
namespace edges=halo2::native_input_edges_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t header=0x10000000u,players=0x10010000u,inputs=0x10020000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f54f4u,header);
    for(std::uint32_t at=header;at<header+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(header+0x44u,players);
    for(std::uint32_t at=players;at<players+4u*0x21cu;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=inputs;at<inputs+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);c.abi.gp[1]=inputs+4u*pick(2);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{header,players,inputs};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s){return edges::bridge(s,[](auto &v){original::native_frame_misc_reference_1521b0(v);});}
void reference(WorldState &s){original::run_1521b0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_INPUT_EDGES");
    std::mt19937 rng(0x1521b0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,edges::coverage().load(),unsigned(edges::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
