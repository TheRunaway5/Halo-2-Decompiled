// Native TU1.5 biped recovery (E52D0) against the unmodified translated body
// of the live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// E7E80, 111B10, C1A70 and 2031F0 (through the dispatch) are hash-driven
// stubs of their complete entry ABIs (and arguments): each may change the
// biped's physics mode, countdown or flags, writes scratch, the caller-saved
// registers, XMM, flags and (by policy) MXCSR are random, and by policy EBX,
// EBP, ESI or EDI change or the stack is left unbalanced. Fixtures author
// countdowns 0 to 3, physics modes 2, 3 and others, definitions with and
// without recovery, held bytes, seats NONE or not and the seated bit; one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_biped_recovery.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_biped_recovery_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,objects=0x10020000u,instances=0x10030000u,definitions=0x10040000u,
    scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const std::uint32_t arguments=target==0x111b10u?7u:target==0xc1a70u?1u:0u;
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto object=objects+0x400u*std::uint32_t(rng()%4);
    if(rng()%4==0)w(s.memory,s.observe,target+1u,object+0x3dcu,1,std::uint32_t(rng()%4));
    if(rng()%6==0)w(s.memory,s.observe,target+2u,object+0x39du,1,std::uint32_t(rng()%3));
    if(rng()%6==0)w(s.memory,s.observe,target+3u,object+4u,4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng())&0xffffff01u;
    if(policy&2u && rng()%13==0)s.gp[5]=rng()%3?(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4):0xffffffffu;
    if(policy&2u && rng()%13==0)s.gp[6]=12u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=objects+0x400u*std::uint32_t(rng()%4);
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
static void run_e7e80(WorldState &s){stub(s,0xe7e80u);}
static void run_111b10(WorldState &s){stub(s,0x111b10u);}
static void run_c1a70(WorldState &s){stub(s,0xc1a70u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x2031f0u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,instances);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<4;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,std::uint32_t(rng()));m.put(datums+12u*k+8u,objects+0x400u*k);}
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,definitions+0x200u*k);}
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=definitions+0x200u*k;at<definitions+0x200u*k+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(definitions+0x200u*k+0x1f0u,(std::uint32_t(rng())&~0x400u)|(pick(4)?0x400u:0u));}
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x400u*k;
        for(std::uint32_t at=object;at<object+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object,(std::uint32_t(rng())&0xffff0000u)|pick(4));
        m.put(object+0x39du,pick(3)?1u:pick(4),1);m.put(object+0x3dcu,pick(4)?3u:pick(4),1);
        m.put(object+0x10au,pick(4)?(std::uint32_t(rng())&~4u):std::uint32_t(rng()),1);
        m.put(object+0x112u,pick(3)?pick(8):0xffffu,2);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xe0000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,objects,instances,definitions};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_e52d0(v);},[](auto &v){original::run_e7e80(v);},
        [](auto &v){original::run_111b10(v);},[](auto &v){original::run_c1a70(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_e52d0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_BIPED_RECOVERY");
    std::mt19937 rng(0xe52d0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
