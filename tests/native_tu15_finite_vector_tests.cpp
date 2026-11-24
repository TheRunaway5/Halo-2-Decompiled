// Native TU1.5 finite vector check (B16C0) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author vectors whose components are
// ordinary, infinite, NaN or denormal (any one, several, none); one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_finite_vector.hpp"
using namespace frame_misc_harness;
namespace vector_check=halo2::native_finite_vector_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t vectors=0x10000000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    static constexpr std::uint32_t special[]{0x7fc00000u,0x7f800000u,0xff800000u,0xffc00001u,0x00000001u,0x80000000u};
    for(std::uint32_t at=vectors;at<vectors+0x40u;at+=4)
        m.put(at,pick(5)==0?special[pick(6)]:std::bit_cast<std::uint32_t>(float(int(rng()%4001)-2000)/10.0f));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x170000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=vectors+4u*pick(12);
    if(shape==15)m.drop(vectors);
    return c;
}
bool native(WorldState &s){return vector_check::bridge(s,[](auto &v){original::native_frame_misc_reference_b16c0(v);});}
void reference(WorldState &s){original::run_b16c0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_FINITE_VECTOR");
    std::mt19937 rng(0xb16c0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,vector_check::coverage().load(),unsigned(vector_check::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
