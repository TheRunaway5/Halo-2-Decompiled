#define SDL_MAIN_HANDLED
#include "halo2/renderer.hpp"
#include "halo2/model.hpp"
#include "halo2/content.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_camouflage_material.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/render_view.hpp"
#include "halo2/world_material.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool ok,const char* why){if(!ok)throw FormatError(why);}
struct Image {unsigned width{},height{};std::vector<unsigned char> rgb;};
struct Layout {
    unsigned width=1280,height=480,views=2;
    unsigned columns()const{return views==1?1:2;}
    unsigned rows()const{return views==4?2:1;}
    std::array<unsigned,4> rectangle(unsigned view)const {
        return {view%columns()*width/columns(),view/columns()*height/rows(),
            (view%columns()+1)*width/columns(),(view/columns()+1)*height/rows()};
    }
};
Image read(const std::filesystem::path& path) {
    Image result;std::ifstream in(path,std::ios::binary);std::string magic;unsigned max{};
    in>>magic>>result.width>>result.height>>max;in.get();check(magic=="P6" && max==255,"Bad camouflage capture");
    result.rgb.resize(std::size_t(result.width)*result.height*3);
    in.read(reinterpret_cast<char*>(result.rgb.data()),result.rgb.size());check(bool(in),"Truncated camouflage frame");return result;
}
unsigned difference(const Image& a,const Image& b,const Layout& layout,unsigned view) {
    check(a.width==b.width && a.height==b.height,"Camouflage extent changed");unsigned changed{};
    const auto r=layout.rectangle(view);
    for(unsigned y=r[1];y<r[3];++y)for(unsigned x=r[0];x<r[2];++x)
        for(unsigned c=0;c<3;++c)changed+=a.rgb[(y*a.width+x)*3+c]!=b.rgb[(y*a.width+x)*3+c];
    return changed;
}
unsigned full_scene_error(const unsigned char* actual,const unsigned char* scene) {
    // Original layer18 implementation1 retains a Fresnel highlight even at
    // primary1: scene + scene^2 * edge^2, clamped, with one weight for RGB.
    // Compare against that source envelope; plain background equality would
    // reject the original effect. The separate native fragment GPU test
    // checks the exact combiner equation and pixel-coordinate sampling.
    std::array<double,5> weights{0,1,0,0,0};
    for(unsigned c=0;c<3;++c)if(scene[c])weights[c+2]=std::clamp(
        255.0*(int(actual[c])-int(scene[c]))/(double(scene[c])*scene[c]),0.0,1.0);
    double error=765;
    for(double weight:weights) {
        double candidate=0;
        for(unsigned c=0;c<3;++c)candidate+=std::abs(double(actual[c])-
            std::min(255.0,scene[c]+weight*scene[c]*scene[c]/255.0));
        error=std::min(error,candidate);
    }
    return unsigned(std::lround(error));
}
}
int main(int argc,char** argv)try {
    check(argc>=3,"Supply map, owned capture directory, optional --extent W H / --views N / --no-validation");
    Layout layout;bool validation=true;
    for(int i=3;i<argc;++i) {
        const std::string option=argv[i];
        if(option=="--no-validation")validation=false;
        else if(option=="--extent" && i+2<argc) {
            layout.width=unsigned(std::stoul(argv[++i]));layout.height=unsigned(std::stoul(argv[++i]));
        } else if(option=="--views" && i+1<argc)layout.views=unsigned(std::stoul(argv[++i]));
        else throw FormatError("Unknown camouflage test option");
    }
    check(layout.width>=320 && layout.height>=240 && layout.width<=7680 && layout.height<=4320 &&
        (layout.views==1 || layout.views==2 || layout.views==4),"Invalid camouflage test extent/layout");
    Cache cache(argv[1]);const auto content=cache.load_content(0,true);
    SDL_SetMainReady();check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> window(SDL_CreateWindow("Camouflage scene composition",0,0,int(layout.width),int(layout.height),
        SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN),&SDL_DestroyWindow);check(bool(window),SDL_GetError());
    const std::filesystem::path output=argv[2];std::filesystem::create_directories(output);
    unsigned cases{},responses{};
    for(unsigned species:{2u,3u})for(bool first_person:{false,true})for(unsigned camera_mode:{0u,1u,2u}) {
        const bool native_camera=camera_mode!=0,source_rectangle=camera_mode==2;
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
        // Keep the scene pattern broader than the authored refraction lobes.
        // Exact source-pixel addressing/Fresnel has its own numerical GPU gate.
        constexpr unsigned checker_side=4;
        Material background;background.diffuse={checker_side,checker_side,11,
            std::vector<std::byte>(checker_side*checker_side*4),{}};
        for(unsigned y=0;y<checker_side;++y)for(unsigned x=0;x<checker_side;++x) {
            const unsigned n=(y*checker_side+x)*4;const bool lit=(x+y)%2;
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
        Renderer renderer(window.get(),mesh,validation);
        const auto capture=[&](float a,float b,const char* label,bool fog=false) {
            std::vector<CamouflageBinding> bindings(layout.views);
            std::vector<ModelView> views(layout.views);
            std::vector<ModelObjectDraw> owners(layout.views);
            for(unsigned i=0;i<layout.views;++i) {
                bindings[i]={0,root_count,i?b:a};
                auto &v=views[i];v.camera=camera;
                v.left=float(i%layout.columns())*640.f/layout.columns();v.top=float(i/layout.columns())*480.f/layout.rows();
                v.width=640.f/layout.columns();v.height=480.f/layout.rows();v.full_surface=true;
                v.background=backdrop;v.camouflage={&bindings[i],1};
                if(first_person)v.foreground=posed;else v.vertices=posed;
                if(native_camera) {
                    SourceFrustumProjectionInput input;
                    auto physical=full_surface_viewport({v.left,v.top,v.width,v.height},layout.width,layout.height);
                    if(source_rectangle) {
                        physical.left+=float(unsigned(physical.width*3.f/8.f));
                        physical.top+=float(unsigned(physical.height/4.f));
                        physical.width=float(unsigned(physical.width/2.f));
                        physical.height=float(unsigned(physical.height/2.f));
                        v.source_ui_viewport=std::array<std::int32_t,4>{std::int32_t(physical.top),std::int32_t(physical.left),
                            std::int32_t(physical.top+physical.height),std::int32_t(physical.left+physical.width)};
                    }
                    input.projection_angle=camera_vertical_field_of_view(camera,physical.width/physical.height);
                    input.outer=input.safe={std::int16_t(physical.top),std::int16_t(physical.left),
                        std::int16_t(physical.top+physical.height),std::int16_t(physical.left+physical.width)};
                    const auto bank=source_view_projection({camera.position,{-1,0,0},{0,0,1},.015625f,1024.f,{},
                        source_frustum_projection(input).projection_program});
                    SourceViewShaderConstants constants;constants.world=source_view_shader_constants(bank,1024.f,false);
                    constants.reflection=constants.world;
                    const auto fp=source_view_shader_constants(bank,1024.f,true);
                    std::copy_n(fp.begin(),4,constants.foreground.begin());v.source_view_shader_constants=constants;
                    if(!first_person) {
                        v.vertices={};owners[i].vertices=posed;owners[i].camouflage=i?b:a;
                        v.objects={&owners[i],1};
                    }
                }
                if(fog)v.camouflage_fog=SourceShaderFogContext{0x12340000u,1.f,30.f,{-1,0,0},{0,0,0}};
            }
            const auto path=output/(std::to_string(species)+(first_person?"-fp-":"-world-")+
                (source_rectangle?"native-rectangle-":native_camera?"native-":"fallback-")+label+".ppm");
            for(unsigned n=0;n<4;++n)if(renderer.draw(camera,path,{},false,{},{},{},views)) {
                auto image=read(path);
                check(image.width==layout.width && image.height==layout.height,"Camouflage drawable did not use the requested resolution");
                return image;
            }
            throw FormatError("Camouflage did not render");
        };
        const auto zero=capture(0,0,"zero"),mixed=capture(.75f,0,"mixed"),full=capture(1,0,"full"),all_full=capture(1,1,"all-full");
        auto saved_pose=std::move(posed);posed.clear();const auto background_only=capture(0,0,"background");posed=std::move(saved_pose);
        std::uint64_t total_error{},total_normal{};
        for(unsigned view=0;view<layout.views;++view) {
            unsigned opaque_before{},opaque_after{};std::uint64_t full_error{},normal_error{};
            const auto rectangle=layout.rectangle(view);
            for(unsigned y=rectangle[1];y<rectangle[3];++y)for(unsigned x=rectangle[0];x<rectangle[2];++x) {
                const auto n=(y*zero.width+x)*3;
                if(!zero.rgb[n] && !zero.rgb[n+1] && !zero.rgb[n+2] &&
                    std::max({background_only.rgb[n],background_only.rgb[n+1],background_only.rgb[n+2]})>32) {
                    ++opaque_before;opaque_after+=!all_full.rgb[n] && !all_full.rgb[n+1] && !all_full.rgb[n+2];
                    full_error+=full_scene_error(&all_full.rgb[n],&background_only.rgb[n]);
                    for(unsigned c=0;c<3;++c) {
                        normal_error+=background_only.rgb[n+c];
                    }
                }
            }
            std::cout<<"Reconstruction "<<species<<' '<<first_person<<' '<<native_camera<<" view="<<view
                <<" opaque="<<opaque_before<<'/'<<opaque_after<<" error="<<full_error<<'/'<<normal_error<<std::endl;
            check(opaque_before>100 && opaque_after*10<opaque_before,"Full camouflage retained opaque body parts");
            check(full_error*8<normal_error,"Full camouflage did not reconstruct the scene behind its silhouette");
            total_error+=full_error;total_normal+=normal_error;
        }
        const auto delta=[&](const Image& a,const Image& b,unsigned view){return difference(a,b,layout,view);};
        const auto unchanged_other_views=[&](const Image& a,const Image& b) {
            for(unsigned view=1;view<layout.views;++view)check(!delta(a,b,view),"Camouflage changed the other player's view");
        };
        const auto repeat=capture(.75f,0,"repeat"),swapped=capture(0,.75f,"swapped"),reset=capture(0,0,"reset");
        unchanged_other_views(zero,mixed);unchanged_other_views(mixed,repeat);unchanged_other_views(zero,reset);
        check(!delta(mixed,repeat,0),"Camouflage retained another frame's scene");
        check(!delta(zero,swapped,0),"Swapped camouflage leaked to the prior owner");
        check(!delta(zero,reset,0),"Normal draw did not return after camouflage");
        const auto changed=delta(zero,mixed,0),complete=delta(zero,full,0);
        check(changed>100 && complete>100,"Camouflage clone produced no measured scene response");
        const auto fog=capture(.5f,0,"fog",true);unchanged_other_views(zero,fog);
        const auto both_fog=capture(.5f,.5f,"both-fog",true),both_fog_repeat=capture(.5f,.5f,"both-fog-repeat",true);
        for(unsigned view=0;view<layout.views;++view)check(delta(zero,both_fog,view)>100 &&
            !delta(both_fog,both_fog_repeat,view),"Camouflage fog phases lost their shared command/fence lifecycle or view isolation");
        const auto saved_layout=layout;
        layout.width=800;layout.height=600;
        SDL_SetWindowSize(window.get(),int(layout.width),int(layout.height));SDL_PumpEvents();renderer.resized();
        (void)capture(1,1,"resized");
        layout=saved_layout;
        SDL_SetWindowSize(window.get(),int(layout.width),int(layout.height));SDL_PumpEvents();renderer.resized();
        const auto restored=capture(1,1,"restored");
        for(unsigned view=0;view<layout.views;++view)check(!delta(all_full,restored,view),
            "Camouflage scene copy or viewport changed after resolution restoration");
        responses+=changed+complete;++cases;
        std::cout<<layout.width<<'x'<<layout.height<<" views="<<layout.views<<' '<<species<<' '<<(first_person?"FP":"world")
            <<" camera_mode="<<camera_mode
            <<": scene response "<<changed<<'/'<<complete<<" bytes; reconstruction "<<total_error<<'/'<<total_normal
            <<"; owner/reset/alias/resize controls passed\n";
    }
    check(cases==12 && source_camouflage_fade_byte(.99609375f)==255 &&
        source_camouflage_implementation(0,254)==0 && source_camouflage_implementation(0,255)==1 &&
        source_camouflage_implementation(0x8000,255)==2,"Camouflage variant boundary changed");
    std::cout<<cases<<" posed species/view compositions, "<<responses<<" response bytes\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
