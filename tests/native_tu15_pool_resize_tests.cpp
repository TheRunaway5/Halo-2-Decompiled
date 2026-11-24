// Native TU1.5 pool resize (191130) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 33B590, 1910A0 and 191220 are hash-driven stubs of their complete entry
// ABIs (and arguments): 1910A0 answers a new block (into the label's slot)
// or failure; each writes scratch, the caller-saved registers, XMM, flags and
// (by policy) MXCSR are random, and by policy EBX, EBP, ESI or EDI change or
// the stack is left unbalanced. Fixtures author guarded and plain pools,
// blocks with a next block or at the pool's end, sizes that fit (aligned or
// not) and that move, both string directions, pools that keep owners or
// not, and one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "runtime/native_tu15_pool_resize.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_pool_resize_detail;
namespace {
constexpr std::uint32_t pools=0x10000000u,blocks=0x10010000u,fresh=0x10020000u,handles=0x10030000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    if(target==0x1910a0u)for(std::uint32_t k=1;k<=3;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    std::uint32_t answer=std::uint32_t(rng());
    if(target==0x1910a0u) {
        answer&=0xffffff00u;
        if(rng()%6){answer|=1u+std::uint32_t(rng()%255);w(s.memory,s.observe,0x1910a1u,r(s.memory,s.gp[4]+4u),4,fresh+0x40u+0x10u*std::uint32_t(rng()%4));}
    }
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=answer;s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=handles+4u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[5]=pools+0x40u;
    if(policy&2u && rng()%13==0)s.gp[6]=blocks+0x10u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng()%0x100);
    if(policy&2u && rng()%17==0)s.other_flags^=0x400u;
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=(target==0x1910a0u?16u:4u)+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_33b590(WorldState &s){stub(s,0x33b590u);}
static void run_1910a0(WorldState &s){stub(s,0x1910a0u);}
static void run_191220(WorldState &s){stub(s,0x191220u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    const bool guarded=pick(3)==0;
    // The pool, and a second one a stub may leave in EBP.
    for(std::uint32_t k=0;k<2;++k) {
        const auto pool=pools+0x40u*k;
        for(std::uint32_t at=pool;at<pool+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(pool+0x28u,blocks);m.put(pool+0x2cu,0x200u+0x40u*pick(4));m.put(pool+0x30u,pick(0x1000));
        m.put(pool+0x3cu,pick(2)?0u:1u,1);m.put(pool+0x3fu,(k?pick(2)==0:guarded)?1u:0u,1);
    }
    for(std::uint32_t at=blocks;at<blocks+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=fresh;at<fresh+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto data=blocks+0x40u+0x10u*pick(8);
    const auto header=data-0x10u;
    m.put(header,(guarded?0x20u:0x10u)+0x10u*(1u+pick(8))+(pick(6)?0u:pick(4)));
    m.put(header+8u,pick(3)?0u:header+0x40u+0x10u*pick(8));
    for(std::uint32_t at=handles;at<handles+0x10u;at+=4)m.put(at,data);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));m.put(stack+4u,pools);m.put(stack+8u,std::uint32_t(rng()));m.put(stack+12u,pick(1000));
    random_abi(rng,c,stack);
    c.abi.gp[0]=pick(4)?pick(0x80):pick(0x400);c.abi.gp[3]=handles;
    if(rng()%4==0)c.abi.eflags|=0x400u;else c.abi.eflags&=~0x400u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{pools,blocks,fresh,handles};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_191130(v);},[](auto &v){original::run_33b590(v);},
        [](auto &v){original::run_1910a0(v);},[](auto &v){original::run_191220(v);});
}
void reference(WorldState &s){original::run_191130(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_POOL_RESIZE");
    std::mt19937 rng(0x191130u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
