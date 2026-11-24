// Native TU1.5 node matrix gather (16B240) against the unmodified translated
// body of the sandbox special unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The child 145560 (the unit's dispatch route) is a genuine stand-in whose
// effects are a hash of its complete entry ABI, return address and argument
// word: it writes a destination matrix word, returns EAX/ECX/EDX, flags and
// the direction flag, and occasionally clobbers callee-saved registers
// (EBX the node count, EBP the frame) or leaves the stack unbalanced.
// Fixtures author the tag instances and model definitions (node counts zero,
// negative, within and beyond the capacity), node maps with NONE and
// out-of-range entries, the transform (zero or not), sources, destinations
// (overlapping the sources), the direction flag set and clear; one region
// removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_node_gather.hpp"
using namespace frame_misc_harness;
namespace gather=halo2::native_node_gather_detail;
namespace {
constexpr std::uint32_t tags=0x10000000u,definitions=0x10100000u,maps=0x10200000u,sources=0x10300000u,
    destinations=0x10400000u,transforms=0x10500000u,scratch=0x10600000u;
constexpr std::uint32_t definition_count=6u,none=0xffffffffu;
std::uint32_t stack=0x20010000u;
void transform_stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x145560u);
    for(unsigned k=0;k<=1;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x145561u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    const auto destination=r(s.memory,s.gp[4]+4u);
    w(s.memory,s.observe,0x145562u,destination+4u*std::uint32_t(rng()%13),4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=which==5?s.gp[5]+4u*std::uint32_t(rng()%3):(rng()%3?std::uint32_t(rng()%64):0x80000000u|std::uint32_t(rng()%64));
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(rng()%5==0)s.other_flags^=0x400u;                       // the direction flag
    std::uint32_t cleanup=4u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x145560u)throw std::runtime_error("Unbound sandbox dispatch target");
    transform_stub(s);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    stack=0x20010000u+pick(64)*4u;
    m.put(0x4f0414u,tags);
    for(std::uint32_t k=0;k<definition_count;++k) {
        for(unsigned w=0;w<4;++w)m.put(tags+k*16u+4u*w,std::uint32_t(rng()));
        m.put(tags+k*16u+8u,definitions+k*0x100u);
        for(std::uint32_t at=definitions+k*0x100u;at<definitions+k*0x100u+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
        std::uint32_t count=pick(12);
        if(pick(10)==0)count=0x80000000u|pick(4);
        if(pick(8)==0)count=40u+pick(30);
        m.put(definitions+k*0x100u+0x48u,count);
    }
    // The node map, sources, destinations and a transform.
    const std::uint32_t source_count=8u+pick(16);
    for(std::uint32_t k=0;k<80u;++k) {
        std::uint32_t entry=pick(source_count);
        if(pick(4)==0)entry=none;
        if(shape==12 && pick(6)==0)entry=source_count+pick(200);     // past the sources (fault)
        m.put(maps+4u*k,entry);
    }
    for(std::uint32_t at=sources-0x40u;at<sources+0x34u*24u+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=destinations-0x40u;at<destinations+0x34u*80u+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=transforms;at<transforms+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x400u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x168da4u);
    m.put(stack+4u,pick(2)?0u:transforms);
    const auto destination=shape==9?sources+0x34u*pick(4):destinations+4u*pick(4);   // overlapping the sources
    m.put(stack+8u,destination);
    std::uint32_t capacity=pick(70);
    if(shape==10)capacity=0x80000000u|pick(8);
    m.put(stack+0xcu,capacity);
    m.put(stack+0x10u,sources);m.put(stack+0x14u,maps);
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(definition_count);
    if(shape==7)c.abi.eflags|=0x400u;else c.abi.eflags&=~0x400u;   // the direction flag
    if(shape==15) {
        static constexpr std::uint32_t regions[]{tags,definitions,maps,sources,destinations,0x4f0000u,transforms,scratch};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return gather::bridge(s,[](auto &v){original::native_frame_misc_reference_16b240(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_16b240(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_NODE_GATHER");
    std::mt19937 rng(0x16b240u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,gather::coverage().load(),unsigned(gather::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
