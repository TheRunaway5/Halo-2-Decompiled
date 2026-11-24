// Native TU1.5 shape ray hit (2E9140) against the unmodified translated body
// of the live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The virtual calls (through the unit's dispatch, at the targets the
// authored vtables name) are hash-driven stubs of their complete entry ABIs:
// the shape's cast hashes the input and the output, writes the hit flag (0
// half the time) and the output's normal, and answers the flag (now and then
// another flag) in EAX (RET C); the collector's hashes the body and the
// output (RET 8). The caller-saved registers, XMM, flags and (by policy)
// MXCSR are random, and by policy EBP (another authored frame) or ESI (the
// other collector) change or the stack is left unbalanced. Fixtures author
// shapes, collectors and bodies with two vtables each, finite, infinite and
// NaN normals and columns; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_shape_ray_hit.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_shape_ray_hit_detail;
namespace {
constexpr std::uint32_t shapes=0x10000000u,collectors=0x10010000u,cd_bodies=0x10020000u,motions=0x10030000u,vtables=0x10040000u,
    inputs=0x10050000u,frames=0x10060000u,flags_area=0x10070000u,scratch=0x10600000u;
constexpr std::uint32_t casts[]{0x2f5000u,0x2f5100u},adds[]{0x2f6000u,0x2f6100u};
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const bool casting=target==casts[0]||target==casts[1];
    const unsigned args=casting?3u:2u;
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    const auto output=r(s.memory,s.gp[4]+(casting?12u:8u));
    for(std::uint32_t k=0;k<6;++k)h=mix(h,r(s.memory,output+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(casting) {
        const auto flag=r(s.memory,s.gp[4]+4u);
        w(s.memory,s.observe,target+1u,flag,1,rng()%2?0u:1u+std::uint32_t(rng()%255));
        for(std::uint32_t k=0;k<4;++k)if(rng()%4)w(s.memory,s.observe,target+2u,output+4u*k,4,rng()%9?std::uint32_t(rng()%0x7f800000u):0x7fc00001u);
    }
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    const auto flag=r(s.memory,s.gp[4]+4u);
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(casting)s.gp[0]=rng()%9?flag:flags_area+std::uint32_t(rng()%4);
    if(policy&2u && rng()%9==0)s.gp[5]=frames+0x80u+0x80u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%9==0)s.gp[6]=collectors+0x40u*std::uint32_t(rng()%2);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%13==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=(casting?16u:12u)+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=casts[0] && target!=casts[1] && target!=adds[0] && target!=adds[1])throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t component(std::mt19937 &rng) {
    switch(rng()%12) {
    case 0:return 0x7fc00000u|std::uint32_t(rng()%0x100);
    case 1:return 0x7f800000u|(rng()%2?0x80000000u:0u);
    case 2:return 0u;
    default:{float f=float(int(rng()%2001)-1000)/37.0f;std::uint32_t v;std::memcpy(&v,&f,4);return v;}
    }
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    for(std::uint32_t at=vtables;at<vtables+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(vtables+0x20u,casts[0]);m.put(vtables+0x80u+0x20u,casts[1]);m.put(vtables+0x40u,adds[0]);m.put(vtables+0xc0u,adds[1]);
    for(std::uint32_t at=shapes;at<shapes+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k)m.put(shapes+0x40u*k,vtables+0x80u*pick(2));
    for(std::uint32_t at=collectors;at<collectors+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k)m.put(collectors+0x40u*k,vtables+0x40u+0x80u*pick(2));
    for(std::uint32_t at=cd_bodies;at<cd_bodies+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k)m.put(cd_bodies+0x40u*k+8u,motions+0x100u*pick(2)+(pick(6)?0u:4u*pick(4)));
    for(std::uint32_t at=motions;at<motions+0x200u;at+=4)m.put(at,component(rng));
    for(std::uint32_t at=inputs;at<inputs+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=flags_area;at<flags_area+0x10u;at+=4)m.put(at,pick(2)?0u:std::uint32_t(rng()));
    for(std::uint32_t at=frames;at<frames+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,component(rng));
    m.put(stack,0x2e0000u+pick(0x100));
    m.put(stack+4u,inputs+4u*pick(8));m.put(stack+8u,cd_bodies+0x40u*pick(2));m.put(stack+12u,collectors+0x40u*pick(2));
    random_abi(rng,c,stack);
    c.abi.gp[1]=shapes+0x40u*pick(2);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{shapes,collectors,cd_bodies,motions,vtables};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_2e9140(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_2e9140(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SHAPE_RAY_HIT");
    std::mt19937 rng(0x2e9140u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
