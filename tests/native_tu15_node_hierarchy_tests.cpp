// Native TU1.5 node hierarchy transform (170730) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The child 145560 (the transform combine) is a genuine stand-in whose
// effects are a hash of its complete entry ABI, return address and argument
// word: it writes the child's matrix, returns EAX/ECX/EDX (ECX becomes the
// recursion's parent), flags and XMM, and occasionally clobbers
// callee-saved registers or leaves the stack unbalanced. The recursion runs
// through the bridge first, as the bound unit does. Fixtures author model
// definitions with node trees (first child and sibling links, NONE and
// negative indices, indices at and past the node count and the matrix
// limit), matrices and parent transforms; one region removed injects a
// fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_node_hierarchy.hpp"
#include <algorithm>
#include <numeric>
using namespace frame_misc_harness;
namespace hierarchy=halo2::native_node_hierarchy_detail;
namespace {
constexpr std::uint32_t model=0x10000000u,nodes=0x10100000u,matrices=0x10200000u,transforms=0x10300000u,
    scratch=0x10600000u;
std::uint32_t stack=0x20010000u;
void combine_stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x145560u);
    for(unsigned k=0;k<=1;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x145561u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    w(s.memory,s.observe,0x145562u,r(s.memory,s.gp[4]+4u)+4u*std::uint32_t(rng()%13),4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());
    s.gp[1]=rng()%4?s.gp[1]:transforms+4u*std::uint32_t(rng()%8);
    s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        s.gp[saved[rng()%4]]=rng()%3?std::uint32_t(rng()%64):0x80000000u|std::uint32_t(rng()%64);
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=4u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_145560(WorldState &s){combine_stub(s);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    stack=0x20010000u+pick(64)*4u;
    // A node tree: every node gets a parent among the earlier nodes.
    const std::uint32_t count=1u+pick(40);
    for(std::uint32_t at=model;at<model+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(model+0x48u,shape==10?0x80000000u|pick(4):count);m.put(model+0x4cu,nodes);
    const std::uint32_t built=count+(shape==11?3u:0u);                // a few nodes past the count
    for(std::uint32_t k=0;k<built+2u;++k) {
        const auto node=nodes+k*0x60u;
        for(std::uint32_t at=node;at<node+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(node+6u,0xffffu,2);m.put(node+8u,0xffffu,2);
    }
    std::vector<std::uint32_t> last_child(built,0xffffffffu);
    for(std::uint32_t k=1;k<built;++k) {
        const auto parent=pick(k);
        if(last_child[parent]==0xffffffffu)m.put(nodes+parent*0x60u+6u,k,2);
        else m.put(nodes+last_child[parent]*0x60u+8u,k,2);
        last_child[parent]=k;
    }
    if(shape==12)m.put(nodes+pick(built)*0x60u+8u,0x8000u|pick(4),2);     // a negative sibling
    for(std::uint32_t at=matrices;at<matrices+0x34u*(built+4u);at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=transforms;at<transforms+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x3000u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x170000u+pick(0x100));
    m.put(stack+4u,model);m.put(stack+8u,transforms+4u*pick(8));
    m.put(stack+0xcu,pick(4)?0u:pick(built));
    std::uint32_t limit=built+pick(4);
    if(pick(4)==0)limit=pick(built+1u);
    m.put(stack+0x10u,limit);m.put(stack+0x14u,matrices);
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{model,nodes,matrices,transforms,scratch,nodes+0x1000u,matrices+0x1000u,model};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s);
void descend(WorldState &s){if(!native(s))original::run_170730(s);}
bool native(WorldState &s) {
    return hierarchy::bridge(s,[](auto &v){original::native_frame_misc_reference_170730(v);},
        [](auto &v){original::run_145560(v);},[](auto &v){descend(v);});
}
void reference(WorldState &s){original::run_170730(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_NODE_HIERARCHY");
    std::mt19937 rng(0x170730u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,hierarchy::coverage().load(),unsigned(hierarchy::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
