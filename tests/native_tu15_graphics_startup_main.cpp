#include "helpers/native_tu15_startup_test.hpp"
#include "halo2/renderer.hpp"
#include <cstdlib>
#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
int main(int argc,char** argv)try {
    if(SDL_Init(SDL_INIT_VIDEO)!=0)throw std::runtime_error(SDL_GetError());
    auto* window=SDL_CreateWindow("Native TU1.5 graphics startup",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
    if(!window)throw std::runtime_error(SDL_GetError());
    int result;bool palette_probe=false,palette_temporary=false,depth_probe=false;
    nlohmann::json depth_samples=nlohmann::json::array();
    std::filesystem::path capture_path,palette_output;
    for(int i=1;i+1<argc;i+=2) {
        if(std::string_view(argv[i])=="--frontier" && (std::string_view(argv[i+1])=="graphics-palette" || std::string_view(argv[i+1])=="graphics-constants" || std::string_view(argv[i+1])=="graphics-depth" || std::string_view(argv[i+1])=="graphics-depth-state"))palette_probe=true;
        if(std::string_view(argv[i])=="--frontier" && (std::string_view(argv[i+1])=="graphics-depth" || std::string_view(argv[i+1])=="graphics-depth-state"))depth_probe=true;
        if(std::string_view(argv[i])=="--capture")capture_path=argv[i+1];
    }
    if(palette_probe) {
        palette_temporary=capture_path.empty();
        palette_output=palette_temporary?std::filesystem::temp_directory_path()/("halo2-palette-gpu-"+
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())):
            capture_path.parent_path()/(capture_path.stem().string()+"-palette-gpu");
        std::filesystem::create_directories(palette_output);
    }
    if(depth_probe && SDL_setenv("HALO2_DEPTH_DIAGNOSTICS","1",1))throw std::runtime_error("Cannot enable actual depth readback");
    std::ostringstream diagnostics;auto* previous=std::cerr.rdbuf(diagnostics.rdbuf());
    std::string gpu;halo2::RendererDeviceLease observed;halo2::RendererPaletteFactory observed_palettes;halo2::RendererConstantMode observed_constants;halo2::RendererDepthMode observed_depth;
    try {
        halo2::Mesh mesh;halo2::Material material;
        halo2::BspAtmosphere depth_atmosphere;
        depth_atmosphere.gradient={1,1,11,{std::byte{0},std::byte{0},std::byte{0},std::byte{0}}};
        material.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};
        mesh.materials.push_back(material);
        if(depth_probe) {
            // Saturated red remains an unambiguous owner color through the
            // existing world bloom pass; this fixture tests depth admission.
            auto second=material;second.diffuse.pixels={std::byte{255},std::byte{0},std::byte{0},std::byte{255}};
            mesh.materials.push_back(second);
        }
        if(palette_probe)mesh.vertices={{{4,-2,-1},{1,1,1},{0,0},0},
            {{4,2,-1},{1,1,1},{0,0},0},{{4,0,2},{1,1,1},{0,0},0}};
        const auto* selected=std::getenv("HALO2_NATIVE_GPU_SHADER_DIR");
        halo2::Renderer renderer(window,mesh,true,{},nullptr,{},nullptr,
            selected?std::filesystem::path(selected):std::filesystem::path{});
        gpu=renderer.device_name();
        auto lease=renderer.device_lease();
        observed=lease;
        if(lease.acquire().native_address().value<=0xffffffffu)
            throw std::runtime_error("Native graphics device validation requires an actual address above 4GiB");
        auto palettes=renderer.palette_factory();observed_palettes=palettes;
        auto constants=renderer.constant_mode_control();observed_constants=constants;
        auto depth=renderer.depth_mode_control();observed_depth=depth;
        halo2::Camera camera;camera.field_of_view=60;
        if(palette_probe && !renderer.draw(camera,palette_output/"before.ppm"))
            throw std::runtime_error("Native palette baseline draw failed");
        result=halo2::run_native_tu15_startup_test(argc,argv,std::move(lease),palettes,constants,depth);
        if(!result && palette_probe) {
            for(unsigned stage=0;stage<4;++stage)if(!palettes.bound(stage).valid())
                throw std::runtime_error("Native palette stage has no live renderer color resource");
            const std::array<std::byte,1> index{std::byte{0}};
            renderer.update_palettized_image(0,0,index,1,1,false);
            if(!renderer.draw(camera,palette_output/"after.ppm"))throw std::runtime_error("Native palette draw failed");
            std::ifstream before(palette_output/"before.ppm",std::ios::binary),after(palette_output/"after.ppm",std::ios::binary);
            const std::vector<char> a{std::istreambuf_iterator<char>(before),{}},b{std::istreambuf_iterator<char>(after),{}};
            if(a.empty() || a==b)throw std::runtime_error("Native palette color resource did not reach the GPU image");
        }
        if(!result && depth_probe) {
            renderer.scene_bsp_atmosphere(depth_atmosphere);
            // Two overlapping triangles, near then far. A constant source c3
            // makes their integer W equal (LEQUAL lets the second win). A
            // position-dependent c3 makes the far triangle fail the same test.
            std::vector<halo2::Vertex> overlap=mesh.vertices;
            for(auto vertex:mesh.vertices) {
                vertex.position=vertex.position*2.f;vertex.material=1;overlap.push_back(vertex);
            }
            halo2::ModelView view;view.camera=camera;view.width=640;view.height=480;
            view.full_surface=true;view.vertices=overlap;view.atmosphere=halo2::BspFogSample{};
            view.source_view_shader_constants=halo2::SourceViewShaderConstants{};
            for(unsigned n=0;n<2;++n) {
                const auto expected=n?65536u:123456u;
                view.source_view_shader_constants->world[3]=n?halo2::ShaderVector{16384,0,0,0}:halo2::ShaderVector{0,0,0,123456.5f};
                const auto path=palette_output/("depth-"+std::to_string(n)+".ppm");
                if(!renderer.draw(camera,path,{},false,{}, {},{},std::span(&view,1)))
                    throw std::runtime_error("Native W-depth probe draw failed");
                std::ifstream input(path.string()+".depth-d24-low24.bin",std::ios::binary|std::ios::ate);
                if(input.tellg()!=640*480*4)throw std::runtime_error("Native W-depth readback has a different extent");
                input.seekg((240*640+320)*4);std::array<unsigned char,4> bytes{};
                input.read(reinterpret_cast<char*>(bytes.data()),4);
                const auto actual=std::uint32_t(bytes[0])|(std::uint32_t(bytes[1])<<8)|(std::uint32_t(bytes[2])<<16)|(std::uint32_t(bytes[3])<<24);
                if(!input || actual!=expected)throw std::runtime_error("Actual native integer W depth differs: "+std::to_string(actual));
                std::ifstream ppm(path,std::ios::binary);std::string magic;unsigned width,height,maximum;
                ppm>>magic>>width>>height>>maximum;ppm.get();
                if(magic!="P6" || width!=640 || height!=480 || maximum!=255)throw std::runtime_error("Native W-depth color readback has a different format");
                ppm.seekg((240*640+320)*3,std::ios::cur);std::array<unsigned char,3> rgb{};
                ppm.read(reinterpret_cast<char*>(rgb.data()),3);
                const auto color=n?std::array<unsigned char,3>{126,126,255}:std::array<unsigned char,3>{255,0,0};
                if(!ppm || rgb!=color)throw std::runtime_error("Actual native W-depth write/LEQUAL occlusion differs");
                depth_samples.push_back({{"raw_d24",actual},{"expected_d24",expected},{"rgb",rgb},{"expected_rgb",color},
                    {"case",n?"far W rejected":"equal W accepted"},{"pixel",{320,240}}});
            }
        }
    }catch(...) {std::cerr.rdbuf(previous);std::cerr<<diagnostics.str();SDL_DestroyWindow(window);SDL_Quit();throw;}
    std::cerr.rdbuf(previous);std::cerr<<diagnostics.str();
    SDL_DestroyWindow(window);SDL_Quit();
    if(observed_depth.valid())throw std::runtime_error("Departed renderer retained a live depth-mode control");
    if(observed_constants.valid())throw std::runtime_error("Departed renderer retained a live constant-mode control");
    if(observed_palettes.valid())throw std::runtime_error("Departed renderer retained a live palette factory");
    if(palette_temporary)std::filesystem::remove_all(palette_output);
    if(observed.valid())throw std::runtime_error("Departed renderer retained its source device lease");
    if(diagnostics.str().find("Vulkan validation:")!=std::string::npos)
        throw std::runtime_error("Native graphics startup emitted Vulkan validation messages");
    if(!result)for(int i=1;i+1<argc;i+=2)if(std::string_view(argv[i])=="--capture") {
        std::ifstream input(argv[i+1]);auto report=nlohmann::json::parse(input);input.close();
        report["native_vulkan_validation_enabled"]=true;report["native_vulkan_validation_messages"]=0;
        if(palette_probe) {
            report["native_palette_gpu_upload_draw_verified"]=true;
            report["native_palette_factory_retirement_verified"]=true;
            report["native_palette_gpu_images"]=palette_output.string();
        }
        if(depth_probe) {report["native_depth_gpu_samples"]=depth_samples;report["native_depth_gpu_attachment_and_occlusion_verified"]=true;report["native_depth_control_retirement_verified"]=true;}
        report["native_gpu_name"]=gpu;report["native_renderer_retirement_verified"]=true;report["native_constant_control_retirement_verified"]=true;
        std::ofstream output(argv[i+1]);output<<report.dump(2)<<'\n';
        if(!output)throw std::runtime_error("Cannot retain native Vulkan validation evidence");
    }
    return result;
}catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
