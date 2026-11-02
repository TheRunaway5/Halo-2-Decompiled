#define SDL_MAIN_HANDLED
#include "halo2/model.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/pstencil/material.hpp"
#include "halo2/pstencil/specular_mask.hpp"
#include "halo2/renderer.hpp"
#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void require(bool value,const char* why) {if(!value)throw FormatError(why);}
std::vector<unsigned char> image(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);std::string magic;unsigned width{},height{},maximum{};
    input>>magic>>width>>height>>maximum;input.get();
    require(magic=="P6" && maximum==255,"Invalid player-color capture");
    std::vector<unsigned char> pixels(std::size_t(width)*height*3);
    input.read(reinterpret_cast<char*>(pixels.data()),pixels.size());
    require(bool(input),"Incomplete player-color capture");return pixels;
}
unsigned differences(const auto& a,const auto& b) {
    require(a.size()==b.size(),"Player-color capture size changed");unsigned count{};
    for(unsigned n=0;n<a.size();n+=3)
        count+=!std::equal(a.begin()+n,a.begin()+n+3,b.begin()+n);
    return count;
}
void native_camera(ModelView& view) {
    SourceFrustumProjectionInput projection;
    projection.projection_angle=view.camera.field_of_view*3.14159265358979323846f/180.f;
    projection.outer=projection.safe={0,0,120,160};
    const auto camera=source_view_projection({view.camera.position,view.camera.forward,view.camera.up,
        .015625f,1024.f,{},source_frustum_projection(projection).projection_program});
    const auto bank=source_view_shader_bank(camera,1024.f,view.camera.position,{0,0,160,120},false);
    view.source_view_shader_constants.emplace();
    std::copy_n(bank.begin(),7,view.source_view_shader_constants->world.begin());
    view.source_view_shader_constants->reflection=view.source_view_shader_constants->world;
}
}
int main(int argc,char** argv)try {
    require(argc==2 || argc==3,"Supply multiplayer cache and optional player representation");
    const unsigned representation=argc==3?std::stoul(argv[2]):2;
    require(representation==2 || representation==3,"Supply Spartan2 or Elite3 representation");
    const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");
    require(scratch && *scratch,"Run player-color captures through tools/run_test.py");
    if(!std::getenv("DISPLAY"))return 77;
    SDL_SetMainReady();require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    struct Quit {~Quit(){SDL_Quit();}} quit;
    const std::filesystem::path output=std::filesystem::path(scratch)/"player-colors";
    std::filesystem::create_directories(output);
    Cache cache(argv[1]);
    const auto content=cache.load_content(0,true);
    const auto unit=content.player_representations.at(representation).biped.datum;
    auto model=cache.load_model(unit,true);
    ObjectLightingState light;light.ambient={.5f,.5f,.5f};
    light.primary_color={.5f,.5f,.5f};light.primary_direction={0,0,1};light.accuracy=1;
    for(auto& material:model.mesh.materials)if(!material.lods.empty())
        require(prepare_dynamic_material(cache,material,light,true),"Actual player material did not bind");
    const auto definition=cache.load_initial_model_definition(unit);
    SourceDynamicModelShaders shaders(cache,model,definition.render_model);
    auto vertices=pose_model(model,nullptr,0);
    SourceModelShaderView shader_view;shader_view.shader=source_shader_lod_view(0,480,false,false);
    shader_view.part_shader_multiplier=1;shaders.select(vertices,{10000,255},shader_view);
    unsigned triangle=~0u;float area{};
    for(unsigned n=0;n+2<vertices.size();n+=3) {
        // The expanded model also retains shadow-only source parts. Inspect
        // an authored visible opaque part, as the live color collector does.
        if(vertices[n].source_part_type!=2 && vertices[n].source_part_type!=3)continue;
        const auto& material=model.mesh.materials.at(vertices[n].material);
        if(!material.name.ends_with(representation==2?"\\masterchief":"\\torso"))continue;
        const auto size=length(cross(vertices[n+1].position-vertices[n].position,
            vertices[n+2].position-vertices[n].position));
        if(size>area){area=size;triangle=n;}
    }
    require(triangle!=~0u,"Actual player armor triangle is absent");
    const auto slot=vertices[triangle].material;
    Mesh mesh;mesh.materials=model.mesh.materials;
    mesh.vertices.assign(vertices.begin()+triangle,vertices.begin()+triangle+3);
    if(representation==3) {
        mesh.vertices.clear();
        for(const auto& vertex:vertices)if(vertex.material==slot &&
            (vertex.source_part_type==2 || vertex.source_part_type==3))mesh.vertices.push_back(vertex);
    }
    const auto other_slot=unsigned(mesh.materials.size());
    mesh.materials.push_back(mesh.materials.at(slot));
    pstencil::MaterialBank masks(cache,mesh);
    const auto source_count=mesh.materials.size();
    for(unsigned n=0;n<source_count;++n) {
        const auto authored=mesh.materials[n];masks.preload(authored,0,{});
    }
    const auto mask=pstencil::texture_mask_materials(mesh.materials).at(slot);
    if(representation==2)
        require(mask!=~0u && mask!=slot,"Actual Spartan shader has no shared pStencil texture pass");
    std::cout<<"Armor shader "<<mesh.materials[slot].name<<" LOD "
        <<mesh.materials[slot].selected_shader_lod<<", shared texture slot "<<mask
        <<", part type "<<mesh.vertices[0].source_part_type<<'\n';
    Vec3 center{};float radius{};
    for(const auto& vertex:mesh.vertices)center=center+vertex.position;
    center=center/float(mesh.vertices.size());
    auto normal=normalized(cross(vertices[triangle+1].position-vertices[triangle].position,
        vertices[triangle+2].position-vertices[triangle].position));
    for(const auto& vertex:mesh.vertices)radius=std::max(radius,length(vertex.position-center));
    Camera camera;camera.position=center+normal*(radius*3.f);camera.forward=normal*-1.f;
    camera.up=std::abs(normal.z)>.9f?Vec3{0,1,0}:Vec3{0,0,1};
    camera.oriented=true;camera.field_of_view=60;
    auto window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>(SDL_CreateWindow(
        "pStencil player colors",0,0,160,120,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN),SDL_DestroyWindow);
    require(bool(window),SDL_GetError());
    Renderer renderer(window.get(),mesh,true);renderer.scene_clear_color({.01f,.01f,.01f});
    const auto capture=[&](pstencil::GraphicsMode mode,Vec3 color,const char* name) {
        auto settings=renderer.display_settings();settings.graphics_mode=mode;renderer.display_settings(settings);
        apply_object_colors(mesh.materials[slot],{{color,color,color,color}});
        apply_object_lighting(mesh.materials[slot],light);
        renderer.update_material_constants(mesh.materials);
        ModelView view;view.camera=camera;view.width=640;view.height=480;
        view.vertices=mesh.vertices;view.full_surface=true;
        native_camera(view);
        const auto path=output/name;
        for(unsigned n=0;n<5;++n)if(renderer.draw(camera,path,{},false,{},{},{},{&view,1}))return image(path);
        throw FormatError("Player-color frame did not submit");
    };
    const Vec3 red{.7f,.03f,.03f},blue{.03f,.03f,.7f};
    auto original_red=capture(pstencil::GraphicsMode::Original,red,"original-red.ppm");
    auto original_blue=capture(pstencil::GraphicsMode::Original,blue,"original-blue.ppm");
    auto original_changed=differences(original_red,original_blue);
    if(original_changed<=50) {
        // Authored model parts can have the opposite cull orientation. The
        // positive control selects a visible inspection side before pStencil.
        normal=normal*-1.f;camera.position=center+normal*(radius*3.f);camera.forward=normal*-1.f;
        original_red=capture(pstencil::GraphicsMode::Original,red,"original-red.ppm");
        original_blue=capture(pstencil::GraphicsMode::Original,blue,"original-blue.ppm");
        original_changed=differences(original_red,original_blue);
        std::cout<<"Opposite inspection side changes "<<original_changed<<" pixels\n";
    }
    std::cout<<"Original armor color changes "<<original_changed<<" pixels\n";
    unsigned covered{};
    for(unsigned n=0;n<original_red.size();n+=3)
        covered+=!std::equal(original_red.begin()+n,original_red.begin()+n+3,original_red.begin());
    std::cout<<"Original inspection covers "<<covered<<" pixels\n";
    require(original_changed>50,"Actual armor fixture does not expose player color");
    const auto stencil_red=capture(pstencil::GraphicsMode::pStencil,red,"pstencil-red.ppm");
    const auto stencil_blue=capture(pstencil::GraphicsMode::pStencil,blue,"pstencil-blue.ppm");
    const auto stencil_white=capture(pstencil::GraphicsMode::pStencil,{1,1,1},"pstencil-white.ppm");
    const unsigned background=stencil_white[0]+stencil_white[1]+stencil_white[2];
    unsigned visible{};
    for(unsigned n=0;n<stencil_white.size();n+=3)
        visible+=unsigned(stencil_white[n]+stencil_white[n+1]+stencil_white[n+2])>background+3;
    require(visible>50,"pStencil armor fixture is not visible against the clear color");
    const auto stencil_changed=differences(stencil_red,stencil_blue);
    std::cout<<"pStencil armor color changes "<<stencil_changed<<" pixels; red differs from white at "
        <<differences(stencil_red,stencil_white)<<" pixels\n";
    require(stencil_changed>50,"pStencil loses the player's armor color to the shared white texture pass");
    const auto repeat=capture(pstencil::GraphicsMode::pStencil,red,"pstencil-red-repeat.ppm");
    require(differences(stencil_red,repeat)==0,"pStencil retains the previous player's color");
    const auto restored=capture(pstencil::GraphicsMode::Original,red,"original-restored.ppm");
    require(differences(original_red,restored)==0,"Original changed after pStencil color switching");
    // Living players use separately colored material roots while sharing the
    // same shader/texture residency. Put both roots in one packed view so a
    // global last-writer color cannot pass as independent owner binding.
    auto pair_camera=camera;pair_camera.position=center+normal*(radius*4.5f);
    const auto right=normalized(cross(camera.forward,camera.up));
    std::vector<Vertex> pair;
    for(unsigned owner=0;owner<2;++owner)for(auto vertex:mesh.vertices) {
        vertex.position=vertex.position+right*((owner?1.f:-1.f)*radius*1.2f);
        vertex.material=owner?other_slot:slot;pair.push_back(vertex);
    }
    const auto capture_pair=[&](bool reverse,const char* name) {
        auto settings=renderer.display_settings();settings.graphics_mode=pstencil::GraphicsMode::pStencil;
        renderer.display_settings(settings);
        for(unsigned owner=0;owner<2;++owner) {
            const auto color=(bool(owner)!=reverse)?blue:red;
            auto& material=mesh.materials[owner?other_slot:slot];
            apply_object_colors(material,{{color,color,color,color}});
            apply_object_lighting(material,light);
        }
        renderer.update_material_constants(mesh.materials);
        ModelView view;view.camera=pair_camera;view.width=640;view.height=480;
        view.vertices=pair;view.full_surface=true;
        native_camera(view);
        const auto path=output/name;
        for(unsigned n=0;n<5;++n)if(renderer.draw(pair_camera,path,{},false,{},{},{},{&view,1}))return image(path);
        throw FormatError("Two-player color frame did not submit");
    };
    const auto pair_before=capture_pair(false,"two-players.ppm");
    const auto pair_after=capture_pair(true,"two-players-reversed.ppm");
    require(pair_before.size()==160u*120u*3u,"Two-player target dimensions changed");
    std::array<unsigned,2> redder{},bluer{};
    for(unsigned n=0;n<pair_before.size();n+=3) {
        const unsigned half=(n/3)%160>=80;
        redder[half]+=pair_before[n]>pair_after[n] && pair_before[n+2]<pair_after[n+2];
        bluer[half]+=pair_before[n]<pair_after[n] && pair_before[n+2]>pair_after[n+2];
    }
    std::cout<<"Two packed roots: redder="<<redder[0]<<','<<redder[1]
        <<" bluer="<<bluer[0]<<','<<bluer[1]<<'\n';
    require((redder[0]>10 && bluer[1]>10) || (bluer[0]>10 && redder[1]>10),
        "Two players sharing one pStencil texture pass lost their independent armor colors");
    require(differences(pair_before,capture_pair(false,"two-players-restored.ppm"))==0,
        "A player's previous armor color leaked into another packed draw");
    std::cout<<"PASS actual player armor colors survive pStencil accumulation and color/mode changes\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
