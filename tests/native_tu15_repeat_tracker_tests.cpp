// Native TU1.5 repeat tracker (1F0F90) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author trackers and sources of the same
// and other kinds (negative too) and handles, counted kinds and others,
// flagged or not, and trackers overlapping their source; one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_repeat_tracker.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_repeat_tracker_detail;
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
    for(std::uint32_t at=records;at<records+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto from=records+4u*pick(16);
    const auto tracked=shape==7?from+0x20u+2u*pick(8):records+0x100u+4u*pick(16);
    static constexpr std::uint32_t kinds[]{0u,1u,2u,3u,4u,5u,6u,7u,0xffffu,0x8001u};
    const auto kind=kinds[pick(10)];
    m.put(from+0x28u,kind,2);
    if(shape!=7) {
        m.put(tracked,pick(2)?kind:kinds[pick(10)],2);
        if(pick(2))m.put(tracked+4u,r(m,from+0x2cu));
    }
    m.put(from+0x1cu,pick(256),1);
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1f0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=tracked;c.abi.gp[1]=from;
    if(shape==15)m.drop(records);
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1f0f90(v);});}
void reference(WorldState &s){original::run_1f0f90(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_REPEAT_TRACKER");
    std::mt19937 rng(0x1f0f90u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
