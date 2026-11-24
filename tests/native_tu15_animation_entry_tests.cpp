// Native TU1.5 animation entry value (1E5A70) against the unmodified
// translated body of the physics live support unit (the validator repeats
// it with the AI frame unit's), extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1262F0 is a hash-driven stub of its complete entry ABI and argument (EAX
// the row): it answers EAX, pops the tag, writes scratch, the caller-saved
// registers, XMM, flags and (by policy) MXCSR are random, and by policy ESI
// changes or the stack is left unbalanced. Fixtures author references with
// either word NONE (-1) or an index within or past the counts (negative
// too), list entries naming a tag or NONE, and entries computed or plain;
// one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_animation_entry.hpp"
using namespace frame_misc_harness;
namespace entry=halo2::native_animation_entry_detail;
namespace {
constexpr std::uint32_t tags=0x10180000u,definitions=0x10200000u,lists=0x10300000u,entries=0x10400000u,rows=0x10500000u,
    scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[6]=entries+0x6cu*std::uint32_t(rng()%4);
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
static void run_1262f0(WorldState &s){stub(s,0x1262f0u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f0414u,tags);
    for(std::uint32_t d=0;d<4;++d) {
        const auto definition=definitions+0x1000u*d;
        m.put(tags+16u*d+8u,definition);m.put(tags+16u*(0x1000u+d)+8u,definition);
        for(std::uint32_t at=definition;at<definition+0xc0u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(definition+0x4cu,pick(8)==0?0xffffffffu:2u+pick(3));
        m.put(definition+0x50u,lists+0x1000u*d);
        for(std::uint32_t e=0;e<5;++e) {
            for(std::uint32_t at=lists+0x1000u*d+32u*e;at<lists+0x1000u*d+32u*e+32u;at+=4)m.put(at,std::uint32_t(rng()));
            m.put(lists+0x1000u*d+32u*e+4u,pick(8)==0?0xffffffffu:(std::uint32_t(rng())<<16)|(0x1000u*pick(2)+pick(4)));
        }
        m.put(definition+0x30u,entries+0x1000u*d);
        for(std::uint32_t at=entries+0x1000u*d;at<entries+0x1000u*d+0x6cu*5u;at+=4)m.put(at,std::uint32_t(rng()));
        for(std::uint32_t e=0;e<5;++e)m.put(entries+0x1000u*d+0x6cu*e+0x24u,pick(8));
        m.put(definition+0xb0u,rows+0x1000u*d);
        for(std::uint32_t at=rows+0x1000u*d;at<rows+0x1000u*d+0xa0u;at+=4)m.put(at,std::uint32_t(rng()));
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1e0000u+pick(0x100));
    const auto low=pick(4)==0?0xffffu:pick(8)==0?0x8000u+pick(4):pick(8)==0?4u:pick(2);
    const auto high=pick(8)==0?0xffffu:pick(4);
    m.put(stack+4u,(high<<16)|low);
    random_abi(rng,c,stack);
    c.abi.gp[1]=definitions+0x1000u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{tags,definitions+0x1000u,lists+0x2000u,entries+0x3000u};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s){return entry::bridge(s,[](auto &v){original::native_frame_misc_reference_1e5a70(v);},[](auto &v){original::run_1262f0(v);});}
void reference(WorldState &s){original::run_1e5a70(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ANIMATION_ENTRY");
    std::mt19937 rng(0x1e5a70u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,entry::coverage().load(),unsigned(entry::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
