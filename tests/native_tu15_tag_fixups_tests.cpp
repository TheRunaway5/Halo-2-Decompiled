// Native TU1.5 tag group fixups (11EF70) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The tag iterator (125790) is a stub that walks a fixture list of handles
// through the iterator's position (then NONE); the fixups and the two
// postprocessing passes are hash-driven stubs of their complete entry ABIs
// (1F2C70's argument too, through the unit's dispatch): each writes
// scratch, the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy EDI changes or the stack is left unbalanced.
// Fixtures author tags of every group the pass knows and others; one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_tag_fixups.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_tag_fixups_detail;
namespace {
constexpr std::uint32_t instances=0x10000000u,definitions=0x10010000u,list=0x10020000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x125790u) {
        const auto position=r(s.memory,s.gp[7]+0xcu);
        s.gp[0]=position<r(s.memory,list)?r(s.memory,list+4u+4u*(position&15u)):0xffffffffu;
        w(s.memory,s.observe,target,s.gp[7]+0xcu,4,position+1u);
    } else s.gp[0]=std::uint32_t(rng());
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target!=0x125790u && policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(target!=0x125790u && policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*arguments+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_125790(WorldState &s){stub(s,0x125790u,0);}
static void run_1caf40(WorldState &s){stub(s,0x1caf40u,0);}
static void run_21a580(WorldState &s){stub(s,0x21a580u,0);}
static void run_20d810(WorldState &s){stub(s,0x20d810u,0);}
static void run_11f060(WorldState &s){stub(s,0x11f060u,0);}
static void run_165b30(WorldState &s){stub(s,0x165b30u,0);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x1f2c70u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target,1);
}
#include "body.inc"
#include "reference.inc"
void step(WorldState &s,std::uint32_t target) {
    switch(target) {
    case 0x125790u:run_125790(s);return;
    case 0x1caf40u:run_1caf40(s);return;
    case 0x21a580u:run_21a580(s);return;
    case 0x20d810u:run_20d810(s);return;
    case 0x11f060u:run_11f060(s);return;
    case 0x165b30u:run_165b30(s);return;
    case 0x1f2c70u:dispatch(s,target,0x11f02cu,false);return;
    }
    throw std::runtime_error("Unbound sandbox step");
}
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    static constexpr std::uint32_t groups[]{0x63726561u,0x62697064u,0x636f6c6cu,0x70686d6fu,0x76656869u,0x6d6f6465u,0x62697063u,0x76656868u,0u};
    m.put(0x4f0414u,instances);
    for(std::uint32_t k=0;k<16;++k) {
        m.put(instances+16u*k,groups[pick(9)]);m.put(instances+16u*k+4u,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,definitions+0x400u*(k%4));m.put(instances+16u*k+12u,std::uint32_t(rng()));
    }
    for(std::uint32_t at=definitions;at<definitions+0x1000u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(list,pick(10));
    for(std::uint32_t k=0;k<16;++k)m.put(list+4u+4u*k,(std::uint32_t(rng())&0x7fff0000u)|pick(16));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x190000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{instances,definitions};
        m.drop(regions[pick(2)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_11ef70(v);},
        [](auto &v,std::uint32_t target){original::step(v,target);});
}
void reference(WorldState &s){original::run_11ef70(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_TAG_FIXUPS");
    std::mt19937 rng(0x11ef70u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
