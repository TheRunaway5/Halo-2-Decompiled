// Native TU1.5 body place (1D9190) against the unmodified translated body of
// the scene device constructor unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 2E4470 and 2E4350 (through the unit's dispatch) and 2E4650 and 2E58F0
// (direct) are hash-driven stubs of their complete entry ABIs: each hashes
// its arguments and the bytes they point at, writes its output (the inverse,
// the composed rows, the quaternion; 2E58F0 the body), and returns (RET 4;
// 2E58F0 RET 0); the caller-saved registers, XMM, flags and (by policy)
// MXCSR are random, and by policy EBP (another authored frame), ESI (the
// other model) or EDI (another index) change or the stack is left
// unbalanced. Fixtures author models with and without the offset bit, body
// indices 0 to 7, rates of either sign, and finite, infinite and NaN
// components; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/renderer/native_tu15_body_place.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_body_place_detail;
namespace {
constexpr std::uint32_t models=0x10000000u,bodies=0x10010000u,matrices=0x10020000u,times=0x10030000u,frames=0x10040000u,
    rigid=0x10050000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const unsigned args=target==0x2e58f0u?4u:1u;
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    const auto hash_block=[&](std::uint32_t address,unsigned words){for(unsigned k=0;k<words;++k)h=mix(h,r(s.memory,address+4u*k));};
    if(target==0x2e4650u)hash_block(s.gp[1],16);
    if(target==0x2e4350u)hash_block(r(s.memory,s.gp[4]+4u),16);
    if(target==0x2e58f0u){hash_block(r(s.memory,s.gp[4]+4u),4);hash_block(r(s.memory,s.gp[4]+8u),4);}
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x2e4470u)for(std::uint32_t k=0;k<16;++k)w(s.memory,s.observe,0x2e4471u,s.gp[1]+4u*k,4,std::uint32_t(rng()));
    if(target==0x2e4650u)for(std::uint32_t k=0;k<16;++k)if(rng()%3)w(s.memory,s.observe,0x2e4651u,s.gp[1]+4u*k,4,std::uint32_t(rng()));
    if(target==0x2e4350u)for(std::uint32_t k=0;k<4;++k)w(s.memory,s.observe,0x2e4351u,s.gp[1]+4u*k,4,std::uint32_t(rng()));
    if(target==0x2e58f0u)w(s.memory,s.observe,0x2e58f1u,r(s.memory,s.gp[4]+16u)+4u*std::uint32_t(rng()%4),4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%9==0)s.gp[5]=frames+0x80u+0x80u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%9==0)s.gp[6]=models+0x80u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%9==0)s.gp[7]=std::uint32_t(rng()%8);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%13==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=(target==0x2e58f0u?4u:8u)+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_2e4650(WorldState &s){stub(s,0x2e4650u);}
static void run_2e58f0(WorldState &s){stub(s,0x2e58f0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x2e4470u && target!=0x2e4350u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x51df54u,times+4u*pick(4));
    for(std::uint32_t at=times;at<times+0x40u;at+=4)m.put(at,pick(3)?std::uint32_t(rng()):std::uint32_t(rng())|0x80008000u);
    for(std::uint32_t at=models;at<models+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k) {
        const auto model=models+0x80u*k;
        m.put(model+4u,std::uint32_t(rng())&~0x40u|(pick(2)?0x40u:0u));m.put(model+0x70u,bodies+0x100u*pick(2));
    }
    for(std::uint32_t at=bodies;at<bodies+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t table=0;table<2;++table)for(std::uint32_t k=0;k<8;++k)m.put(bodies+0x100u*table+0x60u*k+0x40u,rigid+0x80u*pick(4));
    for(std::uint32_t at=rigid;at<rigid+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=matrices;at<matrices+0x100u;at+=4)m.put(at,component(rng));
    for(std::uint32_t at=frames;at<frames+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x140u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1d0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=matrices+0x40u*pick(4);c.abi.gp[6]=models+0x80u*pick(2);c.abi.gp[7]=pick(8);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{models,bodies,matrices,times};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1d9190(v);},[](auto &v){original::run_2e4650(v);},
        [](auto &v){original::run_2e58f0(v);},[](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1d9190(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_BODY_PLACE");
    std::mt19937 rng(0x1d9190u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
