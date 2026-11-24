// Native TU1.5 body list ray (315190) against the unmodified translated body
// of the live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// Each shape's cast (through the unit's dispatch, at the targets the
// authored vtables name) is a hash-driven stub of its complete entry ABI: it
// hashes its arguments, the local ray input and the XMM registers, writes
// the body, and returns (RET C); the caller-saved registers, XMM, flags and
// (by policy) MXCSR are random, and by policy EBX, ESI (the cursor), EDI (the
// other input) or EBP (another authored frame) change or the stack is left
// unbalanced. Fixtures author lists of -1 to 5 bodies, bodies without a
// shape, inputs with the filter enabled or not and worlds with a filter or
// not, finite, infinite and NaN rays and motions, motions aligned and not;
// one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_body_list_ray.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_body_list_ray_detail;
namespace {
constexpr std::uint32_t lists=0x10000000u,worlds=0x10010000u,arrays=0x10020000u,cd_bodies=0x10030000u,motions=0x10040000u,
    shapes=0x10050000u,vtables=0x10060000u,inputs=0x10070000u,frames=0x10080000u,scratch=0x10600000u;
constexpr std::uint32_t casts[]{0x2f7000u,0x2f7100u};
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=3;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    const auto ray=r(s.memory,s.gp[4]+4u);
    for(std::uint32_t k=0;k<10;++k)h=mix(h,r(s.memory,ray+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,r(s.memory,s.gp[4]+8u)+4u+8u*std::uint32_t(rng()%2),4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%9==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%9==0)s.gp[5]=frames+0x80u+0x80u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%9==0)s.gp[6]=arrays+0x100u*std::uint32_t(rng()%2)+4u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%9==0)s.gp[7]=inputs+0x40u*std::uint32_t(rng()%2);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%61==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=16u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=casts[0] && target!=casts[1])throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t component(std::mt19937 &rng) {
    switch(rng()%14) {
    case 0:return 0x7fc00000u|std::uint32_t(rng()%0x100);
    case 1:return 0x7f800000u|(rng()%2?0x80000000u:0u);
    case 2:return 0u;
    default:{float f=float(int(rng()%2001)-1000)/37.0f;std::uint32_t v;std::memcpy(&v,&f,4);return v;}
    }
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    for(std::uint32_t at=lists;at<lists+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k) {
        const auto list=lists+0x100u*k;
        m.put(list+8u,worlds+0x100u*pick(2));m.put(list+0x80u,arrays+0x100u*pick(2));m.put(list+0x84u,pick(8)?pick(6):0u-pick(2));
    }
    for(std::uint32_t at=worlds;at<worlds+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k)m.put(worlds+0x100u*k+0xd0u,pick(3)?0x10090000u+4u*pick(16):0u);
    for(std::uint32_t at=arrays;at<arrays+0x200u;at+=4)m.put(at,cd_bodies+0x20u*pick(8));
    for(std::uint32_t at=cd_bodies;at<cd_bodies+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<8;++k) {
        m.put(cd_bodies+0x20u*k,pick(5)?shapes+0x20u*pick(4):0u);
        m.put(cd_bodies+0x20u*k+8u,motions+0x80u*pick(4)+(pick(6)?0u:4u*pick(4)));
    }
    for(std::uint32_t at=shapes;at<shapes+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(shapes+0x20u*k,vtables+0x40u*pick(2));
    for(std::uint32_t at=vtables;at<vtables+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(vtables+0x24u,casts[0]);m.put(vtables+0x64u,casts[1]);
    for(std::uint32_t at=motions;at<motions+0x240u;at+=4)m.put(at,component(rng));
    for(std::uint32_t at=inputs;at<inputs+0x80u;at+=4)m.put(at,component(rng));
    for(std::uint32_t k=0;k<2;++k){m.put(inputs+0x40u*k+0x20u,pick(2)?0u:1u+pick(255),1);m.put(inputs+0x40u*k+0x24u,std::uint32_t(rng()));}
    for(std::uint32_t at=frames;at<frames+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x310000u+pick(0x100));m.put(stack+4u,inputs+0x40u*pick(2));m.put(stack+8u,0x100a0000u+4u*pick(16));
    random_abi(rng,c,stack);
    c.abi.gp[1]=lists+0x100u*pick(2);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{lists,worlds,arrays,cd_bodies,motions,shapes,inputs};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_315190(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_315190(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_BODY_LIST_RAY");
    std::mt19937 rng(0x315190u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
