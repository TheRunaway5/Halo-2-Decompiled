#include "halo2/renderer.hpp"
#include "halo2/model.hpp"
#include "halo2/appearance.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/bitmap_frame_animation.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
using Frame=std::vector<unsigned char>;
unsigned comparison_failures{};
Frame read(const std::filesystem::path &path){
    std::ifstream file(path,std::ios::binary);std::string magic;unsigned width{},height{},maximum{};
    file>>magic>>width>>height>>maximum;file.get();
    check(magic=="P6" && width==640 && height==480 && maximum==255,"Owner bitmap capture header differs");
    Frame pixels(std::size_t(width)*height*3);
    file.read(reinterpret_cast<char *>(pixels.data()),pixels.size());
    check(bool(file),"Owner bitmap capture is truncated");return pixels;
}
void compare(const Frame &actual,const Frame &expected){
    check(actual.size()==expected.size(),"Owner bitmap capture extent differs");unsigned maximum{};
    for(std::size_t n=0;n<actual.size();++n)
        maximum=std::max(maximum,unsigned(std::abs(int(actual[n])-int(expected[n]))));
    if(maximum>1){
        ++comparison_failures;
        std::cerr<<"Shared Elite material ignored its owner's bitmap frame input: maximum RGB difference "<<maximum<<'\n';
    }
}
ShaderInput input(float value){return [value](std::uint32_t name){return name==0x0b001a0au?value:0.f;};}
void animate(Material &material,float value){
    const auto child=[&](std::shared_ptr<Material>&pass){
        if(!pass)return;if(!pass.unique())pass=std::make_shared<Material>(*pass);animate(*pass,value);
    };
    child(material.environment_pass);
    for(auto &pass:material.world_overlays_before)child(pass);
    for(auto &pass:material.world_overlays_after)child(pass);
    for(auto &pass:material.world_bloom_passes)child(pass);
    animate_forward_material(material,0,input(value));animate_material_bitmaps(material,0,input(value));
}
Frame capture(Renderer &renderer,const ModelView &view,const std::filesystem::path &path){
    for(unsigned attempt=0;attempt<5;++attempt)
        if(renderer.draw(view.camera,path,{},false,{},{},{},std::span(&view,1)))return read(path);
    throw FormatError("Owner bitmap capture did not submit");
}
}
int main(int argc,char **argv)try{
    check(argc==4,"Supply MAP_DIRECTORY OUTPUT SHADER_DIRECTORY");
    if(!std::getenv("DISPLAY"))return 77;
    const Cache cache(std::filesystem::path(argv[1])/"lockout.map");
    const auto tag=cache.query_tag({.group=tag_groups::biped,.name="objects\\characters\\elite\\elite_mp"});
    check(bool(tag),"Original Elite multiplayer definition is missing");
    auto model=cache.load_model(tag->datum,true);
    const auto torso=std::find_if(model.mesh.materials.begin(),model.mesh.materials.end(),
        [](const auto &material){return material.name=="objects\\characters\\elite\\shaders\\torso";});
    check(torso!=model.mesh.materials.end(),"Actual Elite torso material is missing");
    const auto slot=unsigned(torso-model.mesh.materials.begin());
    Material material=*torso;material.selected_shader_lod=0;
    check(prepare_dynamic_material(cache,material,{},true),"Actual Elite high detail material is unsupported");
    apply_object_colors(material,cache.load_object_colors(tag->datum));
    check(material.animated_textures[1].size()==2,"Actual Elite detail frame bank was not prepared");
    const auto animations=bitmap_frame_animations(material);
    check(!animations.empty(),"Actual Elite detail owner frame plan is missing");
    auto fixed=material;fixed.detail=fixed.animated_textures[1][0];fixed.animated_textures[1].clear();
    check(bitmap_frame_animations(fixed).empty(),"Fixed replacement image retained an owner frame plan");
    auto invalid=animations;invalid.front().image_count=0;bool rejected=false;
    try{(void)animate_bitmap_frames(invalid,0,input(1),{});}catch(const FormatError &){rejected=true;}
    check(rejected,"Invalid submitted owner frame count was accepted");
    animate(material,0);check(material.texture_frames[1]==1,"Burn control did not select its original frame");
    Mesh mesh;mesh.materials.push_back(material);
    for(float value:{1.f,0.f}){
        auto copy=material;animate(copy,value);mesh.materials.push_back(std::move(copy));
    }
    const auto posed=pose_model(model);std::array<std::vector<Vertex>,2> vertices;
    Vec3 minimum{1.e9f,1.e9f,1.e9f},maximum{-1.e9f,-1.e9f,-1.e9f};
    for(auto vertex:posed)if(vertex.material==slot){
        minimum={std::min(minimum.x,vertex.position.x),std::min(minimum.y,vertex.position.y),std::min(minimum.z,vertex.position.z)};
        maximum={std::max(maximum.x,vertex.position.x),std::max(maximum.y,vertex.position.y),std::max(maximum.z,vertex.position.z)};
        vertex.material=0;vertices[0].push_back(vertex);
    }
    check(vertices[0].size()>100,"Actual Elite torso has no submitted triangle bank");
    const auto center=(minimum+maximum)*.5f;vertices[1]=vertices[0];
    for(unsigned owner=0;owner<2;++owner)for(auto &vertex:vertices[owner])
        vertex.position=vertex.position-center+Vec3{0,owner?.35f:-.35f,0};
    const auto light=object_lighting(cache.load_object_lighting_definition(),{{.2f,.3f,.7f},0x8080ff,0x404080},0);
    std::array<ModelObjectDraw,2> owners;
    for(unsigned owner=0;owner<2;++owner)owners[owner]={vertices[owner],light};
    ModelView view;view.camera.position={-1.4f,0,0};view.camera.forward={1,0,0};view.camera.up={0,0,1};
    view.camera.oriented=true;view.camera.field_of_view=70;view.width=640;view.height=480;view.full_surface=true;view.objects=owners;
    const std::filesystem::path output=argv[2],shaders=argv[3];
    std::filesystem::create_directories(output);
    check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    const auto window=[](){return std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>(
        SDL_CreateWindow("Original per-owner Elite bitmap frames",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN),SDL_DestroyWindow);};
    auto actual_window=window();check(bool(actual_window),SDL_GetError());
    unsigned different{};
    {
        Renderer actual(actual_window.get(),mesh,false,{},nullptr,{},nullptr,shaders);
        Frame first;unsigned cases{};
        for(const auto values:std::array{std::array{1.f,0.f},std::array{0.f,1.f},std::array{1.f,0.f}}){
            for(unsigned owner=0;owner<2;++owner)owners[owner].shader_input=input(values[owner]);
            std::array<std::vector<Vertex>,2> independent_vertices=vertices;
            std::array<ModelObjectDraw,2> independent_owners;
            for(unsigned owner=0;owner<2;++owner){
                for(auto &vertex:independent_vertices[owner])vertex.material=values[owner]==1.f?1:2;
                independent_owners[owner]={independent_vertices[owner],light};
            }
            auto reference_view=view;reference_view.objects=independent_owners;
            const auto name=std::to_string(cases++);
            const auto pixels=capture(actual,view,output/(name+"-shared.ppm"));
            const auto controls=capture(actual,reference_view,output/(name+"-independent.ppm"));
            compare(pixels,controls);
            if(first.empty())first=pixels;else if(cases==2)
                for(std::size_t n=0;n<pixels.size();++n)different+=std::abs(int(pixels[n])-int(first[n]))>2;
            else compare(pixels,first);
            std::swap(owners[0],owners[1]);
            compare(capture(actual,view,output/(name+"-reordered.ppm")),controls);
            std::swap(owners[0],owners[1]);
        }
    }
    actual_window.reset();SDL_Quit();
    check(!comparison_failures,"Per-owner bitmap frame captures differ from independent original frames");
    check(different>100,"Normal/burn owner inputs produced no visible original texture response");
    std::cout<<"PASS two actual Elite torso owners: normal/burn/normal, reordered draws and independent bitmap frame controls; "<<different<<" changed channels\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';SDL_Quit();return 1;}
