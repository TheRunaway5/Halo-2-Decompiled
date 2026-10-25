#define SDL_MAIN_HANDLED
#include "halo2/renderer.hpp"
#include "halo2/native_weapon_projectile_scene.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/light_volume.hpp"
#include "halo2/content.hpp"
#include "halo2/collision.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct Frame {unsigned width{},height{};std::vector<unsigned char> rgb;};
Frame read(const std::filesystem::path &path) {
    std::ifstream in(path,std::ios::binary);Frame f;std::string magic;unsigned maximum;
    in>>magic>>f.width>>f.height>>maximum;in.get();check(magic=="P6"&&maximum==255,"Invalid glow capture");
    f.rgb.resize(std::size_t(f.width)*f.height*3);in.read(reinterpret_cast<char*>(f.rgb.data()),f.rgb.size());
    check(bool(in),"Truncated glow capture");return f;
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply prepared MCC Coagulation map");
    if(!std::getenv("DISPLAY"))return 77;
    const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Glow GPU regression requires owned scratch");
    Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path());check(cache.is_mcc(),"Prepared MCC content required");
    const auto found=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto &t) {
        return t.group=="proj"&&t.name.ends_with("banshee\\weapon\\banshee_bolt");});
    check(found!=cache.tags().end(),"Authored Banshee bolt is absent");
    const auto content=cache.load_bsp_content(0);CollisionWorld collision(content.collision);Mesh mesh;
    Material backdrop;backdrop.diffuse.width=backdrop.diffuse.height=1;backdrop.diffuse.format=11;
    for(const auto value:std::array<unsigned char,4>{80,120,160,255})backdrop.diffuse.pixels.push_back(std::byte(value));
    mesh.materials.push_back(backdrop);
    NativeWeaponProjectileScene scene(cache,0,mesh,collision,std::array{found->datum},2048);
    // Declared presentation inputs isolate material delivery. No source birth,
    // hit, lifetime, visibility or GPU result is supplied by this fixture.
    const std::array live{NativeWeaponProjectile{0x12340001u,found->datum,{2,0,0},{1,0,0},{1,0,0},{0,0,1},1}};
    scene.synchronize(live);
    std::vector<Vertex> background;
    for(const auto uv:std::array{Vec2{0,0},Vec2{1,0},Vec2{1,1},Vec2{0,0},Vec2{1,1},Vec2{0,1}}) {
        Vertex v;v.position={4,(1-2*uv.x)*10,(1-2*uv.y)*10};v.normal={-1,0,0};
        v.tangent={0,-1,0};v.binormal={0,0,-1};v.color={1,1,1};v.uv=uv;v.material=0;background.push_back(v);
    }
    SDL_SetMainReady();check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> window(SDL_CreateWindow("Projectile glow material banks",0,0,640,480,
        SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN),SDL_DestroyWindow);check(bool(window),SDL_GetError());
    Renderer renderer(window.get(),mesh,false,{},nullptr,{},nullptr,HALO2_SHADER_DIR);
    std::array<ModelView,2> views;std::array<unsigned,2> uploads{};
    std::array<std::span<const SourceLightVolumeDraw>,2> lights;
    const auto expected=light_volume_program().words;
    for(unsigned i=0;i<2;++i) {
        auto &v=views[i];v.full_surface=true;v.left=0;v.top=float(i*240);v.width=640;v.height=240;v.vertices=background;
        SourceFrustumProjectionInput input;input.projection_angle=.8655884265899658f;input.outer=input.safe={0,0,240,640};
        const auto projection=source_frustum_projection(input);
        const auto camera=source_view_projection({{0,0,0},{1,0,0},{0,0,1},.015625f,1024.f,{},projection.projection_program});
        lights[i]=scene.light_volume_view(i,{1,0,0},camera,{0,float(i*240),640,240},1024.f);
        check(!lights[i].empty(),"Actual Banshee definition produced no glow");
        v.completed_material_upload_observer=[&,i](const RendererCompletedMaterialUpload &upload) {
            if(std::none_of(lights[i].begin(),lights[i].end(),[&](const auto &draw) {
                return std::any_of(draw.bitmap_materials.begin(),draw.bitmap_materials.end(),[&](const auto &m){return m.material==upload.root_slot;});}))return;
            check(upload.material_uniform.size()>=sizeof(expected)&&!std::memcmp(upload.material_uniform.data(),expected.data(),sizeof(expected)),
                "Per-view glow upload lost its unchanged original pixel program");++uploads[i];
        };
    }
    const auto capture=[&](const char *label) {
        const auto path=std::filesystem::path(scratch)/(std::string(label)+".ppm");
        for(unsigned retry=0;retry<5;++retry)if(renderer.draw(views.front().camera,path,{},false,{},{},{},views))return read(path);
        throw FormatError("Glow frame did not submit");
    };
    const auto baseline=capture("background");
    for(unsigned i=0;i<2;++i)views[i].light_volumes=lights[i];
    const auto glow=capture("glow"),repeat=capture("repeat");
    check(glow.rgb==repeat.rgb,"Static glow fixture is not repeatable");
    bool programs=true,pixels=true;
    for(unsigned view=0;view<2;++view) {
        unsigned dark{},bright{};
        for(unsigned y=view*240;y<(view+1)*240;++y)for(unsigned x=0;x<640;++x)for(unsigned c=0;c<3;++c) {
            const auto at=(std::size_t(y)*640+x)*3+c;
            dark+=int(glow.rgb[at])+1<int(baseline.rgb[at]);bright+=int(glow.rgb[at])>int(baseline.rgb[at])+1;
        }
        std::cout<<"view="<<view<<" original_program_uploads="<<uploads[view]<<" darkened_components="<<dark<<" brightened_components="<<bright<<'\n'<<std::flush;
        programs&=uploads[view]>0;pixels&=dark==0&&bright>0;
    }
    check(programs,"Projectile glow material skipped the actual per-view uniform upload");
    check(pixels,"Original zero-alpha additive glow darkened the background or stayed invisible");
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
