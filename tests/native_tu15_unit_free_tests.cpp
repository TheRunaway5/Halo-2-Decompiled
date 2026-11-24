// Native TU1.5 unit free test (CAAA0) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// E9530 and 105950 are hash-driven stubs of their complete entry ABI: AL
// mostly clear, the caller-saved registers, XMM, flags and (by policy)
// MXCSR random, by policy a callee-saved register moved or the stack left
// unbalanced. Fixtures author units with and without parents, flag blocks
// (signed offsets) with and without the blocking bits, and slots (NONE
// indices, NONE handles); one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_unit_free.hpp"
using namespace frame_misc_harness;
namespace unit_free=halo2::native_unit_free_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,units=0x10100000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%8?0u:1u+std::uint32_t(rng()%255));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=which==6?units+0x400u*std::uint32_t(rng()%4):which==5?std::uint32_t(rng()%3):which==3?headers:std::uint32_t(rng());
    }
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
static void run_e9530(WorldState &s){stub(s,0xe9530u);}
static void run_105950(WorldState &s){stub(s,0x105950u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    for(std::uint32_t k=0;k<4;++k) {
        const auto unit=units+0x400u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,unit);
        for(std::uint32_t at=unit;at<unit+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(unit+0x14u,pick(4)==0?std::uint32_t(rng()):0xffffffffu);
        const auto block=pick(4)==0?0xfff0u+4u*pick(4):0x380u+4u*pick(8);
        m.put(unit+0x346u,block,2);
        m.put(unit+std::uint32_t(std::int32_t(std::int16_t(block)))+4u,std::uint32_t(rng())&(pick(4)?~(3u<<26):~0u));
        for(std::uint32_t s=0;s<2;++s)m.put(unit+0x212u+s,pick(3)==0?0xffu:pick(4),1);
        for(std::uint32_t s=0;s<4;++s)m.put(unit+0x218u+4u*s,pick(3)==0?0xffffffffu:std::uint32_t(rng()));
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x60u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,units+0x400u*2u};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return unit_free::bridge(s,[](auto &v){original::native_frame_misc_reference_caaa0(v);},
        [](auto &v){original::run_e9530(v);},[](auto &v){original::run_105950(v);});
}
void reference(WorldState &s){original::run_caaa0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_UNIT_FREE");
    std::mt19937 rng(0xcaaa0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,unit_free::coverage().load(),unsigned(unit_free::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
