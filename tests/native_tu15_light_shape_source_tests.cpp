// Native TU1.5 light shape (C3A20) against the unmodified translated bodies
// of the light shape and death cycle units (the map world support copy is
// the light shape's), extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). C3960 and C39B0 (direct or through the unit's
// dispatch) are hash-driven stubs of their complete entry ABIs: each writes
// the record's kind (0..4) and scratch, the caller-saved registers, XMM,
// flags and (by policy) MXCSR are random, and by policy EDI changes or the
// stack is left unbalanced. Fixtures author lights with and without a
// marker and structure entries flagged or not; one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "pstencil/native_tu15_light_shape_source.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_light_shape_source_detail;
namespace {
constexpr std::uint32_t lights=0x10000000u,data=0x10010000u,instances=0x10020000u,structure=0x10030000u,entries=0x10040000u,
    records=0x10050000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target,s.gp[2],4,std::uint32_t(rng()%5));
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%9==0)s.gp[7]=records+0x40u*std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_c3960(WorldState &s){stub(s,0xc3960u);}
static void run_c39b0(WorldState &s){stub(s,0xc39b0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0xc39b0u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
}
#include "body.inc"
#include "reference.inc"
void step(WorldState &s,std::uint32_t target,std::uint32_t){stub(s,target);}
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbdcu,lights);m.put(0x4f0414u,instances);m.put(0x4ecc20u,structure);
    for(std::uint32_t at=lights;at<lights+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(lights+0x44u,data);
    for(std::uint32_t at=data;at<data+0x110u*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k){m.put(data+0x110u*k+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));m.put(data+0x110u*k+8u,pick(3)?pick(4):0xffffffffu);}
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,0x10070000u+0x100u*k);}
    for(std::uint32_t at=structure;at<structure+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(structure+0xecu,entries);
    for(std::uint32_t at=entries;at<entries+0x6cu*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=records;at<records+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc30000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())&0xffff0000u)|pick(4);c.abi.gp[7]=records+0x40u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{lights,data,instances,structure,entries};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_c3a20(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::step(v,target,site);});
}
void reference(WorldState &s){original::run_c3a20(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_LIGHT_SHAPE_SOURCE");
    std::mt19937 rng(0xc3a20u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
