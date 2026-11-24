// Native TU1.5 animation names (1D45A0) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1E5810 is a hash-driven stub of its complete entry ABI (and its three
// arguments): it answers an entry (its name NONE, 30000D9 or another) or
// none, writes scratch; the caller-saved registers, XMM, flags and (by
// policy) MXCSR are random, and by policy EBX, EBP, ESI or EDI change or the
// stack is left unbalanced. Fixtures author names unset, none, NONE, the
// fixed second and others, records with each of those, and one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/animation/native_tu15_animation_names.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_animation_names_detail;
namespace {
constexpr std::uint32_t records=0x10000000u,instances=0x10010000u,definitions=0x10020000u,outputs=0x10030000u,entries=0x10040000u,
    scratch=0x10600000u;
constexpr std::uint32_t names[]{0x7000101u,0x7000001u,0xffffffffu,0x400054bu,0x30000d9u,0x5000010u,0x6000020u};
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=3;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto entry=entries+0x10u*std::uint32_t(rng()%4);
    w(s.memory,s.observe,0x1e5811u,entry+4u,4,names[2u+rng()%5]);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=rng()%4?entry:0u;s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=records+0x100u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[5]=names[rng()%7];
    if(policy&2u && rng()%13==0)s.gp[6]=outputs+0x10u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=names[rng()%7];
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=16u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1e5810(WorldState &s){stub(s,0x1e5810u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto name=[&]{return names[pick(7)];};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4f0414u,instances);
    for(std::uint32_t k=0;k<2;++k) {
        const auto record=records+0x100u*k;
        for(std::uint32_t at=record;at<record+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(record+0x68u,(std::uint32_t(rng())&0xffff0000u)|pick(2));
        for(std::uint32_t at=record+0x70u;at<record+0x80u;at+=4)m.put(at,name());
    }
    for(std::uint32_t k=0;k<2;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*(k+1u);at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,definitions+0x100u*k);}
    for(std::uint32_t at=definitions;at<definitions+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=outputs;at<outputs+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=entries;at<entries+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));m.put(stack+4u,records+0x100u*pick(2));m.put(stack+8u,name());
    random_abi(rng,c,stack);
    c.abi.gp[0]=name();c.abi.gp[1]=name();c.abi.gp[2]=name();c.abi.gp[6]=outputs+0x10u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{records,instances,definitions,outputs,entries};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1d45a0(v);},[](auto &v){original::run_1e5810(v);});}
void reference(WorldState &s){original::run_1d45a0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ANIMATION_NAMES");
    std::mt19937 rng(0x1d45a0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
