// Native TU1.5 object activation (B92C0) against the unmodified translated
// body of the physics live support unit (the validator repeats it with the
// AI frame unit's, which dispatches 1CC3F0), extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1CC3F0 is reached through the unit's dispatch, a hash-driven stub of its
// complete entry ABI (ECX the handle): it writes the object or scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy ESI changes or the stack is left unbalanced. Fixtures author
// headers active or not, words NONE or not, parents, every type (past 31
// too) and attachment bit 14; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_object_activate.hpp"
using namespace frame_misc_harness;
namespace activate=halo2::native_object_activate_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,counters=0x10180000u,
    scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    const std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,rng()%2?scratch+std::uint32_t(rng()%64)*4u:objects+0x400u*std::uint32_t(rng()%4)+4u,4,
      std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%9==0)s.gp[6]=objects+0x400u*std::uint32_t(rng()%4);
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
static void run_1cc3f0(WorldState &s){stub(s,0x1cc3f0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x1cc3f0u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x4ecbd0u,headers);m.put(0x4eabc4u,counters+4u*pick(4));
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    for(std::uint32_t at=counters;at<counters+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    static constexpr std::uint32_t types[]{0u,1u,2u,3u,7u,11u,12u,13u,32u,0xffu};
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x400u*k+4u*k;
        m.put(header_data+12u*k,(std::uint32_t(rng())&0xffffu)|((pick(3)==0?1u:0u)|(pick(256)&~1u))<<16|types[pick(10)]<<24);
        m.put(header_data+12u*k+4u,pick(4)==0?0xffffu:pick(0xffff),2);
        m.put(header_data+12u*k+6u,pick(0x10000),2);
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0x14u,pick(4)==0?std::uint32_t(rng()):0xffffffffu);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xb0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[7]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x800u,counters};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return activate::bridge(s,[](auto &v){original::native_frame_misc_reference_b92c0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_b92c0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_ACTIVATE");
    std::mt19937 rng(0xb92c0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,activate::coverage().load(),unsigned(activate::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
