// Native TU1.5 resource acquire (1262F0) against the unmodified translated
// bodies of the AI frame and physics live support units, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 126190, 16E8A0, 125FB0, 16E770, 1260C0 and 1262B0 (direct or through the
// unit's dispatch, as each unit calls them) are hash-driven stubs of their
// complete entry ABIs and arguments: 126190 answers a request or NONE,
// 125FB0 a block, 16E770 a record (and now and then another pool in EDX),
// 1260C0 and 1262B0 rewrite the resource's block; all write scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBX, ESI or EDI change or the stack is left unbalanced.
// Fixtures author resources with and without a block, and caches of shifts
// past 15 and 31 too (SHL masks its count); one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/animation/native_tu15_resource_acquire.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_resource_acquire_detail;
namespace {
constexpr std::uint32_t resources=0x10000000u,pools=0x10010000u,records=0x10020000u,cache=0x10030000u,blocks=0x10040000u,
    scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto handle=[&](){return (std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%8);};
    if((target==0x1260c0u || target==0x1262b0u) && rng()%4==0)w(s.memory,s.observe,target,resources+0x10u*std::uint32_t(rng()%4)+0xcu,4,handle());
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    switch(target) {
    case 0x126190u:s.gp[0]=rng()%3?0xffffffffu:handle();break;
    case 0x125fb0u:case 0x16e770u:s.gp[0]=handle();break;
    default:s.gp[0]=std::uint32_t(rng());break;
    }
    s.gp[1]=std::uint32_t(rng());
    s.gp[2]=target==0x16e770u?(rng()%5?s.gp[2]:pools+0x40u):std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[6]=resources+0x10u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%11==0)s.gp[7]=handle();
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*arguments+cleanup;
}
std::uint32_t arguments_of(std::uint32_t target){return target==0x125fb0u || target==0x1260c0u?2u:0u;}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_126190(WorldState &s){stub(s,0x126190u,0);}
static void run_16e8a0(WorldState &s){stub(s,0x16e8a0u,0);}
static void run_16e770(WorldState &s){stub(s,0x16e770u,0);}
static void run_1260c0(WorldState &s){stub(s,0x1260c0u,2);}
static void run_1262b0(WorldState &s){stub(s,0x1262b0u,0);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    switch(target) {
    case 0x16e8a0u:case 0x125fb0u:case 0x16e770u:case 0x1260c0u:stub(s,target,arguments_of(target));return;
    }
    throw std::runtime_error("Unbound sandbox dispatch target");
}
#include "body.inc"
#include "reference.inc"
// The native route reaches each child as its unit does; here all are the stubs.
void step(WorldState &s,std::uint32_t target,std::uint32_t){if(target==0x126190u || target==0x1262b0u)stub(s,target,0);else dispatch(s,target,0,false);}
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f041cu,pools);m.put(0x4f0418u,pools+0x40u*pick(2));m.put(0x4f0424u,cache);m.put(0x4f0420u,std::uint32_t(rng()));
    for(std::uint32_t at=resources;at<resources+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(resources+0x10u*k+0xcu,pick(3)?(std::uint32_t(rng())&0xffff0000u)|pick(8):0xffffffffu);
    // Two pools: the first's records, the second's (which 16E770 may leave in EDX).
    for(std::uint32_t at=pools;at<pools+0xc0u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(pools+0x44u,records);m.put(pools+0x84u,records+0x100u);
    for(std::uint32_t at=records;at<records+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=cache;at<cache+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(cache+0x34u,pick(4)?pick(12):16u+pick(24));m.put(cache+0x64u,cache+0x100u);
    for(std::uint32_t at=cache+0x100u;at<cache+0x150u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(cache+0x144u,blocks);
    for(std::uint32_t at=blocks;at<blocks+24u*8u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x120000u+pick(0x100));m.put(stack+4u,std::uint32_t(rng()));
    random_abi(rng,c,stack);
    c.abi.gp[0]=resources+0x10u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{resources,pools,records,cache,blocks};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1262f0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::step(v,target,site);});
}
void reference(WorldState &s){original::run_1262f0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_RESOURCE_ACQUIRE");
    std::mt19937 rng(0x1262f0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
