#include "halo2/atmosphere.hpp"
#include "gpu_compute.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
namespace {
using namespace halo2;
struct Fog {std::array<std::uint32_t,60> program;std::array<ShaderVector,2> lookup;};
struct Fixture {Fog material;ShaderVector depth,scene;};
struct CapturedPixel {std::array<std::uint8_t,4> before,after;std::uint32_t depth;};
static_assert(sizeof(Fixture)==304 && sizeof(CapturedPixel)==12);
}
int main(int argc,char **argv) {
    try {
        if(argc!=3)throw std::runtime_error("menu map and captured eye-fog pixels required");
        const auto atmosphere=Cache(argv[1]).load_menu_atmosphere();
        // c7 of the original Vulkan frame, before event 1872. Captured
        // eye-fog constant is ARGB 0x41201B16; no texture stages are enabled.
        const auto fog=menu_fog_draw(atmosphere,{-2.78095865f,-34.3112793f,219.911346f});
        if(!fog)throw std::runtime_error("captured eye-fog pass was skipped");
        std::array<std::uint32_t,60> captured{};
        captured[8]=0x01200000;captured[9]=0x1100;captured[43]=0x41201b16;captured[53]=0x11001;
        if(fog->program.words!=captured)throw std::runtime_error("selected fog program differs from original capture");
        std::ifstream file(argv[2],std::ios::binary|std::ios::ate);
        if(!file || file.tellg()!=4096*sizeof(CapturedPixel))throw std::runtime_error("eye-fog fixture size differs");
        std::vector<CapturedPixel> original(4096);file.seekg(0);
        file.read(reinterpret_cast<char*>(original.data()),original.size()*sizeof(CapturedPixel));
        std::vector<Fixture> fixtures;unsigned covered=0;
        for(const auto &p:original) {
            // This captured D24S8 surface returns depth in the low 24 bits
            // of each u32. The packed raw-color alias was checked over the
            // complete frame by export_title_haze_fixture.py.
            Fixture f{{fog->program.words,fog->lookup_coordinates},{float(p.depth&0xffffffu)/16777215.f,0,0,0},{}};
            for(unsigned c=0;c<4;++c)f.scene[c]=float(p.before[c])/255.f;
            if(p.depth==0xffffffffu)++covered;
            fixtures.push_back(f);
        }
        halo2_test::Compute<Fixture,1> compute(HALO2_EYE_FOG_TEST_SHADER);
        const auto result=compute.run(fixtures);unsigned maximum=0;
        for(unsigned i=0;i<result.size();++i)for(unsigned c=0;c<3;++c) {
            if(!std::isfinite(result[i][c]))throw std::runtime_error("nonfinite eye-fog pixel");
            const auto byte=unsigned(std::lround(std::clamp(result[i][c],0.f,1.f)*255));
            const auto error=unsigned(std::abs(int(byte)-int(original[i].after[c])));
            maximum=std::max(maximum,error);
            if(error>1)throw std::runtime_error("eye-fog capture mismatch at pixel "+std::to_string(i));
            if(original[i].depth!=0xffffffffu && byte!=original[i].before[c])
                throw std::runtime_error("eye-fog changed a geometry pixel");
        }
        if(covered!=1575)throw std::runtime_error("captured coverage differs");
        std::cout<<result.size()<<" original eye-fog pixels passed, including "<<covered
                 <<" background pixels; maximum error "<<maximum<<" byte\n";
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
