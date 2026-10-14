#include "halo2/renderer.hpp"
#include <vulkan/vulkan.h>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
using namespace halo2;
namespace {
void require(bool value,const char *message) {
    if(!value)throw std::runtime_error(message);
}
std::vector<char> bytes(const std::filesystem::path &path) {
    std::ifstream input(path,std::ios::binary);
    require(bool(input),"missing native device probe image");
    return {std::istreambuf_iterator<char>(input),{}};
}
}
int main(int argc,char **argv) {
    try {
        require(argc==2,"usage: renderer_device_tests output-directory");
        const auto output=std::filesystem::absolute(argv[1]);
        std::filesystem::create_directories(output);
        require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
        auto *window=SDL_CreateWindow("Source world device admission",0,0,640,480,
            SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
        require(window,SDL_GetError());
        Mesh mesh;Material material;
        material.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};
        mesh.materials.push_back(material);
        mesh.vertices={{{4,-2,-1},{1,1,1},{-4,-4},0},
            {{4,2,-1},{1,1,1},{-4,-4},0},{{4,0,2},{1,1,1},{-4,-4},0}};
        Camera camera;camera.field_of_view=60;
        SourceWorldDeviceState state;
        std::string device;
        RendererDeviceLease lease;
        std::optional<RendererDeviceLease::Use> retained;
        std::ostringstream validation;
        auto *previous=std::cerr.rdbuf(validation.rdbuf());
        try {
            {
                Renderer renderer(window,mesh,true);
                lease=renderer.device_lease();retained.emplace(lease.acquire());
                require(lease.valid() && retained->native_address().value>0xffffffffu,
                    "native renderer lease lacks its actual widened device identity");
                state=renderer.source_world_device_state();device=renderer.device_name();
                require(state.instance_api_version>=VK_API_VERSION_1_1,
                    "ordinary native renderer lost its Vulkan1.1 contract");
                require(!state.demote_to_helper_enabled ||
                    (state.instance_api_version>=VK_API_VERSION_1_3 &&
                     state.physical_api_version>=VK_API_VERSION_1_3),
                    "SPIR-V1.6 demote was enabled through an older API contract");
                require(state.depth_clamp_enabled,"original sky depth clamp was lost");
                bool rejected_fog=false;
                try {
                    BspFogSample unbound;
                    renderer.draw(camera,{}, {},false,{}, {},{}, {},{}, {},&unbound);
                }catch(const FormatError &) {rejected_fog=true;}
                require(rejected_fog,"mainmenu aggregate was admitted without a title atmosphere");
                require(renderer.draw(camera,output/"ordinary.ppm"),"ordinary native draw failed");
                renderer.release_presentation();
                require(!lease.valid(),"released presentation retained a valid source renderer lease");
                require(renderer.draw(camera,output/"rebuilt.ppm"),"native presentation rebuild failed");
                require(renderer.device_lease().valid() && !lease.valid(),
                    "presentation rebuild revived a retired lease or failed to issue a live replacement");
            }
            require(!lease.valid(),"departed renderer retained a valid logical device lease");
            bool retired=false;
            try{lease.acquire();}catch(const std::runtime_error &){retired=true;}
            require(retired && retained->native_address().value>0xffffffffu,
                "retirement admitted a new use or discarded an active device implementation");
            retained.reset(); // Release old presentation ownership before the replacement scene.
            if(state.custom_border_colors_enabled) {
                auto &sampler=mesh.materials.front().texture_states.front();
                sampler.address_u=sampler.address_v=sampler.address_w=4;
                sampler.border_color=0xff902050;
                Renderer renderer(window,mesh,true);
                const auto replacement=renderer.source_world_device_state();
                require(replacement.demote_to_helper_enabled==state.demote_to_helper_enabled &&
                    replacement.custom_border_colors_enabled,
                    "replacement lost demote/custom-border feature chain");
                require(renderer.draw(camera,output/"custom-border.ppm"),"custom-border draw failed");
                require(bytes(output/"ordinary.ppm")!=bytes(output/"custom-border.ppm"),
                    "authored custom border did not affect native sampling");
            }
        }catch(...) {
            std::cerr.rdbuf(previous);std::cerr<<validation.str();
            SDL_DestroyWindow(window);SDL_Quit();throw;
        }
        std::cerr.rdbuf(previous);
        SDL_DestroyWindow(window);SDL_Quit();
        require(validation.str().empty(),validation.str().c_str());
        std::ofstream report(output/"device-state.json");
        report<<"{\n  \"device\": \""<<device<<"\",\n"
            <<"  \"instance_api_version\": "<<state.instance_api_version<<",\n"
            <<"  \"physical_api_version\": "<<state.physical_api_version<<",\n"
            <<"  \"geometry_shader_enabled\": "<<state.geometry_shader_enabled<<",\n"
            <<"  \"geometry_point_size_enabled\": "<<state.geometry_point_size_enabled<<",\n"
            <<"  \"depth_clamp_enabled\": "<<state.depth_clamp_enabled<<",\n"
            <<"  \"demote_to_helper_enabled\": "<<state.demote_to_helper_enabled<<",\n"
            <<"  \"anisotropy_enabled\": "<<state.anisotropy_enabled<<",\n"
            <<"  \"custom_border_colors_enabled\": "<<state.custom_border_colors_enabled<<",\n"
            <<"  \"maximum_anisotropy\": "<<state.maximum_anisotropy<<",\n"
            <<"  \"maximum_lod_bias\": "<<state.maximum_lod_bias<<",\n"
            <<"  \"validation_messages\": 0,\n"
            <<"  \"scope\": \"live logical-device negotiation and ordinary/custom-border draws; no raw-world pipeline or visual parity claim\"\n}\n";
        require(bool(report),"cannot save native device state");
        std::cout<<"Native Vulkan negotiation, retained custom-border chain, sampling and cleanup passed; "
            <<"demote="<<state.demote_to_helper_enabled<<'\n';
        return 0;
    }catch(const std::exception &error) {
        std::cerr<<error.what()<<'\n';return 1;
    }
}
