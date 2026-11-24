// Native TU1.5 reference pose (1CF070) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1E5DF0 and 212D40 are hash-driven stubs of their complete entry ABIs and
// arguments: 1E5DF0 fills the record (EDI) and pops 8 bytes, 212D40 writes
// the vector and the value and pops 4, the caller-saved registers, XMM,
// flags and (by policy) MXCSR are random, and by policy EBX, ESI or EDI
// change or the stack is left unbalanced. Fixtures author references whose
// word +6 is NONE or not and outputs over the reference; one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_reference_pose.hpp"
using namespace frame_misc_harness;
namespace pose=halo2::native_reference_pose_detail;
namespace {
constexpr std::uint32_t references=0x10000000u,outputs=0x10010000u,tags=0x10180000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto into=target==0x1e5df0u?s.gp[7]+4u*std::uint32_t(rng()%3):(rng()%2?s.gp[1]+4u*std::uint32_t(rng()%3):s.gp[0]);
    w(s.memory,s.observe,target+1u,rng()%4?into:scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[3]=outputs+4u*std::uint32_t(rng()%8);
    if(policy&2u && rng()%11==0)s.gp[6]=outputs+4u*std::uint32_t(rng()%8);
    if(policy&2u && rng()%11==0)s.gp[7]=std::uint32_t(rng());
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
static void run_1e5df0(WorldState &s){stub(s,0x1e5df0u,2);}
static void run_212d40(WorldState &s){stub(s,0x212d40u,1);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f0414u,tags);
    for(std::uint32_t at=tags;at<tags+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=references;at<references+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto reference_address=references+4u*pick(8);
    m.put(reference_address,(std::uint32_t(rng())<<16)|pick(16));
    m.put(reference_address+6u,pick(4)==0?0xffffu:pick(0xffff),2);
    for(std::uint32_t at=outputs;at<outputs+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1c0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=reference_address;c.abi.gp[6]=outputs+4u*pick(4);c.abi.gp[3]=outputs+0x20u+4u*pick(4);
    if(shape==7)c.abi.gp[6]=reference_address-4u+4u*pick(3);        // the vector over the reference
    if(shape==15) {
        static constexpr std::uint32_t regions[]{references,outputs,tags};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return pose::bridge(s,[](auto &v){original::native_frame_misc_reference_1cf070(v);},
        [](auto &v){original::run_1e5df0(v);},[](auto &v){original::run_212d40(v);});
}
void reference(WorldState &s){original::run_1cf070(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_REFERENCE_POSE");
    std::mt19937 rng(0x1cf070u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,pose::coverage().load(),unsigned(pose::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
