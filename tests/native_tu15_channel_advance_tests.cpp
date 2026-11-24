// Native TU1.5 channel advance (1CE950) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1CEE60, 1E3150, 1CF700, 1D3660, B8780 and 1CE810 are hash-driven stubs of
// their complete entry ABIs and arguments: 1E3150 and B8780 answer an
// animation, 1D3660 a pointer to the next animation, 1CF700 events (and now
// and then a changed animation or index for the channel); each writes
// scratch, the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy EBX, EBP, ESI or EDI change or the stack is left
// unbalanced. Fixtures author channels with and without an animation index,
// frames below 0, inside, at and past the last frame (NaN too), options
// looping or not, switching or not, resetting the rate or not, and
// animations with and without a loop frame; one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/animation/native_tu15_channel_advance.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_channel_advance_detail;
namespace {
constexpr std::uint32_t instances=0x10000000u,graphs=0x10010000u,channels=0x10020000u,animations=0x10030000u,nexts=0x10040000u,
    scratch=0x10600000u;
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const std::uint32_t arguments=target==0x1cf700u?6u:target==0x1e3150u||target==0x1d3660u||target==0x1ce810u?1u:0u;
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto channel=channels+0x40u*std::uint32_t(rng()%2);
    if(target==0x1cf700u && rng()%10==0)w(s.memory,s.observe,0x1cf701u,channel+4u,2,std::uint32_t(rng()%3));
    if(target==0x1cf700u && rng()%10==0)w(s.memory,s.observe,0x1cf702u,channel+6u,2,std::uint32_t(rng()%3));
    if(target==0x1cf700u && rng()%10==0)w(s.memory,s.observe,0x1cf703u,channel+0x12u,2,std::uint32_t(rng()));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());
    if(target==0x1e3150u||target==0xb8780u)s.gp[0]=animations+0x40u*std::uint32_t(rng()%4);
    if(target==0x1d3660u)s.gp[0]=nexts+4u*std::uint32_t(rng()%4);
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[5]=channels+0x40u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[6]=animations+0x40u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng())&0x00030003u;
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
static void run_1cee60(WorldState &s){stub(s,0x1cee60u);}
static void run_1e3150(WorldState &s){stub(s,0x1e3150u);}
static void run_1cf700(WorldState &s){stub(s,0x1cf700u);}
static void run_1d3660(WorldState &s){stub(s,0x1d3660u);}
static void run_b8780(WorldState &s){stub(s,0xb8780u);}
static void run_1ce810(WorldState &s){stub(s,0x1ce810u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f0414u,instances);m.put(0x46a0ecu,bits_of(0.001f));m.put(0x46a0d0u,bits_of(1.f));
    for(std::uint32_t k=0;k<2;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,graphs+0x40u*k);}
    for(std::uint32_t at=graphs;at<graphs+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k) {
        const auto channel=channels+0x40u*k;
        for(std::uint32_t at=channel;at<channel+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(channel,(std::uint32_t(rng())&0xffff0000u)|pick(2));
        m.put(channel+4u,pick(3),2);m.put(channel+6u,pick(5)?pick(3):0xffffu,2);
        m.put(channel+0x12u,(std::uint32_t(rng())&~0x2au)|(pick(3)?2u:0u)|(pick(2)?0x20u:0u)|(pick(2)?8u:0u),2);
        m.put(channel+0x1cu,bits_of(float(pick(20))));
    }
    for(std::uint32_t k=0;k<4;++k) {
        const auto animation=animations+0x40u*k;
        for(std::uint32_t at=animation;at<animation+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(animation+0x14u,pick(6)?pick(30):0x10000u-pick(3),2);m.put(animation+0x2eu,pick(2)?0u:pick(10),2);
    }
    for(std::uint32_t at=nexts;at<nexts+0x10u;at+=4)m.put(at,std::uint32_t(rng())&0x00030003u);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0xc0u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    static constexpr float frames[]{-3.f,0.f,5.f,12.5f,29.999f,40.f,100.f};
    m.put(stack,0x1c0000u+pick(0x100));m.put(stack+4u,channels+0x40u*pick(2));
    m.put(stack+8u,pick(12)?bits_of(frames[pick(7)]):0x7fc00000u);
    m.put(stack+12u,std::uint32_t(rng()));m.put(stack+16u,std::uint32_t(rng()));m.put(stack+20u,std::uint32_t(rng()));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{instances,graphs,channels,animations,nexts};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1ce950(v);},[](auto &v){original::run_1cee60(v);},
        [](auto &v){original::run_1e3150(v);},[](auto &v){original::run_1cf700(v);},[](auto &v){original::run_1d3660(v);},
        [](auto &v){original::run_b8780(v);},[](auto &v){original::run_1ce810(v);});
}
void reference(WorldState &s){original::run_1ce950(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CHANNEL_ADVANCE");
    std::mt19937 rng(0x1ce950u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
