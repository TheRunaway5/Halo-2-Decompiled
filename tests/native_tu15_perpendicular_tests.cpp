// Native TU1.5 perpendicular (11FA50) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author vectors (ordinary, vertical, short
// and zero horizontal lengths, NaN, infinite and denormal components), the
// epsilon (also zero, negative and NaN), the default axis and results
// (also the vector itself), on a live x87 stack (a nearly full one is
// refused); one region removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary with
// branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_perpendicular.hpp"
using namespace frame_misc_harness;
namespace perpendicular=halo2::native_perpendicular_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t vectors=0x10000000u,axes=0x10100000u,results=0x10200000u;
std::uint32_t component(std::mt19937 &rng) {
    static constexpr std::uint32_t special[]{0u,0x80000000u,0x7fc00000u,0x7f800000u,0xff800000u,0x00000001u,0x3f800000u,0xbf800000u,
        0x38d1b717u,0xb8d1b717u,0x7f7fffffu,0x00800000u};
    switch(rng()%6) {
    case 0:return special[rng()%12];
    case 1:return std::bit_cast<std::uint32_t>(float(int(rng()%2001)-1000)/997.0f*1e-4f);
    case 2:return std::uint32_t(rng());
    default:return std::bit_cast<std::uint32_t>(float(int(rng()%2001)-1000)/37.0f);
    }
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=vectors;at<vectors+0x100u;at+=4)m.put(at,component(rng));
    const auto vector=vectors+4u*pick(60);
    if(shape==3||shape==4){m.put(vector,shape==3?0u:component(rng)&0x807fffffu);m.put(vector+4u,shape==3?0x80000000u:0u);}
    static constexpr std::uint32_t epsilons[]{0x38d1b717u,0x38d1b717u,0x3a83126fu,0u,0xb8d1b717u,0x7fc00000u,0x7f800000u,0x00000001u};
    m.put(0x46a0ecu,epsilons[pick(8)]);
    m.put(0x46a0d0u,pick(6)?0x3f800000u:component(rng));
    for(std::uint32_t at=axes;at<axes+0x40u;at+=4)m.put(at,component(rng));
    m.put(0x474d08u,axes+4u*pick(12));
    for(std::uint32_t at=results;at<results+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto result=shape==7?vector:shape==8?vector+4u:results+4u*pick(12);
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x110000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[1]=vector;c.abi.gp[2]=result;
    if(shape==14 && pick(2)) {                  // a stack with fewer than two free registers is refused
        c.abi.x87_depth=7u+pick(2);
        for(unsigned i=0;i<c.abi.x87_depth;++i)c.abi.x87[i]=(long double)(int(rng()%2001)-1000)/7.0L;
    }
    if(shape==15) {
        static constexpr std::uint32_t regions[]{vectors,axes,results,0x46a000u,0x474000u};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s){return perpendicular::bridge(s,[](auto &v){original::native_frame_misc_reference_11fa50(v);});}
void reference(WorldState &s){original::run_11fa50(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PERPENDICULAR");
    std::mt19937 rng(0x11fa50u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t,c.abi.x87_depth>6u))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,perpendicular::coverage().load(),unsigned(perpendicular::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
