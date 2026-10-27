#include "gpu_postprocess.hpp"
#include "halo2/water.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cmath>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const std::string &label){if(!value)throw FormatError(label);}
float number(const nlohmann::json &word){return std::bit_cast<float>(word.get<std::uint32_t>());}
struct Uniform {std::array<std::uint32_t,60> program;ShaderVector rectangle,taps;};
static_assert(sizeof(Uniform)==272);
float linear(const halo2_test::PostImage &image,float u,float v,unsigned channel) {
    const float x=u*image.width-.5f,y=v*image.height-.5f;
    const int ix=int(std::floor(x)),iy=int(std::floor(y));const float dx=x-ix,dy=y-iy;
    const auto texel=[&](int xx,int yy) {
        xx=std::clamp(xx,0,int(image.width)-1);yy=std::clamp(yy,0,int(image.height)-1);
        return float(std::to_integer<unsigned>(image.pixels[(std::size_t(yy)*image.width+xx)*4+channel]));
    };
    return (texel(ix,iy)*(1-dx)+texel(ix+1,iy)*dx)*(1-dy)+
           (texel(ix,iy+1)*(1-dx)+texel(ix+1,iy+1)*dx)*dy;
}
void save(const std::filesystem::path &path,std::span<const std::byte> bytes) {
    std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    check(bool(file),"Cannot save water-copy GPU evidence");
}
}
int main(int argc,char **argv)try {
    check(argc==5,"Original destination fixture, shader directory, capture directory and native copy manifest required");
    std::ifstream file(argv[1]);check(bool(file),"Missing original destination fixture");
    const auto fixture=nlohmann::json::parse(file);
    check(fixture.at("schema")=="original-scene-copy-destinations-v1" &&
        fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Invalid original copy evidence");
    std::ifstream native_file(argv[4]);check(bool(native_file),"Native water-copy manifest missing");
    const auto native=nlohmann::json::parse(native_file);
    check(native.at("schema")=="HALO2_NATIVE_WATER_COPY_COMBINER_V1" &&
        native.at("runtime_instruction_interpreter")==false &&
        native.at("program_words").get<std::array<std::uint32_t,60>>()==water_copy_program().words,
        "Direct native water-copy combiner differs from all60 actual source program words");
    const auto shaders=std::filesystem::path(argv[2]),output=std::filesystem::path(argv[3]);
    std::filesystem::create_directories(output);
    const auto &inputs=fixture.at("inputs"),&rows=fixture.at("versions").at("retail").at("results");
    unsigned calls=0;std::size_t checks=0,old_differences=0;float maximum=0;
    for(const auto extent:std::array{std::array{640u,480u},std::array{1280u,720u}}) {
        halo2_test::PostImage source{0,extent[0],extent[1]};source.pixels.resize(std::size_t(extent[0])*extent[1]*4);
        // Declared high-frequency host inputs expose tap spacing and clipping.
        // They are not captured Xbox scene images or a raster-center oracle.
        for(unsigned y=0;y<extent[1];++y)for(unsigned x=0;x<extent[0];++x)for(unsigned c=0;c<4;++c)
            source.pixels[(std::size_t(y)*extent[0]+x)*4+c]=std::byte((x*(37+11*c)+y*(19+7*c)+(x^y)*13)%256);
        const auto prefix=std::to_string(extent[0])+"x"+std::to_string(extent[1]);
        save(output/(prefix+"-source.rgba"),source.pixels);
        for(unsigned i=0;i<inputs.size();++i) {
            const auto &input=inputs.at(i);if(input.at("destination_argument")!=22)continue;
            const auto &row=rows.at(i);check(row.at("complete_original_return")==true,"Incomplete original destination call");
            const auto rectangle=input.at("camera_rectangle_shorts").get<std::array<std::int16_t,4>>();
            check(row.at("source_dimensions")==nlohmann::json::array({640,480}) &&
                row.at("destination_dimensions")==nlohmann::json::array({512,256}),"Actual original copy dimensions changed");
            Uniform uniform{water_copy_program().words,{float(rectangle[1])/640,float(rectangle[0])/480,
                float(int(rectangle[3])-int(rectangle[1]))/640,float(int(rectangle[2])-int(rectangle[0]))/480},{}};
            const auto taps=water_copy_taps(uniform.rectangle[2],uniform.rectangle[3]);
            uniform.taps={taps[0][0],taps[0][1],taps[1][0],taps[2][1]};
            std::array<Vec2,4> original_offsets;
            for(unsigned tap=0;tap<4;++tap) {
                const auto &bits=row.at("vertices").at(0).at("attributes").at(tap+1).at("value_bits");
                original_offsets[tap]={(number(bits.at(0))-rectangle[1])/640,(number(bits.at(1))-rectangle[0])/480};
            }
            halo2_test::Postprocess renderer;
            const auto image=renderer.render(shaders/"fog.vert.spv",shaders/"water_copy.frag.spv",2,
                std::as_bytes(std::span{&uniform,1}),{&source,1},512,256,halo2_test::PostBlend::Disabled);
            const auto name=prefix+"-case"+std::to_string(i);
            save(output/(name+".rgba"),image);save(output/(name+"-uniform.bin"),std::as_bytes(std::span{&uniform,1}));
            for(unsigned y=0;y<256;++y)for(unsigned x=0;x<512;++x)for(unsigned c=0;c<4;++c) {
                const float u=uniform.rectangle[0]+(float(x)+.5f)/512*uniform.rectangle[2];
                const float v=uniform.rectangle[1]+(float(y)+.5f)/256*uniform.rectangle[3];
                float expected=0,old=0;
                for(unsigned tap=0;tap<4;++tap) {
                    expected+=linear(source,u+original_offsets[tap].x,v+original_offsets[tap].y,c)*.25f;
                    old+=linear(source,u+((tap&1)?.25f:-.25f)/extent[0],v+((tap&2)?.25f:-.25f)/extent[1],c)*.25f;
                }
                const float error=std::abs(float(std::to_integer<unsigned>(image[(std::size_t(y)*512+x)*4+c]))-expected);
                maximum=std::max(maximum,error);check(error<=2.f,"Host water-copy filtering differs at "+name+" pixel"+
                    std::to_string(x)+","+std::to_string(y)+" channel"+std::to_string(c)+" error"+std::to_string(error));
                old_differences+=std::abs(expected-old)>3.f;++checks;
            }
            ++calls;
        }
    }
    check(calls==24 && old_differences>100000,"Water-copy coverage cannot distinguish prior tap spacing");
    std::cout<<calls<<" production water-copy shader calls, "<<checks<<" host RGBA channel checks; maximum error "
        <<maximum<<" bytes; prior taps differ by more than3 bytes in "<<old_differences<<" channels\n";
    std::cout<<"Declared host images and existing Vulkan pixel centers; original GPU sampling and normal water flow remain unverified\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
