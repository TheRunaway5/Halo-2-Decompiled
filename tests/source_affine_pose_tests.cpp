#include "halo2/model.hpp"
#include "halo2/transform.hpp"
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
void require(bool v,const char*m){if(!v)throw FormatError(m);}
std::array<std::uint32_t,13> result(const std::array<std::uint32_t,8>& words){
    auto f=[&](unsigned i){return std::bit_cast<float>(words[i]);};
    RenderModel model;ModelBone bone;bone.name=0;bone.bind={{f(4),f(5),f(6)},{f(0),f(1),f(2),f(3)},f(7)};
    model.bones.push_back(bone);
    return std::bit_cast<std::array<std::uint32_t,13>>(pose_model_node(model,0));
}
}
int main(int argc,char**argv){try{
    const bool probe=argc==2&&std::string_view(argv[1])=="--probe";
    std::ifstream file;if(!probe){file.open("tests/fixtures/retail_source_affine_pose.tsv");require(bool(file),"missing original affine pose fixture");}
    auto&input=probe?std::cin:file;unsigned count=0;std::array<std::uint32_t,8> words;
    while(input>>words[0]){for(unsigned i=1;i<8;++i)input>>words[i];require(bool(input),"truncated affine pose input");const auto actual=result(words);
        if(probe){for(unsigned i=0;i<13;++i)std::cout<<(i?" ":"")<<actual[i];std::cout<<'\n';}
        else {std::array<std::uint32_t,13> expected;for(auto&w:expected)input>>w;require(bool(input),"truncated affine pose output");require(actual==expected,"original affine pose basis/translation/scale differs");}
        ++count;
    }
    require(input.eof(),"invalid affine pose fixture");if(!probe){require(count>128,"incomplete affine pose fixture");std::cout<<count<<" original unit-quaternion affine pose fixtures passed\n";}
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
