// Native TU1.5 team query (15C7F0) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author sessions with and without variant
// records, game modes, team masks and flags, and teams past the mask's
// width; one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_team_query.hpp"
using namespace frame_misc_harness;
namespace team=halo2::native_team_query_detail;
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
    for(std::uint32_t at=region;at<region+0x2000u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(0x4f63b8u,region);m.put(region+0xc14u,pick(8));
    for(std::uint32_t k=0;k<8;++k)m.put(0x56dfa0u+4u*k,pick(4)==0?0u:std::uint32_t(rng()));
    m.put(0x4f3218u,region+0x1000u);m.put(region+0x1180u,pick(3)?7u:pick(10));
    m.put(0x51df9cu,region+0x1800u);
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x100000u+pick(0x100));
    m.put(stack+4u,(std::uint32_t(rng())&0xffffff00u)|(pick(4)==0?pick(256):pick(16)));
    random_abi(rng,c,stack);
    if(shape==15)m.drop(pick(2)?region:region+0x1000u);
    return c;
}
bool native(WorldState &s){return team::bridge(s,[](auto &v){original::native_frame_misc_reference_15c7f0(v);});}
void reference(WorldState &s){original::run_15c7f0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_TEAM_QUERY");
    std::mt19937 rng(0x15c7f0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,team::coverage().load(),unsigned(team::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
