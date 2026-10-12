#include "halo2/renderer.hpp"
#include "halo2/sky.hpp"
#include "halo2/reference_player.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/shader_animation.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void require(bool b,const std::string &s){if(!b)throw FormatError(s);}
struct Image{unsigned width{},height{};std::vector<unsigned char> rgb;};
Image read(const std::filesystem::path &path){std::ifstream f(path,std::ios::binary);Image i;std::string m;unsigned max;f>>m>>i.width>>i.height>>max;f.get();require(m=="P6"&&max==255,"invalid sky image");i.rgb.resize(std::size_t(i.width)*i.height*3);f.read(reinterpret_cast<char*>(i.rgb.data()),i.rgb.size());require(bool(f),"short sky image");return i;}
Image capture(Renderer &r,const ModelView &v,const std::filesystem::path &p){for(unsigned n=0;n<5;++n)if(r.draw(v.camera,p,{},false,{},{},{},{&v,1}))return read(p);throw FormatError("sky frame did not submit");}
struct ValidationLog{std::ostringstream output;std::streambuf *previous=std::cerr.rdbuf(output.rdbuf());~ValidationLog(){std::cerr.rdbuf(previous);if(!output.str().empty())std::cerr<<output.str();}};
struct Run{std::size_t first{},count{};unsigned material{},part{};};
ModelView view(const ReferencePlayerState &ref,unsigned w,unsigned h,std::span<const Vertex> vertices){
    ModelView v;v.width=640;v.height=480;v.full_surface=true;v.background=vertices;v.atmosphere=BspFogSample{};
    v.camera.oriented=true;v.camera.position=ref.camera_position;v.camera.forward=ref.camera_forward;v.camera.up=ref.camera_up;
    v.camera.projection_vertical_angle=ref.camera_vertical_angle;v.camera.field_of_view=ref.camera_vertical_angle*57.295779513f;
    const RenderViewport outer{0,0,float(w),float(h)},safe{float(w)*.075f,float(h)*.075f,float(w)*.85f,float(h)*.85f};
    const auto p=source_camera_projection(ref.camera_vertical_angle,outer,safe,false);v.camera.projection_clip_scale=p.clip_scale;v.camera.projection_clip_offset=p.clip_offset;
    SourceFrustumProjectionInput f;f.projection_angle=ref.camera_vertical_angle;f.outer={0,0,std::int16_t(h),std::int16_t(w)};
    f.safe={std::int16_t(h*.075f),std::int16_t(w*.075f),std::int16_t(h*.925f),std::int16_t(w*.925f)};f.scenario_present=true;f.scenario_type=1;
    const auto b=source_view_projection({ref.camera_position,ref.camera_forward,ref.camera_up,.015625f,1024.f,{},source_frustum_projection(f).projection_program});
    v.source_view_shader_constants.emplace();v.source_view_shader_constants->world=source_view_shader_constants(b,1024.f,false);return v;
}
}
int main(int argc,char **argv){try{
    require(argc==2,"sky test needs output directory or --inventory");const auto root=std::filesystem::path(HALO2_SOURCE_DIR);Cache cache(root/"assets/disc/maps/zanzibar.map");
    const auto sky=cache.load_visible_sky(0);require(bool(sky),"Zanzibar sky missing");auto model=cache.load_model(sky->model,true);
    const auto atmosphere=cache.load_bsp_atmosphere(0);
    const auto ref=load_reference_player_state(root/"tests/fixtures/retail_zanzibar_player.reference");auto vertices=pose_sky(model,*sky,ref.camera_position);std::vector<Run> runs;
    for(unsigned i=0;i<vertices.size();++i){const auto &v=vertices[i];if(runs.empty()||runs.back().material!=v.material||runs.back().part!=v.source_part)runs.push_back({i,0,v.material,v.source_part});++runs.back().count;}
    for(const auto &r:runs){const auto &m=model.mesh.materials.at(r.material);std::cout<<"part "<<r.part<<" material "<<r.material<<" vertices "<<r.count<<" "<<m.name<<" images "<<m.diffuse.width<<","<<m.detail.width<<","<<m.bump.width<<","<<m.auxiliary.width<<'\n';}
    require(runs.size()==3,"source sky fixture part count changed");require(runs[0].count==180&&runs[1].count==96&&runs[2].count==6,"authored sky part order does not match original20 8097 then8102 then8110");
    require(runs[0].material>runs[1].material&&runs[1].material>runs[2].material,"sky negative control no longer exercises sorted-material reversal");
    if(std::string(argv[1])=="--inventory")return 0;
    const auto output=std::filesystem::path(argv[1]);std::filesystem::create_directories(output);
    std::vector<Vertex> reversed;for(auto it=runs.rbegin();it!=runs.rend();++it)reversed.insert(reversed.end(),vertices.begin()+it->first,vertices.begin()+it->first+it->count);
    require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());auto *window=SDL_CreateWindow("Authored sky layer ordering",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN);require(window,SDL_GetError());
    Mesh mesh=model.mesh;mesh.vertices.clear();ValidationLog diagnostics;
    {Renderer reused(window,mesh,true);reused.scene_bsp_atmosphere(atmosphere);for(const auto extent:std::array{std::array{640,480},std::array{1280,720},std::array{640,480}}){const unsigned w=extent[0],h=extent[1];SDL_SetWindowSize(window,w,h);SDL_PumpEvents();reused.resized();
        const auto suffix=std::to_string(w)+"x"+std::to_string(h);const auto actual=view(ref,w,h,vertices),negative=view(ref,w,h,reversed);
        const auto a=capture(reused,actual,output/("ordered-"+suffix+".ppm")),b=capture(reused,negative,output/("reversed-"+suffix+".ppm"));
        require(a.width==w&&a.height==h,"sky ignored drawable resolution");std::size_t changed=0;unsigned maximum=0;for(unsigned i=0;i<a.rgb.size();++i){const auto d=unsigned(std::abs(int(a.rgb[i])-int(b.rgb[i])));changed+=d>4;maximum=std::max(maximum,d);}
        require(changed>100&&maximum>16,"authored alpha sky layers did not respond to reversed ordering");const auto repeated=capture(reused,actual,output/("repeat-"+suffix+".ppm"));require(repeated.rgb==a.rgb,"ordered sky retained a reversed batch");
        {auto *other=SDL_CreateWindow("Fresh authored sky",0,0,w,h,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);require(other,SDL_GetError());{Renderer fresh(other,mesh,true);fresh.scene_bsp_atmosphere(atmosphere);const auto f=capture(fresh,actual,output/("fresh-"+suffix+".ppm"));require(f.rgb==a.rgb,"sky fresh/reused resize differed");}SDL_DestroyWindow(other);}
        std::cout<<suffix<<": "<<changed<<" channels respond, maximum "<<maximum<<"; repeat/fresh exact\n";
    }}SDL_DestroyWindow(window);SDL_Quit();require(diagnostics.output.str().empty(),"sky Vulkan diagnostics: "+diagnostics.output.str());
    std::cout<<"Original20 ordering is8097(A),8102(B),8110(moon). This test preserves actual authored part streams/programs/images and explicit source camera, with near-start texture phase. It proves ordering and resize/reuse; it does not claim complete original-frame appearance.\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';SDL_Quit();return 1;}}
