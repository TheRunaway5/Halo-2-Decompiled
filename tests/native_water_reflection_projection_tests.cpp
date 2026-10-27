#include "halo2/source_water_reflection_tu15.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/math.hpp"
#include "helpers/native_water_reflection_projection_kernel.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <bit>
#include <cmath>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <filesystem>
using namespace halo2;using Json=nlohmann::json;
void need(bool b,const char *s){if(!b)throw FormatError(s);}
int main(int argc,char **argv)try {
    need(argc==3,"Provide exact fixture and owned output");
    const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Fixed reflection comparison requires owned test scratch");
    const auto relative=std::filesystem::weakly_canonical(argv[2]).lexically_relative(std::filesystem::canonical(scratch));
    need(!relative.empty()&&!relative.is_absolute(),"Fixed reflection output escapes owned scratch");
    for(const auto &part:relative)need(part!="..","Fixed reflection output escapes owned scratch");
    std::ifstream in(argv[1]);
    std::vector<std::byte> code;std::string line;
    while(std::getline(in,line)){if(!line.starts_with("P stencil_texture 0 "))continue;
        std::istringstream row(line);char kind;std::string leaf;unsigned cls,count;row>>kind>>leaf>>cls>>count;
        for(unsigned i=0;i<count;++i){unsigned semantic;row>>semantic;}
        std::string hex;row>>hex;for(unsigned i=0;i<hex.size();i+=2)code.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));break;
    }
    need(!code.empty(),"Authenticated stencil_texture class0 source program absent");
    const auto program=decode_vertex_program(code);Json rows=Json::array();std::size_t matrix_w_mismatches{},depth_mismatches{},native_output_mismatches{},native_clip_mismatches{},positive_w{},negative_w{},zero_w{},nonfinite_clip{};
    for(float near:{.015625f,1.f})for(float far:{32.f,1024.f})for(float height:{0.f,7.f}) {
        SourceFrustumProjectionInput project;project.projection_angle=1.f;project.outer=project.safe={0,0,480,640};
        const auto projection=source_frustum_projection(project);
        const Vec3 eye{3,5,7},forward{1,0,0},up{0,0,1};
        const auto bank=source_view_projection({eye,forward,up,near,far,{},projection.projection_program});
        std::array<std::byte,0x74> raw{};const auto put=[&](unsigned p,const auto &value){std::memcpy(raw.data()+p,&value,sizeof(value));};
        put(0,eye);put(12,forward);put(24,up);put(0x28,project.projection_angle);put(0x30,project.outer);put(0x38,project.safe);
        put(0x40,near);put(0x44,far);
        const auto reflection=source_water_reflection_tu15(raw,bank,height);
        for(float distance:{near*.5f,near,near*2.f,far*.5f,far,far*1.01f,-near*.5f,-near,-far*.5f,-far,0.f}) {
            const auto &view=reflection.rendering.camera_to_world;
            const auto point=view.position+view.basis[2]*(-distance)+view.basis[0]*(distance*.13f);
            VertexProgramState state;state.attributes[0]={point.x,point.y,point.z,1};state.attributes[1]={.2f,.3f,0,1};
            std::copy(reflection.constants.begin(),reflection.constants.end(),state.constants.begin());
            if(distance==0.f)state.constants[3]={0,0,0,0}; // Explicit exact-W0 arithmetic control.
            state.constants[18]={1,0,0,0};state.constants[19]={0,1,0,0};state.constants[172]={1,1,0,0};
            auto native=state;native_water_reflection_projection_test::run(native);
            execute_vertex_program(program,state);const auto screen=state.outputs[0];
            const bool native_exact=std::memcmp(native.outputs.data(),state.outputs.data(),sizeof(state.outputs))==0;native_output_mismatches+=!native_exact;
            std::array<float,4> matrix{};
            for(unsigned r=0;r<4;++r){const auto &c=state.constants[r];
                matrix[r]=((point.x*c[0]+point.y*c[1])+point.z*c[2])+c[3];if(r==1)matrix[r]=-matrix[r];}
            const auto final_clip=[](ShaderVector value) {
                value[0]=std::trunc(value[0]*16.f)/16.f;value[1]=std::trunc(value[1]*16.f)/16.f;
                value[3]=(value[3]<0.f?-1.f:1.f)*std::clamp(std::abs(value[3]),5.421010862427522e-20f,1.8446744073709552e19f);
                value[0]=(value[0]*2.f/512.f-1.f)*value[3];value[1]=(value[1]*2.f/256.f-1.f)*value[3];
                value[2]=(value[2]/16777215.f)*value[3];return value;
            };
            const auto aot=final_clip(screen),native_clip=final_clip(native.outputs[0]);
            const bool fullclip_exact=std::memcmp(native_clip.data(),aot.data(),sizeof(aot))==0;native_clip_mismatches+=!fullclip_exact;
            need(std::isfinite(aot[3])&&aot[3]!=0.f,"Final W clamp lost its finite nonzero output");
            positive_w+=screen[3]>0.f;negative_w+=screen[3]<0.f;zero_w+=screen[3]==0.f;
            const bool finite_clip=std::ranges::all_of(aot,[](float v){return std::isfinite(v);});nonfinite_clip+=!finite_clip;
            const bool same_w=std::bit_cast<unsigned>(aot[3])==std::bit_cast<unsigned>(matrix[3]);
            const bool same_depth=std::abs(aot[2]/aot[3]-matrix[2]/matrix[3])<1e-5f;
            matrix_w_mismatches+=!same_w;depth_mismatches+=!same_depth;
            rows.push_back({{"exact_w_zero_operand_control",distance==0.f},{"finite_raster_domain",finite_clip},{"zero_w_framebuffer_admitted",false},{"near",near},{"far",far},{"water_height",height},{"distance",distance},
                {"world",{point.x,point.y,point.z}},{"raw_program_output",screen},{"aot_final_clip",aot},{"proposed_matrix_clip",matrix},
                {"matrix_w_exact",same_w},{"ndc_depth_close",same_depth},{"native_all_outputs_bit_exact",native_exact},{"native_fullclip_bit_exact",fullclip_exact},{"native_fullclip",native_clip},{"native_fullclip_bits",std::bit_cast<std::array<std::uint32_t,4>>(native_clip)},{"reference_fullclip_bits",std::bit_cast<std::array<std::uint32_t,4>>(aot)}});
        }
    }
    Json report{{"schema","HALO2_WATER_REFLECTION_FIXED_NATIVE_CLIP_DIFFERENTIAL_V1"},{"executed",true},
        {"passed",native_output_mismatches==0&&native_clip_mismatches==0&&positive_w>0&&negative_w>0&&zero_w>0},{"matrix_only_not_accepted",true},{"native_output_mismatches",native_output_mismatches},{"native_clip_mismatches",native_clip_mismatches},
        {"positive_w_cases",positive_w},{"negative_w_cases",negative_w},{"exact_zero_w_cases",zero_w},{"nonfinite_clip_cases",nonfinite_clip},{"rows",rows},{"cases",rows.size()},{"matrix_w_mismatches",matrix_w_mismatches},{"ndc_depth_mismatches",depth_mismatches},
        {"scope","Offline-only authenticated source stencil_texture arithmetic plus shared AOT final conversion, including finite negativeW and explicit exact-W0 operand controls. Nonfinite exact-W0 clip output is not admitted as a framebuffer domain; no GPU or production execution"}};
    std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<Json{{"cases",rows.size()},{"native_output_mismatches",native_output_mismatches},{"native_clip_mismatches",native_clip_mismatches}}<<'\n';return report.at("passed").get<bool>()?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
