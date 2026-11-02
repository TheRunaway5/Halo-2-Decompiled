#include "gpu_postprocess.hpp"
#include "scalar_combiners.hpp"
#include "halo2/source_camouflage_material.hpp"
#include "halo2/source_shader_texture.hpp"
#include "halo2/world_material_uniform.hpp"
#include <cmath>
#include <iostream>

using namespace halo2;
namespace {
void check(bool ok,const char* why){if(!ok)throw FormatError(why);}
constexpr unsigned count=32;
struct Probe {std::array<ShaderVector,4> coordinates;ShaderVector color,lighting;};
struct Probes {std::array<std::uint32_t,4> count{::count,0,0,0};std::array<Probe,::count> values;};
static_assert(sizeof(Probes)==3088);
ShaderVector sample(const halo2_test::PostImage& image,float x,float y) {
    x-=.5f;y-=.5f;const int ix=int(std::floor(x)),iy=int(std::floor(y));
    const float dx=x-ix,dy=y-iy;
    const auto texel=[&](int a,int b,unsigned c) {
        a=std::clamp(a,0,int(image.width)-1);b=std::clamp(b,0,int(image.height)-1);
        return float(std::to_integer<unsigned>(image.pixels[(std::size_t(b)*image.width+a)*4+c]))/255.f;
    };
    ShaderVector out{};
    for(unsigned c=0;c<4;++c)out[c]=(texel(ix,iy,c)*(1-dx)+texel(ix+1,iy,c)*dx)*(1-dy)+
        (texel(ix,iy+1,c)*(1-dx)+texel(ix+1,iy+1,c)*dx)*dy;
    return out;
}
}
int main(int argc,char** argv)try {
    check(argc==4,"Supply original map, native shader directory and probe vertex shader");
    Cache cache(argv[1]);const auto authored=cache.load_material(source_camouflage_shader(cache));
    const auto material=prepare_camouflage_material(cache,authored,18,1,1.f);
    std::cout<<"Original full-pass normal-map frames: "<<material.animated_textures[0].size()<<'\n';
    const auto &program=material.forward_program.words;
    // Whole original binder banks retain these exact full-pass operations:
    // scene RGB + scene RGB squared * (1 - clamp(2*v0.b-1)) squared.
    check(program[34]==0x0a0a0000u && program[45]==0x40u &&
        program[0]==0x44200000u && program[1]==0x3c3c0000u &&
        program[26]==0xc0u && program[27]==0xc0u &&
        program[8]==0x040f000au && program[9]==0x1c330080u &&
        program[54]==0x1a621u && program[55]==0x11u,
        "Original full camouflage/Fresnel source identity changed");
    unsigned channels{},maximum{},highlight_controls{},dimension_controls{};
    for(const auto size:std::array<std::array<unsigned,2>,6>{{{320,240},{640,360},{960,540},{1920,1080},{639,359},{1,1}}}) {
        halo2_test::Postprocess gpu;
        std::vector<halo2_test::PostImage> images;
        for(unsigned stage=0;stage<4;++stage) {
            halo2_test::PostImage image{stage,stage==2?size[0]:1,stage==2?size[1]:1};
            image.pixels.resize(std::size_t(image.width)*image.height*4);
            for(unsigned y=0;y<image.height;++y)for(unsigned x=0;x<image.width;++x) {
                const std::size_t n=(std::size_t(y)*image.width+x)*4;
                if(stage==2) {
                    image.pixels[n]=std::byte(25+170*x/image.width);
                    image.pixels[n+1]=std::byte(30+160*y/image.height);
                    image.pixels[n+2]=std::byte(45+110*(x+y)/(image.width+image.height));
                } else {image.pixels[n]=image.pixels[n+1]=std::byte{128};image.pixels[n+2]=std::byte{255};}
                image.pixels[n+3]=std::byte{255};
            }
            images.push_back(std::move(image));
        }
        for(unsigned stage=0;stage<4;++stage) {
            halo2_test::PostImage cube{5,1,1};cube.view_type=VK_IMAGE_VIEW_TYPE_CUBE;cube.array_element=stage;
            cube.pixels.assign(24,std::byte{255});images.push_back(std::move(cube));
            halo2_test::PostImage volume{6,1,1};volume.view_type=VK_IMAGE_VIEW_TYPE_3D;volume.array_element=stage;
            volume.pixels.assign(4,std::byte{255});images.push_back(std::move(volume));
        }
        Probes probes;
        for(unsigned i=0;i<count;++i) {
            const float x=(float((i*7)%29)+.375f)/29.f*size[0];
            const float y=(float((i*11)%31)+.625f)/31.f*size[1];
            auto &p=probes.values[i];
            // SIGNED_DOT_PRODUCT maps neutral128/128/255 to0/0/1.
            p.coordinates={ShaderVector{.5f,.5f,0,1},ShaderVector{0,0,x,1},
                ShaderVector{0,0,y,1},ShaderVector{0,0,1,1}};
            p.color={.5f,.5f,float(i%5)*.25f,1};p.lighting={0,0,0,0};
        }
        WorldMaterialUniform uniform{};uniform.program=program;uniform.flags={0,1,material.forward_vertex,1};
        uniform.texture_effects[2][1]=1; // Only the scene uses source-pixel coordinates.
        const auto shader=std::filesystem::path(argv[2])/native_fragment_kernel_path("world",program);
        // Production uploads constants only. Native execution must not need
        // the original instruction/control words or an instruction count.
        const auto uploaded=native_material_gpu_uniform(uniform);
        const auto actual=gpu.render(argv[3],shader,4,std::as_bytes(std::span(&uploaded,1)),images,count,1,
            halo2_test::PostBlend::Disabled,{},std::as_bytes(std::span(&probes,1)),count*6,8);
        for(unsigned i=0;i<count;++i) {
            const auto &p=probes.values[i];const auto scene=sample(images[2],p.coordinates[1][2],p.coordinates[2][2]);
            const std::array<ShaderVector,4> textures{ShaderVector{128.f/255,128.f/255,1,1},{},scene,ShaderVector{1,1,1,1}};
            const auto reference=halo2_test::scalar_combiners(program,textures,p.color,p.lighting);
            const float edge=1-std::clamp(2*p.color[2]-1,0.f,1.f);
            for(unsigned c=0;c<3;++c)check(std::abs(reference[c]-std::clamp(scene[c]+scene[c]*scene[c]*edge*edge,0.f,1.f))<1e-6f,
                "Independent full camouflage equation disagrees with original combiner");
            for(unsigned c=0;c<4;++c) {
                const int expected=int(std::lround(reference[c]*255));
                const unsigned error=unsigned(std::abs(int(std::to_integer<unsigned>(actual[i*4+c]))-expected));
                maximum=std::max(maximum,error);++channels;
                check(error<=1,"Native camouflage pixel differs from original scalar combiner/scene-coordinate reference");
                if(c<3)highlight_controls+=std::abs(expected-int(std::lround(scene[c]*255)))>8;
            }
            const auto wrong=sample(images[2],p.coordinates[1][2]*size[0],p.coordinates[2][2]*size[1]);
            dimension_controls+=std::abs(wrong[0]-scene[0])>.1f;
        }
        std::cout<<size[0]<<'x'<<size[1]<<": original full-pass scene sampling and Fresnel matched\n";
    }
    check(highlight_controls>100 && dimension_controls>50,"Camouflage Fresnel/dimension negative controls lost sensitivity");
    std::cout<<channels<<" native RGBA channels; maximum error "<<maximum<<"; Fresnel/size controls "
        <<highlight_controls<<'/'<<dimension_controls<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
