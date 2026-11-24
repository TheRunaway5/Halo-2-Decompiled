// Native TU1.5 channel events (1CF700) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The callback (CALL EBX, through the dispatch) is a hash-driven stub of its
// complete entry ABI, its three arguments and the record they point at: it
// writes scratch, the caller-saved registers, XMM, flags and (by policy)
// MXCSR are random, and by policy EBX, EBP, ESI or EDI change or the stack
// is left unbalanced. Fixtures author steps forward, backward and empty
// (NaN too), the three passes on and off, events, sounds and effects on,
// inside and outside the range, references missing or without a tag, every
// gate bit against the options, and no callback; one region removed injects
// a fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/animation/native_tu15_channel_events.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_channel_events_detail;
namespace {
constexpr std::uint32_t animations=0x10000000u,lists=0x10010000u,definitions=0x10020000u,references=0x10030000u,scratch=0x10600000u;
constexpr std::uint32_t callback=0x1d0000u;
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,callback);
    for(std::uint32_t k=1;k<=3;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    const auto record=r(s.memory,s.gp[4]+12u);
    for(std::uint32_t k=0;k<20;k+=4)h=mix(h,r(s.memory,record+k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,callback+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=rng()%2?0u:callback;
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng()%6);
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng()%6);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng()%6);
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
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=callback)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    for(std::uint32_t at=animations;at<animations+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(animations+0x4cu,pick(6)?pick(7):0u-pick(2));m.put(animations+0x50u,lists);
    m.put(animations+0x54u,pick(6)?pick(7):0u-pick(2));m.put(animations+0x58u,lists+0x100u);
    m.put(animations+0x5cu,pick(6)?pick(7):0u-pick(2));m.put(animations+0x60u,lists+0x200u);
    for(std::uint32_t k=0;k<8;++k){m.put(lists+4u*k,pick(20),2);m.put(lists+4u*k+2u,pick(12),2);}
    for(std::uint32_t k=0;k<8;++k){m.put(lists+0x100u+8u*k,pick(5)?pick(4):0xffffu,2);m.put(lists+0x100u+8u*k+2u,pick(12),2);m.put(lists+0x104u+8u*k,std::uint32_t(rng()));}
    for(std::uint32_t k=0;k<8;++k){m.put(lists+0x200u+4u*k,pick(5)?pick(4):0xffffu,2);m.put(lists+0x200u+4u*k+2u,pick(12),2);}
    for(std::uint32_t at=definitions;at<definitions+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(definitions+0x18u,pick(10)?references:0u);m.put(definitions+0x20u,pick(10)?references+0x40u:0u);
    for(std::uint32_t k=0;k<8;++k){m.put(references+12u*k,std::uint32_t(rng()));m.put(references+12u*k+4u,pick(5)?std::uint32_t(rng()):0xffffffffu);
        m.put(references+12u*k+8u,std::uint32_t(rng())&0x36u,2);}
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1c0000u+pick(0x100));m.put(stack+4u,definitions);m.put(stack+8u,animations);
    m.put(stack+12u,std::uint32_t(rng())&(pick(4)?0x3000u:0x33a0u));
    m.put(stack+16u,pick(12)?bits_of(float(pick(12))):0x7fc00000u);m.put(stack+20u,bits_of(float(pick(12))));
    m.put(stack+24u,std::uint32_t(rng()));
    random_abi(rng,c,stack);
    c.abi.gp[3]=pick(6)?callback:0u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{animations,lists,definitions,references};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1cf700(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1cf700(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CHANNEL_EVENTS");
    std::mt19937 rng(0x1cf700u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
