#include "halo2/source_grenade_scene.hpp"
#include "halo2/renderer.hpp"
#include "halo2/collision.hpp"
#include "halo2/original_data.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char*why){if(!value)throw FormatError(why);}
struct Frame{unsigned width{},height{};std::vector<unsigned char> pixels;};
Frame read(const std::filesystem::path&path){
    std::ifstream file(path,std::ios::binary);std::string magic;unsigned maximum;Frame result;
    file>>magic>>result.width>>result.height>>maximum;file.get();
    check(magic=="P6"&&maximum==255,"Invalid owner draw capture");
    result.pixels.resize(std::size_t(result.width)*result.height*3);
    file.read(reinterpret_cast<char*>(result.pixels.data()),result.pixels.size());
    check(bool(file),"Truncated owner draw capture");return result;
}
void compare(const Frame&a,const Frame&b){
    check(a.width==b.width&&a.height==b.height,"Owner capture size changed");
    unsigned maximum{};
    for(std::size_t i=0;i<a.pixels.size();++i)
        maximum=std::max(maximum,unsigned(std::abs(int(a.pixels[i])-int(b.pixels[i]))));
    check(maximum<=1,"Shared textures leaked another owner's material constants");
}
void view_direction(Material&m,Vec3 forward){
    const auto child=[&](std::shared_ptr<Material>&p){if(p){if(!p.unique())p=std::make_shared<Material>(*p);view_direction(*p,forward);}};
    child(m.environment_pass);child(m.planar_fog_pass);
    for(auto&p:m.world_overlays_before)child(p);for(auto&p:m.world_overlays_after)child(p);
    for(auto&p:m.world_bloom_passes)child(p);for(auto&p:m.world_planar_passes)child(p);
    if(!m.forward_pass)return;
    auto&pass=m.passes.at(m.forward_pass_index);std::vector<ShaderRemap> mappings;
    for(const auto&mapping:pass.pixel_externs)if((mapping.component&7)==4&&mapping.source==0)mappings.push_back(mapping);
    const std::array<ShaderVector,1> colors{{{forward.x,forward.y,forward.z,0}}};
    remap_pixel_externs(m.forward_program,{},colors,mappings,pass.fragments,m.shader_switches);
    pass.pixel_externs.clear();
}
Frame capture(Renderer&renderer,Camera camera,std::span<const ModelView> views,const std::filesystem::path&path){
    for(unsigned attempt=0;attempt<5;++attempt)
        if(renderer.draw(camera,path,{},false,{},{},{},views))return read(path);
    throw FormatError("Owner draw capture failed to render");
}
}
int main(int argc,char**argv){try{
    check(argc==5,"Supply MAP SHARED_MAPS UPDATE_XBE OUTPUT");
    if(!std::getenv("DISPLAY"))return 77;
    Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
    check(bool(cache.original_data()->object_construction_operands),"Original constructor operands unavailable");
    const auto content=cache.load_content(0,true);CollisionWorld collision(content.collision);Mesh mesh;
    SourceGrenadeScene scene(cache,0,mesh,collision,0x800,cache.original_data()->object_construction_operands->one_bits);
    std::array<NativeGrenadeProjectile,2> projectiles;
    for(unsigned i=0;i<2;++i){auto&p=projectiles[i];p.datum=0x12340002+i;p.type=0;
        p.definition=scene.banks()[0].definition;p.position={.45f,i?.13f:-.13f,0};p.forward={1,0,0};p.up={0,0,1};}
    scene.synchronize(projectiles);
    SourceObjectViewLod selector;selector.view_index=0;selector.local_players=1;
    selector.input=[](std::size_t,std::uint32_t,Vec3,float){return SourceObjectLodInput{0,0,0};};
    selector.shader=SourceModelShaderView{{{0,0,-1,1000},240},source_shader_lod_view(0,480,false,false),
        source_geometry_lod_policy(1).coverage_multiplier,source_shader_lod_player_multiplier(1),false};
    const auto packets=scene.draw_view(0,selector);check(packets.size()==2,"Actual grenade templates omitted owners");
    const auto lighting_definition=cache.load_object_lighting_definition();
    const std::array lights{
        object_lighting(lighting_definition,{{.2f,.3f,.7f},0xff8080,0xc04020},0),
        object_lighting(lighting_definition,{{-.7f,.2f,.3f},0x204080,0x2080c0},0)};
    std::array<ModelObjectDraw,2> objects;
    for(unsigned i=0;i<2;++i)objects[i]={packets[i].vertices,lights[i]};
    Camera camera;camera.field_of_view=70;
    Mesh expected;expected.materials=mesh.materials;
    expected.materials.insert(expected.materials.end(),mesh.materials.begin(),mesh.materials.end());
    for(unsigned i=0;i<2;++i)for(unsigned m=0;m<mesh.materials.size();++m){
        auto&material=expected.materials[i*mesh.materials.size()+m];apply_object_lighting(material,lights[i]);
        view_direction(material,direction(camera.yaw,camera.pitch));}
    std::vector<Vertex> expected_vertices;
    for(unsigned i=0;i<2;++i)for(auto vertex:packets[i].vertices){
        vertex.material+=unsigned(i*mesh.materials.size());expected_vertices.push_back(vertex);}
    check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    const auto make_window=[](){return std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>(
        SDL_CreateWindow("Shared original grenade images / independent owners",0,0,960,720,
            SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN|SDL_WINDOW_RESIZABLE),&SDL_DestroyWindow);};
    auto window=make_window(),reference_window=make_window();check(bool(window)&&bool(reference_window),SDL_GetError());
    const std::filesystem::path output=argv[4];std::filesystem::create_directories(output);
    {
        Renderer renderer(window.get(),mesh,false),reference(reference_window.get(),expected,false);
        for(const auto size:std::array<std::array<int,2>,3>{{{960,720},{1280,720},{960,720}}}){
            SDL_SetWindowSize(window.get(),size[0],size[1]);SDL_SetWindowSize(reference_window.get(),size[0],size[1]);
            SDL_PumpEvents();renderer.resized();reference.resized();
            for(unsigned players:{1u,4u}){
                std::vector<ModelView> views(players),references(players);
                for(unsigned i=0;i<players;++i){auto&view=views[i];view.camera=camera;view.full_surface=true;
                    view.left=players==1?0.f:float(i%2)*320.f;view.top=players==1?0.f:float(i/2)*240.f;
                    view.width=players==1?640.f:320.f;view.height=players==1?480.f:240.f;view.objects=objects;
                    references[i]=view;references[i].objects={};references[i].vertices=expected_vertices;}
                const auto stem=std::to_string(size[0])+"x"+std::to_string(size[1])+"-"+std::to_string(players);
                const auto actual=capture(renderer,camera,views,output/(stem+"-shared.ppm"));
                const auto independent=capture(reference,camera,references,output/(stem+"-independent.ppm"));
                compare(actual,independent);
                // A reversed owner's light must produce a measurable response;
                // otherwise equal blank images could hide a dropped object path.
                std::swap(objects[0].lighting,objects[1].lighting);
                const auto swapped=capture(renderer,camera,views,output/(stem+"-swapped.ppm"));
                unsigned changed{};for(std::size_t p=0;p<actual.pixels.size();++p)
                    changed+=std::abs(int(actual.pixels[p])-int(swapped.pixels[p]))>2;
                std::swap(objects[0].lighting,objects[1].lighting);
                check(changed>100,"Actual grenade draws produced no independently lit pixels");
                // Retiring all owners must leave no stale descriptor/draw bank.
                for(auto&view:views)view.objects={};capture(renderer,camera,views,output/(stem+"-retired.ppm"));
                for(auto&view:views)view.objects=objects;
                compare(actual,capture(renderer,camera,views,output/(stem+"-recreated.ppm")));
                std::cout<<stem<<": shared original samplers/images match separate source-bound owners; "<<changed<<" changed components\n";
            }
        }
    }
    window.reset();reference_window.reset();SDL_Quit();
    std::cout<<"PASS independent same-material object constants in one/four views, storage growth, retirement and resize\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
