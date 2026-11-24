// Native TU1.5 animation mask query (113C70) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1E3150 is a hash-driven stub of its complete entry ABI and argument (EDX
// the definition): it answers an entry (or 0) whose mask it may rewrite,
// pops its argument, the caller-saved registers, XMM, flags and (by policy)
// MXCSR are random, and by policy BL, EBP or ESI change or the stack is
// left unbalanced. Fixtures author both blocks (signed offsets) with slots
// empty (NONE high words) or not and bits within and past 31; one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_animation_mask.hpp"
using namespace frame_misc_harness;
namespace mask=halo2::native_animation_mask_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,tags=0x10180000u,
    entries=0x10200000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,rng()%2?scratch+std::uint32_t(rng()%64)*4u:entries+4u*std::uint32_t(rng()%16)+0x18u,2,
      std::uint32_t(rng()));
    s.gp[0]=rng()%5==0?0u:entries+0x20u*std::uint32_t(rng()%4);s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%9==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[5]=std::uint32_t(rng()%40);
    if(policy&2u && rng()%13==0)s.gp[6]=objects+0x800u*std::uint32_t(rng()%4)+0x400u;
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=8u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1e3150(WorldState &s){stub(s,0x1e3150u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,tags);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    for(std::uint32_t at=tags;at<tags+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=entries;at<entries+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x800u*k+0x400u+4u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object-0x400u;at<object+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        const auto first=pick(3)==0?0x10000u-0x200u:0x180u,second=pick(3)==0?0x10000u-0x300u:0x200u;
        m.put(object+0x12au,first,2);m.put(object+0x33eu,second,2);
        const auto a=object+std::uint32_t(std::int32_t(std::int16_t(first))),b=object+std::uint32_t(std::int32_t(std::int16_t(second)))+0x9cu;
        m.put(a,(std::uint32_t(rng())<<16)|pick(16));m.put(b,(std::uint32_t(rng())<<16)|pick(16));
        m.put(a+6u,pick(3)==0?0xffffu:pick(0xffff),2);m.put(b+6u,pick(3)==0?0xffffu:pick(0xffff),2);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x110000u+pick(0x100));
    m.put(stack+4u,pick(4)==0?std::uint32_t(rng()):pick(16));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x800u,tags,entries};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s){return mask::bridge(s,[](auto &v){original::native_frame_misc_reference_113c70(v);},[](auto &v){original::run_1e3150(v);});}
void reference(WorldState &s){original::run_113c70(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ANIMATION_MASK");
    std::mt19937 rng(0x113c70u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,mask::coverage().load(),unsigned(mask::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
