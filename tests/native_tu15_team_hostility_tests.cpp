// Native TU1.5 team hostility (1E7810) against the unmodified translated body
// of the physics live support unit (the validator repeats it with the AI
// frame unit's, which dispatches 160A60), extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// C1E90 (direct) and 160A60 (through the unit's dispatch) are hash-driven
// stubs of their complete entry ABIs and arguments: each answers AL and
// pops its arguments (4 and 8 bytes), the caller-saved registers, XMM,
// flags and (by policy) MXCSR are random, and by policy the stack is left
// unbalanced. Fixtures author teams NONE, within and out of 0..15
// (negative too) and every game mode; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_team_hostility.hpp"
using namespace frame_misc_harness;
namespace hostility=halo2::native_team_hostility_detail;
namespace {
constexpr std::uint32_t state=0x10000000u,alliances=0x10010000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=rng()%2?std::uint32_t(rng())&~0xffu:std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*arguments+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_c1e90(WorldState &s){stub(s,0xc1e90u,1);}
static void run_160a60(WorldState &s){stub(s,0x160a60u,2);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x160a60u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target,2);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f3218u,state);m.put(0x5020c4u,alliances);
    for(std::uint32_t at=state;at<state+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(state+8u,pick(3)==0?pick(5):1u+pick(2));
    for(std::uint32_t at=alliances;at<alliances+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1e0000u+pick(0x100));
    random_abi(rng,c,stack);
    const auto team=[&]{return pick(6)==0?0xffffu:pick(6)==0?0x8000u+pick(16):pick(6)==0?16u+pick(4):pick(16);};
    c.abi.gp[1]=(c.abi.gp[1]&0xffff0000u)|team();c.abi.gp[6]=(c.abi.gp[6]&0xffff0000u)|team();
    if(shape==15)m.drop(pick(2)?state:alliances);
    return c;
}
bool native(WorldState &s) {
    return hostility::bridge(s,[](auto &v){original::native_frame_misc_reference_1e7810(v);},[](auto &v){original::run_c1e90(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1e7810(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_TEAM_HOSTILITY");
    std::mt19937 rng(0x1e7810u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,hostility::coverage().load(),unsigned(hostility::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
