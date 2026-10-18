// Exercise the actual renderer's presentation path with a tiny, asset-free mesh.
#include "halo2/renderer.hpp"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
int main(int argc,char** argv){try {
    if(argc!=3)throw FormatError("SECONDS FRESH_OUTPUT");
    const std::filesystem::path output=argv[2];if(std::filesystem::exists(output))throw FormatError("Output exists");
    std::filesystem::create_directories(output);const auto timings=output/"timings.tsv";setenv("HALO2_RENDER_TIMINGS",timings.c_str(),1);
    if(SDL_Init(SDL_INIT_VIDEO))throw FormatError(SDL_GetError());
    auto* window=SDL_CreateWindow("pStencil native renderer presentation",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1600,900,
        SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI|SDL_WINDOW_SHOWN);
    if(!window)throw FormatError(SDL_GetError());unsigned frames{};double elapsed{};
    {
        Mesh mesh;Material material;material.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};mesh.materials.push_back(material);
        mesh.vertices={{{4,-2,-1},{1,1,1},{0,0},0},{{4,2,-1},{1,1,1},{1,0},0},{{4,0,2},{1,1,1},{.5f,1},0}};
        Renderer renderer(window,mesh,true);Camera camera;camera.field_of_view=60;renderer.measure_frames(true);
        const auto start=std::chrono::steady_clock::now();bool running=true;
        while(running && (elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count())<std::stod(argv[1])) {
            SDL_Event event;while(SDL_PollEvent(&event)) {
                if(event.type==SDL_QUIT)running=false;
                if(event.type==SDL_WINDOWEVENT && event.window.event==SDL_WINDOWEVENT_SIZE_CHANGED)renderer.resized();
            }
            if(renderer.draw(camera))++frames;
        }
    }
    const nlohmann::json result{{"scope","Actual renderer with a diagnostic triangle; no game asset or visual parity claim"},{"frames",frames},{"seconds",elapsed},
        {"capture_present_mode",std::getenv("HALO2_PSTENCIL_CAPTURE_PRESENT_MODE")?std::getenv("HALO2_PSTENCIL_CAPTURE_PRESENT_MODE"):"default"}};
    std::ofstream(output/"result.json")<<result.dump(2)<<'\n';std::cout<<result.dump()<<'\n';SDL_DestroyWindow(window);SDL_Quit();return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
