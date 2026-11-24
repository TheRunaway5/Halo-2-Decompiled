// Native TU1.5 basis matrix (144BF0) against the unmodified translated body
// (the AI frame unit's copy; the validator repeats it with the scene device
// constructor unit's), extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). 144BF0 has no children. Fixtures author axes
// and positions (special floats, distinct NaN payloads, operand-order
// sensitive values), the global 1.0, and matrices that overlap the inputs;
// one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_basis_matrix.hpp"
using namespace frame_misc_harness;
namespace basis=halo2::native_basis_matrix_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t vectors=0x10000000u,matrices=0x10010000u;
std::uint32_t quiet_nan(std::mt19937 &rng) {
    static constexpr std::uint32_t payloads[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7fc12345u,0xffc54321u,0xff800003u};
    return payloads[rng()%6];
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    const auto value=[&]() {
        if(shape==4 && pick(4)==0)return quiet_nan(rng);
        if(shape==3 && pick(4)==0) {
            static constexpr std::uint32_t values[]{0x7f800000u,0xff800000u,0x00000001u,0x80000000u,0x00000000u,0x7f7fffffu};
            return values[pick(6)];
        }
        return std::bit_cast<std::uint32_t>(float(int(pick(2001))-1000)/250.0f);
    };
    for(std::uint32_t at=vectors;at<vectors+0x40u;at+=4)m.put(at,value());
    for(std::uint32_t at=matrices;at<matrices+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(0x46a0d0u,pick(8)?std::bit_cast<std::uint32_t>(1.0f):value());
    const auto forward=vectors+4u*pick(4),up=vectors+0x10u+4u*pick(4),position=vectors+0x20u+4u*pick(4);
    const auto target=shape==9?vectors+4u*pick(4):matrices+4u*pick(8);     // overlapping the inputs
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x145000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[1]=forward;c.abi.gp[0]=up;c.abi.gp[6]=position;c.abi.gp[2]=target;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{vectors,matrices,0x46a000u};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s){return basis::bridge(s,[](auto &v){original::native_frame_misc_reference_144bf0(v);});}
void reference(WorldState &s){original::run_144bf0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_BASIS_MATRIX");
    std::mt19937 rng(0x144bf0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,basis::coverage().load(),unsigned(basis::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
