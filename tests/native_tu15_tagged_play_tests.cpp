// Native TU1.5 tagged play (1D6430) against the unmodified translated body of
// the physics live support unit (the validator repeats it with the AI frame
// and AI actor units'), extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). 1D45A0 and 1E5FB0 are hash-driven stubs of
// their complete entry ABIs and arguments: 1D45A0 writes the frame's words
// and pops two arguments, 1E5FB0 pops four; both write scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EDI changes or the stack is left unbalanced. Fixtures author
// objects with and without tags; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_tagged_play.hpp"
using namespace frame_misc_harness;
namespace tagged=halo2::native_tagged_play_detail;
namespace {
constexpr std::uint32_t objects=0x10000000u,tags=0x10010000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x1d45a0u)for(std::uint32_t k=0;k<3;++k)w(s.memory,s.observe,target+1u,s.gp[6]+4u*k,4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=objects+0x100u*std::uint32_t(rng()%2);
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
static void run_1d45a0(WorldState &s){stub(s,0x1d45a0u,2);}
static void run_1e5fb0(WorldState &s){stub(s,0x1e5fb0u,4);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=objects;at<objects+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t g=0;g<2;++g)m.put(objects+0x100u*g+0x68u,pick(4)==0?0xffffffffu:(std::uint32_t(rng())<<16)|pick(4));
    m.put(0x4f0414u,tags);
    for(std::uint32_t at=tags;at<tags+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1d0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[7]=objects+0x100u*pick(2);
    if(shape==15)m.drop(pick(2)?objects:tags);
    return c;
}
bool native(WorldState &s) {
    return tagged::bridge(s,[](auto &v){original::native_frame_misc_reference_1d6430(v);},
        [](auto &v){original::run_1d45a0(v);},[](auto &v){original::run_1e5fb0(v);});
}
void reference(WorldState &s){original::run_1d6430(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_TAGGED_PLAY");
    std::mt19937 rng(0x1d6430u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,tagged::coverage().load(),unsigned(tagged::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
