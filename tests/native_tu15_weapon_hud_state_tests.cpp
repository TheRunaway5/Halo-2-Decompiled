// Native TU1.5 weapon HUD state (13DA89) against the unmodified translated
// body of the weapon HUD unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1055F0 is a hash-driven stub of its complete entry ABI and arguments: it
// writes the status's fields, scratch, the caller-saved registers, XMM,
// flags and (by policy) MXCSR are random, and by policy ESI, EDI or EBX
// change, the direction flag flips or the stack is left unbalanced.
// Fixtures author weapons of several objects and definitions and both copy
// directions; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/weapons/native_tu15_weapon_hud_state.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_weapon_hud_state_detail;
namespace {
constexpr std::uint32_t objects=0x10000000u,data=0x10010000u,weapons=0x10020000u,instances=0x10030000u,definitions=0x10040000u,
    states=0x10050000u,outputs=0x10500000u,scratch=0x10600000u;
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x1055f0u);
    h=mix(h,r(s.memory,s.gp[4]+4u));h=mix(h,r(s.memory,s.gp[4]+8u));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto status=r(s.memory,s.gp[4]+8u);
    for(std::uint32_t k=0;k<0x30u;k+=4)if(rng()%3)w(s.memory,s.observe,0x1055f0u,status+k,4,std::uint32_t(rng()));
    w(s.memory,s.observe,0x1055f2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=states+0x40u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=rng()%2?0xffu:std::uint32_t(rng());
    if(policy&2u && rng()%17==0)s.other_flags^=0x400u;
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=12u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1055f0(WorldState &s){stub(s);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,objects);m.put(0x4f0414u,instances);
    for(std::uint32_t at=objects;at<objects+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(objects+0x44u,data);
    for(std::uint32_t k=0;k<4;++k){m.put(data+12u*k,std::uint32_t(rng()));m.put(data+12u*k+4u,std::uint32_t(rng()));m.put(data+12u*k+8u,weapons+0x40u*k);}
    for(std::uint32_t k=0;k<4;++k)m.put(weapons+0x40u*k,(std::uint32_t(rng())&0xffff0000u)|pick(4)|(pick(4)==0?0x100u:0u));
    for(std::uint32_t k=0;k<0x104;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,definitions+0x400u*(k%4));}
    for(std::uint32_t at=definitions;at<definitions+0x1000u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=states;at<states+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=outputs;at<outputs+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x130000u+pick(0x100));m.put(stack+4u,outputs+4u*pick(8));
    random_abi(rng,c,stack);
    c.abi.gp[0]=states+0x40u+0x40u*pick(4);c.abi.gp[1]=(std::uint32_t(rng())&0xffff0000u)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{objects,data,weapons,instances,definitions,states,outputs};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_13da89(v);},[](auto &v){original::run_1055f0(v);});}
void reference(WorldState &s){original::run_13da89(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_WEAPON_HUD_STATE");
    std::mt19937 rng(0x13da89u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
