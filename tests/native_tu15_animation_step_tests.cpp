// Native TU1.5 animation step (1D39C0) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1CEB70, 1E3150, 1D38A0, 1D3930 and 1CE950 are hash-driven stubs of their
// complete entry ABIs (and the arguments they are handed): 1E3150 answers a
// record (its frames and flags authored); each writes scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and by
// policy EBX, EBP, ESI or EDI change or the stack is left unbalanced.
// Fixtures author channels in use or not, owners and holds, blends flagged or
// not with silent records or not, the first channel's switches, blending on
// and off, frames positive, zero and negative, durations giving ratios below
// 0, inside and above 1, and one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/animation/native_tu15_animation_step.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_animation_step_detail;
namespace {
constexpr std::uint32_t states=0x10000000u,instances=0x10010000u,records=0x10020000u,scratch=0x10600000u;
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=5;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=target==0x1e3150u?records+0x40u*std::uint32_t(rng()%4):std::uint32_t(rng());
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=rng()%2?0xffffffffu:std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=states+0x100u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[7]=states+0x20u*std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    static constexpr std::pair<std::uint32_t,std::uint32_t> pops[]{{0x1ceb70u,16u},{0x1e3150u,8u},{0x1d38a0u,20u},{0x1d3930u,4u},{0x1ce950u,24u}};
    for(const auto &[t,n]:pops)if(t==target)s.gp[4]+=n+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1ceb70(WorldState &s){stub(s,0x1ceb70u);}
static void run_1e3150(WorldState &s){stub(s,0x1e3150u);}
static void run_1d38a0(WorldState &s){stub(s,0x1d38a0u);}
static void run_1d3930(WorldState &s){stub(s,0x1d3930u);}
static void run_1ce950(WorldState &s){stub(s,0x1ce950u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4f0414u,instances);m.put(0x47c1b4u,pick(4)?1u:0u,1);m.put(0x46a0d0u,bits_of(1.f));
    for(std::uint32_t k=0;k<2;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*(k+1u);at+=4)m.put(at,std::uint32_t(rng()));}
    for(std::uint32_t at=records;at<records+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k){m.put(records+0x40u*k+0x14u,pick(5)?1u+pick(60):pick(2)?0u:0xfff0u,2);m.put(records+0x40u*k+0x18u,pick(4),1);}
    static constexpr float durations[]{0.f,10.f,30.f,90.f,-5.f,0.5f};
    for(std::uint32_t k=0;k<2;++k) {
        const auto st=states+0x100u*k;
        for(std::uint32_t at=st;at<st+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        for(std::uint32_t ch=0;ch<3;++ch) {
            const auto c0=st+0x20u*ch;
            m.put(c0,pick(4)?(std::uint32_t(rng())&0xffff0000u)|pick(2):0xffffffffu);
            m.put(c0+6u,pick(5)?pick(4):0xffffu,2);m.put(c0+0x1cu,bits_of(durations[pick(6)]));
        }
        m.put(st+0x40u,pick(3)?(std::uint32_t(rng())&0xffff0000u)|pick(2):0xffffffffu);m.put(st+0x46u,pick(4)?pick(4):0xffffu,2);
        m.put(st+0x51u,pick(3)?0u:pick(16),1);m.put(st+0x68u,pick(4)?std::uint32_t(rng()):0xffffffffu);m.put(st+0x6cu,pick(4)?0u:1u,1);
        m.put(st+0x12u,pick(2),1);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[6]=states+0x100u*pick(2);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{states,instances,records};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1d39c0(v);},[](auto &v){original::run_1ceb70(v);},
        [](auto &v){original::run_1e3150(v);},[](auto &v){original::run_1d38a0(v);},[](auto &v){original::run_1d3930(v);},
        [](auto &v){original::run_1ce950(v);});
}
void reference(WorldState &s){original::run_1d39c0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ANIMATION_STEP");
    std::mt19937 rng(0x1d39c0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
