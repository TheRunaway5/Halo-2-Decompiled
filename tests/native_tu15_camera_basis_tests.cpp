// E05D0 against the canonical TU1.5 generated body. Each run compares the
// caller ABI, floating point state, stores, and memory, including overlapped
// vectors and the slot that holds the measured length.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_camera_basis.hpp"

using namespace frame_misc_harness;
namespace owner=halo2::native_camera_basis_detail;

namespace original {
using namespace halo2::physics_world_runtime_detail;
#include "body.inc"
#include "reference.inc"
}

namespace {
constexpr std::uint32_t direction=0x10001000u,fallback=0x10003000u;
constexpr std::uint32_t forward=0x10005000u,perpendicular=0x10007000u,default_vector=0x10009000u;
constexpr std::uint32_t stack=0x20002080u;

Case author(std::mt19937 &random,unsigned index) {
    Case test;
    auto &memory=test.memory;
    random_abi(random,test,stack);
    const auto shape=index%20u;

    for(const auto address:{direction,fallback,forward,perpendicular,default_vector,stack})
        for(std::uint32_t offset=0;offset<48u;offset+=4u)
            memory.put(address-16u+offset,std::uint32_t(random()));
    test.abi.gp[1]=forward;
    test.abi.gp[2]=direction;
    test.abi.gp[6]=fallback;
    test.abi.gp[7]=perpendicular;

    memory.put(0x474cd8u,default_vector);
    memory.put(0x46a0d0u,u(1.0f));
    memory.put(0x46a0ecu,u(0.001f));
    memory.put(default_vector,u(0.25f));
    memory.put(default_vector+4u,u(-0.75f));
    memory.put(direction,u(1.25f));
    memory.put(direction+4u,u(-2.5f));
    memory.put(direction+8u,shape%2u?u(0.5f):u(-0.5f));
    memory.put(fallback,u(0.6f));
    memory.put(fallback+4u,u(0.8f));

    // Both ordinary fallback paths, including the default vector path.
    if(shape>=1u&&shape<=6u) {
        memory.put(direction,0u);
        memory.put(direction+4u,u(-0.0f));
    }
    if(shape==3u||shape==4u) {
        memory.put(fallback,0u);
        memory.put(fallback+4u,0u);
    }
    // A zero epsilon reaches the explicit post-scale ordered-zero gate.
    if(shape==5u||shape==6u)memory.put(0x46a0ecu,0u);
    if(shape==6u) {
        memory.put(fallback,0u);
        memory.put(fallback+4u,0u);
    }
    if(shape>=7u&&shape<=10u) {
        std::mt19937_64 floats(random());
        memory.put(direction,special_float(floats));
        memory.put(direction+4u,shape==7u?special_float(floats):std::uint32_t(random()));
        memory.put(direction+8u,special_float(floats));
        memory.put(fallback,special_float(floats));
        memory.put(fallback+4u,special_float(floats));
    }
    // Distinct NaNs make the normalized X word depend on the MULSS destination.
    if(index%80u==7u) {
        memory.put(direction,0xffc00000u);
        memory.put(direction+4u,0x7f800001u);
    }
    if(shape==11u)test.abi.gp[1]=direction;
    if(shape==12u)test.abi.gp[1]=direction+4u;
    if(shape==13u)test.abi.gp[1]=fallback;
    if(shape==14u)test.abi.gp[7]=forward;
    if(shape==15u)test.abi.gp[7]=forward+4u;
    if(shape==16u)test.abi.gp[1]=stack-4u;
    if(shape==17u)memory.drop(direction);
    if(shape==18u)memory.drop(perpendicular);
    if(shape==19u)memory.drop(0x46a0ecu);
    return test;
}

bool native(WorldState &state) {
    return owner::bridge(state,[](auto &value){original::native_frame_misc_reference_e05d0(value);});
}
void reference(WorldState &state){original::run_e05d0(state);}
}

int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CAMERA_BASIS");
    std::mt19937 random(0xe05d0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals totals;
    for(unsigned index=0;index<cases;++index)
        if(!compare(index,author(random,index),native,reference,totals))return 1;
    if(!census_mode)check_refusals(author(random,0u),native);
    report(cases,totals,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &error){std::fprintf(stderr,"%s\n",error.what());return 1;}
