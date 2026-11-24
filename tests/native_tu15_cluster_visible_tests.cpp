// Native TU1.5 cluster_visible query (11e780) against the unmodified translated body of its
// unit, extracted by tools/validate_native_frame_misc_tu15.py into body.inc,
// plus the verification reference the production binder generates
// (reference.inc). There are no children. Fixtures author the globals and
// records the query reads across every outcome; one region removed injects
// a fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_cluster_visible.hpp"
using namespace frame_misc_harness;
namespace query=halo2::native_cluster_visible_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t region=0x10000000u,headers=0x10100000u,header_data=0x10110000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=region;at<region+0x2000u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(0x4f3218u,region);
    for(std::uint32_t at=region+0x1000u;at<region+0x1300u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x100000u+pick(0x100));
    random_abi(rng,c,stack);
    const auto header=region+0x40u+4u*pick(16);m.put(header+4u,pick(4)==0?0x8000u|pick(64):pick(512),2);c.abi.gp[0]=header;
    if(shape==15)m.drop(pick(2)?region:region+0x1000u);
    return c;
}
bool native(WorldState &s){return query::bridge(s,[](auto &v){original::native_frame_misc_reference_11e780(v);});}
void reference(WorldState &s){original::run_11e780(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SMALL_QUERIES");
    std::mt19937 rng(0x11e780u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,query::coverage().load(),unsigned(query::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
