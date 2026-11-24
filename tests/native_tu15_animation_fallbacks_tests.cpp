// Native TU1.5 animation fallbacks (1E5F20) against the unmodified translated
// body of the physics live support unit (the validator repeats it with the
// AI frame and AI actor units'), extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1E5E40 is a hash-driven stub of its complete entry ABI and five
// arguments: it pops them, writes scratch or the arguments' slots, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBP, ESI or EDI change or the stack is left unbalanced.
// Fixtures author every forced byte and third argument; one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_animation_fallbacks.hpp"
using namespace frame_misc_harness;
namespace fallbacks=halo2::native_animation_fallbacks_detail;
namespace {
constexpr std::uint32_t scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=5;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,rng()%3?scratch+std::uint32_t(rng()%64)*4u:s.gp[4]+0x1cu+4u*std::uint32_t(rng()%4),4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[5]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[6]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[7]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+20u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1e5e40(WorldState &s){stub(s,0x1e5e40u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1e0000u+pick(0x100));
    m.put(stack+0xcu,(std::uint32_t(rng())&~0xffu)|(pick(3)==0?0u:1u+pick(255)));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(c.abi.gp[0]&~0xffu)|(pick(3)==0?1u+pick(255):0u);
    if(shape==15)m.drop(scratch);
    return c;
}
bool native(WorldState &s){return fallbacks::bridge(s,[](auto &v){original::native_frame_misc_reference_1e5f20(v);},[](auto &v){original::run_1e5e40(v);});}
void reference(WorldState &s){original::run_1e5f20(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ANIMATION_FALLBACKS");
    std::mt19937 rng(0x1e5f20u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,fallbacks::coverage().load(),unsigned(fallbacks::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
