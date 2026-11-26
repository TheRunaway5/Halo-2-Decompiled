// Compare the 2E4350 Havok rotation conversion against its source body. The
// fixture preserves the original word-strided matrix and tests all dominant
// axes, overlap, FP modes, unordered compares, and memory faults.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_rotation_quaternion.hpp"

using namespace frame_misc_harness;
namespace owner=halo2::native_rotation_quaternion_detail;

namespace original {
using namespace halo2::physics_world_runtime_detail;
#include "body.inc"
#include "reference.inc"
}

namespace {
constexpr std::uint32_t input=0x10001000u,output=0x10003000u, stack=0x20002080u;
constexpr std::uint32_t matrix_slot(std::uint32_t matrix,unsigned row,unsigned column) {
    return matrix+16u*column+4u*row;
}

Case author(std::mt19937 &random,unsigned index) {
    Case test;
    auto &memory=test.memory;
    random_abi(random,test,stack);
    const auto shape=index%24u;
    const auto source=shape==17u?stack-20u:input;
    auto target=output;
    if(shape==13u||shape==14u)target=source;
    if(shape==15u)target=source+4u;
    if(shape==16u)target=stack-12u;
    test.abi.gp[1]=target;
    for(const auto address:{source,target,stack})
        for(std::uint32_t offset=0u;offset<0x80u;offset+=4u)
            memory.put(address-0x30u+offset,std::uint32_t(random()));

    const auto set_element=[&](unsigned row,unsigned column,float value) {
        memory.put(matrix_slot(source,row,column),u(value));
    };
    for(unsigned row=0u;row<3u;++row)
        for(unsigned column=0u;column<3u;++column)
            set_element(row,column,float(int(random()%1024u)-512)/1024.0f);

    if(shape==0u||shape==1u||shape==13u) {
        set_element(0u,0u,1.0f);
        set_element(1u,1u,1.0f);
        set_element(2u,2u,1.0f);
    }
    if((shape>=2u&&shape<=4u)||shape==14u||shape==16u) {
        const auto major=shape>=14u?1u:shape-2u;
        for(unsigned axis=0u;axis<3u;++axis)set_element(axis,axis,axis==major?1.0f:-1.0f);
    }
    if(shape==5u||shape==6u||shape==7u)
        for(unsigned axis=0u;axis<3u;++axis)set_element(axis,axis,shape==5u?0.0f:shape==6u?-0.0f:-1.0f);
    if(shape==8u) {
        memory.put(source,0x7fc12345u);
        memory.put(source+0x14u,0xffc00019u);
    }
    if(shape==9u) {
        memory.put(source,0x7f800001u);
        memory.put(source+0x14u,0x00000001u);
    }
    if(shape==10u) {
        memory.put(source,0x7f800000u);
        memory.put(source+0x14u,0xff800000u);
    }
    if(shape==11u||shape==12u) {
        std::mt19937_64 floats(random());
        for(unsigned row=0u;row<3u;++row)
            for(unsigned column=0u;column<3u;++column)
                memory.put(matrix_slot(source,row,column),special_float(floats));
    }
    memory.put(stack+4u,source);
    memory.put(0x46a0e8u,0u);
    memory.put(0x46a0d0u,u(1.0f));
    memory.put(0x46a0ccu,u(0.5f));
    if(shape==19u)memory.drop(source);
    if(shape==20u)memory.drop(target);
    if(shape==21u)memory.drop(0x46a0e8u);
    if(shape==22u)memory.drop(stack);
    if(shape==23u) {
        test.abi.gp[1]=output+0xff8u;
        memory.put(output+0xff8u,0u);
        memory.put(output+0xffcu,0u);
    }
    return test;
}

bool native(WorldState &state) {
    return owner::bridge(state,[](auto &value){original::native_frame_misc_reference_2e4350(value);});
}
void reference(WorldState &state){original::run_2e4350(state);}
}

int main(int argc,char **argv)try {
    const auto count=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ROTATION_QUATERNION");
    std::mt19937 random(0x2e4350u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals totals;
    for(unsigned index=0;index<count;++index)
        if(!compare(index,author(random,index),native,reference,totals))return 1;
    if(!census_mode)check_refusals(author(random,0u),native);
    report(count,totals,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &error){std::fprintf(stderr,"%s\n",error.what());return 1;}
