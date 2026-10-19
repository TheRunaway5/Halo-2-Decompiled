#include "halo2/renderer.hpp"
#include "halo2/world_material.hpp"
#include <cstring>
#include <iostream>
using namespace halo2;
int main(int argc,char** argv)try {
    if(argc!=2)throw FormatError("supply original Lockout cache");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1]);const auto shader=cache.load_multiplayer_hill_shader();
    auto material=cache.load_material(shader);
    if(!prepare_simple_world_material(cache,material))throw FormatError("authored hill pass unavailable");
    Mesh mesh;mesh.vertices.resize(3);mesh.materials.push_back(std::move(material));
    std::vector<Vertex> triangles{{{4,-1,-1},{1,1,1},{0,0},0},
        {{4,1,-1},{1,1,1},{1,0},0},{{4,0,1},{1,1,1},{.5f,1},0}};
    SourceTransparentCommand command;command.callback=0x41020;
    const std::array<unsigned,2> payload{shader,1234};
    std::memcpy(command.payload.data(),payload.data(),sizeof(payload));
    ObjectLightingState light;light.ambient={.2f,.3f,.4f};light.primary_color={.6f,.5f,.4f};
    std::vector<SourceTransparentDrawBinding> bindings{
        SourceTransparentGeometryDraw{shader,1234,triangles,light,0}};
    ModelView view;view.width=640;view.height=480;view.full_surface=true;view.camera.field_of_view=90;
    view.world_transparent=SourceTransparentDrawBank{{&command,1},0,bindings};
    if(SDL_Init(SDL_INIT_VIDEO))throw FormatError(SDL_GetError());
    auto* window=SDL_CreateWindow("Transparent lighting ownership",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
    if(!window)throw FormatError(SDL_GetError());
    try {
        Renderer renderer(window,mesh,true);
        for(unsigned n=0;n<8;++n) {
            std::get<SourceTransparentGeometryDraw>(bindings.front()).lighting->ambient.x=float(n)/16;
            if(!renderer.draw(view.camera,{}, {},false,{},{},{},std::span{&view,1}))
                throw FormatError("transparent lighting ownership frame did not submit");
        }
    }catch(...) {SDL_DestroyWindow(window);SDL_Quit();throw;}
    SDL_DestroyWindow(window);SDL_Quit();
    std::cout<<"8 actual Renderer frames consumed caller-owned transparent lighting after command planning\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
