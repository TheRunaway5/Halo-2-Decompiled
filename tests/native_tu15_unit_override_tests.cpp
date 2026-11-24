// Native TU1.5 unit_override query (e3310) against the unmodified translated body of its
// unit, extracted by tools/validate_native_frame_misc_tu15.py into body.inc,
// plus the verification reference the production binder generates
// (reference.inc). There are no children. Fixtures author the globals and
// records the query reads across every outcome; one region removed injects
// a fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_unit_override.hpp"
using namespace frame_misc_harness;
namespace query=halo2::native_unit_override_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t region=0x10000000u,headers=0x10100000u,header_data=0x10110000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=region;at<region+0x2000u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(0x4ecbd0u,headers);for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));m.put(headers+0x44u,header_data);
    for(std::uint32_t k=0;k<4;++k){const auto unit=region+0x400u*k;m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));m.put(header_data+12u*k+8u,unit);
        m.put(unit+0x3dcu,pick(2)?6u:pick(8),1);m.put(unit+0x3f8u,pick(3)?1u+pick(255):0u,1);}
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x100000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15)m.drop(pick(2)?region:region+0x1000u);
    return c;
}
bool native(WorldState &s){return query::bridge(s,[](auto &v){original::native_frame_misc_reference_e3310(v);});}
void reference(WorldState &s){original::run_e3310(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SMALL_QUERIES");
    std::mt19937 rng(0xe3310u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,query::coverage().load(),unsigned(query::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
