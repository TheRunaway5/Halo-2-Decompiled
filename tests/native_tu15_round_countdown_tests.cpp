// Native TU1.5 round countdown (15E870) against the unmodified translated
// body of the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 164EF0 and 15DD50 (direct), B1A10 and 1A87A0 (through the unit's dispatch)
// and 15DDC0 (the World's required owner) are hash-driven stubs of their
// complete entry ABIs: 164EF0 answers ticks around each announced second,
// zero and below, B1A10 returns with RET 4, 1A87A0 hashes the event,
// 15DD50 answers EAX, 15DDC0 returns with RET 4; the caller-saved registers,
// XMM, flags and (by policy) MXCSR are random, and by policy EBX, EBP, ESI or
// EDI change. Fixtures author sessions running or not, timed and untimed
// variants, mode 4, the last announced seconds equal or not; one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_round_countdown.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_round_countdown_detail;
namespace {
constexpr std::uint32_t sessions=0x10000000u,variants=0x10010000u,times=0x10020000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const unsigned args=target==0xb1a10u||target==0x15ddc0u?1u:0u;
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    if(target==0x1a87a0u)for(std::uint32_t k=0;k<9;++k)h=mix(h,r(s.memory,s.gp[0]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x164ef0u) {
        static constexpr std::uint32_t seconds[]{1800u,900u,300u,60u,30u,10u,0u,5u,1000u};
        s.gp[0]=rng()%7?seconds[rng()%9]*30u+std::uint32_t(rng()%3):0u-std::uint32_t(rng()%40);
    }
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng()%3)-1u;
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[6]=target==0x164ef0u?sessions:std::uint32_t(rng()%2000);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    s.gp[4]+=4u+4u*args;
}
const SourcePhysicsContactRuntimeServices::RequiredOwner required=[](SourcePhysicsContactMemory &m,const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsContactRuntimeAbi &a,const SourcePhysicsContactRecordObserver &o) {
    if(call.target!=0x15ddc0u)throw std::runtime_error("Unowned required target");
    SourcePhysicsWorldRuntimeServices none;
    WorldState s{m,o,none,SourcePhysicsContactImage::TitleUpdate15};s.load(a);
    stub(s,call.target);
    a=s.save();
};
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_164ef0(WorldState &s){stub(s,0x164ef0u);}
static void run_15dd50(WorldState &s){stub(s,0x15dd50u);}
static void run_15ddc0(WorldState &s){stub(s,0x15ddc0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0xb1a10u && target!=0x1a87a0u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4f63b8u,sessions);m.put(0x4f3218u,variants);m.put(0x51df54u,times);
    for(std::uint32_t at=sessions;at<sessions+0xc20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(sessions+0x6cu,pick(6)?1u:pick(3),2);m.put(sessions+0xc04u,pick(6)?1u:pick(3));m.put(sessions+0xc14u,pick(4));
    const auto tick=std::uint32_t(rng()%200000u);
    m.put(sessions+0x70u,pick(4)?tick-pick(100000):tick+pick(100));
    static constexpr std::uint32_t seconds[]{1800u,900u,300u,60u,30u,10u,0u};
    m.put(sessions+0xe0u,pick(3)?seconds[pick(7)]:std::uint32_t(rng()),2);
    for(std::uint32_t k=0;k<4;++k)m.put(0x56dfa0u+4u*k,pick(8)?0x10030000u+0x20u*k:0u);
    for(std::uint32_t at=variants;at<variants+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(variants+0xcu,pick(5)?pick(4):4u,1);m.put(variants+0x190u,pick(4)?1u+pick(600):0u);
    for(std::uint32_t at=times;at<times+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    float rate=pick(4)?1.0f/30.0f:pick(2)?float(pick(3))/7.0f:-1.0f/30.0f;std::uint32_t v;std::memcpy(&v,&rate,4);
    if(pick(16)==0)v=0x7fc00000u;
    m.put(times+4u,v);m.put(times+8u,tick);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{sessions,variants,times,0x56d000u};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_15e870(v);},[](auto &v){original::run_164ef0(v);},
        [](auto &v){original::run_15dd50(v);},[](auto &v){v.required(0x15e9dfu,0x15ddc0u,false);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_15e870(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ROUND_COUNTDOWN");
    std::mt19937 rng(0x15e870u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t,false,required))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
