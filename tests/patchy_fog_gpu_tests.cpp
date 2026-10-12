#include "halo2/atmosphere.hpp"
#include "gpu_compute.hpp"
#include "scalar_combiners.hpp"
#include <cmath>
#include <iostream>
#include <random>
namespace {
using namespace halo2;
struct Patchy {std::array<std::uint32_t,60> program;std::array<ShaderVector,8> rows;ShaderVector controls,lookup;};
struct Fixture {Patchy material;std::array<std::uint32_t,60> composite;ShaderVector coordinate;
    std::array<ShaderVector,4> samples;std::array<ShaderVector,256> gradient;};
static_assert(sizeof(Patchy)==400 && sizeof(Fixture)==4816);
ShaderVector gradient(const Fixture &f,float uv) {
    const float p=std::clamp(uv,0.f,1.f)*16-.5f;const int a=int(std::floor(p));const float t=p-a;
    auto at=[&](int x,int y)->const ShaderVector& {return f.gradient[std::clamp(y,0,15)*16+std::clamp(x,0,15)];};
    ShaderVector out{};for(unsigned c=0;c<4;++c)out[c]=(at(a,a)[c]*(1-t)+at(a+1,a)[c]*t)*(1-t)+(at(a,a+1)[c]*(1-t)+at(a+1,a+1)[c]*t)*t;
    return out;
}
}
int main(int argc,char **argv) {
 try {
    if(argc!=2)throw std::runtime_error("maps directory required");
    const auto atmosphere=Cache(std::filesystem::path(argv[1])/"mainmenu.map").load_menu_atmosphere();
    const auto &fog=atmosphere.patchy.value();PatchyFogState state;std::uint32_t seed=0x27aa0;
    std::mt19937 random(0x26880);std::uniform_real_distribution<float> unorm(0,1);
    std::vector<Fixture> fixtures;std::vector<ShaderVector> expected;
    for(unsigned frame=0;frame<12;++frame) {
        Transform view;view.position={float(frame)*.2f,float(frame)*.3f,0.f-float(frame)*.6f};
        const auto patch=patchy_fog_frame(fog,state,seed,view,float(frame)/6-1,1.f,3.f/4,float(frame)/30);
        for(unsigned depth=0;depth<=1024;depth+=4) {
            const float range=1/(fog.distance[1]-fog.distance[0]);
            Fixture f{{patch.noise.words,patch.rows,{patch.vertical,480,360,0},{1024*range,0,0.f-fog.distance[0]*range,0}},
                patch.direct.words,{unorm(random),unorm(random),float(depth)/1024,0},{},{}};
            for(auto &s:f.samples)for(float &v:s)v=unorm(random);
            for(unsigned i=0;i<256;++i)for(unsigned c=0;c<4;++c)f.gradient[i][c]=float(std::to_integer<unsigned>(atmosphere.gradient.pixels[i*4+c]))/255;
            const float x=f.coordinate[0]*2-1,y=f.coordinate[1]*2-1;
            for(unsigned i=0;i<4;++i) {
                const auto &r=patch.rows[i*2],&s=patch.rows[i*2+1];
                expected.push_back({x*r[0]-y*r[1]*patch.vertical+r[3],x*s[0]-y*s[1]*patch.vertical+s[3],0,0});
            }
            const auto noise=halo2_test::scalar_combiners(patch.noise.words,f.samples);expected.push_back(noise);
            ShaderVector target{};for(unsigned c=0;c<4;++c)target[c]=float(std::nearbyint(double(noise[c]*255)))/255;
            const auto z=std::uint32_t(std::round(f.coordinate[2]*16777215));
            const float coordinate=f.material.lookup[0]*(float(z>>8)/65535)+f.material.lookup[2];
            const std::array<ShaderVector,4> samples{{{float((z>>8)&255)/255,float(z&255)/255,0,float(z>>16)/255},{},gradient(f,coordinate),target}};
            expected.push_back(halo2_test::scalar_combiners(patch.direct.words,samples));fixtures.push_back(f);
        }
    }
    halo2_test::Compute<Fixture,6> compute(HALO2_PATCHY_TEST_SHADER);const auto actual=compute.run(fixtures);
    float maximum=0;for(unsigned i=0;i<actual.size();++i)for(unsigned c=0;c<4;++c) {
        const float error=std::abs(actual[i][c]-expected[i][c]);maximum=std::max(maximum,error);
        if(!std::isfinite(actual[i][c]) || error>2e-5f)throw std::runtime_error("patchy fog GPU mismatch at "+std::to_string(i)+" channel "+std::to_string(c));
    }
    std::cout<<fixtures.size()<<" patchy noise/coordinates/depth/composite GPU fixtures passed; maximum error "<<maximum<<'\n';
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
