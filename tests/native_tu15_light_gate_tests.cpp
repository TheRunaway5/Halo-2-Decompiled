// Native TU1.5 light gate (31500) against the unmodified translated body of
// the light portal unit, extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). 3E9B0 (through the unit's dispatch) is a
// hash-driven stub of its complete entry ABI: it answers AL, writes scratch,
// the caller-saved registers, XMM, flags and (by policy) MXCSR are random,
// and by policy EBX or EDI change or the stack is left unbalanced. Fixtures
// author lights NONE or not, with portals or none, and definitions with
// every gate flag; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "pstencil/native_tu15_light_gate.hpp"
using namespace frame_misc_harness;
namespace gate=halo2::native_light_gate_detail;
namespace {
constexpr std::uint32_t header=0x10000000u,lights=0x10010000u,tags=0x10020000u,definitions=0x10030000u,scratch=0x10600000u;
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x3e9b0u);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x3e9b1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=rng()%2?std::uint32_t(rng())&~0xffu:std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=definitions+0x10u*std::uint32_t(rng()%2);
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
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x3e9b0u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s);
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
    m.put(0x4ecbdcu,header);m.put(0x4f0414u,tags);
    for(std::uint32_t at=header;at<header+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(header+0x44u,lights);
    for(std::uint32_t at=lights;at<lights+4u*0x110u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k) {
        m.put(lights+0x110u*k+4u,(std::uint32_t(rng())<<16)|pick(4));
        m.put(lights+0x110u*k+0x54u,pick(3)==0?0xffffu:pick(8),2);
    }
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=tags+16u*k;at<tags+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));m.put(tags+16u*k+8u,definitions+0x10u*(k%2));}
    for(std::uint32_t d=0;d<2;++d)m.put(definitions+0x10u*d,(std::uint32_t(rng())&~0x60u)|(pick(4)<<5));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x31000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=pick(6)==0?0xffffffffu:(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{header,lights,tags,definitions};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return gate::bridge(s,[](auto &v){original::native_frame_misc_reference_31500(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_31500(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_LIGHT_GATE");
    std::mt19937 rng(0x31500u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,gate::coverage().load(),unsigned(gate::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
