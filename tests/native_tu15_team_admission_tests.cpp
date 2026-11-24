// Native TU1.5 team admission (161560) against the unmodified translated
// body of the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author sessions with and without a
// variant record, the team NONE and others (past 31 too), every game mode,
// mask and flag, and the +0x1C4 word 1 or not; one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_team_admission.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_team_admission_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t session=0x10000000u,state=0x10010000u,mask=0x10020000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f63b8u,session);
    for(std::uint32_t at=session+0xc00u;at<session+0xc20u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto index=pick(4);
    m.put(session+0xc14u,index);
    for(std::uint32_t k=0;k<4;++k)m.put(0x56dfa0u+4u*k,pick(5)==0?0u:std::uint32_t(rng())|1u);
    m.put(0x4f3218u,state);
    for(std::uint32_t at=state+0x170u;at<state+0x240u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(state+0x180u,pick(3)?7u:pick(9));
    m.put(state+0x1c4u,pick(2)?1u:pick(4));
    m.put(0x51df9cu,mask+2u*pick(4));
    for(std::uint32_t at=mask;at<mask+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x160000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[1]=pick(6)==0?0xffffffffu:pick(5)==0?std::uint32_t(rng()):pick(16);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{session,state,mask};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_161560(v);});}
void reference(WorldState &s){original::run_161560(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_TEAM_ADMISSION");
    std::mt19937 rng(0x161560u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
