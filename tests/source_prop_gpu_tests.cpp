#include "halo2/source_prop_scene.hpp"
#include "halo2/source_prop_initial_state.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/content.hpp"
#include "halo2/collision.hpp"
#include "halo2/renderer.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
using namespace halo2;
void require(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
struct Image {unsigned width{},height{};std::vector<unsigned char> pixels;};
Image read(const std::filesystem::path& path){
    std::ifstream file(path,std::ios::binary);std::string magic;unsigned maximum;Image image;
    file>>magic>>image.width>>image.height>>maximum;file.get();
    require(magic=="P6" && maximum==255,"invalid prop GPU capture");
    image.pixels.resize(std::size_t(image.width)*image.height*3);
    file.read(reinterpret_cast<char*>(image.pixels.data()),std::streamsize(image.pixels.size()));
    require(bool(file),"truncated prop GPU capture");return image;
}
struct Difference {unsigned maximum{};std::size_t changed{};};
Difference difference(const Image& a,const Image& b,unsigned first_half=0,unsigned halves=2){
    require(a.width==b.width && a.height==b.height,"prop GPU capture extent changed");Difference result;
    for(unsigned y=0;y<a.height;++y)for(unsigned x=first_half*a.width/2;x<(first_half+halves)*a.width/2;++x)
        for(unsigned channel=0;channel<3;++channel){const auto i=(std::size_t(y)*a.width+x)*3+channel;
            const auto error=unsigned(std::abs(int(a.pixels[i])-int(b.pixels[i])));
            result.maximum=std::max(result.maximum,error);result.changed+=error>1;}
    return result;
}
struct ValidationLog {
    std::ostringstream output;std::streambuf* previous=std::cerr.rdbuf(output.rdbuf());
    ~ValidationLog(){std::cerr.rdbuf(previous);if(!output.str().empty())std::cerr<<output.str();}
};
using Window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>;
Window make_window(int width,int height){
    Window result(SDL_CreateWindow("Source prop light bank isolation",0,0,width,height,
        SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN),&SDL_DestroyWindow);
    require(bool(result),SDL_GetError());return result;
}
bool consumes_object_lighting(const Material& material){
    if(material.environment_pass && consumes_object_lighting(*material.environment_pass))return true;
    if(!material.forward_pass || material.forward_pass_index>=material.passes.size())return false;
    const auto& pass=material.passes[material.forward_pass_index];
    const auto lighting=[](unsigned source){return source>=14 && source<=16;};
    return std::any_of(pass.vertex_externs.begin(),pass.vertex_externs.end(),[&](const auto& r){return lighting(r.source);}) ||
           std::any_of(pass.pixel_externs.begin(),pass.pixel_externs.end(),[&](const auto& r){return lighting(r.source);});
}
Image capture(Renderer& renderer,std::span<const ModelView> views,const std::filesystem::path& path){
    for(unsigned attempt=0;attempt<5;++attempt)
        if(renderer.draw(views.front().camera,path,{},false,{},{},{},views))return read(path);
    throw std::runtime_error("source prop GPU frame did not submit");
}
void verify(SDL_Window* window,const std::filesystem::path& maps,const std::filesystem::path& output){
    const Cache cache(maps/"headlong.map");const auto content=cache.load_content(0,true);
    CollisionWorld collision(content.collision);Mesh mesh;SourcePropScene props(cache,content.objects,0,mesh,collision);
    SceneObjects objects(content,0);props.synchronize(objects);SourcePropInitialState initial;
    props.lighting_query_state([&](std::size_t index,std::uint32_t handle,const Transform& node,float scale){
        const auto definition=content.objects.at(index).definition;
        if(!initial.contains(handle))initial.capture(handle,definition,cache.load_object_shadow_definition(definition),node,scale);
        const auto state=initial.retained(handle,definition,node,scale,false);
        return SourcePropQueryState{{state.sphere.center,state.sphere.radius},state.velocity,state.query_runtime_flags,state.frozen};
    });
    // These are the complete original constructor-query banks for the actual
    // authored frozen initial state, not synthetic directional light values.
    props.update(0,{},1,mesh);std::vector<Vertex> world;std::vector<ObjectLightingBinding> owners;
    props.append(world,owners);require(!world.empty() && !owners.empty(),"ordinary prop fixture is empty");
    unsigned material=0;std::size_t most=0;
    for(unsigned i=0;i<mesh.materials.size();++i)if(consumes_object_lighting(mesh.materials[i])){
        const auto count=std::count_if(world.begin(),world.end(),[&](const auto& v){return v.material==i;});
        if(std::size_t(count)>most){material=i;most=std::size_t(count);}}
    require(most>100,"no resident ordinary prop material consumes source lighting");
    const auto owner=std::find_if(owners.begin(),owners.end(),[&](const auto& o){
        return material>=o.first_material && material-o.first_material<o.material_count;});
    require(owner!=owners.end(),"resident prop material lost its object light bank");
    std::vector<Vertex> geometry;Vec3 lower{INFINITY,INFINITY,INFINITY},upper{-INFINITY,-INFINITY,-INFINITY};
    for(const auto& v:world)if(v.material==material){geometry.push_back(v);lower=minimum(lower,v.position);upper=maximum(upper,v.position);}
    Camera camera;camera.oriented=true;const auto target=(lower+upper)*.5f;
    camera.position=target+normalized(Vec3{-1,-1,.4f})*length(upper-lower)*1.3f;
    camera.forward=normalized(target-camera.position);
    camera.up=normalized(cross(camera.forward,normalized(cross(Vec3{0,0,1},camera.forward))));
    std::array<std::vector<Vertex>,2> view_geometry;
    for(unsigned i=0;i<2;++i){
        SourceObjectViewLod lod;lod.view_index=i;lod.local_players=2;
        lod.input=[i](std::size_t,std::uint32_t handle,Vec3 center,float radius){
            require(handle!=null_datum && std::isfinite(center.x) && radius>=0,"prop GPU view lost source identity/sphere");
            //Controlled original distance inputs exercise real authoredLOD3
            //andLOD0 streams. They are probe inputs, not a live camera claim.
            return SourceObjectLodInput{i==0?7.f:100.f,0,0};
        };
        std::vector<Vertex> submitted;std::vector<ObjectLightingBinding> lighting;
        props.append_view(submitted,lighting,lod);
        for(const auto&v:submitted)if(v.material==material)view_geometry[i].push_back(v);
    }
    require(view_geometry[0].size()>100 && view_geometry[1].size()>100 &&
        view_geometry[0].size()!=view_geometry[1].size(),"prop GPU probe did not select distinct real LOD streams");
    std::array<ObjectLightingBinding,2> light{*owner,*owner};
    // A deliberate zero-light negative control keeps source geometry, authored
    // textures/colors, camera and pass programs unchanged. It is not a live
    // object state or an expected original framebuffer.
    auto zero=*owner;zero.state={};
    SDL_SetWindowSize(window,640,480);SDL_PumpEvents();Renderer reused(window,mesh,true);
    const std::array<std::array<int,2>,3> extents{{{640,480},{1280,720},{640,480}}};
    for(unsigned step=0;step<extents.size();++step){const auto[width,height]=extents[step];
        SDL_SetWindowSize(window,width,height);SDL_PumpEvents();reused.resized();reused.update_material_constants(mesh.materials);
        std::array<ModelView,2> views;
        for(unsigned i=0;i<2;++i){const auto grid=source_view_grid(2,i,false);
            const auto angles=source_camera_angles(content.player.camera_field_of_view,1.3333333730697632f,grid.grid,grid.span);
            auto& view=views[i];view.camera=camera;view.camera.projection_vertical_angle=angles.projection_angle;
            view.camera.field_of_view=angles.projection_angle*57.295779513f;
            view.left=float(i*320);view.width=320;view.height=480;view.full_surface=true;view.vertices=view_geometry[i];
            const RenderViewport outer{float(i*width/2),0,float(width/2),float(height)};
            const RenderViewport safe{outer.left+outer.width*.075f,outer.height*.075f,outer.width*.85f,outer.height*.85f};
            const auto projection=source_camera_projection(angles.projection_angle,outer,safe,false);
            view.camera.projection_clip_scale=projection.clip_scale;view.camera.projection_clip_offset=projection.clip_offset;
            view.object_lighting=std::span(&light[i],1);}
        const auto stem="headlong-prop-step"+std::to_string(step)+"-"+std::to_string(width)+"x"+std::to_string(height);
        const auto baseline=capture(reused,views,output/(stem+"-source-light.ppm"));
        auto fresh_window=make_window(width,height);Renderer fresh(fresh_window.get(),mesh,true);
        const auto expected=capture(fresh,views,output/(stem+"-fresh.ppm"));
        require(difference(baseline,expected).maximum<=1,"prop source light bank changed after renderer reuse/resize");
        require(baseline.width==unsigned(width) && baseline.height==unsigned(height),"prop stale resize extent");
        for(unsigned changed=0;changed<2;++changed){
            views[changed].vertices=view_geometry[1-changed];
            const auto control=capture(reused,views,output/(stem+"-wrong-lod"+std::to_string(changed)+".ppm"));
            const auto response=difference(baseline,control,changed,1);
            const auto isolated=difference(baseline,control,1-changed,1);
            require(response.changed>100,"distinct source prop LOD streams had no measured GPU response");
            require(isolated.maximum<=1,"per-view prop geometry selection leaked into another player");
            views[changed].vertices=view_geometry[changed];
            const auto restored=capture(reused,views,output/(stem+"-restored-lod"+std::to_string(changed)+".ppm"));
            require(difference(baseline,restored).maximum<=1,"prop source LOD geometry did not restore");
            std::cout<<stem<<" LOD"<<changed<<": "<<response.changed<<" changed bytes, maximum "<<response.maximum
                <<"; other-view maximum "<<isolated.maximum<<'\n'<<std::flush;
        }
        for(unsigned changed=0;changed<2;++changed){light[changed]=zero;
            const auto control=capture(reused,views,output/(stem+"-zero-owner"+std::to_string(changed)+".ppm"));
            const auto response=difference(baseline,control,changed,1);
            const auto isolated=difference(baseline,control,1-changed,1);
            require(response.changed>100,"source prop ordinary light bank has no measured GPU response");
            require(isolated.maximum<=1,"ordinary prop light update leaked into the other player view");
            light[changed]=*owner;
            const auto restored=capture(reused,views,output/(stem+"-restored-owner"+std::to_string(changed)+".ppm"));
            require(difference(baseline,restored).maximum<=1,"source prop light bank did not restore after negative control");
            std::cout<<stem<<" owner"<<changed<<": "<<response.changed<<" changed bytes, maximum "<<response.maximum
                <<"; other-view maximum "<<isolated.maximum<<'\n'<<std::flush;}
    }
    std::cout<<"Actual resident material: "<<mesh.materials[material].name<<" ("<<geometry.size()<<" sourceLOD5 vertices; selected "<<view_geometry[0].size()<<"/"<<view_geometry[1].size()<<")\n";
}
}
int main(int argc,char** argv){try{
    require(argc==3,"expected retail map directory and prop capture directory");
#if defined(__linux__)
    if(!std::getenv("DISPLAY") && !std::getenv("WAYLAND_DISPLAY"))return 77;
    setenv("VK_LAYER_VALIDATE_SYNC","1",1);
#endif
    require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    const std::filesystem::path output=argv[2];std::filesystem::create_directories(output);
    std::string validation;{ValidationLog log;auto window=make_window(640,480);verify(window.get(),argv[1],output);
        window.reset();validation=log.output.str();}
    SDL_Quit();require(validation.find("Vulkan validation:")==std::string::npos,
        "Vulkan/synchronization validation reported:\n"+validation);return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';SDL_Quit();return 1;}}
