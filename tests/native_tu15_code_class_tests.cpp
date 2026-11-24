// Native TU1.5 code class (DF820) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children and no memory reads. Fixtures author every listed
// code, its neighbours, and random codes (negative too). Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_code_class.hpp"
using namespace frame_misc_harness;
namespace code_class=halo2::native_code_class_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xd0000u+pick(0x100));
    random_abi(rng,c,stack);
    static constexpr std::uint32_t codes[]{0x0400000cu,0x0900000eu,0x0a00000fu,0x0800001eu,0x09000015u,0x09000016u,
        0x0a000014u,0x0a000017u,0x0d000042u};
    const auto base=codes[pick(9)];
    c.abi.gp[1]=pick(5)==0?std::uint32_t(rng()):base+std::uint32_t(int(pick(5))-2);
    return c;
}
bool native(WorldState &s){return code_class::bridge(s,[](auto &v){original::native_frame_misc_reference_df820(v);});}
void reference(WorldState &s){original::run_df820(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CODE_CLASS");
    std::mt19937 rng(0xdf820u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,code_class::coverage().load(),unsigned(code_class::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
