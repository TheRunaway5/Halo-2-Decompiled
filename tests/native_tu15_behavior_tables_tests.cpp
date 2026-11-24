// Native TU1.5 behavior tables (1AF970) against the unmodified translated
// body of the AI actor unit, extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). 1AFA90 is a hash-driven stub of its complete
// entry ABI (and its four arguments): it fills a few of the frame's slots and
// answers their count (none at times, a negative count rarely), writes
// scratch; the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy EBX, EBP, ESI or EDI change or the stack is left
// unbalanced. Fixtures author categories with and without entries (repeated
// behaviors among them), descriptors absent, of kind 1 and of others, both
// stack alignments, the direction flag set at times (its backward clear
// overruns the frame's cursors, a fault in both), and one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "campaign/ai/native_tu15_behavior_tables.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_behavior_tables_detail;
namespace {
constexpr std::uint32_t entries=0x10000000u,descriptors=0x10010000u,defaults=0x10020000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=4;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto slots=r(s.memory,s.gp[4]+4u);
    const auto count=rng()%5?std::uint32_t(rng()%4):rng()%8?0u:0xfffeu;
    for(std::uint32_t k=0;k<(count&0xffu)&&k<4u;++k)w(s.memory,s.observe,0x1afa91u,slots+4u*k,4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(rng()%30000?0u:std::uint32_t(rng()%4)<<16)|count;s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    // About 880 calls a case: the register policies are rare per call.
    if(policy&2u && rng()%10000==0)s.gp[3]=s.gp[3]+0x50u;
    if(policy&2u && rng()%10000==0)s.gp[5]=s.gp[5]+4u;
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%20000==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=20u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1afa90(WorldState &s){stub(s,0x1afa90u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    for(std::uint32_t k=0;k<0x14u;++k) {
        const auto count=pick(3)?pick(5):0u;
        m.put(0x47d590u+8u*k,entries+0x40u*k);m.put(0x47d594u+8u*k,(std::uint32_t(rng())&0xffff0000u)|(pick(12)?count:0x8000u|count));
        for(std::uint32_t e=0;e<5u;++e) {
            const auto entry=entries+0x40u*k+12u*e;
            m.put(entry,(std::uint32_t(rng())&0xffff0000u)|pick(0x83));m.put(entry+4u,std::uint32_t(rng()));m.put(entry+8u,std::uint32_t(rng()));
        }
    }
    for(std::uint32_t k=0;k<0x83u;++k) {
        const auto d=descriptors+0x50u*k;
        m.put(0x47b478u+4u*k,pick(3)?d:0u);
        for(std::uint32_t at=d;at<d+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(d+2u,pick(3)?1u:pick(4),2);m.put(d+0x44u,pick(8),2);m.put(d+0x48u,defaults+0x20u*pick(4));
    }
    for(std::uint32_t at=0x4ff798u;at<0x4ff79au+0x83u*0x50u+8u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=0x4fd538u-0x100u;at<0x4fd538u+0x8000u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=defaults;at<defaults+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x400u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    random_abi(rng,c,stack);
    if(rng()%24==0){c.abi.eflags|=0x400u;for(std::uint32_t at=stack-0x800u;at<stack-0x400u;at+=4)m.put(at,std::uint32_t(rng()));}
    else c.abi.eflags&=~0x400u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{entries,descriptors,defaults};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1af970(v);},[](auto &v){original::run_1afa90(v);});}
void reference(WorldState &s){original::run_1af970(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_BEHAVIOR_TABLES");
    std::mt19937 rng(0x1af970u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
