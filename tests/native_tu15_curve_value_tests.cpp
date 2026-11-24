// Native TU1.5 curve value (13E670) against the unmodified translated bodies
// of the lens draw and physics live support units, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 13DE70 is a hash-driven stub of its complete entry ABI and arguments: it
// answers a fraction in XMM0 (below zero, inside, above one, the ends, NaN),
// writes scratch, the caller-saved registers, XMM, flags and (by policy)
// MXCSR are random, and by policy EDI changes or the stack is left
// unbalanced. Fixtures author functions with and without a curve and a
// count, curves with and without the 0xF0 flags, and ends of every sign;
// one region removed injects a fault. Modes: <cases> [census|verify]. Prints
// a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_curve_value.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_curve_value_detail;
namespace {
constexpr std::uint32_t functions=0x10000000u,curves=0x10010000u,scratch=0x10600000u;
constexpr std::uint32_t fractions[]{0x3f000000u,0x3e800000u,0u,0x3f800000u,0xbf000000u,0x40000000u,0x7fc00000u,0x80000000u,0x3f7fffffu};
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x13de70u);
    h=mix(h,r(s.memory,s.gp[4]+4u));h=mix(h,r(s.memory,s.gp[4]+8u));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x13de72u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=functions+0x10u*std::uint32_t(rng()%8);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    for(unsigned k=0;k<4;++k){std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
        std::memcpy(&s.xmm[k],lanes.data(),16);}
    std::array<std::uint32_t,4> lanes{fractions[rng()%9],std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[0],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=12u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_13de70(WorldState &s){stub(s);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x46a0d0u,0x3f800000u);
    static constexpr std::uint32_t floats[]{0x3f800000u,0x40a00000u,0xc0400000u,0u,0x7fc00000u,0x3dcccccdu,0x7f7fffffu};
    for(std::uint32_t k=0;k<8;++k) {
        const auto f=functions+0x10u*k;
        m.put(f,pick(4)?1u+pick(4):pick(2)?0u:0x80000000u);m.put(f+4u,pick(6)?curves+0x10u*pick(8):0u);
        m.put(f+8u,std::uint32_t(rng()));m.put(f+12u,std::uint32_t(rng()));
        const auto cv=curves+0x10u*k;
        m.put(cv,std::uint32_t(rng()));m.put(cv+1u,pick(4)==0?0x10u<<pick(4):pick(16),1);
        m.put(cv+4u,floats[pick(7)]);m.put(cv+8u,floats[pick(7)]);m.put(cv+12u,std::uint32_t(rng()));
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x130000u+pick(0x100));m.put(stack+4u,std::uint32_t(rng()));m.put(stack+8u,std::uint32_t(rng()));
    random_abi(rng,c,stack);
    c.abi.gp[0]=functions+0x10u*pick(8);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{functions,curves};
        m.drop(regions[pick(2)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_13e670(v);},[](auto &v){original::run_13de70(v);});}
void reference(WorldState &s){original::run_13e670(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CURVE_VALUE");
    std::mt19937 rng(0x13e670u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
