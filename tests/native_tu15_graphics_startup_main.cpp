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
    int result;bool palette_probe=false,palette_temporary=false;
    std::filesystem::path capture_path,palette_output;
    for(int i=1;i+1<argc;i+=2) {
        if(std::string_view(argv[i])=="--frontier" && (std::string_view(argv[i+1])=="graphics-palette" || std::string_view(argv[i+1])=="graphics-constants"))palette_probe=true;
        if(std::string_view(argv[i])=="--capture")capture_path=argv[i+1];
    }
    if(palette_probe) {
        palette_temporary=capture_path.empty();
        palette_output=palette_temporary?std::filesystem::temp_directory_path()/("halo2-palette-gpu-"+
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())):
            capture_path.parent_path()/(capture_path.stem().string()+"-palette-gpu");
        std::filesystem::create_directories(palette_output);
    }
    std::ostringstream diagnostics;auto* previous=std::cerr.rdbuf(diagnostics.rdbuf());
    std::string gpu;halo2::RendererDeviceLease observed;halo2::RendererPaletteFactory observed_palettes;halo2::RendererConstantMode observed_constants;
    try {
        halo2::Mesh mesh;halo2::Material material;
        material.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};
        mesh.materials.push_back(material);
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
        halo2::Camera camera;camera.field_of_view=60;
        if(palette_probe && !renderer.draw(camera,palette_output/"before.ppm"))
            throw std::runtime_error("Native palette baseline draw failed");
        result=halo2::run_native_tu15_startup_test(argc,argv,std::move(lease),palettes,constants);
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
    }catch(...) {std::cerr.rdbuf(previous);std::cerr<<diagnostics.str();SDL_DestroyWindow(window);SDL_Quit();throw;}
    std::cerr.rdbuf(previous);std::cerr<<diagnostics.str();
    SDL_DestroyWindow(window);SDL_Quit();
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
        report["native_gpu_name"]=gpu;report["native_renderer_retirement_verified"]=true;report["native_constant_control_retirement_verified"]=true;
        std::ofstream output(argv[i+1]);output<<report.dump(2)<<'\n';
        if(!output)throw std::runtime_error("Cannot retain native Vulkan validation evidence");
    }
    return result;
}catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
