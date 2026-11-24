// Native TU1.5 object point surface (14D410) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 130980, 145200, 1F7E00, 1F8040, 243F20 and 1211C0 (through the dispatch,
// or direct in the unit's copy) are hash-driven stubs of their complete
// entry ABIs: each hashes what it reads (its pushed arguments), writes
// scratch and its outputs (145200 the point, 1F7E00 the buffer, 243F20 the
// vertex and the material, 1211C0 its two outputs and now and then the
// result's first word), and answers AL (1F8040
// the shape in EAX); the caller-saved registers, XMM, flags and (by policy)
// MXCSR are random, and by policy EBX, EBP, ESI or EDI change or the stack
// is left unbalanced. Fixtures author models with regions, permutations
// and BSPs, objects with and without nodes, parts keyed and not, leaves of
// references in and out of range, on and off the plane, and sections; one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_object_point_surface.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_object_point_surface_detail;
namespace {
constexpr std::uint32_t definitions=0x10000000u,model=0x10001000u,regions=0x10002000u,permutations=0x10002800u,bsps=0x10003000u,
    model_nodes=0x10003800u,shape_refs=0x10004000u,parts=0x10004800u,leaves=0x10005000u,references=0x10005800u,data=0x10006000u,
    sections=0x10006800u,lists=0x10007000u,objects=0x10008000u,entries=0x10008800u,object=0x10009000u,record=0x1000a000u,
    shape=0x1000b000u,surfaces=0x1000b800u,result=0x1000c000u,scratch=0x10600000u;
std::uint32_t arguments(std::uint32_t target) {
    switch(target){case 0x1f7e00u:case 0x243f20u:return 1u;case 0x1f8040u:case 0x1211c0u:return 2u;default:return 0u;}
}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const auto args=arguments(target);
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto arg=[&](unsigned k){return r(s.memory,s.gp[4]+4u*k);};
    const auto edx=s.gp[2];
    if(target==0x145200u && rng()%2)for(std::uint32_t k=0;k<3;++k)w(s.memory,s.observe,target+1u,s.gp[2]+4u*k,4,std::uint32_t(rng()));
    if(target==0x1f7e00u && rng()%2)w(s.memory,s.observe,target+1u,s.gp[6],4,std::uint32_t(rng()));
    if(target==0x243f20u) {
        for(std::uint32_t k=0;k<3;++k)w(s.memory,s.observe,target+1u,s.gp[1]+4u*k,4,std::uint32_t(rng()));
        w(s.memory,s.observe,target+2u,s.gp[7],4,std::uint32_t(rng()));
    }
    if(target==0x1211c0u && rng()%2){w(s.memory,s.observe,target+1u,arg(1),4,std::uint32_t(rng()));w(s.memory,s.observe,target+2u,arg(2),4,std::uint32_t(rng()));}
    // Now and then the result's first word too (the section search reads it).
    if(target==0x1211c0u && rng()%3==0)w(s.memory,s.observe,target+3u,arg(1)-0xcu,4,std::uint32_t(rng()%8));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x145200u && rng()%4)s.gp[2]=edx;
    if(target==0x130980u||target==0x1f7e00u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%5?1u+std::uint32_t(rng()%255):0u);
    if(target==0x1211c0u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%3?0u:1u+std::uint32_t(rng()%255));
    if(target==0x1f8040u)s.gp[0]=rng()%8?shape:std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=rng()%2?model:parts;
    if(policy&2u && rng()%13==0)s.gp[5]=result+4u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=record+4u*std::uint32_t(rng()%4);
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
[[maybe_unused]] static void run_1f7e00(WorldState &s){stub(s,0x1f7e00u);}
[[maybe_unused]] static void run_1f8040(WorldState &s){stub(s,0x1f8040u);}
[[maybe_unused]] static void run_243f20(WorldState &s){stub(s,0x243f20u);}
[[maybe_unused]] static void run_1211c0(WorldState &s){stub(s,0x1211c0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    switch(target) {
    case 0x130980u:case 0x145200u:case 0x1f7e00u:case 0x1f8040u:case 0x243f20u:case 0x1211c0u:stub(s,target);return;
    default:throw std::runtime_error("Unbound sandbox dispatch target");
    }
}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    const unsigned shape_kind=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    const auto fill=[&](std::uint32_t from,std::uint32_t size){for(std::uint32_t at=from;at<from+size;at+=4)m.put(at,std::uint32_t(rng()));};
    m.put(0x4f0414u,definitions);m.put(0x4ecbd0u,objects);
    for(std::uint32_t k=0;k<4;++k){fill(definitions+16u*k,16u);m.put(definitions+16u*k+8u,model);}
    fill(model,0x90u);
    m.put(model+0x20u,regions);m.put(model+0x28u,bsps);m.put(model+0x4cu,model_nodes);m.put(model+0x80u,shape_refs);
    // Regions 0-3 with permutations 0-3 naming BSPs 0-1 (or 0xFF).
    for(std::uint32_t k=0;k<4;++k){fill(regions+16u*k,16u);m.put(regions+16u*k+0xcu,permutations+0x40u*k);}
    fill(permutations,0x100u);
    for(std::uint32_t k=0;k<16;++k)m.put(permutations+16u*k+0xcu,chance(6)?0xffu:pick(2),2);
    for(std::uint32_t k=0;k<2;++k){fill(bsps+0x5cu*k,0x5cu);m.put(bsps+0x5cu*k+0x34u,data);}
    fill(model_nodes,0x60u*4u);
    for(std::uint32_t k=0;k<2;++k){m.put(shape_refs+8u*k,std::uint32_t(rng()));m.put(shape_refs+8u*k+4u,parts);}
    // The BSP's data: sections, vertex lists, and parts named by node.
    fill(data,0x70u);
    const auto section_total=chance(4)?0u:1u+pick(3);
    m.put(data,section_total);m.put(data+4u,sections);m.put(data+0x24u,lists);
    const auto part_total=chance(8)?0u:1u+pick(4);
    m.put(data+0x64u,part_total);m.put(data+0x68u,data+0x80u);
    for(std::uint32_t k=0;k<4;++k)m.put(data+0x80u+k,pick(4),1);
    fill(sections,0x48u*3u);
    for(std::uint32_t k=0;k<3;++k){m.put(sections+0x48u*k+6u,pick(8),2);m.put(sections+0x48u*k+8u,pick(8),2);}
    for(std::uint32_t k=0;k<16;++k)m.put(lists+2u*k,pick(64),2);
    const auto key=std::uint32_t(rng());
    for(std::uint32_t k=0;k<4;++k) {
        const auto p=parts+16u*k;
        m.put(p,chance(5)?std::uint32_t(rng()):key);m.put(p+4u,leaves);
        m.put(p+8u,chance(8)?0u:2u+pick(6));m.put(p+0xcu,references);
    }
    for(std::uint32_t k=0;k<4;++k){m.put(leaves+8u*k,std::uint32_t(rng()),2);m.put(leaves+8u*k+2u,chance(6)?0u:1u+pick(4),2);
        m.put(leaves+8u*k+4u,chance(8)?0xffffffffu:pick(6));}
    for(std::uint32_t k=0;k<8;++k){m.put(references+8u*k,pick(8),2);m.put(references+8u*k+2u,std::uint32_t(rng()),2);
        m.put(references+8u*k+4u,chance(10)?0xffffffffu:pick(6));}
    // The shape 1F8040 answers: its surfaces' planes and key.
    fill(shape,0x20u);
    m.put(shape,2u+pick(4));m.put(shape+4u,surfaces);m.put(shape+0x10u,key);
    const auto plane=pick(4);
    for(std::uint32_t k=0;k<8;++k){m.put(surfaces+8u*k,chance(3)?plane:pick(4),2);m.put(surfaces+8u*k+2u,std::uint32_t(rng()),2);
        m.put(surfaces+8u*k+4u,std::uint32_t(rng()));}
    // The object: its node bytes, nodes and region permutation bytes.
    fill(objects,0x50u);m.put(objects+0x44u,entries);
    for(std::uint32_t k=0;k<4;++k){fill(entries+12u*k,12u);m.put(entries+12u*k+8u,object);}
    fill(object,0x200u);
    // Node bytes of whole matrices, none, or less than one (26-51: no node, though half a matrix's bytes).
    m.put(object+0x114u,chance(3)?0u:chance(4)?26u+pick(26):0x34u*(1u+pick(3)),2);m.put(object+0x116u,0x140u,2);m.put(object+0x11au,0x180u,2);
    for(std::uint32_t k=0;k<4;++k)m.put(object+0x180u+k,chance(5)?0xffu:pick(4),1);
    fill(record,0x60u);
    m.put(record+0x40u,(std::uint32_t(rng())&0xffff0000u)|pick(4));m.put(record+0x44u,pick(4),2);m.put(record+0x46u,pick(4),2);
    m.put(record+0x4cu,pick(4));m.put(record+0x54u,(std::uint32_t(rng())&0xffff8000u)|plane);
    fill(result,0x40u);
    fill(scratch,0x100u);
    fill(stack-0x100u,0x140u);
    m.put(stack,0x14d000u+pick(0x100));m.put(stack+4u,record);m.put(stack+8u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    m.put(stack+12u,result);
    random_abi(rng,c,stack);
    if(shape_kind==15) {
        static constexpr std::uint32_t areas[]{definitions,model,regions,bsps,parts,leaves,references,data,object,record};
        m.drop(areas[pick(10)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_14d410(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_14d410(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_POINT_SURFACE");
    std::mt19937 rng(0x14d410u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
