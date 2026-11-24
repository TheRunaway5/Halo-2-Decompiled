// Native TU1.5 unit animation update (1157D0) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The seven children are hash-driven stubs of their complete entry ABIs and
// arguments (CE230 leaves its arguments for 116280 to pop): the first three
// answer AL, CE230 a value, 1E14F0 writes its channel; all write scratch,
// the caller-saved registers, XMM, flags and (by policy) MXCSR are random,
// and by policy EBX, EBP, ESI or EDI change or the stack is left
// unbalanced. Fixtures author units whose definitions carry bit 11 or not,
// states with and without an animation, graph, mode and links, and
// animation indices; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_unit_animation_update.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_unit_animation_update_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,objects=0x10020000u,instances=0x10030000u,definitions=0x10040000u,
    scratch=0x10600000u;
std::uint32_t popped(std::uint32_t target){return target==0x1e14f0u?1u:target==0x116340u || target==0x116280u?3u:0u;}
void stub(WorldState &s,std::uint32_t target) {
    const auto arguments=target==0xce230u?2u:popped(target);
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x1e14f0u)w(s.memory,s.observe,target,s.gp[1],4,r(s.memory,s.gp[4]+4u));
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%2?1u:0u);s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=(std::uint32_t(rng())&0xffffff00u)|std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[5]=objects+0x400u*std::uint32_t(rng()%4)+0x100u;
    if(policy&2u && rng()%13==0)s.gp[6]=objects+0x400u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*popped(target)+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_113b70(WorldState &s){stub(s,0x113b70u);}
static void run_1139d0(WorldState &s){stub(s,0x1139d0u);}
static void run_cfb40(WorldState &s){stub(s,0xcfb40u);}
static void run_1e14f0(WorldState &s){stub(s,0x1e14f0u);}
static void run_116340(WorldState &s){stub(s,0x116340u);}
static void run_ce230(WorldState &s){stub(s,0xce230u);}
static void run_116280(WorldState &s){stub(s,0x116280u);}
#include "body.inc"
#include "reference.inc"
void step(WorldState &s,std::uint32_t target){stub(s,target);}
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,instances);m.put(0x450170u,0x3f800000u);m.put(0x46aa4cu,0x3e800000u);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<4;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,std::uint32_t(rng()));m.put(datums+12u*k+8u,objects+0x400u*k);}
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x400u*k;
        for(std::uint32_t at=object;at<object+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object,(std::uint32_t(rng())&0xffff0000u)|pick(4));
        m.put(object+0x33eu,0x100u,2);m.put(object+0x12au,0x200u,2);m.put(object+0x212u,pick(3)?pick(4):0xffu,1);
        const auto state=object+0x200u;
        m.put(state,pick(6)?std::uint32_t(rng()):0xffffffffu);m.put(state+6u,pick(6)?pick(8):0xffffu,2);
        m.put(state+0x68u,pick(6)?std::uint32_t(rng()):0xffffffffu);
        m.put(state+0x74u,pick(4)?std::uint32_t(rng()):0xffffffffu);m.put(state+0x78u,pick(4)?std::uint32_t(rng()):0xffffffffu);
    }
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,definitions+0x100u*k);}
    for(std::uint32_t at=definitions;at<definitions+0x400u;at+=4)m.put(at,std::uint32_t(rng())&(pick(4)?~0x800u:~0u));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xb90000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    m.put(stack+8u,std::uint32_t(rng()));m.put(stack+12u,std::uint32_t(rng()));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,objects,instances,definitions};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1157d0(v);},
        [](auto &v,std::uint32_t target){original::step(v,target);});
}
void reference(WorldState &s){original::run_1157d0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_UNIT_ANIMATION_UPDATE");
    std::mt19937 rng(0x1157d0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
