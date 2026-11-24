// Native TU1.5 point surface (14D790) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 130980, 145200, 171010 and 1211C0 (through the dispatch, or direct in the
// unit's copy) are hash-driven stubs of their complete entry ABIs: each
// hashes what it reads (its pushed arguments), writes scratch and its
// outputs (145200 the point, 171010 the projection, 1211C0 its two
// outputs), and 130980 and 1211C0 answer AL; the caller-saved registers,
// XMM, flags and (by policy) MXCSR are random, and by policy EBX, EBP, ESI
// or EDI change or the stack is left unbalanced. Fixtures author structure
// leaves and instance leaves, references on and off the plane, surfaces and
// vertices in and out of range; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_point_surface.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_point_surface_detail;
namespace {
constexpr std::uint32_t structure=0x10000000u,leaves=0x10001000u,references=0x10002000u,nodes=0x10003000u,definitions=0x10004000u,
    instances=0x10005000u,blocks=0x10006000u,surfaces=0x10007000u,lists=0x10008000u,vertex_blocks=0x10009000u,vertices=0x1000a000u,
    records=0x10010000u,results=0x10011000u,scratch=0x10600000u;
std::uint32_t arguments(std::uint32_t target){return target==0x171010u||target==0x1211c0u?2u:0u;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const auto args=arguments(target);
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto arg=[&](unsigned k){return r(s.memory,s.gp[4]+4u*k);};
    if(target==0x145200u && rng()%2)for(std::uint32_t k=0;k<3;++k)w(s.memory,s.observe,target+1u,s.gp[6]+4u*k,4,std::uint32_t(rng()));
    if(target==0x171010u)for(std::uint32_t k=0;k<3;++k)w(s.memory,s.observe,target+1u,arg(2)+4u*k,4,std::uint32_t(rng()));
    if(target==0x1211c0u && rng()%2){w(s.memory,s.observe,target+1u,arg(1),4,std::uint32_t(rng()));w(s.memory,s.observe,target+2u,arg(2),4,std::uint32_t(rng()));}
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x130980u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%4?1u+std::uint32_t(rng()%255):0u);
    if(target==0x1211c0u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%3?0u:1u+std::uint32_t(rng()%255));
    if(policy&2u && rng()%13==0)s.gp[3]=rng()%2?structure:blocks+0x100u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[5]=references+8u*std::uint32_t(rng()%16);
    if(policy&2u && rng()%13==0)s.gp[6]=lists+2u*std::uint32_t(rng()%16);
    if(policy&2u && rng()%13==0)s.gp[7]=rng()%2?0u:definitions+0xc8u*std::uint32_t(rng()%2);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*args+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
// Each unit's copy calls its children directly or through the dispatch.
[[maybe_unused]] static void run_130980(WorldState &s){stub(s,0x130980u);}
[[maybe_unused]] static void run_145200(WorldState &s){stub(s,0x145200u);}
[[maybe_unused]] static void run_171010(WorldState &s){stub(s,0x171010u);}
[[maybe_unused]] static void run_1211c0(WorldState &s){stub(s,0x1211c0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x130980u && target!=0x145200u && target!=0x171010u && target!=0x1211c0u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x4ecc18u,structure);
    for(std::uint32_t at=structure;at<structure+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(structure+0x18u,blocks+0x300u);m.put(structure+0x30u,leaves);m.put(structure+0x50u,references);m.put(structure+0xa0u,nodes);
    m.put(structure+0x13cu,definitions);m.put(structure+0x144u,instances);
    // Blocks 0-2 surfaces of nodes and definitions, block 3 the structure's.
    for(std::uint32_t k=0;k<4;++k) {
        const auto b=blocks+0x100u*k;
        for(std::uint32_t at=b;at<b+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(b+4u,surfaces);m.put(b+0x24u,lists);m.put(b+0x3cu,vertex_blocks+0x10u*pick(2));
    }
    for(std::uint32_t k=0;k<2;++k){const auto v=vertex_blocks+0x10u*k;m.put(v,std::uint32_t(rng()));m.put(v+2u,4u+pick(8),2);
        const bool absent=chance(6);m.put(v+8u,absent?0u:vertices-0x100u*k);m.put(v+0xcu,absent?0u:0x100u*k);}
    for(std::uint32_t at=vertices;at<vertices+12u*16u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<24;++k)m.put(lists+2u*k,chance(5)?8u+pick(8):pick(12),2);
    for(std::uint32_t k=0;k<8;++k){m.put(surfaces+8u*k,pick(4),2);m.put(surfaces+8u*k+2u,std::uint32_t(rng()),2);m.put(surfaces+8u*k+4u,std::uint32_t(rng()));}
    for(std::uint32_t k=0;k<4;++k){const auto l=leaves+8u*k;m.put(l,pick(2),2);m.put(l+2u,pick(5),2);m.put(l+4u,pick(8));}
    for(std::uint32_t k=0;k<16;++k){const auto e=references+8u*k;m.put(e,pick(20),2);m.put(e+2u,std::uint32_t(rng()),2);m.put(e+4u,chance(5)?0xffffffffu:pick(8));}
    for(std::uint32_t k=0;k<2;++k) {
        const auto d=nodes+0xb0u*k;
        for(std::uint32_t at=d;at<d+0xb0u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(d+0x50u,chance(5)?0u:blocks+0x100u*pick(3));
    }
    for(std::uint32_t k=0;k<2;++k) {
        const auto d=definitions+0xc8u*k;
        for(std::uint32_t at=d;at<d+0xc8u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(d+0x50u,chance(5)?0u:blocks+0x100u*pick(3));m.put(d+0x74u,surfaces);m.put(d+0xb8u,chance(5)?0u:1u+pick(4));
        m.put(d+0xbcu,leaves);m.put(d+0xc4u,references);
    }
    for(std::uint32_t k=0;k<2;++k){for(std::uint32_t at=instances+0x58u*k;at<instances+0x58u*(k+1u);at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+0x58u*k+0x34u,pick(2),2);}
    for(std::uint32_t at=records;at<records+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto record_kind=chance(6)?2u:chance(2)?1u:3u;
    m.put(records,record_kind);m.put(records+0x3cu,record_kind==3u||chance(8)?pick(2):0xffffffffu);
    m.put(records+0x4cu,pick(4));m.put(records+0x54u,(std::uint32_t(rng())&0xffff8000u)|pick(4));
    for(std::uint32_t at=results;at<results+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x14d000u+pick(0x100));m.put(stack+4u,records);m.put(stack+8u,results);
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{structure,leaves,references,nodes,definitions,blocks,surfaces,lists,vertices,records};
        m.drop(regions[pick(10)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_14d790(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_14d790(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_POINT_SURFACE");
    std::mt19937 rng(0x14d790u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
