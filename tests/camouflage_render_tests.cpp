#define SDL_MAIN_HANDLED
#include "halo2/renderer.hpp"
#include "halo2/model.hpp"
#include "halo2/content.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_camouflage_material.hpp"
#include "halo2/world_material.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool ok,const char* why){if(!ok)throw FormatError(why);}
struct Image {unsigned width{},height{};std::vector<unsigned char> rgb;};
Image read(const std::filesystem::path& path) {
    Image result;std::ifstream in(path,std::ios::binary);std::string magic;unsigned max{};
    in>>magic>>result.width>>result.height>>max;in.get();check(magic=="P6" && max==255,"Bad camouflage capture");
    result.rgb.resize(std::size_t(result.width)*result.height*3);
    in.read(reinterpret_cast<char*>(result.rgb.data()),result.rgb.size());check(bool(in),"Truncated camouflage frame");return result;
}
unsigned difference(const Image& a,const Image& b,unsigned view) {
    check(a.width==b.width && a.height==b.height,"Camouflage extent changed");unsigned changed{};
    for(unsigned y=0;y<a.height;++y)for(unsigned x=view*a.width/2;x<(view+1)*a.width/2;++x)
        for(unsigned c=0;c<3;++c)changed+=a.rgb[(y*a.width+x)*3+c]!=b.rgb[(y*a.width+x)*3+c];
    return changed;
}
}
int main(int argc,char** argv)try {
    check(argc==3 || (argc==4 && std::string(argv[3])=="--no-validation"),"Supply map, owned capture directory and optional --no-validation");
    Cache cache(argv[1]);const auto content=cache.load_content(0,true);
    SDL_SetMainReady();check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> window(SDL_CreateWindow("Camouflage scene composition",0,0,1280,480,
        SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN),&SDL_DestroyWindow);check(bool(window),SDL_GetError());
    const std::filesystem::path output=argv[2];std::filesystem::create_directories(output);
    unsigned cases{},responses{};
    for(unsigned species:{2u,3u})for(bool first_person:{false,true}) {
        const auto &r=content.player_representations.at(species);
        auto model=cache.load_model(first_person?r.hands:r.biped.model,true);
        for(auto &m:model.mesh.materials) {
            if(m.lods.empty())continue;
            check(prepare_dynamic_material(cache,m,{},true,0.f),"Body ordinary material unresolved");
            prepare_camouflage_materials(cache,m);
            check(std::count_if(m.world_overlays_after.begin(),m.world_overlays_after.end(),
                [](const auto& pass){return pass->camouflage_group_pass;})==5,
                "Global clone omitted an original body part's override bank");
        }
        Mesh mesh;mesh.materials=model.mesh.materials;
        Material background;background.diffuse={8,8,11,std::vector<std::byte>(256),{}};
        for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x) {
            const unsigned n=(y*8+x)*4;const bool lit=(x+y)%2;
            background.diffuse.pixels[n]=std::byte(lit?220:25);background.diffuse.pixels[n+1]=std::byte(lit?60:170);
            background.diffuse.pixels[n+2]=std::byte(lit?40:240);background.diffuse.pixels[n+3]=std::byte{255};
        }
        const auto root_count=unsigned(mesh.materials.size());mesh.materials.push_back(std::move(background));
        auto posed=pose_model(model); // Actual authored bind hierarchy.
        for(auto &v:posed)v.color={1,1,1};
        Vec3 lo{1e9f,1e9f,1e9f},hi{-1e9f,-1e9f,-1e9f};
        for(const auto &v:posed){lo.x=std::min(lo.x,v.position.x);lo.y=std::min(lo.y,v.position.y);lo.z=std::min(lo.z,v.position.z);
            hi.x=std::max(hi.x,v.position.x);hi.y=std::max(hi.y,v.position.y);hi.z=std::max(hi.z,v.position.z);}
        const auto center=(lo+hi)*.5f;const auto radius=std::max(length(hi-lo)*.5f,.1f);
        Camera camera;camera.position=center+Vec3{radius*2.8f,0,0};camera.yaw=3.14159265358979323846f;
        std::vector<Vertex> backdrop;
        for(auto uv:std::array{Vec2{0,0},Vec2{1,0},Vec2{1,1},Vec2{0,0},Vec2{1,1},Vec2{0,1}}) {
            Vertex v;v.position=center+Vec3{-radius,(uv.x*2-1)*radius*3,(1-uv.y*2)*radius*3};
            v.uv=uv;v.color={1,1,1};v.material=root_count;backdrop.push_back(v);
        }
        Renderer renderer(window.get(),mesh,argc==3);
        const auto capture=[&](float a,float b,const char* label,bool fog=false) {
            std::array<CamouflageBinding,2> bindings{{{0,root_count,a},{0,root_count,b}}};
            std::array<ModelView,2> views;
            for(unsigned i=0;i<2;++i) {
                auto &v=views[i];v.camera=camera;v.left=i*320.f;v.width=320;v.height=480;v.full_surface=true;
                v.background=backdrop;v.camouflage={&bindings[i],1};
                if(first_person)v.foreground=posed;else v.vertices=posed;
                if(fog)v.camouflage_fog=SourceShaderFogContext{0x12340000u,1.f,30.f,{-1,0,0},{0,0,0}};
            }
            const auto path=output/(std::to_string(species)+(first_person?"-fp-":"-world-")+label+".ppm");
            for(unsigned n=0;n<4;++n)if(renderer.draw(camera,path,{},false,{},{},{},views))return read(path);
            throw FormatError("Camouflage did not render");
        };
        const auto zero=capture(0,0,"zero"),mixed=capture(.75f,0,"mixed"),full=capture(1,0,"full");
        auto saved_pose=std::move(posed);posed.clear();const auto background_only=capture(0,0,"background");posed=std::move(saved_pose);
        unsigned opaque_before{},opaque_after{};
        for(unsigned y=0;y<zero.height;++y)for(unsigned x=0;x<zero.width/2;++x) {
            const auto n=(y*zero.width+x)*3;
            if(!zero.rgb[n] && !zero.rgb[n+1] && !zero.rgb[n+2] &&
                std::max({background_only.rgb[n],background_only.rgb[n+1],background_only.rgb[n+2]})>32) {
                ++opaque_before;opaque_after+=!full.rgb[n] && !full.rgb[n+1] && !full.rgb[n+2];
            }
        }
        check(opaque_before>100 && opaque_after*10<opaque_before,"Full camouflage retained opaque body parts");
        const auto repeat=capture(.75f,0,"repeat"),swapped=capture(0,.75f,"swapped"),reset=capture(0,0,"reset");
        check(!difference(zero,mixed,1),"Camouflage changed the other player's view");
        check(!difference(mixed,repeat,0) && !difference(mixed,repeat,1),"Camouflage retained another frame's scene");
        check(!difference(zero,swapped,0),"Swapped camouflage leaked to the prior owner");
        check(!difference(zero,reset,0) && !difference(zero,reset,1),"Normal draw did not return after camouflage");
        const auto changed=difference(zero,mixed,0),complete=difference(zero,full,0);
        check(changed>100 && complete>100,"Camouflage clone produced no measured scene response");
        const auto fog=capture(.5f,0,"fog",true);check(!difference(zero,fog,1),"Camouflage alias changed another view");
        responses+=changed+complete;++cases;
        std::cout<<species<<' '<<(first_person?"FP":"world")<<": scene response "<<changed<<'/'<<complete<<" bytes; owner/reset/alias controls passed\n";
    }
    check(cases==4 && source_camouflage_fade_byte(.99609375f)==255 &&
        source_camouflage_implementation(0,254)==0 && source_camouflage_implementation(0,255)==1 &&
        source_camouflage_implementation(0x8000,255)==2,"Camouflage variant boundary changed");
    std::cout<<cases<<" posed species/view compositions, "<<responses<<" response bytes\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
