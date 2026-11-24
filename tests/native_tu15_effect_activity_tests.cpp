// Native TU1.5 effect activity (17C5C0) against the unmodified translated
// body of the physics live support unit (the validator repeats it with the
// death cycle unit's), extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). The children 3EB10, 11E7B0, 11E780 and 17A4A0
// are hash-driven stubs of their complete entry ABI: AL accepts or not, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy a callee-saved register moves to another effect or tag. Fixtures
// author effects (with and without a location, owned or not, every flag
// combination), tags (both position tests), the game globals (none, holding
// or not); one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_effect_activity.hpp"
using namespace frame_misc_harness;
namespace activity=halo2::native_effect_activity_detail;
namespace {
constexpr std::uint32_t table=0x10000000u,effects=0x10010000u,instances=0x10100000u,tags=0x10180000u,game=0x10200000u,
    scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%2?0u:1u+std::uint32_t(rng()%255));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0) {
        switch(rng()%3) {
        case 0:s.gp[3]=(std::uint32_t(rng())<<16)|std::uint32_t(rng()%4);break;
        case 1:s.gp[6]=effects+0x190u*std::uint32_t(rng()%4);break;
        default:s.gp[7]=tags+0x40u*std::uint32_t(rng()%4);break;
        }
    }
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
static void run_3eb10(WorldState &s){stub(s,0x3eb10u);}
static void run_11e780(WorldState &s){stub(s,0x11e780u);}
static void run_11e7b0(WorldState &s){stub(s,0x11e7b0u);}
static void run_17a4a0(WorldState &s){stub(s,0x17a4a0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    switch(target) {
    case 0x3eb10u:run_3eb10(s);return;
    case 0x11e780u:run_11e780(s);return;
    case 0x11e7b0u:run_11e7b0(s);return;
    case 0x17a4a0u:run_17a4a0(s);return;
    }
    throw std::runtime_error("Unbound dispatch target");
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
    m.put(0x4f720cu,table);
    for(std::uint32_t at=table;at<table+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(table+0x44u,effects);
    m.put(0x4f0414u,instances);
    // Tag indices past 0x1000 leave bits above ECX's low word after its shift.
    for(const std::uint32_t base:{0u,0x1000u})for(std::uint32_t k=0;k<8;++k) {
        const auto instance=instances+16u*(base+k);
        m.put(instance,std::uint32_t(rng()));m.put(instance+4u,std::uint32_t(rng()));
        m.put(instance+8u,tags+0x40u*(k%4));m.put(instance+12u,std::uint32_t(rng()));
    }
    for(std::uint32_t at=tags;at<tags+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k) {
        const auto effect=effects+0x190u*k;
        for(std::uint32_t at=effect;at<effect+0x190u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(effect+4u,(std::uint32_t(rng())<<16)|(pick(2)?0x1000u:0u)|pick(8));
        m.put(effect+0x20u,pick(3)==0?0xffffu:pick(4)==0?0x8000u+pick(64):pick(64),2);
        m.put(effect+0x48u,pick(2)?0xffffffffu:std::uint32_t(rng()));
        m.put(effect+2u,std::uint32_t(rng())&(pick(2)?0x020au:0xffffu),2);
    }
    m.put(0x51df50u,pick(3)==0?0u:game);
    for(std::uint32_t at=game;at<game+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(game+5u,pick(3)==0?1u+pick(255):0u,1);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x170000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[3]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{table,effects,instances,tags,game};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return activity::bridge(s,[](auto &v){original::native_frame_misc_reference_17c5c0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_17c5c0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_EFFECT_ACTIVITY");
    std::mt19937 rng(0x17c5c0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,activity::coverage().load(),unsigned(activity::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
