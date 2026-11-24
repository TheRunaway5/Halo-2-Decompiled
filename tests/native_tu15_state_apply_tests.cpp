// Native TU1.5 state apply (C92C0) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author units and states (pairs equal to
// the unit's or not, choices NONE or not), states overlapping their units;
// one region removed injects a fault; a set direction flag is refused.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "campaign/ai/native_tu15_state_apply.hpp"
using namespace frame_misc_harness;
namespace apply=halo2::native_state_apply_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,units=0x10100000u,states=0x10200000u;
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
    }
    for(std::uint32_t at=states;at<states+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    const std::uint32_t object=pick(4);
    const auto state=shape==7?units+0x400u*object+0x100u:states+4u*pick(32);
    const auto unit=units+0x400u*object;
    if(pick(2))m.put(state+4u,(m.get(state+4u)&0x0000ffffu)|(m.get(unit+0x214u)<<16));       // pair matches
    if(pick(3)==0)m.put(state+8u,(m.get(state+8u)&0x0000ffffu)|0xffff0000u);                  // no choice
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc9000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape!=14)c.abi.eflags&=~0x400u;
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|object;c.abi.gp[1]=state;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,units,states};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s){return apply::bridge(s,[](auto &v){original::native_frame_misc_reference_c92c0(v);});}
void reference(WorldState &s){original::run_c92c0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_STATE_APPLY");
    std::mt19937 rng(0xc92c0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t,(c.abi.eflags&0x400u)!=0))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,apply::coverage().load(),unsigned(apply::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
