// Native TU1.5 player relation (1902E0) against the unmodified translated
// body (the physics live support unit's copy; the validator repeats it with
// the map world support unit's), extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The children 158290 (a player's type) and BCFE0 (the object ancestry) are
// genuine stand-ins whose effects are a hash of their complete entry ABI and
// return address: they return EAX (types 0..5, AL 0 or 1), ECX/EDX and
// flags, write scratch, and occasionally clobber callee-saved registers or
// leave the stack unbalanced. Fixtures author the local player slots
// (absent, present), the players array and units (NONE or not), the flag
// (1 or not) and the index output (null or not); one region removed
// injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_player_relation.hpp"
using namespace frame_misc_harness;
namespace relation=halo2::native_player_relation_detail;
namespace {
constexpr std::uint32_t locals=0x10000000u,players=0x10010000u,records=0x10100000u,outputs=0x10200000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=target==0x158290u?std::uint32_t(rng()%6):(std::uint32_t(rng())&0xffffff00u)|(rng()%4==0?1u:0u);
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=which==7?std::uint32_t(rng()%6):which==5?locals+0xcu+4u*std::uint32_t(rng()%4):std::uint32_t(rng());
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
[[maybe_unused]] static void run_158290(WorldState &s){stub(s,0x158290u);}
[[maybe_unused]] static void run_bcfe0(WorldState &s){stub(s,0xbcfe0u);}
[[maybe_unused]] static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x158290u && target!=0xbcfe0u)throw std::runtime_error("Unbound dispatch target");
    stub(s,target);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f54f0u,locals);m.put(0x4f54f4u,players);
    for(std::uint32_t at=locals;at<locals+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(locals+0xcu+4u*k,pick(3)==0?0xffffffffu:(std::uint32_t(rng()%0x7fff)<<16)|pick(16));
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x44u,records);
    for(std::uint32_t k=0;k<16;++k) {
        m.put(records+0x21cu*k,std::uint32_t(rng()));
        m.put(records+0x21cu*k+0x2cu,pick(4)==0?0xffffffffu:std::uint32_t(rng()));
    }
    for(std::uint32_t at=outputs;at<outputs+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(outputs,shape==5?pick(4):1u,1);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x190400u+pick(0x100));
    m.put(stack+4u,std::uint32_t(rng()));m.put(stack+8u,outputs);m.put(stack+0xcu,pick(4)?outputs+8u:0u);
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{locals,players,records,outputs,0x4f5000u};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return relation::bridge(s,[](auto &v){original::native_frame_misc_reference_1902e0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1902e0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PLAYER_RELATION");
    std::mt19937 rng(0x1902e0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,relation::coverage().load(),unsigned(relation::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
