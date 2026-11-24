// Native TU1.5 motion names (E53E0) against the unmodified translated body
// of the live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1D3BD0 is a hash-driven stub of its complete entry ABI (it answers AL,
// writes scratch, the caller-saved registers, XMM, flags and (by policy)
// MXCSR are random, and by policy EBX, EBP or EDI change or the stack is left
// unbalanced). Fixtures author bipeds still or not, physics modes 2, 5 and
// others, actions idle or not, motions 0 to 12 and past, and the original's
// jump table (now and then an entry bound to no case or unbound); one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_motion_names.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_motion_names_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,objects=0x10020000u,outs=0x10030000u,scratch=0x10600000u;
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x1d3bd0u);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x1d3bd1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%2?0u:1u+std::uint32_t(rng()%255));s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=outs+4u*std::uint32_t(rng()%8);
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng()%13);
    if(policy&2u && rng()%13==0)s.gp[7]=outs+4u*std::uint32_t(rng()%8);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
// The unit's table targets (and one bound target that is no case), as its
// canonical_branch names them.
std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage) {
    for(const auto &c:halo2::native_motion_names::cases)if(c.target==p)return p;
    if(p==0x1d4a9u)return p;
    throw std::runtime_error("Unbound TU1.5 contact dispatch target");
}
static void run_1d3bd0(WorldState &s){stub(s);}
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
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x400u*k;
        for(std::uint32_t at=object;at<object+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0x34cu,pick(5)?pick(3)*2u:1u,2);
        static constexpr std::uint32_t modes[]{2u,2u,5u,5u,0u,3u};
        m.put(object+0x3dcu,modes[pick(6)],1);m.put(object+0x12au,pick(2)?pick(0x100)*4u:0x10000u-pick(0x40)*4u,2);
    }
    for(std::uint32_t at=outs;at<outs+0x20u;at+=4)m.put(at,pick(3)?0x400000cu:std::uint32_t(rng()));
    for(std::uint32_t k=0;k<11;++k) {
        const auto ordered=halo2::native_motion_names::cases[k].target;
        const auto any=halo2::native_motion_names::cases[pick(11)].target;
        m.put(0xe54c0u+4u*k,shape==9 && pick(4)==0?(pick(2)?0xe5400u:0x1d4a9u):pick(4)?ordered:any);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xe0000u+pick(0x100));m.put(stack+4u,pick(6)?pick(13):std::uint32_t(rng()));m.put(stack+8u,std::uint32_t(rng()));
    m.put(stack+12u,outs+0x20u+pick(8));
    for(std::uint32_t at=outs+0x20u;at<outs+0x30u;at+=4)m.put(at,std::uint32_t(rng()));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())&0xffff0000u)|pick(4);c.abi.gp[3]=outs+4u*pick(4);c.abi.gp[7]=outs+0x10u+4u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,objects,outs};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_e53e0(v);},[](auto &v){original::run_1d3bd0(v);},
        [](std::uint32_t p,SourcePhysicsContactImage image){return original::canonical_branch(p,image);});
}
void reference(WorldState &s){original::run_e53e0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_MOTION_NAMES");
    std::mt19937 rng(0xe53e0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
