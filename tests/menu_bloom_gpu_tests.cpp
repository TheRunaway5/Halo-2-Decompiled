#include "halo2/bloom.hpp"
#include "gpu_compute.hpp"
#include "scalar_combiners.hpp"
#include <cmath>
#include <iostream>
#include <random>
using namespace halo2;
struct Bloom {std::array<std::uint32_t,60> program;ShaderVector controls;};
struct Fixture {Bloom material;ShaderVector coordinate;std::array<ShaderVector,4> samples;};
static_assert(sizeof(Bloom)==256 && sizeof(Fixture)==336);
int main() {
 try {
    std::mt19937 random(0x214f0);std::uniform_real_distribution<float> unorm(0,1);
    std::vector<Fixture> fixtures;std::vector<ShaderVector> expected;
    for(unsigned n=0;n<1024;++n) {
        const auto bloom=menu_bloom(n?.3f:0.f,n?.375f:0.f);
        for(unsigned pass=0;pass<6;++pass) {
            const auto &program=pass==0?bloom.threshold:pass==5?bloom.composite:bloom.blur;
            Fixture f{{program.words,{pass<5?bloom.radii[pass]:0,float(pass==5?640:160),float(pass==5?480:120),0}},
                {unorm(random),unorm(random),0,0},{}};
            for(auto &s:f.samples)for(float &v:s)v=unorm(random);
            for(unsigned i=0;i<4;++i)expected.push_back({f.coordinate[0]+float(i&1?1:-1)*f.material.controls[0]*.5f/f.material.controls[1],
                f.coordinate[1]+float(i&2?1:-1)*f.material.controls[0]*.5f/f.material.controls[2],0,0});
            expected.push_back(halo2_test::scalar_combiners(program.words,f.samples));fixtures.push_back(f);
        }
    }
    halo2_test::Compute<Fixture,5> compute(HALO2_BLOOM_TEST_SHADER);const auto actual=compute.run(fixtures);
    float maximum=0;for(unsigned i=0;i<actual.size();++i)for(unsigned c=0;c<4;++c) {
        const float error=std::abs(actual[i][c]-expected[i][c]);maximum=std::max(maximum,error);
        if(!std::isfinite(actual[i][c]) || error>2e-5f)throw std::runtime_error("bloom GPU mismatch at "+std::to_string(i)+" channel "+std::to_string(c));
    }
    std::cout<<fixtures.size()<<" bloom tap/threshold/blur/composite GPU fixtures passed; maximum error "<<maximum<<'\n';
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
