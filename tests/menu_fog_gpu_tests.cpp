#include "halo2/atmosphere.hpp"
#include "halo2/frontend_material.hpp"
#include "halo2/model.hpp"
#include "halo2/render_view.hpp"
#include "halo2/vertex_program.hpp"
#include "gpu_compute.hpp"
#include "scalar_combiners.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
using namespace halo2;
struct MaterialUniform {
    std::array<std::uint32_t,60> program{};
    std::array<ShaderVector,3> transforms{};
    std::array<std::uint32_t,4> flags{};
    std::array<ShaderVector,8> forward_transforms{};
    ShaderVector diffuse{},specular{};
    std::array<VertexInstruction,136> vertex_instructions{};
    std::array<ShaderVector,16> vertex_constants{},vertex_attributes{};
    std::array<std::uint32_t,4> alpha_test{};
    std::array<std::array<std::uint32_t,4>,4> texture_effects{};
    std::array<ShaderVector,6> fog_constants{};
};
static_assert(sizeof(MaterialUniform)==3328 && offsetof(MaterialUniform,fog_constants)==3232);
struct Fixture {
    MaterialUniform material;
    std::array<ShaderVector,7> camera{};
    std::array<ShaderVector,7> vertex{};
    std::array<ShaderVector,512> gradients{};
};
static_assert(sizeof(Fixture)==11744);
}
int main(int argc,char **argv) {
    try {
        if(argc!=2)throw std::runtime_error("maps directory required");
        Cache cache(std::filesystem::path(argv[1])/"mainmenu.map");
        auto mesh=cache.load_bsp(0);prepare_frontend_materials(cache,mesh);
        auto carrier=cache.load_model(0xe1d50060,true);
        prepare_frontend_materials(cache,carrier.mesh);
        std::vector<const Material*> beams;
        for(const auto &m:mesh.materials)if(m.name.ends_with("light_long_volume_noanim")){beams.push_back(&m);break;}
        for(const auto &m:carrier.mesh.materials)
            if(m.name.ends_with("\\antigrav_effects") || m.name.ends_with("\\antigrav_effects2"))beams.push_back(&m);
        if(beams.size()!=3)throw std::runtime_error("original title beam layers missing");
        prepare_menu_bsp_fog(cache,mesh);
        const auto planar=std::find_if(mesh.materials.begin(),mesh.materials.end(),[](const auto &m){return bool(m.planar_fog_pass);});
        if(planar==mesh.materials.end())throw std::runtime_error("original title planar fog layer missing");
        beams.push_back(planar->planar_fog_pass.get());
        const auto atmosphere=cache.load_menu_atmosphere();
        std::vector<Fixture> fixtures;
        std::vector<std::array<ShaderVector,16>> expected;
        bool attenuation=false;
        for(const auto *beam:beams) {
        const auto program=decode_vertex_program(beam->vertex_program);
        for(float eye_z:{210.f,215.82756f,221.82756f,225.f,240.f})
        for(float depth:{0.f,3.f,6.f,30.f,75.f,150.f,300.f})
        for(float vertex_z:{210.f,218.f,225.f,240.f})
        for(bool enabled:{false,true}) {
            Fixture f;
            f.material.vertex_instructions=program.instructions;
            f.material.flags[3]=program.count;
            f.material.flags[2]=beam->forward_vertex;
            f.material.vertex_constants=beam->program_constants[1];
            f.material.vertex_attributes=beam->program_constants[0];
            f.material.fog_constants=menu_fog_constants(atmosphere,{0,0,eye_z},{1,0,0});
            f.camera={ShaderVector{1,0,0,0},ShaderVector{0,-1,0,0},
                ShaderVector{0,0,1,0},ShaderVector{0,0,0,1},
                ShaderVector{1,0,0,enabled?1.f:0.f},ShaderVector{0,0,eye_z,1},
                ShaderVector{0,0,640,480}};
            f.vertex={ShaderVector{depth,0,vertex_z,1},ShaderVector{.25f,.5f,0,1},
                ShaderVector{-1,0,0,1},ShaderVector{0,1,0,1},ShaderVector{0,0,1,1},
                ShaderVector{0,0,0,1},ShaderVector{0,0,0,1}};
            VertexProgramState cpu;
            cpu.constants[0]={1,0,0,0};cpu.constants[1]={0,1,0,0};
            cpu.constants[2]={0,0,1,0};cpu.constants[3]={0,0,0,1};
            cpu.constants[4]={1,0,0,1};cpu.constants[5]={0,1,0,.5f};
            // 2FD90/12FA0 bind the camera's backward vector at c6.
            cpu.constants[6]={-1,0,0,2};cpu.constants[7]={0,0,eye_z,767.8125f};
            const auto viewport=viewport_constants({0,0,640,480,1,false,false,0});
            for(unsigned i=0;i<4;++i)cpu.constants[8+i]=viewport[i];
            if(enabled)for(unsigned i=0;i<6;++i)cpu.constants[12+i]=f.material.fog_constants[i];
            for(unsigned i=0;i<16;++i) {
                cpu.constants[18+i]=f.material.vertex_constants[i];
                cpu.attributes[i]=f.material.vertex_attributes[i];
            }
            cpu.constants[172]={1,1,0,0};
            for(unsigned i=0;i<7;++i)cpu.attributes[i]=f.vertex[i];
            execute_vertex_program(program,cpu);
            if(beam->forward_vertex==5) {
                f.material.program=menu_planar_fog_program(beam->passes.at(0),atmosphere,{0,0,eye_z}).words;
                const Texture *images[]{&beam->diffuse,&beam->detail};
                for(unsigned s=0;s<2;++s)for(unsigned i=0;i<256;++i)for(unsigned c=0;c<4;++c)
                    f.gradients[s*256+i][c]=float(std::to_integer<unsigned>(images[s]->pixels[i*4+c]))/255;
                std::array<ShaderVector,4> samples{{{0,0,0,1},{0,0,0,1},{0,0,0,1},{0,0,0,1}}};
                for(unsigned s=0;s<2;++s) {
                    const auto coordinate=cpu.outputs[9+s];
                    const float x=std::clamp(coordinate[0]/coordinate[3],0.f,1.f)*16-.5f,
                                y=std::clamp(coordinate[1]/coordinate[3],0.f,1.f)*16-.5f;
                    const int ax=int(std::floor(x)),ay=int(std::floor(y));const float fx=x-ax,fy=y-ay;
                    const auto texel=[&](int x,int y){return f.gradients[s*256+std::clamp(y,0,15)*16+std::clamp(x,0,15)];};
                    const auto a=texel(ax,ay),b=texel(ax+1,ay),c=texel(ax,ay+1),d=texel(ax+1,ay+1);
                    for(unsigned i=0;i<4;++i)samples[s][i]=(a[i]*(1-fx)+b[i]*fx)*(1-fy)+(c[i]*(1-fx)+d[i]*fx)*fy;
                }
                auto v0=cpu.outputs[3],v1=cpu.outputs[4];
                for(auto *v:{&v0,&v1})for(float &c:*v)c=std::clamp(c,0.f,1.f);
                cpu.outputs[14]=halo2_test::scalar_combiners(f.material.program,samples,v0,v1);
            }
            if(enabled && cpu.outputs[4][3]<.99f)attenuation=true;
            expected.push_back(cpu.outputs);fixtures.push_back(f);
        }
        }
        if(!attenuation)throw std::runtime_error("fog fixtures never attenuate the original beam");
        halo2_test::Compute<Fixture,16> compute(HALO2_MENU_FOG_TEST_SHADER);
        const auto actual=compute.run(fixtures);
        // Reciprocal/square-root precision differs by GPU; compare the real
        // varying path with a bounded arithmetic tolerance, never Xbox pixels.
        for(unsigned i=0;i<fixtures.size();++i)
        for(unsigned reg:{3u,4u,9u,10u,11u,12u,14u})
        for(unsigned axis=0;axis<4;++axis) {
            const float a=actual[i*16+reg][axis],b=expected[i][reg][axis];
            if(!std::isfinite(a) || std::abs(a-b)>3e-5f*std::max(1.f,std::abs(b)))
                throw std::runtime_error("title fog GPU varying mismatch at fixture "+
                    std::to_string(i)+", register "+std::to_string(reg)+", component "+
                    std::to_string(axis)+": got "+std::to_string(a)+", expected "+std::to_string(b));
        }
        if(std::none_of(expected.begin()+840,expected.end(),[](const auto &values){return values[14][3]>.1f;}))
            throw std::runtime_error("planar fog fixtures never produce opacity");
        std::cout<<fixtures.size()<<" title beam/planar vertex, gradient and combiner GPU fixtures passed\n";
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
