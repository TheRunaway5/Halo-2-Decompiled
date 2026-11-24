// Native TU1.5 scaled reference pose (1CEFB0) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1CF070 is a hash-driven stub of its complete entry ABI and argument: it
// writes the pose's vector and value (ESI, EBX) and pops the count, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBP or EDI change or the stack is left unbalanced. Fixtures
// author references whose word +6 is NONE or not, rates and factors of
// ordinary floats and specials (NaN, infinities, out-of-range for the
// truncation) and outputs over the reference; one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_scaled_pose.hpp"
using namespace frame_misc_harness;
namespace scaled=halo2::native_scaled_pose_detail;
namespace {
constexpr std::uint32_t references=0x10000000u,outputs=0x10010000u,zero=0x10020000u;
std::uint32_t value(std::mt19937_64 &rng) {
    static constexpr std::uint32_t specials[]{0u,0x80000000u,0x7f800000u,0xff800000u,0x7fc00000u,0x4f000000u,0xcf000001u,0x3f800000u};
    if(rng()%6==0)return specials[rng()%8];
    return std::bit_cast<std::uint32_t>(float(int(rng()%20001)-10000)/97.0f);
}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    for(std::uint32_t k=0;k<3;++k)w(s.memory,s.observe,target+1u,s.gp[6]+4u*k,4,value(rng));
    w(s.memory,s.observe,target+2u,s.gp[3],4,value(rng));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[5]=outputs+4u*std::uint32_t(rng()%8);
    if(policy&2u && rng()%11==0)s.gp[7]=outputs+0x20u+4u*std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=8u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1cf070(WorldState &s){stub(s,0x1cf070u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    std::mt19937_64 values(rng());
    m.put(0x474d04u,zero);m.put(0x46a20cu,value(values));
    for(std::uint32_t at=zero;at<zero+0x10u;at+=4)m.put(at,value(values));
    for(std::uint32_t at=references;at<references+0x40u;at+=4)m.put(at,value(values));
    const auto reference_address=references+4u*pick(4);
    m.put(reference_address+6u,pick(4)==0?0xffffu:pick(0xffff),2);
    for(std::uint32_t at=outputs;at<outputs+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1c0000u+pick(0x100));
    m.put(stack+4u,outputs+0x30u+4u*pick(4));
    random_abi(rng,c,stack);
    c.abi.gp[1]=reference_address;c.abi.gp[7]=shape==7?reference_address+4u*pick(6):outputs+4u*pick(8);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{references,outputs,zero};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s){return scaled::bridge(s,[](auto &v){original::native_frame_misc_reference_1cefb0(v);},[](auto &v){original::run_1cf070(v);});}
void reference(WorldState &s){original::run_1cefb0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SCALED_POSE");
    std::mt19937 rng(0x1cefb0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,scaled::coverage().load(),unsigned(scaled::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
