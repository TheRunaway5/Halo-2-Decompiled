// Native TU1.5 round timer (164EF0) against the unmodified translated body of
// the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The display's virtual and B1E80 and 1A8740 (through the unit's dispatch)
// are hash-driven stubs of their complete entry ABIs: the display hashes its
// arguments and answers the remaining ticks or another value (RET C); B1E80
// and 1A8740 hash the event; the caller-saved registers, XMM, flags and (by
// policy) MXCSR are random, and by policy EBX, ESI or EDI change. Fixtures
// author rounds before, at and past their limit, rates and limits of either
// sign, displays for each local player, the announced byte set or not and
// the untimed mode; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_round_timer.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_round_timer_detail;
namespace {
constexpr std::uint32_t variants=0x10000000u,sessions=0x10010000u,times=0x10020000u,displays=0x10030000u,vtables=0x10040000u,
    scratch=0x10600000u;
constexpr std::uint32_t shows[]{0x2fb000u,0x2fb100u};
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const bool show=target==shows[0]||target==shows[1];
    if(show)for(std::uint32_t k=1;k<=3;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    const auto event=target==0xb1e80u?s.gp[2]:target==0x1a8740u?s.gp[7]:0u;
    if(event)for(std::uint32_t k=0;k<9;++k)h=mix(h,r(s.memory,event+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto remaining=show?r(s.memory,s.gp[4]+4u):0u;
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(show)s.gp[0]=rng()%3?remaining:remaining+1u;
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    s.gp[4]+=show?16u:4u;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=shows[0] && target!=shows[1] && target!=0xb1e80u && target!=0x1a8740u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x4f3218u,variants);m.put(0x4f63b8u,sessions);m.put(0x51df54u,times);
    for(std::uint32_t at=variants;at<variants+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(variants+0xcu,pick(3)?pick(6):4u,1);m.put(variants+0x190u,pick(5)?pick(600):0u-pick(10));
    for(std::uint32_t at=sessions;at<sessions+0xc20u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto tick=std::uint32_t(rng()%100000u);
    m.put(sessions+0x70u,pick(4)?tick-pick(40000):tick+pick(100));m.put(sessions+0xc14u,pick(4));
    for(std::uint32_t at=times;at<times+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(times+8u,tick);m.put(times+2u,pick(6)?30u:0x10000u-pick(3),2);
    for(std::uint32_t k=0;k<4;++k)m.put(0x56dfa0u+4u*k,displays+0x20u*k);
    for(std::uint32_t at=displays;at<displays+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(displays+0x20u*k,vtables+0x80u*pick(2));
    for(std::uint32_t at=vtables;at<vtables+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(vtables+0x64u,shows[0]);m.put(vtables+0xe4u,shows[1]);
    m.put(0x56e224u,pick(3)?0u:1u+pick(255),1);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x160000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[3]=(c.abi.gp[3]&0xffffff00u)|(pick(4)?1u+pick(255):0u);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{variants,sessions,times,displays,vtables};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_164ef0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_164ef0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ROUND_TIMER");
    std::mt19937 rng(0x164ef0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
