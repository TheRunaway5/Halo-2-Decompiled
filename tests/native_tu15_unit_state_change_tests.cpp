// Native TU1.5 unit state change (1115D0) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 111D10 and 1D6430 are hash-driven stubs of their complete entry ABIs:
// each hashes what it reads (its pushed arguments), writes scratch, 111D10
// a chosen state into the argument's slot (ESI) and answers AL; the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBX, EBP, ESI or EDI change or the stack is left unbalanced.
// Fixtures author units in every state (and others), flags, held bytes and
// counters on both sides of zero; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_unit_state_change.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_unit_state_change_detail;
namespace {
constexpr std::uint32_t objects=0x10000000u,entries=0x10001000u,unit=0x10002000u,scratch=0x10600000u;
constexpr std::uint32_t states[]{0x50000cbu,0x4000089u,0x6000084u,0x700002cu,0x7000039u,0x70000c9u,0x6000085u,0x6000086u,0x6000087u,
    0x7000101u,0x6000083u,0x700002bu,0x7000045u,0x4000088u};
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const auto args=target==0x1d6430u?3u:0u;
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x111d10u && rng()%4)w(s.memory,s.observe,target+1u,s.gp[6],4,rng()%3?states[rng()%14]:s.gp[7]);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x111d10u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%4?1u+std::uint32_t(rng()%255):0u);
    if(policy&2u && rng()%13==0)s.gp[3]=unit;
    if(policy&2u && rng()%13==0)s.gp[5]=unit+0x200u+std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=rng()%2?states[rng()%14]:std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=states[rng()%14];
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*args+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_111d10(WorldState &s){stub(s,0x111d10u);}
static void run_1d6430(WorldState &s){stub(s,0x1d6430u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x4ecbd0u,objects);
    for(std::uint32_t at=objects;at<objects+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(objects+0x44u,entries);
    for(std::uint32_t k=0;k<4;++k){m.put(entries+12u*k,std::uint32_t(rng()));m.put(entries+12u*k+4u,std::uint32_t(rng()));m.put(entries+12u*k+8u,unit);}
    for(std::uint32_t at=unit;at<unit+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(unit+0x14cu,chance(8)?std::uint32_t(rng()):states[pick(14)]);
    m.put(unit+0x33eu,0x200u,2);m.put(unit+0x12au,chance(4)?0xff00u:0x100u,2);
    m.put(unit+0x203u,chance(3)?0u:chance(4)?0x80u+pick(0x80):pick(6),1);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x60u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x111000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffffff00u)|(chance(3)?1u+pick(255):0u));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())&0xffff0000u)|pick(4);
    c.abi.gp[2]=(std::uint32_t(rng())&0xffffff00u)|(chance(2)?0u:1u+pick(255));
    if(shape==15) {
        static constexpr std::uint32_t areas[]{objects,entries,unit};
        m.drop(areas[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1115d0(v);},
        [](auto &v,std::uint32_t target){
            switch(target){case 0x111d10u:original::run_111d10(v);break;case 0x1d6430u:original::run_1d6430(v);break;
            default:throw std::logic_error("Unbound unit state change child");}});
}
void reference(WorldState &s){original::run_1115d0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_UNIT_STATE_CHANGE");
    std::mt19937 rng(0x1115d0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
