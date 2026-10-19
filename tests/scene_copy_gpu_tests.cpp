#include "gpu_postprocess.hpp"
#include "halo2/source_scene_copy.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cmath>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const std::string &label){if(!value)throw FormatError(label);}
float number(const nlohmann::json &word){return std::bit_cast<float>(word.get<std::uint32_t>());}
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
}
int main(int argc,char **argv)try {
    check(argc==4,"Supply original destination fixture, shaders and small report path");
    std::ifstream file(argv[1]);check(bool(file),"Missing original destination fixture");
    const auto fixture=nlohmann::json::parse(file);
    check(fixture.at("schema")=="original-scene-copy-destinations-v1" &&
        fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Invalid original copy evidence");
    const auto shaders=std::filesystem::path(argv[2]),output=std::filesystem::path(argv[3]);
    const auto &inputs=fixture.at("inputs"),&rows=fixture.at("versions").at("retail").at("results");
    check(inputs.size()==37 && rows.size()==37,"Original scene-copy input coverage changed");
    unsigned calls=0;std::size_t channels=0,legacy_differences=0;float maximum=0;
    nlohmann::json cases=nlohmann::json::array();
    nlohmann::json device;
    for(const auto extent:std::array{std::array{640u,480u},std::array{1280u,720u}}) {
        halo2_test::PostImage source{0,extent[0],extent[1]};source.pixels.resize(std::size_t(extent[0])*extent[1]*4);
        for(unsigned y=0;y<extent[1];++y)for(unsigned x=0;x<extent[0];++x)for(unsigned c=0;c<4;++c)
            source.pixels[(std::size_t(y)*extent[0]+x)*4+c]=std::byte((x*(37+11*c)+y*(19+7*c)+(x^y)*13)%256);
        for(unsigned i=0;i<inputs.size();++i) {
            const auto &input=inputs.at(i),&row=rows.at(i);const std::string name=input.at("name");
            check(row.at("complete_original_return")==true,"Incomplete original copy");
            const auto rectangle=input.at("camera_rectangle_shorts").get<std::array<std::int16_t,4>>();
            const auto coordinate_extent=row.at("source_dimensions").get<std::array<std::int32_t,2>>();
            const auto destination=row.at("destination_dimensions").get<std::array<std::int32_t,2>>();
            check(coordinate_extent==std::array<std::int32_t,2>{640,480},"Original source coordinate extent changed");
            SourceSceneCopyContext context{rectangle,destination[0],destination[1],number(row.at("near_bits")),number(row.at("far_bits"))};
            const auto uniform=source_scene_copy_shader_uniform(context,coordinate_extent);
            for(unsigned w=0;w<60;++w)check(uniform.program[w]==row.at("pixel_program_words").at(w),"Copy shader program differs from original");
            std::array<Vec2,4> offsets;
            for(unsigned tap=0;tap<4;++tap) {
                const auto &bits=row.at("vertices").at(0).at("attributes").at(tap+1).at("value_bits");
                offsets[tap]={(number(bits.at(0))-rectangle[1])/640,(number(bits.at(1))-rectangle[0])/480};
            }
            halo2_test::Postprocess renderer;
            const auto properties=renderer.physical_properties();
            const nlohmann::json actual_device{{"name",properties.deviceName},{"type",properties.deviceType},
                {"vendor_id",properties.vendorID},{"device_id",properties.deviceID},{"driver_version",properties.driverVersion}};
            if(device.is_null())device=actual_device;
            check(device==actual_device,"Copy comparison changed its physical Vulkan device");
            const auto image=renderer.render(shaders/"fog.vert.spv",shaders/"scene_copy.frag.spv",2,
                std::as_bytes(std::span{&uniform,1}),{&source,1},destination[0],destination[1],halo2_test::PostBlend::Disabled);
            float case_max=0;
            for(unsigned y=0;y<unsigned(destination[1]);++y)for(unsigned x=0;x<unsigned(destination[0]);++x)for(unsigned c=0;c<4;++c) {
                // Independent projection of original attributes at host pixel
                // centers; does not reuse the native shader-uniform helper.
                const float u=float(rectangle[1])/640+(float(x)+.5f)/destination[0]*float(int(rectangle[3])-int(rectangle[1]))/640;
                const float v=float(rectangle[0])/480+(float(y)+.5f)/destination[1]*float(int(rectangle[2])-int(rectangle[0]))/480;
                float expected=0;
                for(const auto &offset:offsets)expected+=linear(source,u+offset.x,v+offset.y,c)*.25f;
                const float error=std::abs(float(std::to_integer<unsigned>(image[(std::size_t(y)*destination[0]+x)*4+c]))-expected);
                case_max=std::max(case_max,error);maximum=std::max(maximum,error);
                check(error<=2.f,"Host scene-copy filtering differs at "+name+" pixel"+std::to_string(x)+","+std::to_string(y)+" channel"+std::to_string(c)+" error"+std::to_string(error));
                ++channels;
            }
            if(i==0) {
                // Run the actual existing fixed512x256 water shader against
                // target19. The input must distinguish its wrong footprint.
                halo2_test::Postprocess legacy;
                const auto old=legacy.render(shaders/"fog.vert.spv",shaders/"water_copy.frag.spv",2,
                    std::as_bytes(std::span{&uniform,1}),{&source,1},destination[0],destination[1],halo2_test::PostBlend::Disabled);
                for(std::size_t k=0;k<image.size();++k)
                    legacy_differences+=std::abs(int(std::to_integer<unsigned>(image[k]))-int(std::to_integer<unsigned>(old[k])))>3;
            }
            cases.push_back({{"input",name},{"host_source",extent},{"destination",destination},{"maximum_byte_error",case_max}});
            ++calls;
        }
    }
    check(calls==74 && legacy_differences>100000,"Scene-copy test cannot distinguish the fixed water destination");
    // Invalid source texture facts cannot silently select the window size.
    bool rejected=false;try{source_scene_copy_shader_uniform({{0,0,480,640},320,240,.1f,1024.f},{0,480});}
    catch(const FormatError&){rejected=true;}check(rejected,"Missing source coordinate extent accepted");
    std::filesystem::create_directories(output.parent_path());
    nlohmann::json report{{"passed",true},{"device",device},{"calls",calls},{"checked_rgba_channels",channels},{"maximum_byte_error",maximum},
        {"tolerance_byte_levels",2},{"legacy_fixed_destination_differences_over_3_bytes",legacy_differences},{"cases",cases},
        {"scope","Actual native copy shader with declared host images and original emitted attributes; host Vulkan pixel centers/filtering tolerance; not original Xbox GPU pixels or live hologram scheduling"},
        {"large_artifacts_retained",nlohmann::json::array()}};
    std::ofstream stream(output);stream<<report.dump(2)<<'\n';check(bool(stream),"Cannot retain scene-copy report");
    std::cout<<calls<<" scene-copy GPU calls, "<<channels<<" RGBA checks; max error "<<maximum
        <<" bytes; fixed512x256 control differs in "<<legacy_differences<<" channels\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
