// Native TU1.5 memory copy (2E2E30) against the unmodified translated body
// of the scene device constructor unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author byte counts of 0..70 (and every
// remainder), both copy directions, and overlapping source and destination;
// one region removed injects a fault. Modes: <cases> [census|verify]. Prints
// a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/renderer/native_tu15_memory_copy.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_memory_copy_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t region=0x10000000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    const bool backward=shape==3 || shape==10;
    for(std::uint32_t at=region;at<region+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto bytes=pick(71);
    const auto source=region+0x100u+pick(0x40);
    const auto destination=shape==6?source+pick(9):shape==7?source-pick(9):region+0x200u+pick(0x40);
    m.put(stack,0x2e0000u+pick(0x100));m.put(stack+4u,destination);m.put(stack+8u,source);m.put(stack+12u,bytes);
    for(std::uint32_t at=stack-0x40u;at<stack;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack+16u,std::uint32_t(rng()));
    random_abi(rng,c,stack);
    if(backward)c.abi.eflags|=0x400u;else c.abi.eflags&=~0x400u;
    if(shape==15)m.drop(region);
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_2e2e30(v);});}
void reference(WorldState &s){original::run_2e2e30(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_MEMORY_COPY");
    std::mt19937 rng(0x2e2e30u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
