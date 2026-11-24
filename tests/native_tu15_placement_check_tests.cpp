// Native TU1.5 placement check (175300) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The checks B1720, 1751B0, B16C0, 175280, 175240 and 1752C0 are
// hash-driven stubs of their complete entry ABI: AL passes or fails, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBX moves to another record or the stack is left unbalanced.
// Fixtures author records (none, unchecked, checked) with random parts;
// one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_placement_check.hpp"
using namespace frame_misc_harness;
namespace placement=halo2::native_placement_check_detail;
namespace {
constexpr std::uint32_t records=0x10000000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%7?1u+std::uint32_t(rng()%255):0u);
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=records+0x100u*std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_b1720(WorldState &s){stub(s,0xb1720u);}
static void run_1751b0(WorldState &s){stub(s,0x1751b0u);}
static void run_b16c0(WorldState &s){stub(s,0xb16c0u);}
static void run_175280(WorldState &s){stub(s,0x175280u);}
static void run_175240(WorldState &s){stub(s,0x175240u);}
static void run_1752c0(WorldState &s){stub(s,0x1752c0u);}
void route(WorldState &s,std::uint32_t target) {
    switch(target) {
    case 0xb1720u:run_b1720(s);return;
    case 0x1751b0u:run_1751b0(s);return;
    case 0xb16c0u:run_b16c0(s);return;
    case 0x175280u:run_175280(s);return;
    case 0x175240u:run_175240(s);return;
    case 0x1752c0u:run_1752c0(s);return;
    }
    throw std::runtime_error("Unrouted check");
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
    for(std::uint32_t at=records;at<records+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(records+0x100u*k,(m.get(records+0x100u*k)&~1u)|(pick(5)?1u:0u),1);
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x170000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[3]=pick(10)==0?0u:records+0x100u*pick(4);
    if(shape==15)m.drop(records);
    return c;
}
bool native(WorldState &s) {
    return placement::bridge(s,[](auto &v){original::native_frame_misc_reference_175300(v);},
        [](auto &v,std::uint32_t target){original::route(v,target);});
}
void reference(WorldState &s){original::run_175300(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PLACEMENT_CHECK");
    std::mt19937 rng(0x175300u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,placement::coverage().load(),unsigned(placement::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
