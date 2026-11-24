// Native TU1.5 seated blend (113B70) against the unmodified translated body
// of the live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// C9550 and 113C70 are hash-driven stubs of their complete entry ABIs (and
// argument): each answers AL, writes scratch, the caller-saved registers,
// XMM, flags and (by policy) MXCSR are random, and by policy EBX, ESI or EDI
// change or the stack is left unbalanced. Fixtures author units without a
// parent or a seat, and actions from the original's list, next to it (one
// below and above each) and elsewhere; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_seated_blend.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_seated_blend_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,objects=0x10020000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    if(target==0x113c70u)h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%3?(target==0xc9550u?0u:1u+std::uint32_t(rng()%255)):std::uint32_t(rng()%2));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng())&0xffffff01u;
    if(policy&2u && rng()%13==0)s.gp[6]=objects+0x400u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=(target==0x113c70u?8u:4u)+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_c9550(WorldState &s){stub(s,0xc9550u);}
static void run_113c70(WorldState &s){stub(s,0x113c70u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<4;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,std::uint32_t(rng()));m.put(datums+12u*k+8u,objects+0x400u*k);}
    const auto &names=halo2::native_seated_blend::partial_actions;
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x400u*k;
        for(std::uint32_t at=object;at<object+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0x14u,pick(6)?std::uint32_t(rng()):0xffffffffu);m.put(object+0x1fcu,pick(6)?pick(8):0xffffu,2);
        const auto offset=0x100u+pick(0x40)*4u;
        m.put(object+0x12au,offset,2);
        const auto name=names[pick(sizeof(names)/4)];
        m.put(object+offset+0x7cu,pick(3)==0?std::uint32_t(rng()):name+pick(3)-1u);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x110000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[7]=(std::uint32_t(rng())&0xffff0000u)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,objects};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_113b70(v);},[](auto &v){original::run_c9550(v);},
        [](auto &v){original::run_113c70(v);});
}
void reference(WorldState &s){original::run_113b70(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SEATED_BLEND");
    std::mt19937 rng(0x113b70u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
