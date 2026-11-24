// Native TU1.5 event window (1CEC60) against the unmodified translated body
// of the physics live support unit (the validator repeats it with the AI
// actor unit's copy), extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). 1E3150 is a hash-driven stub of its complete
// entry ABI and argument: it answers an animation, writes scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBX, ESI or EDI change or the stack is left unbalanced.
// Fixtures author channels with and without an animation, rates above, at
// and below 0 (NaN too), frames inside, at the ends of and outside the
// window, and animations of negative, zero, few and many events (more than
// four, so the original's groups run) whose frames fall on, inside and
// outside the window's ends; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/animation/native_tu15_event_window.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_event_window_detail;
namespace {
constexpr std::uint32_t instances=0x10000000u,graphs=0x10010000u,channels=0x10020000u,animations=0x10030000u,events=0x10040000u,
    scratch=0x10600000u;
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x1e3150u);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x1e3151u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=animations+0x60u*std::uint32_t(rng()%3);s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng()%3);
    if(policy&2u && rng()%13==0)s.gp[6]=channels+0x20u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
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
static void run_1e3150(WorldState &s){stub(s);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f0414u,instances);m.put(0x46a0d0u,bits_of(1.f));
    for(std::uint32_t k=0;k<2;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,graphs+0x40u*k);}
    for(std::uint32_t at=graphs;at<graphs+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    static constexpr float rates[]{1.f,0.5f,0.f,-1.f};
    for(std::uint32_t k=0;k<2;++k) {
        const auto channel=channels+0x20u*k;
        for(std::uint32_t at=channel;at<channel+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(channel,(std::uint32_t(rng())&0xffff0000u)|pick(2));m.put(channel+6u,pick(6)?pick(3):0xffffu,2);
        m.put(channel+0x18u,pick(12)?bits_of(rates[pick(4)]):0x7fc00000u);
        m.put(channel+0x1cu,pick(12)?bits_of(float(int(pick(14))-2)):0x7fc00000u);
    }
    for(std::uint32_t k=0;k<3;++k) {
        const auto animation=animations+0x60u*k;
        for(std::uint32_t at=animation;at<animation+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(animation+0x14u,pick(12),2);m.put(animation+0x4cu,pick(6)?pick(14):0u-pick(3));m.put(animation+0x50u,events+0x40u*k);
    }
    for(std::uint32_t k=0;k<3*16;++k){m.put(events+4u*k,pick(20),1);m.put(events+4u*k+1u,std::uint32_t(rng()),1);m.put(events+4u*k+2u,int(pick(15))-1,2);}
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1c0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[6]=channels+0x20u*pick(2);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{instances,graphs,channels,animations,events};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1cec60(v);},[](auto &v){original::run_1e3150(v);});}
void reference(WorldState &s){original::run_1cec60(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_EVENT_WINDOW");
    std::mt19937 rng(0x1cec60u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
