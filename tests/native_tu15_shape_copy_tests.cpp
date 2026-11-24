// Native TU1.5 shape copy (C3960) against the unmodified translated bodies
// of the light shape, death cycle, shadow frame and map world support units
// (identical), extracted by tools/validate_native_frame_misc_tu15.py into
// body.inc, plus the verification reference the production binder generates
// (reference.inc). There are no children. Fixtures author kinds 0 and others
// (negative too) and both clear directions; one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "pstencil/native_tu15_shape_copy.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_shape_copy_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t sources=0x10000000u,records=0x10010000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    const bool backward=shape==4 || shape==9;
    for(std::uint32_t at=sources;at<sources+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(sources+4u,pick(2)?0u:pick(2)?pick(8):0x8000u+pick(8),2);
    for(std::uint32_t at=records;at<records+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc30000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=sources+4u*pick(4);c.abi.gp[2]=records+(backward?0x100u:4u*pick(16));
    if(backward)c.abi.eflags|=0x400u;else c.abi.eflags&=~0x400u;
    if(shape==15)m.drop(pick(2)?sources:records);
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_c3960(v);});}
void reference(WorldState &s){original::run_c3960(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SHAPE_COPY");
    std::mt19937 rng(0xc3960u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
