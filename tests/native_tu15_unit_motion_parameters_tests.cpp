// 1EE3D0 derives the unit motion record. Compare its translated body and
// native owner while exercising both copy directions, aliased records, all
// motion gates, finite and exceptional arithmetic, and the two exits.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_unit_motion_parameters.hpp"

using namespace frame_misc_harness;
namespace owner=halo2::native_unit_motion_parameters_detail;

namespace original {
using namespace halo2::physics_world_runtime_detail;
#include "body.inc"
#include "reference.inc"
}

namespace {
constexpr std::uint32_t input=0x10001000u,output=0x10003000u,descriptor=0x10005000u;
constexpr std::uint32_t globals=0x10007000u,tuning=0x10009000u,stack=0x20002080u;

Case author(std::mt19937 &random,unsigned index) {
    Case test;
    auto &memory=test.memory;
    random_abi(random,test,stack);
    const auto shape=index%24u;
    const auto source=shape==17u?output:(shape==18u?output+4u:input);
    test.abi.gp[0]=output;
    test.abi.gp[2]=source;
    const auto word=[&](std::uint32_t address,float value){memory.put(address,u(value));};
    for(const auto address:{input,output,globals,tuning})
        for(std::uint32_t offset=0u;offset<0x1a0u;offset+=4u)
            memory.put(address+offset,std::uint32_t(random()));
    for(std::uint32_t offset=0u;offset<0x40u;offset+=4u)
        memory.put(stack-0x20u+offset,std::uint32_t(random()));

    memory.put(output+8u,descriptor);
    memory.put(output+0x14u,shape==3u||shape==7u?4u:shape==4u?8u:0u);
    memory.put(descriptor,shape<2u||shape==4u?0u:4u);
    memory.put(stack+4u,shape==2u||shape==3u?1u:0u);
    memory.put(stack+8u,shape==9u||shape==10u?0x80u:0u);
    memory.put(stack+12u,std::uint32_t(random())|0x100u);
    word(stack+16u,0.25f);

    const float direction_x=(shape==6u||shape==8u||shape==10u)?-0.5f:
        shape==7u?2.0f:shape==14u?-2.25f:0.5f;
    word(output+0xdcu,direction_x);
    word(output+0xe0u,shape==7u?1.0f:-0.25f);
    word(output+0xe4u,0.25f);
    memory.put(0x4ecc1cu,globals);
    memory.put(globals+0x134u,tuning);
    word(0x45695cu,0.125f);
    word(0x46a0d0u,1.0f);
    for(std::uint32_t offset=0x24u;offset<=0x4cu;offset+=4u)
        word(tuning+offset,float(offset)*0.125f);

    if(shape==11u) {
        memory.put(output+0xdcu,0x7fc01234u);
        memory.put(output+0xe0u,0xffc05678u);
        memory.put(tuning+0x30u,0x7fc09876u);
        memory.put(stack+16u,0xffc0abcdu);
    }
    if(shape==12u) {
        memory.put(output+0xdcu,0x7f800000u);
        memory.put(output+0xe0u,0xff800000u);
    }
    if(shape==13u) {
        memory.put(output+0xdcu,0x7f800001u);
        memory.put(output+0xe0u,1u);
    }
    if(shape==15u||shape==16u) {
        std::mt19937_64 floats(random());
        for(std::uint32_t offset=0x24u;offset<=0x4cu;offset+=4u)
            memory.put(tuning+offset,special_float(floats));
        memory.put(stack+16u,special_float(floats));
    }
    if(shape==19u)memory.drop(input);
    if(shape==20u)memory.drop(descriptor);
    if(shape==21u)memory.drop(globals);
    if(shape==22u)memory.drop(tuning);
    if(shape==23u)memory.drop(0x46a0d0u);
    return test;
}

bool native(WorldState &state) {
    return owner::bridge(state,[](auto &value){original::native_frame_misc_reference_1ee3d0(value);});
}
void reference(WorldState &state){original::run_1ee3d0(state);}
}

int main(int argc,char **argv)try {
    const auto count=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_UNIT_MOTION_PARAMETERS");
    std::mt19937 random(0x1ee3d0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals totals;
    for(unsigned index=0;index<count;++index)
        if(!compare(index,author(random,index),native,reference,totals))return 1;
    if(!census_mode)check_refusals(author(random,0u),native);
    report(count,totals,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &error){std::fprintf(stderr,"%s\n",error.what());return 1;}
