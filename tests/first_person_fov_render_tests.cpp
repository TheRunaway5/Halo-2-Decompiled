#define SDL_MAIN_HANDLED
#include "halo2/content.hpp"
#include "halo2/gameplay_fov.hpp"
#include "halo2/model.hpp"
#include "halo2/renderer.hpp"
#include "halo2/transform.hpp"
#include "halo2/first_person_ik.hpp"
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool condition,const char *message) {
    if(!condition)throw FormatError(message);
}
std::vector<char> pixels(const std::filesystem::path &path) {
    std::ifstream in(path,std::ios::binary);std::string magic;unsigned width{},height{},maximum{};
    in>>magic>>width>>height>>maximum;in.get();
    check(magic=="P6" && maximum==255,"Invalid first-person framebuffer");
    std::vector<char> rgb(std::size_t(width)*height*3);
    in.read(rgb.data(),rgb.size());check(bool(in),"Incomplete first-person framebuffer");return rgb;
}
}
int main(int argc,char **argv)try {
    check(argc==3,"MAPS OUTPUT required");SDL_SetMainReady();
    const std::filesystem::path maps=argv[1],output=argv[2];
    std::filesystem::create_directories(output);
    Cache cache(maps/"lockout.map");const auto content=cache.load_content(0,true);
    const auto &weapon=content.weapons.at(cache.slayer_starting_weapon());
    const auto gun=cache.load_model(weapon.first_person_model,false);
    const auto hands=cache.load_model(content.first_person_hands,false);
    const auto clip=cache.load_action(weapon.first_person_animation,"any","any","any","idle");
    const auto ik=cache.load_first_person_ik(weapon.first_person_animation,
        weapon.first_person_model,content.first_person_hands);
    const auto pose=pose_first_person_models_world(gun,hands,clip,0.f,weapon.first_person_offset,
        camera_transform({0,0,0},{1,0,0},{0,0,1}),ik,0.f);
    check(!pose.arms.empty() && !pose.weapon.empty(),"Missing authored hands/weapon geometry");
    // Keep the real posed geometry and use controlled colors to isolate lens
    // framing from lighting, material animation, world occlusion and HUD art.
    std::vector<Vertex> foreground=pose.arms;
    foreground.insert(foreground.end(),pose.weapon.begin(),pose.weapon.end());
    for(std::size_t i=0;i<foreground.size();++i) {
        auto &v=foreground[i];v.material=0;v.color=i<pose.arms.size()?Vec3{1,.25f,.1f}:Vec3{.1f,.6f,1};
    }
    Mesh mesh;mesh.vertices.resize(3);mesh.materials.emplace_back();
    check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    auto *window=SDL_CreateWindow("MCC first-person FOV framing",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
    check(window,SDL_GetError());
    {
        Renderer renderer(window,mesh,true);renderer.scene_clear_color({.025f,.035f,.045f});
        for(const auto dimensions:std::array<std::array<int,2>,3>{{{640,480},{1280,720},{2560,720}}}) {
            SDL_SetWindowSize(window,dimensions[0],dimensions[1]);SDL_PumpEvents();renderer.resized();
            const float aspect=float(dimensions[0])/dimensions[1];
            const auto suffix=std::to_string(dimensions[0])+"x"+std::to_string(dimensions[1]);
            const auto capture=[&](unsigned fov,bool independent,bool empty=false) {
                Camera camera;camera.oriented=true;camera.forward={1,0,0};camera.up={0,0,1};
                const float horizontal=gameplay_field_of_view(fov);
                camera.projection_vertical_angle=gameplay_camera_angles(horizontal,horizontal,aspect).projection_angle;
                ModelView view;view.camera=camera;view.width=640;view.height=480;view.full_surface=true;
                if(!empty)view.foreground=foreground;
                if(independent)view.foreground_projection_vertical_angle=gameplay_first_person_projection_angle;
                const auto path=output/(suffix+"-"+std::to_string(fov)+(empty?"-empty":independent?"-mcc":"-world-lens")+".ppm");
                for(unsigned attempt=0;attempt<4;++attempt)
                    if(renderer.draw(camera,path,{},false,{},{},{},{&view,1}))return pixels(path);
                throw FormatError("First-person FOV capture did not submit");
            };
            const auto baseline=capture(78,true,true),normal=capture(78,true),wide=capture(120,true);
            const auto coupled=capture(120,false);
            check(normal==wide,"World FOV changed the independently projected hands/weapon pixels");
            check(normal!=baseline,"First-person framebuffer contains no hands/weapon");
            check(wide!=coupled,"First-person lens control did not change real projected geometry");
            std::cout<<suffix<<": identical hands/weapon pixels at 78 and 120; world-lens control differs\n";
        }
    }
    SDL_DestroyWindow(window);SDL_Quit();return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
