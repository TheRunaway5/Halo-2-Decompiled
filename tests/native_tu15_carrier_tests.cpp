// Native TU1.5 carrier (D2B60) against the unmodified translated body of the
// physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author chains of up to eight objects, each
// parent earlier in the chain (or NONE), attached or not, of every type;
// one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_carrier.hpp"
using namespace frame_misc_harness;
namespace carrier=halo2::native_carrier_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t header=0x10000000u,rows=0x10010000u,objects=0x10020000u,results=0x10030000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,header);
    for(std::uint32_t at=header;at<header+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(header+0x44u,rows);
    for(std::uint32_t k=0;k<8;++k) {
        m.put(rows+12u*k,std::uint32_t(rng()));m.put(rows+12u*k+4u,std::uint32_t(rng()));m.put(rows+12u*k+8u,objects+0x100u*k);
        const auto object=objects+0x100u*k;
        for(std::uint32_t at=object;at<object+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0x14u,k==0 || pick(5)==0?0xffffffffu:(std::uint32_t(rng())<<16)|pick(k));
        m.put(object+4u,pick(4)?std::uint32_t(rng())|(1u<<26):std::uint32_t(rng())&~(1u<<26));
        m.put(object+0xaau,pick(3)?pick(2):pick(32),1);
    }
    for(std::uint32_t at=results;at<results+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xd20000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(8);c.abi.gp[3]=results+4u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{header,rows,objects,results};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s){return carrier::bridge(s,[](auto &v){original::native_frame_misc_reference_d2b60(v);});}
void reference(WorldState &s){original::run_d2b60(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CARRIER");
    std::mt19937 rng(0xd2b60u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,carrier::coverage().load(),unsigned(carrier::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
