// Native TU1.5 deferred action target check (1AE5E0) against the unmodified
// translated body of the combat action unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1AE500 and 1AE6F0 are hash-driven stubs of their complete entry ABIs and
// arguments: both answer AL and write the result byte, scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBX, EBP or ESI change or the stack is left unbalanced.
// Fixtures author objects with and without the passing tag flag, riding or
// not on parents of every type, and child lists (naming later objects
// only, so every list ends) of riders and others of every kind; one region
// removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_action_target_defer.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_action_target_defer_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,objects=0x10020000u,instances=0x10030000u,tags=0x10040000u,
    results=0x10500000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target,r(s.memory,s.gp[4]+4u*arguments),1,std::uint32_t(rng()%2));
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%3==0?1u:0u);s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[5]=results+4u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%11==0)s.gp[6]=objects+0x200u*std::uint32_t(rng()%8);
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
static void run_1ae500(WorldState &s){stub(s,0x1ae500u,4);}
static void run_1ae6f0(WorldState &s){stub(s,0x1ae6f0u,2);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,instances);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    const auto handle=[&](){return pick(6)==0?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|pick(8);};
    for(std::uint32_t k=0;k<8;++k) {
        m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+3u,pick(3)?pick(2):pick(40),1);
        m.put(datums+12u*k+4u,std::uint32_t(rng()));m.put(datums+12u*k+8u,objects+0x200u*k);
        const auto object=objects+0x200u*k;
        for(std::uint32_t at=object;at<object+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object,(std::uint32_t(rng())&0xffff0000u)|pick(4));
        m.put(object+4u,(std::uint32_t(rng())&~(1u<<26))|(pick(2)?1u<<26:0u));
        // Children and siblings name later objects only, so every chain ends.
        const auto later=[&](){return k==7 || pick(3)==0?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|(k+1u+pick(7u-k));};
        m.put(object+0xcu,later());m.put(object+0x10u,later());m.put(object+0x14u,handle());
        m.put(object+0xaau,pick(3)?pick(2):pick(40),1);
    }
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,tags+0x200u*k);}
    for(std::uint32_t at=tags;at<tags+0x800u;at+=4)m.put(at,std::uint32_t(rng())&~(1u<<21));
    for(std::uint32_t k=0;k<4;++k)if(pick(4)==0)m.put(tags+0x200u*k+0x1ecu,std::uint32_t(rng())|(1u<<21));
    for(std::uint32_t at=results;at<results+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1ae000u+pick(0x100));m.put(stack+4u,std::uint32_t(rng()));m.put(stack+8u,std::uint32_t(rng()));m.put(stack+12u,results+4u*pick(4));
    random_abi(rng,c,stack);
    c.abi.gp[7]=(std::uint32_t(rng())&0xffff0000u)|pick(8);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,objects,instances,tags,results};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1ae5e0(v);},[](auto &v){original::run_1ae500(v);},
        [](auto &v){original::run_1ae6f0(v);});
}
void reference(WorldState &s){original::run_1ae5e0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ACTION_TARGET_DEFER");
    std::mt19937 rng(0x1ae5e0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
