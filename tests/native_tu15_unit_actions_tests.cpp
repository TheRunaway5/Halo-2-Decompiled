// Native TU1.5 unit actions (C9010) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The nine updates are hash-driven stubs of their complete entry ABIs and
// arguments: each answers AL (often zero), writes scratch, the caller-saved
// registers, XMM, flags and (by policy) MXCSR are random, and by policy EBX
// or ESI change or the stack is left unbalanced. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_unit_actions.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_unit_actions_detail;
namespace {
constexpr std::uint32_t scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%4==0?std::uint32_t(rng()%256):0u);s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[6]=std::uint32_t(rng());
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
static void run_c6e90(WorldState &s){stub(s,0xc6e90u,1);}
static void run_c7dd0(WorldState &s){stub(s,0xc7dd0u,1);}
static void run_c85a0(WorldState &s){stub(s,0xc85a0u,1);}
static void run_c8390(WorldState &s){stub(s,0xc8390u,0);}
static void run_c8c20(WorldState &s){stub(s,0xc8c20u,0);}
static void run_117340(WorldState &s){stub(s,0x117340u,0);}
static void run_c8e70(WorldState &s){stub(s,0xc8e70u,0);}
static void run_c8cf0(WorldState &s){stub(s,0xc8cf0u,0);}
static void run_c7580(WorldState &s){stub(s,0xc7580u,0);}
#include "body.inc"
#include "reference.inc"
void step(WorldState &s,std::uint32_t target) {
    switch(target) {
    case 0xc6e90u:run_c6e90(s);return;
    case 0xc7dd0u:run_c7dd0(s);return;
    case 0xc85a0u:run_c85a0(s);return;
    case 0xc8390u:run_c8390(s);return;
    case 0xc8c20u:run_c8c20(s);return;
    case 0x117340u:run_117340(s);return;
    case 0xc8e70u:run_c8e70(s);return;
    case 0xc8cf0u:run_c8cf0(s);return;
    case 0xc7580u:run_c7580(s);return;
    }
    throw std::runtime_error("Unbound sandbox step");
}
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xb90000u+pick(0x100));m.put(stack+4u,std::uint32_t(rng()));
    random_abi(rng,c,stack);
    if(n%16==15)m.drop(stack-0x80u);
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_c9010(v);},
        [](auto &v,std::uint32_t target){original::step(v,target);});
}
void reference(WorldState &s){original::run_c9010(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_UNIT_ACTIONS");
    std::mt19937 rng(0xc9010u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
