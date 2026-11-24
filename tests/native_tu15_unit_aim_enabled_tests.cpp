// Native TU1.5 unit aim enabled (1139D0) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// C9550 and 113C70 are hash-driven stubs of their complete entry ABIs: each
// hashes what it reads (its pushed arguments), writes scratch and answers
// AL; the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy EBX, EBP, ESI or EDI change or the stack is left
// unbalanced. Fixtures author channel states and animation states from both
// lists, every pivot of the two searches and its neighbours, and others; the
// code's jump table holds its real bytes. One region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_unit_aim_enabled.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_unit_aim_enabled_detail;
namespace logic=halo2::native_unit_aim_enabled;
namespace {
constexpr std::uint32_t objects=0x10000000u,entries=0x10001000u,unit=0x10002000u,scratch=0x10600000u;
// The searches' pivots (and the lists' names) with their neighbours.
constexpr std::uint32_t pivots[]{0xc000075u,0xa000040u,0x6000007u,0x6000006u,0xffffffffu,0x500000au,0x9000004u,0x9000009u,0xb0005b2u,
    0xa0005b9u,0xa0005bbu,0xc000073u,0xe00067du,0xc0006cdu,0xc000077u,0xc0006b3u,0xe000038u,0xe000606u,0xe00060au,0x130005bcu,
    0x11000074u,0x11000076u,0x140005b3u,0xc00005bu,0xb00005au,0xb000059u,0x400004au,0x600008cu,0xa00003eu,0xc000043u,0xc000058u,
    0xe0000c2u,0xd00003fu,0xd000042u,0xe00003bu,0xf00003au,0x1000003cu,0x80000000u,0x7fffffffu};
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const auto args=target==0x113c70u?1u:0u;
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%3?(target==0xc9550u?0u:1u+std::uint32_t(rng()%255)):(target==0xc9550u?1u+std::uint32_t(rng()%255):0u));
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=unit+0x100u+4u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4);
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
std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_c9550(WorldState &s){stub(s,0xc9550u);}
static void run_113c70(WorldState &s){stub(s,0x113c70u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t name(std::mt19937 &rng,bool animation) {
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    switch(pick(4)) {
    case 0:return animation?logic::blocking_animations[pick(14)]:logic::aiming_states[pick(27)];
    case 1:return pivots[pick(39)];
    case 2:return pivots[pick(39)]+pick(3)-1u;
    default:return pick(2)?std::uint32_t(rng()):0x9000004u+pick(6);
    }
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x4ecbd0u,objects);
    m.put(0x113b58u,0x113acau);m.put(0x113b5cu,0x113ac8u);
    static constexpr std::uint32_t indices[]{0u,0u,1u,1u,0u,0u};
    for(std::uint32_t k=0;k<6;++k)m.put(0x113b60u+k,indices[k],1);
    for(std::uint32_t at=objects;at<objects+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(objects+0x44u,entries);
    for(std::uint32_t k=0;k<4;++k){m.put(entries+12u*k,std::uint32_t(rng()));m.put(entries+12u*k+4u,std::uint32_t(rng()));m.put(entries+12u*k+8u,unit);}
    for(std::uint32_t at=unit;at<unit+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(unit+0x33eu,0x200u,2);m.put(unit+0x12au,0x100u,2);
    m.put(unit+0x2a4u,name(rng,false));m.put(unit+0x17cu,name(rng,true));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x60u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x113000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[7]=(std::uint32_t(rng())&0xffff0000u)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t areas[]{objects,entries,unit};
        m.drop(areas[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1139d0(v);},
        [](auto &v,std::uint32_t target){
            switch(target){case 0xc9550u:original::run_c9550(v);break;case 0x113c70u:original::run_113c70(v);break;
            default:throw std::logic_error("Unbound unit aim enabled child");}});
}
void reference(WorldState &s){original::run_1139d0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_UNIT_AIM_ENABLED");
    std::mt19937 rng(0x1139d0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
