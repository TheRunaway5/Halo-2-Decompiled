// Native TU1.5 object extension (BE3B0) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 191130 is a hash-driven stub of its complete entry ABI (and the pool it is
// handed): it answers success (moving the object's data at times) or
// failure, writes scratch; the caller-saved registers, XMM, flags and (by
// policy) MXCSR are random, and by policy EBX, EBP, ESI or EDI change or the
// stack is left unbalanced. Fixtures author empty and sized blocks,
// alignments, objects' sizes, both string directions, and one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_object_extension.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_object_extension_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,objects=0x10020000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    std::uint32_t answer=std::uint32_t(rng())&0xffffff00u;
    if(rng()%6) {
        answer|=1u+std::uint32_t(rng()%255);
        if(rng()%3==0)w(s.memory,s.observe,0x191131u,s.gp[3],4,objects+0x400u*std::uint32_t(rng()%2));
    }
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=answer;s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=datums+12u*std::uint32_t(rng()%2)+8u;
    if(policy&2u && rng()%13==0)s.gp[5]=datums+12u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[6]=(1u<<(rng()%4))-1u;
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng()%0x20);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=16u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_191130(WorldState &s){stub(s,0x191130u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4eabbcu,0x10700000u+pick(16));
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<2;++k) {
        m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,(std::uint32_t(rng())&0xffffu)|(pick(0x100)<<16));
        m.put(datums+12u*k+8u,objects+0x400u*k);
    }
    for(std::uint32_t at=objects;at<objects+0x800u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|(4u*pick(0x40)));
    m.put(stack+8u,(std::uint32_t(rng())&0xffff0000u)|(pick(5)?pick(0x40):0u));
    m.put(stack+12u,(std::uint32_t(rng())&0xffffff00u)|pick(5));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())&0xffff0000u)|pick(2);
    if(rng()%4==0){c.abi.eflags|=0x400u;m.put(datums+6u,0x100u+pick(0x100),2);m.put(datums+18u,0x100u+pick(0x100),2);}
    else c.abi.eflags&=~0x400u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,objects};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_be3b0(v);},[](auto &v){original::run_191130(v);});}
void reference(WorldState &s){original::run_be3b0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_EXTENSION");
    std::mt19937 rng(0xbe3b0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
