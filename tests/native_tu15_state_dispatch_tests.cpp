// Native TU1.5 state dispatch (1EDDA0) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1F9720, 202CE0 and 1F5230 (direct or through the unit's dispatch) are
// hash-driven stubs of their complete entry ABIs and arguments: each writes
// scratch, the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy EBX, ESI or EDI change or the stack is left
// unbalanced. Fixtures author kinds 1..4 and those past the table either way
// (an unbound entry, or a bound one that is no case); one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_state_dispatch.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_state_dispatch_detail;
namespace {
constexpr std::uint32_t states=0x10000000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    const auto arguments=target==0x1f5230u?0u:1u;
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
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
// The unit's table targets (and one bound target that is no case), as its
// canonical_branch names them.
std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage) {
    switch(p) {
    case 0x1eddb1u:case 0x1eddbau:case 0x1eddbfu:case 0x1eddcdu:case 0x1d4a9u:return p;
    default:throw std::runtime_error("Unbound TU1.5 contact dispatch target");
    }
}
static void run_1f9720(WorldState &s){stub(s,0x1f9720u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x202ce0u && target!=0x1f5230u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
}
#include "body.inc"
#include "reference.inc"
void step(WorldState &s,std::uint32_t target,std::uint32_t){stub(s,target);}
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    static constexpr std::uint32_t targets[]{0x1eddb1u,0x1eddbau,0x1eddbfu,0x1eddcdu};
    for(std::uint32_t k=0;k<4;++k)m.put(0x1edde0u+4u*k,targets[k]);
    m.put(0x1eddddu-1u,pick(2)?0x1d4a9u:std::uint32_t(rng()));
    for(std::uint32_t k=4;k<8;++k)m.put(0x1edde0u+4u*k,pick(2)?0x1d4a9u:std::uint32_t(rng()));
    for(std::uint32_t at=states;at<states+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(states,pick(8)?1u+pick(4):pick(2)?0u:5u+pick(3),1);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1e0000u+pick(0x100));m.put(stack+4u,std::uint32_t(rng()));
    random_abi(rng,c,stack);
    c.abi.gp[2]=states;
    if(shape==15)m.drop(states);
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1edda0(v);},
        [](std::uint32_t p,SourcePhysicsContactImage image){return original::canonical_branch(p,image);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::step(v,target,site);});
}
void reference(WorldState &s){original::run_1edda0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_STATE_DISPATCH");
    std::mt19937 rng(0x1edda0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
