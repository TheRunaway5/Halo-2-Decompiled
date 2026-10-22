#include "halo2/renderer.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <vulkan/vulkan.h>

namespace {
using namespace halo2;
using Json=nlohmann::json;
void check(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F&& function) {
    bool rejected=false;try{function();}catch(const std::exception&){rejected=true;}
    check(rejected,"Unavailable/stale render targets were admitted");
}
std::vector<char> bytes(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);check(bool(input),"Target probe readback is absent");
    return {std::istreambuf_iterator<char>(input),{}};
}
Json inspect(const RendererTargets::Use& use,unsigned width,unsigned height) {
    const auto handle=[](NativeRuntimeAddress address) {
        std::uint64_t value{};std::memcpy(&value,reinterpret_cast<const void*>(address.value),sizeof(value));return value;
    };
    const auto& bank=use.bank();std::set<std::uint64_t> handles;
    check(use.device_address().value>0xffffffffu && bank.generation && !bank.color.empty(),"Native renderer device/epoch is absent");
    Json result{{"generation",bank.generation},{"color",Json::array()}};
    for(const auto& image:bank.color) {
        check(image.image && image.view && image.image_backing.value>0xffffffffu && image.view_backing.value>0xffffffffu &&
            image.width==width && image.height==height && image.samples==1 && image.mips==1 && image.layers==1 &&
            (image.usage&VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) && handles.insert(image.image).second,
            "Native color attachment has invalid identity/storage/creation facts");
        check(handle(image.image_backing)==image.image && handle(image.view_backing)==image.view,
            "Native attachment handle does not resolve to its real renderer backing");
        result["color"].push_back({{"image",image.image},{"view",image.view},{"backing",image.image_backing.value},
            {"format",image.format},{"width",image.width},{"height",image.height},{"usage",image.usage}});
    }
    const auto& depth=bank.depth_stencil;
    check(depth.image && depth.view && !handles.contains(depth.image) && depth.format==VK_FORMAT_D24_UNORM_S8_UINT &&
        depth.width==width && depth.height==height && depth.samples==1 && depth.mips==1 && depth.layers==1 &&
        depth.image_backing.value>0xffffffffu && (depth.usage&VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT),
        "Native integer W-depth attachment lost its actual image or format");
    check(handle(depth.image_backing)==depth.image,
        "Native depth image does not resolve to its real renderer backing");
    result["depth"]={{"image",depth.image},{"view",depth.view},{"backing",depth.image_backing.value},
        {"format",depth.format},{"width",depth.width},{"height",depth.height}};
    if(bank.last_submitted_color) {
        check(*bank.last_submitted_color<bank.color.size(),"Submitted frame does not name a native color attachment");
        result["submitted_color"]=*bank.last_submitted_color;
    }
    return result;
}
}

int main(int argc,char** argv)try {
    if(argc!=2)return 2;
    const std::filesystem::path output=argv[1];std::filesystem::create_directories(output);
    SDL_SetMainReady();check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    auto* window=SDL_CreateWindow("Native renderer targets",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN|SDL_WINDOW_RESIZABLE);
    check(window,SDL_GetError());
    std::ostringstream diagnostics;auto* previous=std::cerr.rdbuf(diagnostics.rdbuf());
    Json report{{"observations",Json::array()},{"source_two_buffer_alias_initializer_complete",false}};
    RendererTargets targets;std::optional<RendererTargets::Use> retained;
    try {
        Mesh mesh;Material material;material.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};
        mesh.materials.push_back(material);mesh.vertices={{{4,-2,-1},{1,1,1},{0,0},0},{{4,2,-1},{1,1,1},{0,0},0},{{4,0,2},{1,1,1},{0,0},0}};
        const auto* shaders=std::getenv("HALO2_NATIVE_GPU_SHADER_DIR");
        auto renderer=std::make_unique<Renderer>(window,mesh,true,std::span<const SourceShadowPlanMaterial>{},
            nullptr,std::function<void()>{},nullptr,shaders?std::filesystem::path(shaders):std::filesystem::path{});
        report["gpu"]=renderer->device_name();targets=renderer->target_resources();
        retained.emplace(targets.acquire());report["observations"].push_back(inspect(*retained,640,480));
        check(!retained->bank().last_submitted_color,"Undrawn target claimed a submitted frame");
        rejects([]{(void)RendererTargets{}.acquire();});
        bool thread_rejected=false;
        std::thread outsider([&]{
            bool denied=false;try{(void)targets.acquire();}catch(const std::invalid_argument&){denied=true;}
            thread_rejected=denied && !retained->valid();
        });outsider.join();
        check(thread_rejected,"Native render target use crossed its owning thread");
        Camera camera;camera.field_of_view=60;
        check(renderer->draw(camera,output/"before.ppm"),"Native target baseline frame was not drawn");
        auto submitted=targets.acquire();report["observations"].push_back(inspect(submitted,640,480));
        check(submitted.bank().last_submitted_color.has_value(),"Actual submitted frame was not observed");
        check(renderer->depth_mode_control().select(2).attachment_handle==submitted.bank().depth_stencil.image,
            "Native source depth mode uses a different attachment");
        check(renderer->draw(camera,output/"after-query.ppm"),"Native target post-query frame was not drawn");
        check(bytes(output/"before.ppm")==bytes(output/"after-query.ppm"),"Target observation changed actual rendered pixels");
        SDL_SetWindowSize(window,800,600);SDL_PumpEvents();renderer->resized();
        check(renderer->draw(camera,output/"resized.ppm"),"Native target resize frame was not drawn");
        check(!retained->valid() && !submitted.valid(),"Destroyed framebuffer generation remained live");
        rejects([&]{(void)retained->bank();});rejects([&]{(void)submitted.bank();});
        auto resized=targets.acquire();report["observations"].push_back(inspect(resized,800,600));
        check(resized.bank().generation>report["observations"][0]["generation"].get<std::uint64_t>(),"Target epoch did not advance");
        DisplaySettings settings;settings.internal_width=320;settings.internal_height=240;
        renderer->display_settings(settings);
        check(renderer->draw(camera,output/"internal-resolution.ppm"),"Native internal-resolution frame was not drawn");
        check(!resized.valid(),"Internal-resolution recreation retained old target handles");
        retained.emplace(targets.acquire());report["observations"].push_back(inspect(*retained,320,240));
        const auto& actual=retained->bank();
        for(const auto& image:actual.color)check((image.usage&(VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT))==
            (VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT),"Offscreen targets lost their actual transfer usage");
        const auto retired_factory=targets;
        renderer->release_presentation();
        check(!targets.valid() && !retained->valid(),"Presentation handoff retained live target handles");
        rejects([&]{(void)retained->bank();});rejects([&]{(void)targets.acquire();});
        check(renderer->draw(camera,output/"after-presentation-handoff.ppm"),"Native presentation was not recreated");
        targets=renderer->target_resources();retained.emplace(targets.acquire());
        report["observations"].push_back(inspect(*retained,320,240));
        check(!retired_factory.valid(),"Recreated presentation revived the retired target factory");
        renderer.reset();check(!targets.valid() && !retained->valid(),"Departed renderer retained live target resources");
        rejects([&]{(void)targets.acquire();});rejects([&]{(void)retained->bank();});
        retained.reset();
        report["read_only_query_pixels_unchanged"]=true;report["retirement_resize_thread_and_absence_rejected"]=true;
        report["presentation_handoff_retires_old_factory"]=true;
    }catch(...) {std::cerr.rdbuf(previous);std::cerr<<diagnostics.str();retained.reset();SDL_DestroyWindow(window);SDL_Quit();throw;}
    std::cerr.rdbuf(previous);std::cerr<<diagnostics.str();SDL_DestroyWindow(window);SDL_Quit();
    check(diagnostics.str().find("Vulkan validation:")==std::string::npos,"Native render target probe emitted validation errors");
    report["vulkan_validation_messages"]=0;report["native_address_bits"]=64;
    std::ofstream result(output/"report.json");result<<report.dump(2)<<'\n';check(bool(result),"Cannot write native target validation report");
    std::cout<<"Native renderer attachments, 64-bit backings, pixels, resize epochs and retirement verified\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
