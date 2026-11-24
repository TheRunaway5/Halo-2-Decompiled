// Native TU1.5 countdown step (1CEB70) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1CE950 is a hash-driven stub of its complete entry ABI and arguments: it
// writes scratch, the caller-saved registers, XMM, flags and (by policy)
// MXCSR are random, and by policy the stack is left unbalanced. Fixtures
// author countdowns of zero, one and more, the enable and blocking bits,
// and rates and bases (NaN too) under the random MXCSR; one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_countdown_step.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_countdown_step_detail;
namespace {
constexpr std::uint32_t records=0x10000000u,times=0x10010000u,scratch=0x10600000u;
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x1ce950u);
    for(std::uint32_t k=1;k<=5;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x1ce952u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=24u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1ce950(WorldState &s){stub(s);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    static constexpr std::uint32_t floats[]{0x3f800000u,0x3d088889u,0x40400000u,0u,0x7fc00000u,0xbf800000u};
    m.put(0x51df54u,times);m.put(0x46a20cu,pick(3)?0x3f000000u:floats[pick(6)]);
    for(std::uint32_t at=times;at<times+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(times+4u,floats[pick(6)]);
    for(std::uint32_t at=records;at<records+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(records+0x10u,pick(3)?0u:pick(2)?1u:pick(256),1);m.put(records+0x11u,pick(2)?0u:pick(16),1);m.put(records+0x12u,pick(4),1);
    m.put(records+0x18u,floats[pick(6)]);m.put(records+0x1cu,floats[pick(6)]);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1c0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=records;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{records,times};
        m.drop(regions[pick(2)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1ceb70(v);},[](auto &v){original::run_1ce950(v);});}
void reference(WorldState &s){original::run_1ceb70(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_COUNTDOWN_STEP");
    std::mt19937 rng(0x1ceb70u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
