#define SDL_MAIN_HANDLED
#include "halo2/content.hpp"
#include "halo2/gameplay_fov.hpp"
#include "halo2/model.hpp"
#include "halo2/renderer.hpp"
#include "halo2/transform.hpp"
#include "halo2/first_person_ik.hpp"
#include "halo2/source_frustum_camera.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

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
    check(argc==3 || argc==4 || argc==5,"MAPS OUTPUT [WEAPON [SPLIT_FIXTURE]] required");SDL_SetMainReady();
    const std::filesystem::path maps=argv[1],output=argv[2];
    std::filesystem::create_directories(output);
    Cache cache(maps/"lockout.map");const auto content=cache.load_content(0,true);
    const auto selected=argc>=4?std::find_if(content.weapons.begin(),content.weapons.end(),[&](const auto &entry) {
        return entry.second.name.ends_with(std::string("\\")+argv[3]);
    }):content.weapons.find(cache.slayer_starting_weapon());
    check(selected!=content.weapons.end(),"Requested FOV weapon is absent from the actual cache");
    const auto &weapon=selected->second;
    const auto gun=cache.load_model(weapon.first_person_model,true);
    const auto hands=cache.load_model(content.first_person_hands,true);
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
    auto textured=foreground;
    for(std::size_t i=0;i<textured.size();++i)
        textured[i].material+=1+(i<pose.arms.size()?0:unsigned(hands.mesh.materials.size()));
    for(std::size_t i=0;i<foreground.size();++i) {
        auto &v=foreground[i];v.material=0;v.color=i<pose.arms.size()?Vec3{1,.25f,.1f}:Vec3{.1f,.6f,1};
    }
    Mesh mesh;mesh.vertices.resize(3);mesh.materials.emplace_back();
    mesh.materials.insert(mesh.materials.end(),hands.mesh.materials.begin(),hands.mesh.materials.end());
    mesh.materials.insert(mesh.materials.end(),gun.mesh.materials.begin(),gun.mesh.materials.end());
    check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    auto *window=SDL_CreateWindow("MCC first-person FOV framing",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
    check(window,SDL_GetError());
    {
        Renderer renderer(window,mesh,true);renderer.scene_clear_color({.025f,.035f,.045f});
        for(const auto dimensions:std::array<std::array<int,2>,3>{{{640,480},{1280,720},{2560,720}}}) {
            SDL_SetWindowSize(window,dimensions[0],dimensions[1]);SDL_PumpEvents();renderer.resized();
            const float aspect=float(dimensions[0])/dimensions[1];
            float foreground_angle=gameplay_first_person_projection_angle;
            const auto suffix=std::to_string(dimensions[0])+"x"+std::to_string(dimensions[1]);
            const auto capture=[&](unsigned fov,bool independent,bool empty=false,bool source_materials=false) {
                Camera camera;camera.oriented=true;camera.forward={1,0,0};camera.up={0,0,1};
                const float horizontal=gameplay_field_of_view(fov);
                camera.projection_vertical_angle=gameplay_camera_angles(horizontal,horizontal,aspect).projection_angle;
                ModelView view;view.camera=camera;view.width=640;view.height=480;view.full_surface=true;
                if(!empty)view.foreground=source_materials?textured:foreground;
                if(independent)view.foreground_projection_vertical_angle=foreground_angle;
                if(source_materials) {
                    const auto bank=[&](float angle) {
                        SourceFrustumProjectionInput input;input.projection_angle=angle;
                        input.outer=input.safe={0,0,std::int16_t(dimensions[1]),std::int16_t(dimensions[0])};
                        return source_view_projection({{},{1,0,0},{0,0,1},.015625f,1024.f,{},
                            source_frustum_projection(input).projection_program});
                    };
                    SourceViewShaderConstants constants;
                    constants.world=source_view_shader_constants(bank(*camera.projection_vertical_angle),1024.f,false);
                    constants.reflection=constants.world;
                    const auto fp=source_view_shader_constants(bank(foreground_angle),1024.f,true);
                    std::copy_n(fp.begin(),4,constants.foreground.begin());view.source_view_shader_constants=constants;
                }
                const auto path=output/(suffix+"-"+std::to_string(fov)+(source_materials?"-source":empty?"-empty":independent?"-mcc":"-world-lens")+".ppm");
                for(unsigned attempt=0;attempt<4;++attempt)
                    if(renderer.draw(camera,path,{},false,{},{},{},{&view,1}))return pixels(path);
                throw FormatError("First-person FOV capture did not submit");
            };
            const auto baseline=capture(78,true,true),normal=capture(78,true),wide=capture(120,true);
            const auto coupled=capture(120,false);
            check(normal==wide,"World FOV changed the independently projected hands/weapon pixels");
            check(normal!=baseline,"First-person framebuffer contains no hands/weapon");
            check(wide!=coupled,"First-person lens control did not change real projected geometry");
            const auto source_normal=capture(78,true,false,true),source_wide=capture(120,true,false,true);
            check(source_normal==source_wide,"World FOV stretched the source-shaded hands/weapon pixels");
            check(source_normal!=baseline,"Source-shaded framebuffer contains no hands/weapon");
            if(argc==5) {
                std::ifstream fixture(argv[4]);check(bool(fixture),"Missing original split lens fixture");
                std::string line;unsigned split_cases{};
                while(std::getline(fixture,line)) {
                    if(line.empty() || line[0]=='#')continue;
                    float authored{},expected{};SourceViewGrid layout;std::istringstream row(line);
                    check(bool(row>>authored>>layout.grid[0]>>layout.grid[1]>>layout.span[0]>>layout.span[1]>>expected),
                        "Invalid original split lens fixture");
                    if(authored>1.3f || layout.grid==std::array<std::int16_t,2>{1,1})continue;
                    foreground_angle=expected;
                    const auto reference=capture(120,true,false,true);
                    foreground_angle=gameplay_first_person_camera_angle(authored,layout);
                    const auto actual=capture(120,true,false,true);
                    check(actual==reference,"Rendered first-person model bypassed the original split lens");
                    check(capture(78,true,false,true)==actual,"World FOV changed split-screen weapon pixels");
                    ++split_cases;
                }
                check(split_cases==5,"Incomplete original split lens framebuffer cohort");
            }
            std::cout<<suffix<<": identical hands/weapon pixels at 78 and 120; world-lens control differs\n";
        }
    }
    SDL_DestroyWindow(window);SDL_Quit();return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
