#include "halo2/renderer.hpp"
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>
#include <vulkan/vulkan.h>

namespace {
using namespace halo2;
using Json=nlohmann::json;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F&& action){bool denied=false;try{action();}catch(const std::exception&){denied=true;}check(denied,"Invalid source-buffer request was admitted");}
struct Frame {unsigned width{},height{};std::vector<std::byte> rgb;};
Frame frame(const std::filesystem::path& path) {
    std::ifstream f(path,std::ios::binary);std::string magic;unsigned maximum{};Frame result;
    f>>magic>>result.width>>result.height>>maximum;f.get();check(magic=="P6" && maximum==255,"Source buffer capture has an invalid format");
    result.rgb.resize(std::size_t(result.width)*result.height*3);
    check(bool(f.read(reinterpret_cast<char*>(result.rgb.data()),std::streamsize(result.rgb.size()))),"Source buffer capture is truncated");return result;
}
std::vector<std::byte> rgb(const RendererTargetImage& image,std::span<const std::byte> raw) {
    check(raw.size()==std::size_t(image.width)*image.height*4,"Physical source bank readback has the wrong size");
    const bool bgra=image.format==VK_FORMAT_B8G8R8A8_UNORM;
    check(bgra || image.format==VK_FORMAT_R8G8B8A8_UNORM,"Physical source bank has an unsupported color format");
    std::vector<std::byte> result(raw.size()/4*3);
    for(std::size_t i=0;i<raw.size()/4;++i){result[i*3]=raw[i*4+(bgra?2:0)];result[i*3+1]=raw[i*4+1];result[i*3+2]=raw[i*4+(bgra?0:2)];}
    return result;
}
std::unique_ptr<Renderer> renderer(SDL_Window* window,const Mesh& mesh) {
    const auto* shader=std::getenv("HALO2_NATIVE_GPU_SHADER_DIR");
    return std::make_unique<Renderer>(window,mesh,true,std::span<const SourceShadowPlanMaterial>{},nullptr,
        std::function<void()>{},nullptr,shader?std::filesystem::path(shader):std::filesystem::path{});
}
struct Case {unsigned width,height,internal_width,internal_height,aa;bool loading;};
void configure(Renderer& renderer,const Case& test) {
    if(test.loading){renderer.prepare_loading();return;}
    DisplaySettings settings;settings.internal_width=test.internal_width;settings.internal_height=test.internal_height;if(test.aa){settings.internal_width=(test.internal_width?test.internal_width:test.width)*2;settings.internal_height=(test.internal_height?test.internal_height:test.height)*2;}
    renderer.display_settings(settings);
}
}

int main(int argc,char** argv)try {
    check(argc==2 || argc==4,"Expected output directory [--map MCC-MAP]");
    const std::filesystem::path output=argv[1];std::filesystem::create_directories(output);
    Mesh mesh;Camera camera;camera.field_of_view=60;std::unique_ptr<Cache> cache;
    if(argc==4) {
        check(std::string_view(argv[2])=="--map","Unknown source-buffer probe option");
        cache=std::make_unique<Cache>(argv[3]);check(cache->is_mcc(),"Source-buffer asset probe requires an MCC cache");
        mesh=cache->load_bsp(0);check(!cache->spawns().empty(),"MCC source-buffer probe has no camera start");
        camera.position=cache->spawns().front().position;camera.position.z+=.65f;camera.yaw=cache->spawns().front().facing;
    } else {
        Material material;material.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};mesh.materials.push_back(material);
        mesh.vertices={{{4,-2,-1},{1,1,1},{0,0},0},{{4,2,-1},{1,1,1},{0,0},0},{{4,0,2},{1,1,1},{0,0},0}};
    }
    SDL_SetMainReady();check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    auto* window=SDL_CreateWindow("Native physical source buffers",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN|SDL_WINDOW_RESIZABLE);check(window,SDL_GetError());
    std::ostringstream diagnostics;auto* previous=std::cerr.rdbuf(diagnostics.rdbuf());Json report{{"cases",Json::array()},{"mcc_asset_probe",bool(cache)}};
    try {
        const std::array<Case,6> cases{{{640,480,0,0,0,false},{800,600,0,0,0,false},
            {800,600,320,240,0,false},{640,480,320,240,1,false},{640,480,320,240,2,false},{640,480,0,0,0,true}}};
        const unsigned case_count=cache?1:cases.size();
        for(unsigned c=0;c<case_count;++c) {
            const auto test=cases[c];SDL_SetWindowSize(window,int(test.width),int(test.height));SDL_PumpEvents();
            Mesh loading;loading.materials.push_back(Material{});loading.materials[0].diffuse={1,1,11,{std::byte{255},std::byte{255},std::byte{255},std::byte{255}}};
            loading.materials[0].ui_full_surface=true;
            loading.materials[0].vista_loading=VistaLoadingMaterial{3,VistaLoadingMaterialBlend::OneOne,{},{}};
            const auto& scene=test.loading?loading:mesh;std::array<Frame,8> reference;
            std::array<std::uint64_t,2> handles{};std::array<std::vector<std::byte>,2> retained;
            Json observation{{"width",test.width},{"height",test.height},{"internal_width",test.internal_width},
                {"internal_height",test.internal_height},{"aa",test.aa},{"loading",test.loading},{"frames",Json::array()}};
            for(unsigned mode=0;mode<2;++mode) {
                auto gpu=renderer(window,scene);report["gpu"]=gpu->device_name();configure(*gpu,test);
                auto access=gpu->target_resources();auto old=access.acquire();
                if(mode) {
                    const auto generation=old.bank().generation;rejects([&]{(void)access.source_buffers(3);});
                    check(access.acquire().bank().generation==generation,"Rejected source-buffer count changed attachments");
                    auto physical=access.source_buffers(2);check(!old.valid(),"Two-buffer realization retained old attachment uses");
                    check(physical.bank().color.size()==2 && physical.bank().source_double_buffer && !physical.bank().source_frames_submitted,
                        "Source graphics did not realize exactly two physical banks");
                    observation["presentation_images"]=physical.bank().presentation_images;
                    for(unsigned slot=0;slot<2;++slot) {
                        handles[slot]=physical.bank().color[slot].image;
                        check(physical.bank().color[slot].image_backing.value>0xffffffffu &&
                            (physical.bank().color[slot].usage&VK_IMAGE_USAGE_SAMPLED_BIT),"Source bank lacks its native sampled image");
                        rejects([&]{(void)physical.source_color_pixels(slot);});
                    }
                    check(handles[0]!=handles[1],"Physical source buffers alias the same image");
                    bool denied=false;std::thread other([&]{try{(void)access.source_buffers(2);}catch(const std::invalid_argument&){denied=true;}});other.join();check(denied,"Another thread configured source buffers");
                }
                for(unsigned n=0;n<8;++n) {
                    const Vec3 clear{float(n%3==0),float(n%3==1),float(n%3==2)};gpu->scene_clear_color(clear);
                    std::vector<Vertex> loading_vertices;
                    for(const auto uv:std::array{Vec2{0,0},Vec2{1,0},Vec2{1,1},Vec2{0,0},Vec2{1,1},Vec2{0,1}})
                        loading_vertices.push_back({{uv.x,uv.y,1},clear,uv,0});
                    const auto path=output/(std::to_string(c)+"-"+std::to_string(mode)+"-"+std::to_string(n)+".ppm");
                    check(test.loading?gpu->draw_loading(0,loading.materials,loading_vertices,path):gpu->draw(camera,path),"Source-buffer frame was not submitted");
                    const auto pixels=frame(path);
                    if(!mode){reference[n]=pixels;continue;}
                    check(pixels.width==reference[n].width && pixels.height==reference[n].height && pixels.rgb==reference[n].rgb,
                        "Native two-buffer presentation changed the rendered pixels");
                    auto use=access.acquire();const auto& bank=use.bank();const unsigned current=n&1u;
                    check(bank.color.size()==2 && bank.source_frames_submitted==n+1 && bank.last_submitted_color==current &&
                        bank.next_source_color==((n+1)&1u),"Source buffer counter/rotation differs");
                    for(unsigned slot=0;slot<2;++slot)check(bank.color[slot].image==handles[slot],"Source physical image changed during presentation rotation");
                    retained[current]=use.source_color_pixels(current);
                    if(bank.color[current].width==pixels.width && bank.color[current].height==pixels.height)
                        check(rgb(bank.color[current],retained[current])==pixels.rgb,"Presented frame is not the selected physical bank");
                    if(n)check(use.source_color_pixels(current^1u)==retained[current^1u],"Rendering one bank overwrote the previous physical image");
                    const auto repeated=access.source_buffers(2);check(repeated.bank().generation==bank.generation &&
                        repeated.bank().source_frames_submitted==bank.source_frames_submitted,"Repeated source configuration reset buffer history");
                    observation["frames"].push_back({{"frame",n+1},{"selected",current},{"next",bank.next_source_color},
                        {"image",handles[current]},{"raw_bytes",retained[current].size()}});
                }
            }
            observation["pixels_equal"]=true;observation["old_bank_preserved"]=true;report["cases"].push_back(observation);
        }
    }catch(...) {std::cerr.rdbuf(previous);std::cerr<<diagnostics.str();SDL_DestroyWindow(window);SDL_Quit();throw;}
    std::cerr.rdbuf(previous);std::cerr<<diagnostics.str();SDL_DestroyWindow(window);SDL_Quit();
    check(diagnostics.str().find("Vulkan validation:")==std::string::npos,"Source color-buffer validation emitted errors");
    report["vulkan_validation_messages"]=0;report["source_alias_initializer_complete"]=false;
    std::ofstream out(output/"report.json");out<<report.dump(2)<<'\n';check(bool(out),"Cannot write source-buffer evidence");
    std::cout<<"Native physical color-bank rotation, retained pixels and identical presentation verified\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
