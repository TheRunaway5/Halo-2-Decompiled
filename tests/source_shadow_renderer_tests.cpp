#include "halo2/renderer.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/scope_copy.hpp"
#include "scalar_combiners.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const std::string &message){if(!value)throw FormatError(message);}
struct Image {unsigned width{},height{};std::vector<unsigned char> pixels;};
Image read(const std::filesystem::path &path) {
    std::ifstream input(path,std::ios::binary);Image result;std::string magic;unsigned maximum;
    input>>magic>>result.width>>result.height>>maximum;input.get();
    require(magic=="P6" && maximum==255,"invalid shadow renderer capture");
    result.pixels.resize(std::size_t(result.width)*result.height*3);
    input.read(reinterpret_cast<char *>(result.pixels.data()),result.pixels.size());
    require(bool(input),"truncated shadow renderer capture");return result;
}
struct Difference {unsigned maximum{};std::size_t changed{};};
Difference difference(const Image &a,const Image &b,unsigned view=0,unsigned views=1) {
    require(a.width==b.width && a.height==b.height,"shadow renderer capture extent changed");Difference result;
    const auto top=view*a.height/views,bottom=(view+1)*a.height/views;
    for(unsigned y=top;y<bottom;++y)for(unsigned x=0;x<a.width;++x)for(unsigned c=0;c<3;++c) {
        const auto at=(std::size_t(y)*a.width+x)*3+c;
        const unsigned error=std::abs(int(a.pixels[at])-int(b.pixels[at]));
        result.maximum=std::max(result.maximum,error);result.changed+=error>1;
    }
    return result;
}
using Window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>;
Window window(int width,int height) {
    Window result(SDL_CreateWindow("Explicit source shadow renderer services",0,0,width,height,
        SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN),SDL_DestroyWindow);
    require(bool(result),SDL_GetError());return result;
}
std::vector<Vertex> quad(unsigned material,float x,float width,float height,float y=0) {
    std::vector<Vertex> result;
    for(const auto uv:std::array{Vec2{0,0},Vec2{1,0},Vec2{1,1},Vec2{0,0},Vec2{1,1},Vec2{0,1}}) {
        Vertex v;v.position={x,(1-uv.x*2)*width+y,(1-uv.y*2)*height};
        v.normal={-1,0,0};v.tangent={0,-1,0};v.binormal={0,0,-1};
        v.uv=uv;v.color={1,1,1};v.material=material;result.push_back(v);
    }
    return result;
}
std::array<ShaderVector,12> camera_bank(unsigned width,unsigned height,unsigned view,unsigned count) {
    const unsigned top=view*height/count,bottom=(view+1)*height/count;
    SourceFrustumProjectionInput projection;
    projection.projection_angle=1.5707963267948966f;
    projection.outer=projection.safe={std::int16_t(top),0,std::int16_t(bottom),std::int16_t(width)};
    // Explicit ordinary source feature/context inputs; these are controlled
    // camera services, not a reconstructed retained gameplay camera.
    const auto program=source_frustum_projection(projection).projection_program;
    SourceViewProjectionInput input;input.forward={1,0,0};input.up={0,0,1};
    input.near_distance=.05f;input.far_distance=1024;input.projection_program=program;
    return source_view_shader_bank(source_view_projection(input),1024,{},
        {0,float(top),float(width),float(bottom-top)},false);
}
struct View {
    std::vector<Vertex> scene;
    std::array<std::vector<Vertex>,2> casters,receivers;
    std::array<SourceShadowPlanDraw,2> generation,application;
    std::array<SourceShadowPlanRoot,2> roots;
    SourceShadowPlan plan;
    ModelView model;
};
Image capture(Renderer &renderer,std::span<const ModelView> views,const std::filesystem::path &path) {
    for(unsigned attempt=0;attempt<5;++attempt)
        if(renderer.draw(views.front().camera,path,{},false,{},{},{},views))return read(path);
    throw FormatError("shadow renderer frame did not submit");
}
template<class F>void rejects(F &&fn){bool rejected=false;try{fn();}catch(const FormatError &){rejected=true;}require(rejected,"invalid source shadow service was admitted");}
void verify(const std::filesystem::path &source,const std::filesystem::path &output) {
    Cache cache(source/"assets/disc/maps/zanzibar.map");
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto &t){
        return t.group=="shad" && t.name=="objects\\weapons\\rifle\\battle_rifle\\shaders\\battle_rifle_lod";
    });require(tag!=cache.tags().end(),"original BR shadow shader missing");
    const auto authored=cache.load_material(tag->datum);
    auto generation=prepare_shadow_material(cache,authored,SourceShadowPhase::Generate,true,false,.75f);
    auto application=prepare_shadow_material(cache,authored,SourceShadowPhase::Apply,true,false,.75f);
    auto quarter=prepare_shadow_material(cache,authored,SourceShadowPhase::Apply,true,false,.25f);
    auto zero=prepare_shadow_material(cache,authored,SourceShadowPhase::Apply,true,false,0);
    require(generation.size()==1 && application.size()==1 && quarter.size()==1 && zero.size()==1,"source shadow pass counts differ");
    const std::array<ShaderVector,4> controlled_samples{{{0,0,0,1},{.5f,.5f,.5f,1},{},{}}};
    const ShaderVector source_d0{.4894518256187439f,.4894518256187439f,.4894518256187439f,.5171411037445068f};
    const auto source_factor=halo2_test::scalar_combiners(application[0].material.forward_program.words,
        controlled_samples,source_d0,{});
    const auto substituted_factor=halo2_test::scalar_combiners(application[0].material.forward_program.words,
        controlled_samples,application[0].material.forward_diffuse,{});
    require(source_factor[0]<substituted_factor[0] && substituted_factor[0]==1,
        "actual shadow receiver program does not consume original oD0 attenuation");
    require(application[0].material.forward_vertex!=3,
        "shadow receiver entered stencil texture's substituted diffuse consumer");
    auto background=authored;require(prepare_dynamic_material(cache,background,{},true),"source scene stencil pass missing");
    require(!background.vertex_program.empty(),"controlled scene lacks original world VP");
    // Only the controlled ordinary scene is a white source copy program.
    // Shadow programs, images, source extern binding and pass state stay
    // authored. This is a service/state fixture, not appearance evidence.
    background.forward_program=scope_copy_frame({}).program;
    background.diffuse={1,1,11,std::vector<std::byte>(4,std::byte{255}),{}};
    background.environment_pass.reset();background.world_overlays_before.clear();background.world_overlays_after.clear();
    background.world_bloom_passes.clear();background.world_planar_passes={};background.planar_fog_pass.reset();
    background.accumulation_texture=false;background.forward_blend=0;
    background.forward_raster.depth_enable=1;background.forward_raster.depth_write=1;
    background.forward_raster.depth_function=0x203;background.forward_raster.cull=0;
    for(auto &frames:background.animated_textures)frames.clear();
    Mesh mesh;mesh.vertices.resize(3);mesh.materials={background,background,
        generation[0].material,application[0].material,generation[0].material,quarter[0].material};
    for(unsigned i=0;i<2;++i) {
        VertexProgramBank bank;bank.first_constant=0;bank.camera_inputs=true;
        const auto camera=camera_bank(640,480,0,1);bank.constants.assign(camera.begin(),camera.end());
        mesh.materials[i].vertex_bank=std::make_shared<VertexProgramBank>(bank);
    }
    const std::array<SourceShadowPlanMaterial,4> resident{{
        {2,&generation[0]},{3,&application[0]},{4,&generation[0]},{5,&quarter[0]}}};
    const auto atmosphere=cache.load_bsp_atmosphere(0);
    auto reused_window=window(640,480);
    auto wrong=mesh;wrong.materials[2].diffuse.pixels.at(0)^=std::byte{1};
    rejects([&]{Renderer bad(reused_window.get(),wrong,true,resident);});
    wrong=mesh;wrong.materials[2].source_shader_datum^=1;
    rejects([&]{Renderer bad(reused_window.get(),wrong,true,resident);});
    wrong=mesh;wrong.materials[3].world_vertex_input_transform=SourceWorldVertexInputTransform::NormalizeSkinNormal;
    rejects([&]{Renderer bad(reused_window.get(),wrong,true,resident);});
    Renderer reused(reused_window.get(),mesh,true,resident);reused.scene_bsp_atmosphere(atmosphere);
    unsigned pairs=0,controls=0;
    for(const auto dimensions:std::array{std::array{640,480},std::array{853,479},std::array{640,480}})
    for(unsigned count=1;count<=2;++count) {
        const auto [width,height]=dimensions;
        SDL_SetWindowSize(reused_window.get(),width,height);SDL_PumpEvents();reused.resized();
        std::array<View,2> fixture;std::array<ModelView,2> views;
        for(unsigned i=0;i<count;++i) {
            auto &v=fixture[i];v.plan.camera=camera_bank(width,height,i,count);
            VertexProgramBank bank;bank.first_constant=0;bank.camera_inputs=true;
            bank.constants.assign(v.plan.camera.begin(),v.plan.camera.end());
            mesh.materials[i].vertex_bank=std::make_shared<VertexProgramBank>(bank);
            const float aspect=float(width)/(float(height)/count);
            v.scene=quad(i,2,2*aspect,2);
            for(unsigned r=0;r<2;++r) {
                v.casters[r]=quad(2,1,.3f,.3f,r?.22f:-.22f);
                v.receivers[r]=quad(3,2,2*aspect,2);
                v.generation[r]=source_shadow_plan_draw(2,generation[0],v.casters[r],{0,true});
                v.application[r]=source_shadow_plan_draw(3,application[0],v.receivers[r],{0,true});
                if(r)v.application[r].program=quarter[0].material.forward_program;
                SourceShadowFrame frame;frame.direction={1,0,0};frame.up={0,0,1};frame.center={1,r?.22f:-.22f,0};frame.virtual_eye={-1,r?.22f:-.22f,0};
                frame.virtual_distance=2;frame.near_offset=-.5f;frame.far_offset=2;
                frame.cone_angle=1.5707963267948966f;frame.opacity=r?.25f:.75f;
                v.roots[r]={source_shadow_uniforms(frame,false),{&v.generation[r],1},{&v.application[r],1}};
            }
            v.plan.roots=v.roots;
            auto &model=v.model;model.camera.oriented=true;model.camera.forward={1,0,0};model.camera.up={0,0,1};model.camera.field_of_view=90;
            model.top=float(i)*480/count;model.width=640;model.height=480.f/count;model.full_surface=true;model.vertices=v.scene;
            model.atmosphere=BspFogSample{};
            SourceViewShaderConstants source_view;std::copy_n(v.plan.camera.begin(),7,source_view.world.begin());
            model.source_view_shader_constants=source_view;model.source_shadows=v.plan;views[i]=model;
        }
        reused.update_material_constants(mesh.materials);
        const auto name=std::to_string(width)+"x"+std::to_string(height)+"-"+std::to_string(count)+"p-"+std::to_string(pairs);
        auto no_shadows=views;for(auto &view:no_shadows)view.source_shadows.reset();
        const auto baseline=capture(reused,std::span(no_shadows).first(count),output/(name+"-baseline.ppm"));
        const auto shared=capture(reused,std::span(views).first(count),output/(name+"-shared.ppm"));
        for(unsigned i=0;i<count;++i)require(difference(shared,baseline,i,count).changed>100,"authored source shadow application has no measured controlled response");
        if(!pairs) {
            for(auto &receiver:fixture[0].receivers)for(auto &vertex:receiver)vertex.normal={1,0,0};
            const auto away=capture(reused,std::span(views).first(count),output/(name+"-zero-d0.ppm"));
            require(difference(away,baseline).maximum<=1,
                "source shadow receiver ignored zero original oD0 attenuation");
            for(auto &receiver:fixture[0].receivers)for(auto &vertex:receiver)vertex.normal={-1,0,0};
            ++controls;
        }
        auto fresh_window=window(width,height);Renderer fresh(fresh_window.get(),mesh,true,resident);fresh.scene_bsp_atmosphere(atmosphere);
        const auto fresh_shared=capture(fresh,std::span(views).first(count),output/(name+"-fresh.ppm"));
        require(difference(shared,fresh_shared).maximum<=1,"queued source shadows changed after renderer reuse/resize");++pairs;
        // Material slot isolation control: bind second roots to distinct
        // resident slots with their actual authored .25 extern17 program.
        for(unsigned i=0;i<count;++i) {
            auto &v=fixture[i];for(auto &vertex:v.casters[1])vertex.material=4;
            for(auto &vertex:v.receivers[1])vertex.material=5;
            v.generation[1].material_slot=4;v.application[1].material_slot=5;
        }
        const auto isolated=capture(reused,std::span(views).first(count),output/(name+"-distinct-slots.ppm"));
        require(difference(shared,isolated).maximum<=1,"repeated shadow material slots leaked completed root constants");++controls;
        for(unsigned i=0;i<count;++i) {
            auto &v=fixture[i];for(auto &vertex:v.casters[1])vertex.material=2;
            for(auto &vertex:v.receivers[1])vertex.material=3;
            v.generation[1].material_slot=2;v.application[1].material_slot=3;
        }
        if(count==2) {
            const auto saved=fixture[1].application;
            for(auto &draw:fixture[1].application)draw.program=zero[0].material.forward_program;
            const auto control=capture(reused,views,output/(name+"-zero-second-view.ppm"));
            require(difference(shared,control,0,2).maximum<=1,"shadow camera/opacity leaked into the other player view");
            require(difference(shared,control,1,2).changed>100,"shadow opacity control had no measured response");
            fixture[1].application=saved;++controls;
        }
        const auto atmosphere_context=views[0].atmosphere;
        views[0].atmosphere.reset();
        rejects([&]{reused.draw(views[0].camera,{}, {},false,{},{},{},std::span(views).first(count));});
        views[0].atmosphere=atmosphere_context;
        const auto source_camera_context=views[0].source_view_shader_constants;
        views[0].source_view_shader_constants.reset();
        rejects([&]{reused.draw(views[0].camera,{}, {},false,{},{},{},std::span(views).first(count));});
        views[0].source_view_shader_constants=source_camera_context;
        views[0].source_view_shader_constants->world[3][0]+=1;
        rejects([&]{reused.draw(views[0].camera,{}, {},false,{},{},{},std::span(views).first(count));});
        views[0].source_view_shader_constants=source_camera_context;
        controls+=3;
        fixture[0].generation[0].raster.reset();
        rejects([&]{reused.draw(views[0].camera,{}, {},false,{},{},{},std::span(views).first(count));});
        fixture[0].generation[0].raster=SourceShadowRasterInput{0,true};
        const auto restored=capture(reused,std::span(views).first(count),output/(name+"-restored.ppm"));
        require(difference(shared,restored).maximum<=1,"rejected shadow plan disturbed later frame state");++controls;
        std::cout<<name<<": shared/fresh/distinct-root-slot/restore controls passed\n"<<std::flush;
    }
    std::cout<<pairs<<" fresh/reused shadow view pairs, "<<controls<<" isolation/admission controls; explicit controlled source services, no live scene parity claim\n";
}
}
int main(int argc,char **argv){try {
    require(argc==2,"shadow renderer test requires capture directory");
#if defined(__linux__)
    if(!std::getenv("DISPLAY") && !std::getenv("WAYLAND_DISPLAY"))return 77;
    setenv("VK_LAYER_VALIDATE_SYNC","1",1);
#endif
    const auto source=std::filesystem::path(HALO2_SOURCE_DIR);
    if(!std::filesystem::exists(source/"assets/disc/maps/zanzibar.map"))return 77;
    const std::filesystem::path output=argv[1];std::filesystem::create_directories(output);
    require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    std::ostringstream validation;auto *previous=std::cerr.rdbuf(validation.rdbuf());
    try{verify(source,output);}catch(...){std::cerr.rdbuf(previous);std::cerr<<validation.str();throw;}
    std::cerr.rdbuf(previous);SDL_Quit();
    require(validation.str().find("Vulkan validation:")==std::string::npos,
        "shadow renderer Vulkan/synchronization diagnostics:\n"+validation.str());return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';SDL_Quit();return 1;}}
