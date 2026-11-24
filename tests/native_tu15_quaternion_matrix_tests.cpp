// Native TU1.5 quaternion matrix (144910) against the unmodified translated
// body of the scene device constructor unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// A leaf: fixtures author unit and scaled quaternions, near-zero ones under
// and over the least norm, denormals, infinities and NaNs, a matrix that
// overlaps the quaternion, and one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/renderer/native_tu15_quaternion_matrix.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_quaternion_matrix_detail;
namespace {
constexpr std::uint32_t quaternions=0x10000000u,matrices=0x10010000u;
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    // A least norm of 1 half the time: unit quaternions then sit exactly on it.
    m.put(0x46a0ecu,pick(2)?bits_of(1e-4f):bits_of(1.f));m.put(0x46a11cu,bits_of(2.f));m.put(0x46a0d0u,bits_of(1.f));
    static constexpr std::uint32_t specials[]{0x7f800000u,0xff800000u,0x7fc00000u,0x00000001u,0x80000000u,0x00400000u};
    for(std::uint32_t at=quaternions;at<quaternions+0x40u;at+=4) {
        const auto r=pick(16);
        m.put(at,r<4?bits_of(float(int(pick(3))-1)):r<10?bits_of(float(int(pick(201))-100)*0.01f):r<13?bits_of(float(int(pick(21))-10)*0.001f):specials[pick(6)]);
    }
    for(std::uint32_t at=matrices;at<matrices+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[1]=quaternions+0x10u*pick(4);
    c.abi.gp[0]=pick(12)?matrices+4u*pick(8):c.abi.gp[1]-4u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{quaternions,matrices};
        m.drop(regions[pick(2)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_144910(v);});}
void reference(WorldState &s){original::run_144910(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_QUATERNION_MATRIX");
    std::mt19937 rng(0x144910u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
