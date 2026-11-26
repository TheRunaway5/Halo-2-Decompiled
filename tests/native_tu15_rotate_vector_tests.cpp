#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_rotate_vector.hpp"

using namespace frame_misc_harness;
namespace rotation=halo2::native_rotate_vector_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}

namespace {
Case authored_case(std::mt19937 &rng,std::mt19937_64 &fp,unsigned n) {
    Case c;
    auto &m=c.memory;
    constexpr std::uint32_t stack=0x20011018u, matrix=0x10008000u;
    auto input=0x10004000u, output=0x10009000u;
    const auto shape=n%12u;
    switch(shape) {
    case 1:output=input;break;
    case 2:output=input+4u;break;
    case 3:output=input-4u;break;
    case 4:output=matrix+4u;break;
    case 5:input=matrix+8u;break;
    case 6:input=output=stack-16u;break;
    case 7:output=0x10009ffcu;break;
    case 8:input=0x10004ffcu;break;
    default:break;
    }
    random_abi(rng,c,stack);
    if((n/12u)%2u) {
        auto &alias=c.abi.active_mmx.emplace();
        alias.known_mask=std::uint8_t(rng());
        for(auto &reg:alias.physical)
            for(auto &byte:reg)byte=std::byte(rng()&0xffu);
        c.abi.x87_depth=0;
        c.abi.x87_status&=~0x3800u;
    }
    c.full_observation=!census_mode && (n/24u)%2u!=0;
    if(c.full_observation) {
        constexpr std::uint32_t rejection_sites[]{0x145b73u,0x145bd5u,0x145c10u};
        if((n/48u)%2u)c.reject_instruction=rejection_sites[n%std::size(rejection_sites)];
    }
    c.abi.gp[0]=output;c.abi.gp[1]=input;c.abi.gp[2]=matrix;
    for(auto address=stack-20u;address<=stack+4u;address+=4u)m.put(address,std::uint32_t(rng()));
    for(auto address=output;address<output+12u;address+=4u)m.put(address,std::uint32_t(rng()));
    const auto bits=[&](){return rng()%3u==0?std::uint32_t(rng()):special_float(fp);};
    for(auto address=input;address<input+12u;address+=4u)m.put(address,bits());
    for(auto address=matrix;address<matrix+36u;address+=4u)m.put(address,bits());
    switch(shape) {
    case 7:m.drop(output+4u);break;
    case 8:m.drop(input+4u);break;
    case 9:m.drop(matrix);break;
    case 10:m.drop(stack);break;
    case 11:m.drop(output);break;
    default:break;
    }
    return c;
}
bool native(WorldState &s){return rotation::bridge(s,original::native_frame_misc_reference_145b70);}
}

int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ROTATE_VECTOR");
    std::mt19937 rng(0x145b70u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    std::mt19937_64 fp(0x145c10u);
    Totals totals;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,authored_case(rng,fp,n),native,original::run_145b70,totals,false,{},true))return 1;
    auto c=authored_case(rng,fp,0);
    SourcePhysicsContactRecordObserver observe;
    SourcePhysicsWorldRuntimeServices services;
    WorldState other_image{c.memory,observe,services,SourcePhysicsContactImage::MapPack11};
    other_image.load(c.abi);
    if(native(other_image))throw std::runtime_error("MapPack11 admitted");
    report(cases,totals,rotation::coverage().load(),unsigned(rotation::branch_count),mode);
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
