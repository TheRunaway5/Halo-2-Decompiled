#include "halo2/cache.hpp"
#include "gpu_compute.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
using namespace halo2;
struct Bloom {std::array<std::uint32_t,60> program;ShaderVector controls;};
struct Fixture {Bloom material;ShaderVector coordinate;std::array<ShaderVector,4> samples;};
static_assert(sizeof(Fixture)==336);
int main(int argc,char **argv) {
 try {
    if(argc!=3)throw std::runtime_error("expected original map and captured pixel fixture");
    const auto program=Cache(argv[1]).load_menu_scene_filter();
    // Captured shader 12869, event 8132, original Xemu Vulkan title frame.
    std::array<std::uint32_t,60> original{};
    original[8]=0x0c010000;original[9]=0x80;original[10]=0x0080b333;
    original[34]=0x08010000;original[43]=0x001a2340;original[45]=0x20c0;
    original[53]=0x11101;original[54]=1;
    if(program.words!=original)throw std::runtime_error("authored title filter differs from original GPU program");
    std::ifstream file(argv[2],std::ios::binary);
    if(!file)throw std::runtime_error("missing original GPU pixel fixture");
    std::vector<Fixture> fixtures;std::vector<std::array<unsigned char,4>> expected;
    std::array<unsigned char,8> pixel;
    while(file.read(reinterpret_cast<char*>(pixel.data()),pixel.size())) {
        Fixture f{{program.words,{0,640,480,0}},{.5f,.5f,0,0},{}};
        for(auto &sample:f.samples)for(unsigned c=0;c<4;++c)sample[c]=pixel[c]/255.f;
        fixtures.push_back(f);expected.push_back({pixel[4],pixel[5],pixel[6],pixel[7]});
    }
    if(fixtures.size()!=4096 || file.gcount()!=0)throw std::runtime_error("invalid original GPU pixel fixture");
    halo2_test::Compute<Fixture,5> compute(HALO2_BLOOM_TEST_SHADER);
    const auto actual=compute.run(fixtures);
    unsigned maximum=0;
    for(unsigned i=0;i<fixtures.size();++i)for(unsigned c=0;c<4;++c) {
        const auto value=actual[i*5+4][c];
        if(!std::isfinite(value))throw std::runtime_error("nonfinite title filter result");
        const int byte=int(std::nearbyint(std::clamp(value,0.f,1.f)*255.f));
        const unsigned error=unsigned(std::abs(byte-int(expected[i][c])));
        maximum=std::max(maximum,error);
        if(error>1)throw std::runtime_error("title filter differs from original at pixel "+std::to_string(i));
    }
    std::cout<<fixtures.size()<<" original Vulkan title-filter pixels passed; maximum 8-bit error "<<maximum<<'\n';
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
