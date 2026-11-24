// Native TU1.5 world bounds (1751B0) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author points whose coordinates lie
// inside, on and outside the limits, special floats, infinities and NaN
// payloads, limits that are themselves special, and every MXCSR mode; one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_world_bounds.hpp"
using namespace frame_misc_harness;
namespace bounds=halo2::native_world_bounds_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t points=0x10000000u;
std::uint32_t coordinate(std::mt19937 &rng,unsigned shape) {
    static constexpr std::uint32_t special[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7f800000u,0xff800000u,0x00000001u,
        0x80000000u,0x00400000u};
    if((shape==3 || shape==4) && rng()%4==0)return special[rng()%8];
    static constexpr float values[]{0.0f,100.0f,-100.0f,5000.0f,-5000.0f,4999.0f,-4999.0f,5001.0f,-5001.0f,1.0e6f,-1.0e6f};
    return std::bit_cast<std::uint32_t>(values[rng()%11]);
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=points;at<points+0x100u;at+=4)m.put(at,pick(3)?std::bit_cast<std::uint32_t>(float(int(pick(200))-100)):coordinate(rng,shape));
    m.put(0x44ea9cu,pick(8)?std::bit_cast<std::uint32_t>(-5000.0f):coordinate(rng,3));
    m.put(0x44eaa0u,pick(8)?std::bit_cast<std::uint32_t>(5000.0f):coordinate(rng,3));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x170000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=points+4u*pick(60);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{points,0x44e000u};
        m.drop(regions[pick(2)]);
    }
    return c;
}
bool native(WorldState &s){return bounds::bridge(s,[](auto &v){original::native_frame_misc_reference_1751b0(v);});}
void reference(WorldState &s){original::run_1751b0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_WORLD_BOUNDS");
    std::mt19937 rng(0x1751b0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,bounds::coverage().load(),unsigned(bounds::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
