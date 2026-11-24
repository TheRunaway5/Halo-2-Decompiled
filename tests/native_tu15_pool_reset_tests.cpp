// Native TU1.5 pool reset (140CF0) against the unmodified translated bodies
// of the renderer transparent storage and particle draw units (the same
// logic), extracted by tools/validate_native_frame_misc_tu15.py into
// body.inc, plus the verification reference the production binder generates
// (reference.inc). There are no children. Fixtures author header words,
// element counts (zero and negative too) and odd element sizes, and both
// clear directions; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/renderer/native_tu15_pool_reset.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_pool_reset_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t pools=0x10000000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    const bool backward=shape==5;
    for(std::uint32_t at=pools;at<pools+0x800u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto pool=pools+(backward?0x400u:0u);
    m.put(pool+0x20u,pick(5));m.put(pool+0x24u,pick(6)?pick(7):0xffffffffu);m.put(pool+0x28u,pick(21));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x140000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[2]=pool;
    if(backward)c.abi.eflags|=0x400u;else c.abi.eflags&=~0x400u;
    if(shape==15)m.drop(pools);
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_140cf0(v);});}
void reference(WorldState &s){original::run_140cf0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_POOL_RESET");
    std::mt19937 rng(0x140cf0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
