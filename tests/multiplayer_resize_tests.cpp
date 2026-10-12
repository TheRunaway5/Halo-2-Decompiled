#include "halo2/bsp_postprocess.hpp"
#include "halo2/hud.hpp"
#include "halo2/math.hpp"
#include "halo2/model.hpp"
#include "halo2/renderer.hpp"
#include "halo2/sky.hpp"
#include "halo2/source_machine_scene.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/collision.hpp"
#include <SDL_vulkan.h>
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
using namespace halo2;
void require(bool condition,const std::string &message) {
    if(!condition)throw std::runtime_error(message);
}
struct Image {unsigned width{},height{};std::vector<unsigned char> pixels;};
Image read(const std::filesystem::path &path) {
    std::ifstream file(path,std::ios::binary);std::string magic;unsigned maximum{};
    Image image;file>>magic>>image.width>>image.height>>maximum;file.get();
    require(magic=="P6" && maximum==255,"invalid framebuffer capture: "+path.string());
    image.pixels.resize(std::size_t(image.width)*image.height*3);
    file.read(reinterpret_cast<char*>(image.pixels.data()),std::streamsize(image.pixels.size()));
    require(bool(file),"truncated framebuffer capture: "+path.string());return image;
}
// The production callback reports synchronously to stderr. Retain its complete
// warning/error output across both drawing and object destruction, then fail.
struct ValidationLog {
    std::ostringstream output;
    std::streambuf *previous=std::cerr.rdbuf(output.rdbuf());
    ~ValidationLog(){std::cerr.rdbuf(previous);if(!output.str().empty())std::cerr<<output.str();}
};
void append(std::vector<Vertex> &output,std::vector<Vertex> vertices,unsigned base) {
    for(auto &vertex:vertices)vertex.material+=base;
    output.insert(output.end(),vertices.begin(),vertices.end());
}
std::array<Camera,4> cameras(const Cache &cache,const BspAtmosphere &bank,
                             const BipedDefinition &player) {
    const auto starts=cache.spawns();require(starts.size()>=4,"missing authored player starts");
    const auto camera=[&](const Spawn &spawn) {
        Camera value;value.position=spawn.position+Vec3{0,0,player.camera_height};
        value.yaw=spawn.facing;value.field_of_view=player.camera_field_of_view*57.295779513f;
        value.horizontal_field_of_view=true;return value;
    };
    std::array<Camera,4> result;
    for(unsigned i=0;i<4;++i)result[i]=camera(starts[i*starts.size()/4]);
    // Exercise different sky banks where authored starts expose them. All
    // camera positions remain original start records with original eye height.
    const auto first=bank.sample(result[0].position,direction(result[0].yaw,result[0].pitch));
    for(const auto &start:starts) {
        const auto value=camera(start);
        const auto sample=bank.sample(value.position,direction(value.yaw,value.pitch));
        if(sample.visible_sky!=first.visible_sky) {result[1]=value;break;}
    }
    return result;
}
struct Scene {
    struct Sky {SkyDefinition definition;RenderModel model;unsigned material{};};
    std::vector<std::optional<Sky>> skies;
    std::vector<unsigned> water_materials;
    std::vector<Vertex> bsp;
    std::array<std::vector<Vertex>,4> world,background,reflection;
    std::array<std::vector<WaterFrame>,4> water_frames;
    std::array<ModelView,4> views;
    std::array<std::vector<ObjectLightingBinding>,4> lighting;
    SourceMachineScene machines;
    Mesh &material_mesh;
    std::int32_t render_frame{};
    Scene(const Cache &cache,Mesh &mesh,const BspAtmosphere &bank,const BspWater &water,
          const LevelContent &content,const CollisionWorld &collision)
        :machines(cache,content.objects,0,mesh,collision),material_mesh(mesh) {
        prepare_bsp_planar_fog(cache,mesh,bank);bsp=mesh.vertices;
        for(unsigned i=0;i<water.definitions.size();++i) {
            const auto &definition=water.definitions[i];
            const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),
                [&](const auto &value){return value.datum==definition.shader;});
            for(auto &material:mesh.materials) {
                if(tag!=cache.tags().end() && material.water_pass && material.name==tag->name)
                    material.water_definition=int(i);
                if(material.water_pass && material.forward_vertex==7)material.water_definition=0;
            }
            water_materials.push_back(unsigned(mesh.materials.size()));
            for(auto material:definition.geometry.materials) {
                material.water_definition=int(i);mesh.materials.push_back(std::move(material));
            }
        }
        for(unsigned i=0;i<bank.skies.size();++i) {
            const auto definition=cache.load_sky(i);skies.emplace_back();
            if(!definition || definition->model==null_datum)continue;
            auto model=cache.load_model(definition->model);const auto base=unsigned(mesh.materials.size());
            mesh.materials.insert(mesh.materials.end(),model.mesh.materials.begin(),model.mesh.materials.end());
            skies.back()=Sky{*definition,std::move(model),base};
        }
    }
    std::span<const ModelView> prepare(const std::array<Camera,4> &camera,
        const BspAtmosphere &bank,const BspWater &water,unsigned count,unsigned width,unsigned height,float time) {
        // Freeze authored initial device state while checking presentation
        // resizing. Source gears have separate discrete-tick regressions.
        machines.update(0,render_frame++,camera[0].position,1.f,material_mesh);
        for(unsigned i=0;i<count;++i) {
            auto &view=views[i];view={};view.camera=camera[i];view.full_surface=true;
            const auto rectangle=hud_viewport(count,i,std::uint64_t(width)*3>std::uint64_t(height)*4).outer;
            view.left=rectangle.left;view.top=rectangle.top;
            view.width=rectangle.right-rectangle.left;view.height=rectangle.bottom-rectangle.top;
            const auto forward=direction(camera[i].yaw,camera[i].pitch);
            view.atmosphere=bank.sample(camera[i].position,forward);view.atmosphere_source=&bank;
            view.effect_time=time;
            const float aspect=(float(width)*view.width)/(float(height)*view.height)*.75f;
            water_frames[i].clear();world[i]=bsp;background[i].clear();reflection[i].clear();lighting[i].clear();
            for(unsigned d=0;d<water.definitions.size();++d) {
                const auto &definition=water.definitions[d];
                water_frames[i].push_back(water_frame(definition,camera[i].position,forward,
                    camera_vertical_field_of_view(camera[i],aspect),1.f/aspect));
                if(!definition.geometry.materials.empty()) {
                    auto vertices=definition.geometry.vertices;
                    for(auto &vertex:vertices)vertex.position=water_world_position(water_frames[i][d],vertex.position);
                    append(world[i],std::move(vertices),water_materials[d]);
                }
            }
            view.water_reflection_camera=camera[i];
            if(!water.definitions.empty()) {
                view.water_reflection_camera.position.z=2.f*water.definitions.front().parameters[33]-camera[i].position.z;
                view.water_reflection_camera.oriented=true;
                view.water_reflection_camera.forward={forward.x,forward.y,-forward.z};
                view.water_reflection_camera.up={0,0,-1};
            }
            const int sky=view.atmosphere->visible_sky;
            if(sky>=0 && std::size_t(sky)<skies.size() && skies[sky]) {
                const auto &definition=*skies[sky];
                append(background[i],pose_sky(definition.model,definition.definition,camera[i].position),definition.material);
                if(!water.definitions.empty())append(reflection[i],pose_sky(definition.model,definition.definition,
                    view.water_reflection_camera.position),definition.material);
            }
            machines.append(world[i],lighting[i]);
            view.vertices=world[i];view.background=background[i];view.water=water_frames[i];
            view.object_lighting=lighting[i];
            view.water_reflection_background=reflection[i];
        }
        return {views.data(),count};
    }
};
void predict(BspPostprocessState &state,std::span<const ModelView> views,
             const BspAtmosphere &bank,unsigned width,unsigned height) {
    for(unsigned i=0;i<views.size();++i) {
        const auto &view=views[i];const auto &sample=*view.atmosphere;
        if(!sample.patchy)continue;
        const auto definition=bank.patchy_definition(sample.patchy_datum);
        require(definition,"missing patchy source bank");
        const auto &camera=view.camera;const auto f=direction(camera.yaw,camera.pitch);
        const auto right=normalized(cross(f,Vec3{0,0,1}));
        const Transform to_world{1,{right,normalized(cross(right,f)),normalized(f)*-1.f},camera.position};
        // Match the actual integer viewport extent passed by Renderer, then
        // execute the already independently retail-verified motion kernel.
        const auto physical_width=unsigned(view.width*(float(width)/640.f));
        const auto physical_height=unsigned(view.height*(float(height)/480.f));
        const float aspect=float(physical_width)/float(physical_height);
        bsp_patchy_fog_frame(*definition,*sample.patchy,state.motion[i],state.random,
            inverse_transform(to_world),f.z,camera_vertical_field_of_view(camera,aspect),
            float(physical_height)/float(physical_width),
            views.front().effect_time);
    }
}
void same_state(const BspPostprocessState &actual,const BspPostprocessState &expected,const std::string &context) {
    require(actual.random==expected.random,context+": shared patchy RNG changed on resize");
    for(unsigned i=0;i<4;++i) {
        const auto a=std::bit_cast<std::array<std::uint32_t,34>>(actual.motion[i]);
        const auto b=std::bit_cast<std::array<std::uint32_t,34>>(expected.motion[i]);
        for(unsigned word=0;word<a.size();++word)require(a[word]==b[word],context+": patchy state view "+
            std::to_string(i)+" word "+std::to_string(word)+" diverged");
    }
}
unsigned verify(SDL_Window *window,const std::filesystem::path &map,const std::filesystem::path &directory) {
    Cache cache(map);auto mesh=cache.load_bsp(0);const auto bank=cache.load_bsp_atmosphere(0);
    const auto water=cache.load_bsp_water(0);const auto content=cache.load_content(0,true);
    const CollisionWorld collision(content.collision);
    const auto camera=cameras(cache,bank,content.player);Scene scene(cache,mesh,bank,water,content,collision);
    if(map.stem()=="beavercreek")require(!water.definitions.empty(),"Beaver Creek water resize fixture missing");
    if(map.stem()=="gemini")require(bank.skies.size()==2,"Gemini multiple-sky fixture changed");
    Renderer renderer(window,mesh,true);renderer.scene_bsp_atmosphere(bank);
    auto expected=renderer.bsp_postprocess_state();require(expected.has_value(),"BSP postprocess state unavailable");
    unsigned checked=0;
    constexpr std::array<std::array<unsigned,2>,4> dimensions{{{640,480},{1280,720},{3840,2160},{1920,1080}}};
    for(const unsigned count:{1u,2u,4u})for(const auto size:dimensions) {
        SDL_SetWindowSize(window,int(size[0]),int(size[1]));SDL_PumpEvents();renderer.resized();
        int width{},height{};SDL_Vulkan_GetDrawableSize(window,&width,&height);
        require(width==int(size[0]) && height==int(size[1]),"SDL drawable did not accept resize fixture");
        const auto name=map.stem().string()+"-"+std::to_string(count)+"p-"+
            std::to_string(width)+"x"+std::to_string(height);
        const auto path=directory/(name+".ppm");
        const auto views=scene.prepare(camera,bank,water,count,unsigned(width),unsigned(height),1.f+float(checked)*.1f);
        renderer.update_material_constants(mesh.materials);
        bool drawn=false;
        for(unsigned retry=0;retry<4 && !drawn;++retry) {
            const auto before=*expected;predict(*expected,views,bank,unsigned(width),unsigned(height));
            drawn=renderer.draw(camera[0],path,{},false,{},{},{},views);
            const auto actual=renderer.bsp_postprocess_state();require(actual.has_value(),"postprocess disappeared during resize");
            if(!drawn && actual->random==before.random && actual->motion[0].time==before.motion[0].time)*expected=before;
            same_state(*actual,*expected,name);
        }
        require(drawn,"resize frame not submitted: "+name);
        const auto image=read(path);require(image.width==unsigned(width) && image.height==unsigned(height),"stale swapchain readback: "+name);
        for(unsigned i=0;i<count;++i) {
            const auto &view=views[i];const unsigned left=unsigned(view.left*float(width)/640.f),top=unsigned(view.top*float(height)/480.f);
            const unsigned columns=unsigned(view.width*float(width)/640.f),rows=unsigned(view.height*float(height)/480.f);
            unsigned minimum=255,maximum=0;std::size_t lit=0;
            for(unsigned y=top;y<top+rows;++y)for(unsigned x=left;x<left+columns;++x)for(unsigned c=0;c<3;++c) {
                const auto value=image.pixels[(std::size_t(y)*width+x)*3+c];
                minimum=std::min(minimum,unsigned(value));maximum=std::max(maximum,unsigned(value));lit+=value!=0;
            }
            require(lit>300 && maximum>minimum+3,"blank or uniform resized viewport: "+name+" view "+std::to_string(i));
        }
        // Submit again without resize: newly rebound descriptors must also
        // remain valid on a different acquired swapchain image.
        predict(*expected,views,bank,unsigned(width),unsigned(height));
        require(renderer.draw(camera[0],{}, {},false,{},{},{},views),"second resize frame not submitted: "+name);
        same_state(*renderer.bsp_postprocess_state(),*expected,name+" second frame");
        ++checked;std::cout<<name<<": geometry/fog/water/postprocess valid; CPU motion/RNG exact\n"<<std::flush;
    }
    return checked;
}
}
int main(int argc,char **argv) {
    try {
        require(argc==3,"original maps and capture directory required");
#if defined(__linux__)
        if(!std::getenv("DISPLAY") && !std::getenv("WAYLAND_DISPLAY"))return 77;
        setenv("VK_LAYER_VALIDATE_SYNC","1",1);
#endif
        const auto maps=std::filesystem::path(argv[1]),directory=std::filesystem::path(argv[2]);
        const auto pack=maps.parent_path().parent_path()/"map-pack/maps";
        std::filesystem::create_directories(directory);require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
        unsigned checked=0;std::string validation;
        {
            ValidationLog log;
            auto *window=SDL_CreateWindow("Multiplayer resolution regression",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
            require(window,SDL_GetError());
            checked+=verify(window,pack/"backwash.map",directory);
            checked+=verify(window,maps/"beavercreek.map",directory);
            checked+=verify(window,pack/"gemini.map",directory);
            SDL_DestroyWindow(window);validation=log.output.str();
        }
        SDL_Quit();
        require(validation.find("Vulkan validation:")==std::string::npos,"Vulkan validation reported during resize:\n"+validation);
        std::cout<<checked<<" production-renderer map/layout/resolution cases passed with synchronization validation and exact persistent motion/RNG\n";
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';SDL_Quit();return 1;}
}
