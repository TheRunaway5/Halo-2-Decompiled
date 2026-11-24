// Native TU1.5 group flood (1DE8E0) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// DDA90 and DDA10 are hash-driven stubs of their complete entry ABIs: each
// hashes what it reads (its pushed pair), writes scratch and answers AL;
// the caller-saved registers, XMM, flags and (by policy) MXCSR are random,
// and by policy EBX, EBP, ESI or EDI change or the stack is left
// unbalanced. The flood of an other group (this routine, recursively) runs
// the native port on the native side, as the binder routes it, and the
// translated body on the other.
// Fixtures author models with nodes indexed or not, edges enabled or not
// with and without pairs, groups (and the root group) with members of every
// kind, and visited sets; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_group_flood.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_group_flood_detail;
namespace {
constexpr std::uint32_t object_base=0x10000000u,model=0x10001000u,model_data=0x10002000u,edges=0x10003000u,pairs=0x10004000u,
    nodes=0x10005000u,kinds=0x10006000u,enabled=0x10007000u,groups=0x10010000u,list=0x10020000u,counts=0x10021000u,
    visited=0x10022000u,failed=0x10023000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=2;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%3?1u+std::uint32_t(rng()%255):0u);
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng()%3)*12u;
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng()%3);
    if(policy&2u && rng()%13==0)s.gp[6]=pairs+12u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=edges+0x18u*std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=12u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_dda90(WorldState &s){stub(s,0xdda90u);}
static void run_dda10(WorldState &s){stub(s,0xdda10u);}
static void run_1de8e0(WorldState &s);
#include "body.inc"
#include "reference.inc"
}
namespace {
bool native(WorldState &s);
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    // Groups 0-3, and now and then 16-39 so that every bit of the visited words is used.
    constexpr std::uint32_t group_count=40u,node_count=8u,edge_count=6u;
    const auto group_id=[&]{return chance(4)?16u+pick(24):pick(4);};
    for(std::uint32_t at=object_base;at<object_base+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(object_base+4u,std::uint32_t(rng())&~0x2000u|(chance(2)?0x2000u:0u));
    for(std::uint32_t at=model;at<model+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(model+0x48u,model_data);
    for(std::uint32_t at=model_data;at<model_data+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(model_data+0x30u,chance(8)?0u:1u+pick(edge_count));m.put(model_data+0x34u,edges);
    m.put(model_data+0x38u,chance(6)?pick(3):node_count);m.put(model_data+0x3cu,nodes);
    for(std::uint32_t k=0;k<edge_count;++k) {
        const auto e=edges+0x18u*k;
        for(std::uint32_t at=e;at<e+0x18u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(e+4u,chance(5)?0xffffu:group_id(),2);m.put(e+6u,chance(5)?0xffffu:group_id(),2);
        m.put(e+8u,chance(3)?0u:chance(6)?0xffffffffu:1u+pick(2));m.put(e+0xcu,pairs+12u*pick(2));
    }
    for(std::uint32_t at=pairs;at<pairs+12u*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<node_count;++k) {
        const auto d=nodes+0x90u*k;
        for(std::uint32_t at=d;at<d+0x90u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(d+0x18u,std::uint32_t(rng())&~0x20u|(chance(2)?0x20u:0u),1);
        m.put(d+0x1cu,chance(3)?k:chance(4)?0x8000u+pick(4):pick(12),2);
    }
    for(std::uint32_t k=0;k<16;++k)m.put(kinds+k,chance(3)?6u+pick(2):pick(6),1);
    m.put(enabled,std::uint32_t(rng())|(chance(2)?0x3fu:0u));
    // Groups: entry k's members from +4, its count at the next entry (+0x44); the root's at +0x4404 and +0x4444.
    for(std::uint32_t at=groups;at<groups+0x44u*(group_count+1u)+4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<group_count;++k) {
        for(std::uint32_t j=0;j<4;++j)m.put(groups+0x44u*k+4u+j,chance(6)?0xffu:pick(node_count),1);
        m.put(groups+0x44u*k+0x44u,chance(5)?0u:chance(8)?0xffffffffu:1u+pick(3));
    }
    for(std::uint32_t at=groups+0x4400u;at<groups+0x4448u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t j=0;j<4;++j)m.put(groups+0x4404u+j,pick(node_count),1);
    m.put(groups+0x4444u,chance(6)?0u:1u+pick(3));
    for(std::uint32_t at=list;at<list+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(counts,pick(8));m.put(visited,std::uint32_t(rng())&(chance(2)?0u:0xfu));m.put(visited+4u,std::uint32_t(rng())&(chance(2)?0u:0xffu));
    m.put(failed,0u);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    const std::uint32_t arguments[]{model,kinds,enabled,groups,std::uint32_t(rng()),std::uint32_t(rng()),
        chance(3)?0xffffffffu:group_id(),list,counts,std::uint32_t(rng()),visited,failed};
    m.put(stack,0x1df000u+pick(0x100));
    for(std::uint32_t k=0;k<12;++k)m.put(stack+4u+4u*k,arguments[k]);
    random_abi(rng,c,stack);
    c.abi.gp[1]=object_base;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{model,model_data,edges,pairs,nodes,kinds,groups,list};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1de8e0(v);},
        [](auto &v,std::uint32_t target){
            switch(target){case 0xdda90u:original::run_dda90(v);break;case 0xdda10u:original::run_dda10(v);break;
            case 0x1de8e0u:native(v);break;default:throw std::logic_error("Unbound group flood child");}});
}
void reference(WorldState &s){original::run_1de8e0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_GROUP_FLOOD");
    std::mt19937 rng(0x1de8e0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
