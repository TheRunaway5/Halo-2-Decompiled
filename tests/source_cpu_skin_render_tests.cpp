#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/source_cpu_skin_inputs.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/shader_animation.hpp"
#include "halo2/renderer.hpp"
#include "halo2/model.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
using namespace halo2;
namespace {
void check(bool value,const std::string&message){if(!value)throw FormatError(message);}
struct Image{unsigned width{},height{};std::vector<unsigned char>rgb;};
Image read(const std::filesystem::path&path){std::ifstream in(path,std::ios::binary);std::string p;Image i;unsigned max;in>>p>>i.width>>i.height>>max;in.get();check(p=="P6"&&max==255,"invalid Elite inspection image");i.rgb.resize(std::size_t(i.width)*i.height*3);in.read(reinterpret_cast<char*>(i.rgb.data()),i.rgb.size());check(bool(in),"short Elite inspection image");return i;}
void copy_inputs(Material&m,bool old){if(old)m.world_vertex_input_transform=SourceWorldVertexInputTransform::Preserve;
    auto child=[&](std::shared_ptr<Material>&p){if(p){p=std::make_shared<Material>(*p);copy_inputs(*p,old);}};child(m.environment_pass);for(auto&p:m.world_overlays_before)child(p);for(auto&p:m.world_overlays_after)child(p);}
unsigned check_bindings(const Material&a,const Material&b){
    check(a.source_shader_datum==b.source_shader_datum && a.forward_pass_index==b.forward_pass_index &&
        a.forward_program.words==b.forward_program.words && a.program_constants==b.program_constants &&
        a.vertex_program==b.vertex_program && a.vertex_declaration==b.vertex_declaration,
        "Elite inspection changed an authored program or constant binding");
    unsigned count=b.world_vertex_input_transform==SourceWorldVertexInputTransform::NormalizeSkinNormal;
    if(count){check(a.world_vertex_input_transform==SourceWorldVertexInputTransform::Preserve,"Elite control retained its adapter");
        const auto packed=pack_source_vertex_declaration(b.vertex_declaration,b.world_vertex_input_transform);
        check((packed[0]>>16&255)==68,"Elite normal selector lost its scoped operation");}
    check(bool(a.environment_pass)==bool(b.environment_pass) && a.world_overlays_before.size()==b.world_overlays_before.size() &&
        a.world_overlays_after.size()==b.world_overlays_after.size(),"Elite inspection changed authored child passes");
    if(a.environment_pass)count+=check_bindings(*a.environment_pass,*b.environment_pass);
    for(unsigned i=0;i<a.world_overlays_before.size();++i)count+=check_bindings(*a.world_overlays_before[i],*b.world_overlays_before[i]);
    for(unsigned i=0;i<a.world_overlays_after.size();++i)count+=check_bindings(*a.world_overlays_after[i],*b.world_overlays_after[i]);return count;}
Image capture(SDL_Window*window,const Mesh&mesh,const Camera&camera,const std::filesystem::path&path){Renderer renderer(window,mesh,true);renderer.scene_clear_color({.025f,.035f,.045f});ModelView view;view.camera=camera;view.width=640;view.height=480;view.vertices=mesh.vertices;view.full_surface=true;
    for(unsigned n=0;n<4;++n)if(renderer.draw(camera,path,{},false,{},{},{},{&view,1}))return read(path);throw FormatError("Elite inspection did not submit");}
}
int main(int argc,char**argv){try{check(argc==2,"Elite renderer inspection needs output directory");
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR),directory=std::filesystem::path(argv[1]);std::filesystem::create_directories(directory);Cache c(root/"assets/disc/maps/cyclotron.map");
    const auto tag=std::find_if(c.tags().begin(),c.tags().end(),[](const auto&t){return t.group=="mode"&&t.name=="objects\\characters\\elite\\fp_arms\\fp_arms";});check(tag!=c.tags().end(),"Elite inspection asset missing");auto m=c.load_model(tag->datum,true);
    ObjectLightingState light;light.ambient={.4f,.4f,.4f};light.primary_color={.7f,.7f,.7f};light.primary_direction={0,0,1};light.accuracy=1;
    for(auto&material:m.mesh.materials)if(!material.lods.empty()){check(prepare_dynamic_material(c,material,light,true),"Elite inspection shader not admitted");animate_forward_material(material,0,{});apply_object_lighting(material,light);}
    SourceDynamicModelShaders shaders(c,m,tag->datum);std::vector<BonePose> locals;for(auto&b:m.bones)locals.push_back(b.bind);
    // Explicit controlled local hand rotation, applied to the authored bind
    // channels. It is not an original animation/controller phase claim.
    const auto hand=std::find_if(m.bones.begin(),m.bones.end(),[&](const auto&b){return c.string_id(b.name)=="l_hand";});check(hand!=m.bones.end(),"Elite authored hand node missing");
    auto&q=locals[std::size_t(hand-m.bones.begin())].rotation;const auto old=q;constexpr float s=.3826834323650898f,k=.9238795325112867f;
    q={old[0]*k+old[3]*s,old[1]*k+old[2]*s,old[2]*k-old[1]*s,old[3]*k-old[0]*s};
    auto vertices=pose_model_locals(m,locals);SourceModelShaderView shader_view;shader_view.shader=source_shader_lod_view(0,480,false,false);shader_view.part_shader_multiplier=1;shaders.select(vertices,{10000,255},shader_view);
    Mesh after;after.materials=m.mesh.materials;after.vertices=vertices;Mesh before=after;for(auto&material:before.materials)copy_inputs(material,true);for(auto&material:after.materials)copy_inputs(material,false);
    Vec3 low{INFINITY,INFINITY,INFINITY},high{-INFINITY,-INFINITY,-INFINITY};for(const auto&v:vertices){low=minimum(low,v.position);high=maximum(high,v.position);}const auto center=(low+high)*.5f;
    Camera camera;camera.oriented=true;camera.position=center+Vec3{0,0,.35f};camera.forward={0,0,-1};camera.up={1,0,0};camera.field_of_view=60;
    check(SDL_Init(SDL_INIT_VIDEO)==0,"Elite inspection SDL initialization failed");auto*window=SDL_CreateWindow("Authored Elite arms input inspection",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);check(window,"Elite inspection window unavailable");
    unsigned bindings=0,selected_bindings=0;for(unsigned i=0;i<after.materials.size();++i){const auto n=check_bindings(before.materials[i],after.materials[i]);bindings+=n;
        if(n && std::any_of(vertices.begin(),vertices.end(),[&](const auto&v){return v.material==i;}))selected_bindings+=n;}
    check(bindings>0 && selected_bindings>0,"Elite source adapter has no selected resident binding");
    for(const auto size:std::array{std::array{640,480},std::array{1280,720}}){SDL_SetWindowSize(window,size[0],size[1]);SDL_PumpEvents();const auto suffix=std::to_string(size[0])+"x"+std::to_string(size[1]);
        const auto a=capture(window,before,camera,directory/("elite-before-"+suffix+".ppm")),b=capture(window,after,camera,directory/("elite-after-"+suffix+".ppm"));check(a.width==b.width&&a.height==b.height,"Elite inspection extent changed");unsigned count=0,max=0;
        for(unsigned i=0;i<a.rgb.size();++i){const unsigned d=unsigned(std::abs(int(a.rgb[i])-int(b.rgb[i])));count+=d!=0;max=std::max(max,d);}std::cout<<suffix<<" authored appearance: "<<count<<" responding RGB bytes, max "<<max<<'\n';
        check(b.width==unsigned(size[0])&&b.height==unsigned(size[1]),"Elite inspection ignored its drawable extent");
        check(std::set<unsigned char>(a.rgb.begin(),a.rgb.end()).size()>64,"authored Elite rendering was empty");
        const auto repeated=capture(window,after,camera,directory/("elite-repeat-"+suffix+".ppm"));
        check(repeated.width==b.width&&repeated.height==b.height&&repeated.rgb==b.rgb,"fresh Elite resident rendering was not deterministic");}
    SDL_DestroyWindow(window);SDL_Quit();
    std::cout<<bindings<<" scoped resident bindings, "<<selected_bindings<<" selected bindings. Authored Elite appearance uses explicit bind channels plus45degree left-hand rotation, camera above bounds, authored shader constants/images and controlled ambient/primary. Neutral authored output is an asset inspection; no original gameplay-frame match or visible correction claim.\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';SDL_Quit();return 1;}}
