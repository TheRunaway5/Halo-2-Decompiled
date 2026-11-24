// Native TU1.5 transition fraction (1E1620) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The curves 110FC0 and 1E1920 are hash-driven stubs: XMM0 below 0, within,
// above 1 or unordered (its upper lanes random), the caller-saved registers
// random, sometimes the global one rewritten, the flags and (by policy)
// MXCSR mixed in. Fixtures author
// transitions (no duration, short and long ones, every curve and others),
// one (1, or a special float), and the curve table (unbound targets too);
// one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_transition_fraction.hpp"
using namespace frame_misc_harness;
namespace fraction=halo2::native_transition_fraction_detail;
namespace {
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    static constexpr std::uint32_t results[]{0xbf000000u,0x3e99999au,0x3f800000u,0x3fc00000u,0x7fc00000u,0x80000000u,0u,0x3f7fffffu};
    // A curve that changes the global one shows the original re-reading it.
    if(rng()%5==0)w(s.memory,s.observe,target+1u,0x46a0d0u,4,results[rng()%8]);
    std::array<std::uint32_t,4> lanes{rng()%3==0?std::uint32_t(rng()):results[rng()%8],std::uint32_t(rng()),
        std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[0],lanes.data(),16);
    for(auto i:{1u,2u,3u})if(rng()%2){std::array<std::uint32_t,4> l{std::uint32_t(rng()),std::uint32_t(rng()),
        std::uint32_t(rng()),std::uint32_t(rng())};std::memcpy(&s.xmm[i],l.data(),16);}
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%7==0)s.gp[3+std::uint32_t(rng()%2)*2u]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    s.gp[4]+=4u;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
// The unit's curve table targets, as its canonical_branch names them.
std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage) {
    switch(p) {
    case 0x1e1661u:case 0x1e1673u:case 0x1e1685u:case 0x1e1699u:return p;
    default:throw std::runtime_error("Unbound TU1.5 contact dispatch target");
    }
}
static void run_110fc0(WorldState &s){stub(s,0x110fc0u);}
static void run_1e1920(WorldState &s){stub(s,0x1e1920u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target==0x1e1920u)run_1e1920(s);
    else throw std::runtime_error("Unexpected dispatch");
}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t transitions=0x10000000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=transitions;at<transitions+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto address=transitions+pick(0xf0u);
    const auto byte=[&](std::uint32_t at,std::uint32_t v){m.put(at&~3u,(m.get(at&~3u)&~(0xffu<<8u*(at&3u)))|((v&0xffu)<<8u*(at&3u)));};
    static constexpr std::int8_t durations[]{0,1,-1,2,3,30,-128,127};
    static constexpr std::int8_t curves[]{0,5,-1,6,-128,127,1,4};
    if(pick(4))byte(address+1u,std::uint32_t(durations[pick(8)]));
    if(pick(4))byte(address+2u,pick(3)?1u+pick(4):std::uint32_t(curves[pick(8)]));
    if(pick(2))byte(address,std::uint32_t(std::int8_t(pick(40))-5));
    static constexpr std::uint32_t ones[]{0x3f800000u,0x3f800000u,0x3f800000u,0x7fc00000u,0u,0xbf800000u,0x00000001u,0x40000000u};
    m.put(0x46a0d0u,shape==9?std::uint32_t(rng()):ones[pick(8)]);
    static constexpr std::uint32_t cases[]{0x1e1661u,0x1e1673u,0x1e1685u,0x1e1699u};
    for(std::uint32_t k=0;k<4;++k)m.put(0x1e16ccu+4u*k,shape==13 && pick(3)==0?0x1e1700u+pick(0x40):cases[k]);
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1e0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=address;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{transitions,0x46a000u,0x1e1000u};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return fraction::bridge(s,[](auto &v){original::native_frame_misc_reference_1e1620(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);},
        [](auto &v){original::run_110fc0(v);});
}
void reference(WorldState &s){original::run_1e1620(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_TRANSITION_FRACTION");
    std::mt19937 rng(0x1e1620u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,fraction::coverage().load(),unsigned(fraction::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
