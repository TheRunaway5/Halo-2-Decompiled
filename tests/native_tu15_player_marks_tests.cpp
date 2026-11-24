// Native TU1.5 player mark counters (165860) against the unmodified
// translated body of the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The children are genuine stand-ins whose effects are a hash of their
// complete entry ABI, the iterator and the return address: 1A8E50 (the
// filtered iterator, the unit's dispatch route) advances the iterator's
// index and, until it passes sixteen, stores a player (sometimes the slot's
// own, records with and without the word +0x28 set) and returns AL; 53780
// returns AL. Both return ECX/EDX, flags and XMM, write scratch, and
// occasionally clobber callee-saved registers or leave the stack
// unbalanced. Fixtures author the four local player handles (none, some or
// all present), the gating bytes, the marks of every player index and the
// counters; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_player_marks.hpp"
using namespace frame_misc_harness;
namespace marks=halo2::native_player_marks_detail;
namespace {
constexpr std::uint32_t globals_block=0x10000000u,player_array=0x10100000u,records=0x10200000u,
    scratch=0x10600000u,none=0xffffffffu;
std::uint32_t own_handles[4]{};
void common(WorldState &s,std::mt19937_64 &rng,std::uint32_t target) {
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=rng()%2?std::uint32_t(rng()%5)-1u:(rng()%2?globals_block:std::uint32_t(rng()));
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
void iterate_stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x1a8e50u);
    h=mix(h,r(s.memory,s.gp[4]));
    const auto iterator=s.gp[6];
    const auto index=r(s.memory,iterator+0xcu);
    h=mix(h,index);
    log_call(h);
    std::mt19937_64 rng(h);
    auto next=index==none?std::uint32_t(rng()%3):index+1u+std::uint32_t(rng()%3);
    if(index==none && rng()%6==0)next=16u;                                       // no player at all
    w(s.memory,s.observe,0x1a8e51u,iterator+0xcu,4,next);
    if(next<16u) {
        std::uint32_t handle=(std::uint32_t(rng()%0x7fff)<<16)|next;
        if(rng()%5==0)handle=own_handles[rng()%4];
        w(s.memory,s.observe,0x1a8e52u,iterator+8u,4,handle);
        w(s.memory,s.observe,0x1a8e53u,iterator,4,records+0x40u*(next&15u));
        s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|1u;
    } else {
        w(s.memory,s.observe,0x1a8e52u,iterator+8u,4,none);
        s.gp[0]=std::uint32_t(rng())&0xffffff00u;
    }
    common(s,rng,0x1a8e50u);
}
void allowed_stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x53780u);
    h=mix(h,r(s.memory,s.gp[4]));
    log_call(h);
    std::mt19937_64 rng(h);
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%4?1u:0u);
    common(s,rng,0x53780u);
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x1a8e50u)throw std::runtime_error("Unbound multiplayer engines dispatch target");
    iterate_stub(s);
}
static void run_53780(WorldState &s){allowed_stub(s);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f54f0u,globals_block);m.put(0x4f54f4u,player_array);
    for(std::uint32_t at=globals_block;at<globals_block+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k) {
        own_handles[k]=(std::uint32_t(rng()%0x7fff)<<16)|pick(16);
        const bool present=shape==1?false:shape==2?true:pick(3)!=0;
        m.put(globals_block+0xcu+4u*k,present?own_handles[k]:none);
    }
    for(std::uint32_t at=player_array;at<player_array+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<16;++k) {
        const auto record=records+0x40u*k;
        for(std::uint32_t at=record;at<record+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(record+0x28u,(std::uint32_t(rng())<<16)|(pick(4)?0xffffu:pick(8)));
    }
    m.put(0x4d61d8u,(std::uint32_t(rng())&0xffffff00u)|(pick(8)?1u:0u));
    m.put(0x48377cu,(std::uint32_t(rng())&0xffffff00u)|(pick(8)?1u:0u));
    m.put(0x535cd8u,(std::uint32_t(rng())&0xffffff00u)|(pick(5)?1u:0u));
    for(std::uint32_t k=0;k<16;++k) {
        m.put(0x535da0u+4u*k,pick(3)?std::uint32_t(rng()):0u);m.put(0x535de0u+4u*k,pick(3)?std::uint32_t(rng()):0u);
    }
    for(std::uint32_t at=0x4f6400u;at<0x4f6500u;at+=4)m.put(at,std::uint32_t(rng())&0x00ff00ffu);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x160000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{globals_block,player_array,records,0x4d6000u,0x483000u,0x535000u,0x4f6000u};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return marks::bridge(s,[](auto &v){original::native_frame_misc_reference_165860(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);},
        [](auto &v){original::run_53780(v);});
}
void reference(WorldState &s){original::run_165860(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PLAYER_MARKS");
    std::mt19937 rng(0x165860u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,marks::coverage().load(),unsigned(marks::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
