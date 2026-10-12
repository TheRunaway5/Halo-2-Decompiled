#include "halo2/source_scenery_scene.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/content.hpp"
#include "halo2/collision.hpp"
#include "halo2/renderer.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const std::string &message){if(!value)throw FormatError(message);}
struct Image {unsigned width{},height{};std::vector<unsigned char> data;};
Image read(const std::filesystem::path &path) {
    std::ifstream f(path,std::ios::binary);std::string magic;unsigned maximum;Image r;
    f>>magic>>r.width>>r.height>>maximum;f.get();require(magic=="P6" && maximum==255,"invalid SCEN GPU image");
    r.data.resize(std::size_t(r.width)*r.height*3);f.read(reinterpret_cast<char*>(r.data.data()),std::streamsize(r.data.size()));
    require(bool(f),"truncated SCEN GPU image");return r;
}
struct Difference {unsigned maximum{};std::size_t changed{};};
Difference difference(const Image &a,const Image &b,unsigned view=0,unsigned count=2) {
    require(a.width==b.width && a.height==b.height,"SCEN GPU extent changed");Difference r;
    for(unsigned y=0;y<a.height;++y)for(unsigned x=view*a.width/2;x<(view+count)*a.width/2;++x)
        for(unsigned c=0;c<3;++c){const auto i=(std::size_t(y)*a.width+x)*3+c;
            const auto d=unsigned(std::abs(int(a.data[i])-int(b.data[i])));r.maximum=std::max(r.maximum,d);r.changed+=d>1;}
    return r;
}
using Window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>;
Window window(int width,int height) {
    Window r(SDL_CreateWindow("Original scenery section lighting",0,0,width,height,
        SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN),&SDL_DestroyWindow);
    require(bool(r),SDL_GetError());return r;
}
Image capture(Renderer &renderer,std::span<const ModelView> views,const std::filesystem::path &path) {
    for(unsigned i=0;i<5;++i)if(renderer.draw(views.front().camera,path,{},false,{},{},{},views))return read(path);
    throw FormatError("SCEN GPU frame did not submit");
}
void verify(const std::filesystem::path &output) {
    Cache cache("assets/disc/maps/headlong.map");const auto content=cache.load_content(0,true);
    const auto found=std::find_if(content.objects.begin(),content.objects.end(),[](const auto &o){return o.type==6 && o.placement==3;});
    require(found!=content.objects.end() && found->definition==0xe1ec0077u,"actual Headlong SCEN fixture missing");
    LevelContent one;one.objects={*found};one.object_names=content.object_names;
    CollisionWorld collision(content.collision);Mesh mesh;SourceSceneryScene scene(cache,one.objects,0,mesh,collision);
    require(scene.stats().baked_models==1,"SCEN GPU fixture requires authored pervertex lighting");
    SceneObjects objects(one,0);scene.synchronize(objects);
    scene.lighting_query_state([&](std::size_t index,std::uint32_t handle,const Transform &node,float scale) {
        require(index==0 && objects.get(handle).placement==0,"SCEN GPU cached sphere lost source identity");
        const auto sphere=source_shadow_sphere(cache.load_object_shadow_definition(found->definition),node,scale);
        return SourceSceneryQueryState{sphere.center,sphere.radius,{},0,true};
    });scene.update_pose(0,0,mesh);
    std::array<std::vector<Vertex>,2> geometry,zero;
    for(unsigned i=0;i<2;++i) {
        SourceObjectViewLod view{i,1,false,[=](std::size_t,std::uint32_t,Vec3,float) {
            return SourceObjectLodInput{i?100000.f:0.f,0,0};}};
        std::vector<ObjectLightingBinding> bindings;std::vector<SourceObjectViewSelection> selected;
        scene.append_view(geometry[i],bindings,view,&selected);
        require(bindings.empty() && selected.size()==1 && selected[0].lod==(i?0u:4u),"SCEN GPU collector/classification changed");
        zero[i]=geometry[i];for(auto &v:zero[i]){v.color={};v.incident={};}
    }
    require(!geometry[0].empty() && !geometry[1].empty() && geometry[0].size()!=geometry[1].size(),
        "actual SCEN low/high source streams are not distinct");
    Vec3 lo{INFINITY,INFINITY,INFINITY},hi{-INFINITY,-INFINITY,-INFINITY};
    for(const auto &v:geometry[0]){lo=minimum(lo,v.position);hi=maximum(hi,v.position);}
    Camera camera;camera.oriented=true;const auto center=(lo+hi)*.5f;
    camera.position=center+normalized(Vec3{-1,-1,.3f})*length(hi-lo)*1.2f;
    camera.forward=normalized(center-camera.position);camera.up=normalized(cross(camera.forward,normalized(cross(Vec3{0,0,1},camera.forward))));
    auto reused_window=window(640,480);Renderer reused(reused_window.get(),mesh,true);
    const std::array<std::array<int,2>,3> sizes{{{640,480},{1280,720},{640,480}}};
    for(unsigned step=0;step<sizes.size();++step) {
        const auto [width,height]=sizes[step];SDL_SetWindowSize(reused_window.get(),width,height);SDL_PumpEvents();reused.resized();
        std::array<ModelView,2> views;for(unsigned i=0;i<2;++i){views[i].camera=camera;views[i].left=float(i*320);
            views[i].width=320;views[i].height=480;views[i].full_surface=true;views[i].vertices=geometry[i];}
        const auto stem="headlong-scenery-"+std::to_string(step)+"-"+std::to_string(width)+"x"+std::to_string(height);
        const auto baseline=capture(reused,views,output/(stem+"-baked.ppm"));
        auto fresh_window=window(width,height);Renderer fresh(fresh_window.get(),mesh,true);
        require(difference(baseline,capture(fresh,views,output/(stem+"-fresh.ppm"))).maximum<=1,
            "SCEN baked streams changed across fresh/reused resize");
        for(unsigned changed=0;changed<2;++changed)for(unsigned kind=0;kind<2;++kind) {
            views[changed].vertices=kind?zero[changed]:geometry[1-changed];
            const auto control=capture(reused,views,output/(stem+"-control"+std::to_string(changed)+"-"+std::to_string(kind)+".ppm"));
            const auto response=difference(baseline,control,changed,1),other=difference(baseline,control,1-changed,1);
            require(response.changed>100,kind?"authored SCEN incident/RGB has no measured GPU response":"source SCEN LOD geometry has no measured GPU response");
            require(other.maximum<=1,"SCEN per-view geometry/light streams leaked between players");
            views[changed].vertices=geometry[changed];
            require(difference(baseline,capture(reused,views,output/(stem+"-restored"+std::to_string(changed)+"-"+std::to_string(kind)+".ppm"))).maximum<=1,
                "SCEN authored streams did not restore");
            std::cout<<stem<<" view"<<changed<<(kind?" baked":" LOD")<<": "<<response.changed
                <<" changed bytes; max "<<response.maximum<<", other view "<<other.maximum<<'\n'<<std::flush;
        }
    }
    std::cout<<"Actual authored SCEN streams: "<<geometry[0].size()<<" LOD4 / "<<geometry[1].size()<<" LOD0 vertices\n";
}
}
int main(int argc,char **argv){try {
    require(argc==2,"expected SCEN GPU capture directory");
#if defined(__linux__)
    if(!std::getenv("DISPLAY") && !std::getenv("WAYLAND_DISPLAY"))return 77;
    setenv("VK_LAYER_VALIDATE_SYNC","1",1);
#endif
    require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());const std::filesystem::path output=argv[1];
    std::filesystem::create_directories(output);std::ostringstream diagnostics;auto *saved=std::cerr.rdbuf(diagnostics.rdbuf());
    try{verify(output);}catch(...){std::cerr.rdbuf(saved);std::cerr<<diagnostics.str();throw;}
    std::cerr.rdbuf(saved);SDL_Quit();require(diagnostics.str().find("Vulkan validation:")==std::string::npos,
        "SCEN Vulkan/synchronization diagnostics:\n"+diagnostics.str());return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';SDL_Quit();return 1;}}
