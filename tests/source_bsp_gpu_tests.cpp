#include "halo2/renderer.hpp"
#include "halo2/source_bsp_admission.hpp"
#include "halo2/source_bsp_shader_scene.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/model.hpp"
#include "halo2/appearance.hpp"
#include "halo2/first_person_ik.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const std::string &message){if(!value)throw FormatError(message);}
struct Image {unsigned width{},height{};std::vector<unsigned char> data;};
Image read(const std::filesystem::path &path) {
    std::ifstream input(path,std::ios::binary);Image result;std::string magic;unsigned maximum;
    input>>magic>>result.width>>result.height>>maximum;input.get();
    require(magic=="P6" && maximum==255,"invalid BSP visibility capture");
    result.data.resize(std::size_t(result.width)*result.height*3);
    input.read(reinterpret_cast<char*>(result.data.data()),std::streamsize(result.data.size()));
    require(bool(input),"short BSP visibility capture");return result;
}
struct Difference {unsigned maximum{};std::size_t changed{};};
Difference difference(const Image &a,const Image &b,unsigned view,unsigned count=1) {
    require(a.width==b.width && a.height==b.height,"BSP capture extent changed");Difference result;
    for(unsigned y=0;y<a.height;++y)for(unsigned x=view*a.width/2;x<(view+count)*a.width/2;++x)
        for(unsigned channel=0;channel<3;++channel) {
            const auto index=(std::size_t(y)*a.width+x)*3+channel;
            const auto delta=unsigned(std::abs(int(a.data[index])-int(b.data[index])));
            result.maximum=std::max(result.maximum,delta);result.changed+=delta>1;
        }
    return result;
}
using Window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>;
Window window(int width,int height) {
    Window result(SDL_CreateWindow("Source BSP per-view visibility",0,0,width,height,
        SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN|SDL_WINDOW_RESIZABLE),&SDL_DestroyWindow);
    require(bool(result),SDL_GetError());return result;
}
Image capture(Renderer &renderer,std::span<const ModelView> views,const std::filesystem::path &path) {
    for(unsigned attempt=0;attempt<5;++attempt)
        if(renderer.draw(views.front().camera,path,{},false,{},{},{},views))return read(path);
    throw FormatError("BSP visibility frame did not submit");
}
void verify(const std::filesystem::path &map,const std::filesystem::path &output) {
    Cache cache(map);auto mesh=cache.load_bsp(0);const auto atmosphere=cache.load_bsp_atmosphere(0);
    const SourceBspShaderScene bsp_shaders(cache,mesh,0);
    std::map<std::pair<int,unsigned>,unsigned> highest_materials;
    for(const auto &vertex:mesh.vertices)
        highest_materials.emplace(std::pair{vertex.bsp_cluster,vertex.source_part},vertex.material);
    const auto portals=cache.load_source_portal_bsp(0);
    const auto bsp=cache.load_source_bsp_admission(0);
    const auto content=cache.load_content(0,true);
    const auto water=cache.load_bsp_water(0);
    for(unsigned definition=0;definition<water.definitions.size();++definition) {
        const auto datum=water.definitions[definition].shader;
        const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),
            [&](const auto &value){return value.datum==datum;});
        for(auto &material:mesh.materials)if(material.water_pass) {
            if(tag!=cache.tags().end() && material.name==tag->name)material.water_definition=int(definition);
            if(material.forward_vertex==7)material.water_definition=0;
        }
    }
    const auto spawns=cache.spawns();require(!spawns.empty(),"BSP visibility fixture has no authored spawn");
    prepare_bsp_planar_fog(cache,mesh,atmosphere);
    Camera camera;camera.oriented=true;
    camera.position=spawns.front().position+Vec3{0,0,content.player.camera_height};
    const auto forward=direction(spawns.front().facing);
    const auto aim=source_observer_aim(forward,{0,0,1},content.player.camera_field_of_view,content.player_screen_offset);
    camera.forward=aim.forward;camera.up=aim.up;
    const float angle=source_camera_angles(content.player.camera_field_of_view).projection_angle;
    camera.field_of_view=angle*57.295779513f;
    const auto fog=atmosphere.sample(camera.position,camera.forward);
    require(fog.cluster>=0,"authored BSP test eye has no containing source cluster");
    const auto &weapon=content.weapons.at(cache.slayer_starting_weapon(5));
    auto gun=cache.load_model(weapon.first_person_model),hands=cache.load_model(content.first_person_hands);
    const auto prepare=[&](RenderModel &model,const ObjectColors &colors) {
        for(auto &material:model.mesh.materials) {
            if(!material.lods.empty())require(prepare_dynamic_material(cache,material,{},true),
                "source view test requires actual first-person material programs");
            apply_object_colors(material,colors);
        }
        const auto base=unsigned(mesh.materials.size());
        mesh.materials.insert(mesh.materials.end(),model.mesh.materials.begin(),model.mesh.materials.end());
        return base;
    };
    const auto gun_base=prepare(gun,cache.load_object_colors(weapon.datum));
    const auto hands_base=prepare(hands,cache.load_object_colors(content.player.datum));
    const auto idle=cache.load_action(weapon.first_person_animation,"any","any","any","idle");
    const auto ik=cache.load_first_person_ik(weapon.first_person_animation,
        weapon.first_person_model,content.first_person_hands);
    auto pose=pose_first_person_models_world_frame(gun,hands,idle,0,weapon.first_person_offset,
        camera_transform(camera.position,camera.forward,camera.up),ik,0);
    for(auto &vertex:pose.weapon)vertex.material+=gun_base;
    for(auto &vertex:pose.arms)vertex.material+=hands_base;
    std::vector<Vertex> foreground=std::move(pose.weapon);
    foreground.insert(foreground.end(),pose.arms.begin(),pose.arms.end());
    std::array<std::uint32_t,16> open;open.fill(0xffffffffu);
    auto reused_window=window(640,480);Renderer reused(reused_window.get(),mesh,true);
    reused.scene_bsp_atmosphere(atmosphere);
    const std::array<std::array<int,2>,3> sizes{{{640,480},{1280,720},{640,480}}};
    for(unsigned step=0;step<sizes.size();++step) {
        const auto [width,height]=sizes[step];
        SDL_SetWindowSize(reused_window.get(),width,height);SDL_PumpEvents();reused.resized();
        std::array<SourceBspAdmission,2> admissions;
        std::array<std::vector<Vertex>,2> worlds;
        std::array<std::vector<WaterFrame>,2> water_frames;
        std::array<ModelView,2> views;
        for(unsigned view=0;view<2;++view) {
            const RenderViewport outer{float(view*width/2),0,float(width/2),float(height)};
            const auto projection=source_camera_projection(angle,outer,outer,false);
            SourceFrustumProjectionInput input;input.projection_angle=angle;
            input.outer={0,std::int16_t(view*width/2),std::int16_t(height),std::int16_t((view+1)*width/2)};
            input.safe=input.outer;input.scenario_present=true;input.scenario_type=cache.load_source_geometry_scenario_type();
            const auto program=source_frustum_projection(input);
            const auto source_camera=source_frustum_camera({camera.position,camera.forward,camera.up,
                .015625f,1024.f,program.clipping_rectangle()});
            const auto visibility=source_portal_visibility(portals,std::span{&source_camera,1},fog.cluster,open);
            const auto contains=source_bsp_fog_admission(atmosphere,{true,true,fog.planar_datum,0});
            SourceBspAdmissionInput admission_input;admission_input.containing_fog=contains;
            admissions[view]=source_bsp_admission(bsp,visibility,admission_input);
            require(!admissions[view].clusters.empty(),"source BSP fixture admitted no cluster geometry");
            auto &v=views[view];v.camera=camera;
            v.camera.projection_clip_scale=projection.clip_scale;v.camera.projection_clip_offset=projection.clip_offset;
            v.left=float(view*320);v.width=320;v.height=480;v.full_surface=true;
            v.atmosphere=fog;v.atmosphere_source=&atmosphere;
            v.source_bsp_admission=&admissions[view];
            const auto render=source_view_projection({camera.position,camera.forward,camera.up,
                .015625f,1024.f,{},program.projection_program});
            worlds[view]=mesh.vertices;
            const auto selected=bsp_shaders.select(worlds[view],{render,admissions[view],
                0,std::int16_t(height),2,program.view_modifier});
            v.vertices=worlds[view];
            std::cout<<map.stem().string()<<" source BSP shader view"<<view<<": "
                <<std::count_if(selected.begin(),selected.end(),[](const auto &part){return part.shader_lod>0;})
                <<" lower shader LOD parts\n";
            SourceViewShaderConstants shader_constants;
            shader_constants.world=source_view_shader_constants(render,1024.f,false);
            const auto fp=source_view_shader_constants(render,1024.f,true);
            std::copy_n(fp.begin(),4,shader_constants.foreground.begin());
            shader_constants.reflection=shader_constants.world;
            for(const auto &definition:water.definitions)
                water_frames[view].push_back(water_frame(definition,camera.position,
                    camera.forward,angle,320.f/480.f,width*3>height*4));
            v.water=water_frames[view];v.water_reflection_camera=v.camera;
            if(!water.definitions.empty()) {
                v.water_reflection_camera.position.z=2.f*water.definitions.front().parameters[33]-camera.position.z;
                v.water_reflection_camera.forward={camera.forward.x,camera.forward.y,-camera.forward.z};
                v.water_reflection_camera.up={0,0,-1};
                const auto &reflection=v.water_reflection_camera;
                const auto reflected=source_view_projection({reflection.position,reflection.forward,reflection.up,
                    .015625f,1024.f,{},program.projection_program});
                shader_constants.reflection=source_view_shader_constants(reflected,1024.f,false);
            }
            v.source_view_shader_constants=shader_constants;
        }
        const auto stem=map.stem().string()+"-bsp-"+std::to_string(step)+"-"+std::to_string(width)+"x"+std::to_string(height);
        const auto baseline=capture(reused,views,output/(stem+"-source.ppm"));
        auto fresh_window=window(width,height);Renderer fresh(fresh_window.get(),mesh,true);
        fresh.scene_bsp_atmosphere(atmosphere);
        require(difference(baseline,capture(fresh,views,output/(stem+"-fresh.ppm")),0,2).maximum<=1,
            "source BSP queues changed across fresh/reused resize");
        for(unsigned view=0;view<2;++view) {
            auto highest=worlds[view];bool changed_binding=false;
            for(auto &vertex:highest) {
                const auto root=highest_materials.at({vertex.bsp_cluster,vertex.source_part});
                changed_binding|=root!=vertex.material;vertex.material=root;
            }
            require(changed_binding,"actual source shader fixture has no selected lower LOD");
            views[view].vertices=highest;
            const auto control=capture(reused,views,output/(stem+"-highest-shader"+std::to_string(view)+".ppm"));
            const auto target=difference(baseline,control,view),other=difference(baseline,control,1-view);
            require(target.changed>0,"selected lower source shader banks have no GPU response");
            require(other.maximum<=1,"selected shader LOD leaked into the other player view");
            views[view].vertices=worlds[view];
            require(difference(baseline,capture(reused,views,output/(stem+"-shader-restored"+std::to_string(view)+".ppm")),0,2).maximum<=1,
                "source shader selection did not restore");
            std::cout<<stem<<" shader view"<<view<<": "<<target.changed
                <<" changed bytes, other view "<<other.maximum<<'\n';
        }
        for(unsigned view=0;view<2;++view) {
            const auto retained=admissions[view];admissions[view].clusters.clear();admissions[view].instances.clear();
            const auto control=capture(reused,views,output/(stem+"-empty"+std::to_string(view)+".ppm"));
            const auto target=difference(baseline,control,view),other=difference(baseline,control,1-view);
            require(target.changed>500,"source BSP admission has no measured renderer effect");
            require(other.maximum<=1,"source BSP queue leaked into the other player view");
            admissions[view]=retained;
            require(difference(baseline,capture(reused,views,output/(stem+"-restored"+std::to_string(view)+".ppm")),0,2).maximum<=1,
                "source BSP queues did not restore after a per-view control");
            std::cout<<stem<<" view"<<view<<": "<<retained.clusters.size()<<" clusters, "
                <<retained.instances.size()<<" instances; "<<target.changed<<" changed bytes, other view "<<other.maximum<<'\n';
        }
        for(unsigned view=0;view<2;++view) {
            const auto retained=*views[view].source_view_shader_constants;
            views[view].source_view_shader_constants->world[0][3]+=4096.f;
            const auto control=capture(reused,views,output/(stem+"-camera"+std::to_string(view)+".ppm"));
            const auto target=difference(baseline,control,view),other=difference(baseline,control,1-view);
            require(target.changed>500,"source world c0 bank has no measured GPU response");
            require(other.maximum<=1,"source world camera bank leaked into the other view");
            views[view].source_view_shader_constants=retained;
            require(difference(baseline,capture(reused,views,output/(stem+"-camera-restored"+std::to_string(view)+".ppm")),0,2).maximum<=1,
                "source camera bank did not restore");
        }
        for(auto &view:views)view.foreground=foreground;
        const auto first_person=capture(reused,views,output/(stem+"-first-person.ppm"));
        for(unsigned view=0;view<2;++view) {
            const auto retained=*views[view].source_view_shader_constants;
            views[view].source_view_shader_constants->foreground[0][3]+=512.f;
            const auto control=capture(reused,views,output/(stem+"-fp-camera"+std::to_string(view)+".ppm"));
            const auto target=difference(first_person,control,view),other=difference(first_person,control,1-view);
            require(target.changed>500,"actual arms/weapon do not consume the original first-person c0 bank");
            require(other.maximum<=1,"source first-person camera bank leaked into the other view");
            views[view].source_view_shader_constants=retained;
            require(difference(first_person,capture(reused,views,output/(stem+"-fp-restored"+std::to_string(view)+".ppm")),0,2).maximum<=1,
                "source first-person camera bank did not restore");
            std::cout<<stem<<" first-person view"<<view<<": "<<target.changed<<" changed bytes, other view "<<other.maximum<<'\n';
        }
    }
}
}
int main(int argc,char **argv){try {
    require(argc==2,"expected BSP GPU capture directory");
#if defined(__linux__)
    if(!std::getenv("DISPLAY") && !std::getenv("WAYLAND_DISPLAY"))return 77;
    setenv("VK_LAYER_VALIDATE_SYNC","1",1);
#endif
    require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());const std::filesystem::path output=argv[1];
    std::filesystem::create_directories(output);std::ostringstream diagnostics;auto *saved=std::cerr.rdbuf(diagnostics.rdbuf());
    try {
        verify("assets/disc/maps/midship.map",output);
        verify("assets/map-pack/maps/gemini.map",output);
    } catch(...) {std::cerr.rdbuf(saved);std::cerr<<diagnostics.str();throw;}
    std::cerr.rdbuf(saved);SDL_Quit();
    require(diagnostics.str().find("Vulkan validation:")==std::string::npos,
        "BSP Vulkan/synchronization diagnostics:\n"+diagnostics.str());return 0;
} catch(const std::exception &error){std::cerr<<error.what()<<'\n';SDL_Quit();return 1;}}
