// Native TU1.5 aligned query (2E9750) against the unmodified translated body
// of the death cycle unit (the validator repeats it with the AI frame and
// physics live support units'), extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 2E96A0 and 30B890 are hash-driven stubs of their complete entry ABIs and
// arguments: 2E96A0 writes the record, 30B890 reads it and pops five
// arguments; both write scratch, the caller-saved registers, XMM, flags and
// (by policy) MXCSR are random, and by policy ESI changes or the stack is
// left unbalanced. Fixtures author stacks of every alignment; one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_aligned_query.hpp"
using namespace frame_misc_harness;
namespace query=halo2::native_aligned_query_detail;
namespace {
constexpr std::uint32_t sources=0x10000000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    if(target==0x30b890u)for(std::uint32_t k=0;k<4;++k)h=mix(h,r(s.memory,s.gp[1]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x2e96a0u)for(std::uint32_t k=0;k<4;++k)w(s.memory,s.observe,target+1u,s.gp[1]+4u*k,4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=sources+0x20u*std::uint32_t(rng()%2);
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
static void run_2e96a0(WorldState &s){stub(s,0x2e96a0u,0);}
static void run_30b890(WorldState &s){stub(s,0x30b890u,5);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=sources;at<sources+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x2e0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[1]=sources+0x20u*pick(2);
    if(shape==15)m.drop(sources);
    return c;
}
bool native(WorldState &s) {
    return query::bridge(s,[](auto &v){original::native_frame_misc_reference_2e9750(v);},
        [](auto &v){original::run_2e96a0(v);},[](auto &v){original::run_30b890(v);});
}
void reference(WorldState &s){original::run_2e9750(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ALIGNED_QUERY");
    std::mt19937 rng(0x2e9750u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,query::coverage().load(),unsigned(query::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
