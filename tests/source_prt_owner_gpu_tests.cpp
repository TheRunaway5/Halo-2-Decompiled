#include "../src/gameplay/objects/source_object_geometry.hpp"
#include "halo2/source_character_shaders.hpp"
#include "halo2/renderer.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
struct Picture {unsigned width{},height{};std::vector<unsigned char> pixels;};
Picture read(const std::filesystem::path &path) {
    std::ifstream file(path,std::ios::binary);std::string magic;unsigned maximum;Picture result;
    file>>magic>>result.width>>result.height>>maximum;file.get();
    check(magic=="P6" && maximum==255,"invalid PRT owner picture");
    result.pixels.resize(std::size_t(result.width)*result.height*3);
    file.read(reinterpret_cast<char*>(result.pixels.data()),result.pixels.size());
    check(bool(file),"truncated PRT owner picture");return result;
}
Picture capture(Renderer &renderer,std::span<const ModelView> views,const std::filesystem::path &path) {
    for(unsigned attempt=0;attempt<5;++attempt)
        if(renderer.draw(views.front().camera,path,{},false,{},{},{},views))return read(path);
    throw FormatError("PRT owner frame not submitted");
}
Material independent(const Material &source) {
    auto result=source;
    const auto copy=[](const std::shared_ptr<Material> &p){return p?std::make_shared<Material>(independent(*p)):nullptr;};
    result.environment_pass=copy(source.environment_pass);result.planar_fog_pass=copy(source.planar_fog_pass);
    for(auto &p:result.world_overlays_before)p=copy(p);
    for(auto &p:result.world_overlays_after)p=copy(p);
    for(auto &p:result.world_bloom_passes)p=copy(p);
    for(auto &p:result.world_planar_passes)p=copy(p);
    return result;
}
using Window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>;
Window window(){return Window(SDL_CreateWindow("Native Scarab PRT owner diagnostic",0,0,960,720,
    SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN|SDL_WINDOW_RESIZABLE),SDL_DestroyWindow);}
void verify(const Cache &cache,std::uint32_t definition,unsigned bsp,const std::filesystem::path &output) {
    const auto initial=cache.load_initial_model_definition(definition);
    const auto selection=cache.load_initial_model_selection(definition,0);
    const auto sh=cache.load_source_prt_lighting(bsp);
    check(sh.structure_matches && !sh.records.empty(),"vehicle PRT has no authored current BSP SH bank");
    SourceCharacterShaderInputs inputs(cache,definition);
    check(inputs.object_type()==1,"PRT fixture did not select a vehicle owner");
    ScenarioObject object;object.definition=definition;object.type=1;
    SourceObjectGeometry geometry;geometry.load(cache,object,initial,selection,initial.render_model);
    Transform root;root.scale=1;root.basis={Vec3{1,0,0},Vec3{0,1,0},Vec3{0,0,1}};
    root.position=sh.records.front().position;
    const auto radius=inputs.sphere(root,1).radius;check(radius>0,"PRT vehicle has no source coverage radius");
    std::array<std::vector<Transform>,2> poses;
    for(unsigned n=0;n<2;++n) {
        const auto basis=authored_basis({n?.65f:-.65f,0,0});
        const auto placement=source_object_transform(root.position+Vec3{0,(n?1.2f:-1.2f)*radius,0},basis.forward,basis.up,1,false);
        poses[n]=pose_model_nodes_world(geometry.model(),placement);
    }
    geometry.pose_world([&](const RenderModel &model){return skin_model_world_nodes(model,poses[0]);});
    Mesh mesh;geometry.prepare(cache,cache.load_object_colors(definition),poses[0],sh,mesh,0.f);
    unsigned bindings=0;
    for(unsigned lod=0;lod<6;++lod) {
        const auto banks=geometry.owner_banks(cache,poses[0],sh,lod);
        check(!banks.empty() && !geometry.at(lod).vertices.empty(),"PRT vehicle lost an authored geometry/section bank");
        for(const auto &binding:banks)check(binding.bank->prt_inputs && binding.bank->first_constant==68,
            "PRT owner coefficient bank lost its original declaration");
        bindings+=unsigned(banks.size());
    }
    std::array<std::vector<Vertex>,2> vertices,reference_vertices;
    std::array<std::vector<SourcePrtMaterialBank>,2> banks;
    for(unsigned n=0;n<2;++n) {
        geometry.pose_world([&](const RenderModel &model){return skin_model_world_nodes(model,poses[n]);});
        vertices[n]=geometry.at(5).vertices;banks[n]=geometry.owner_banks(cache,poses[n],sh,5);
    }
    check(banks[0].size()==banks[1].size(),"same source section acquired another bank extent");
    bool different=false;
    for(unsigned n=0;n<banks[0].size();++n)different|=banks[0][n].bank->constants!=banks[1][n].bank->constants;
    Mesh separate;
    std::array<ModelObjectDraw,2> owners,reference_owners;
    for(unsigned n=0;n<2;++n) {
        const unsigned offset=unsigned(separate.materials.size());
        for(const auto &material:mesh.materials)separate.materials.push_back(independent(material));
        for(const auto &bank:banks[n])separate.materials.at(offset+bank.material).environment_pass->vertex_bank=bank.bank;
        reference_vertices[n]=vertices[n];for(auto &vertex:reference_vertices[n])vertex.material+=offset;
        owners[n]={vertices[n],{},{},{},banks[n]};reference_owners[n]={reference_vertices[n]};
    }
    Vec3 lower{INFINITY,INFINITY,INFINITY},upper{-INFINITY,-INFINITY,-INFINITY};
    for(const auto &owner:vertices)for(const auto &vertex:owner){lower=minimum(lower,vertex.position);upper=maximum(upper,vertex.position);}
    const auto target=(lower+upper)*.5f;Camera camera;camera.oriented=true;camera.field_of_view=70;
    camera.position=target+normalized(Vec3{-1,-.3f,.45f})*length(upper-lower)*1.5f;
    camera.forward=normalized(target-camera.position);camera.up=normalized(cross(camera.forward,normalized(cross(Vec3{0,0,1},camera.forward))));
    auto actual_window=window(),reference_window=window();check(actual_window && reference_window,SDL_GetError());
    std::filesystem::create_directories(output);
    Renderer actual(actual_window.get(),mesh,true),reference(reference_window.get(),separate,true);
    for(const auto size:std::array<std::array<int,2>,2>{{{960,720},{1280,720}}}) {
        SDL_SetWindowSize(actual_window.get(),size[0],size[1]);SDL_SetWindowSize(reference_window.get(),size[0],size[1]);
        SDL_PumpEvents();actual.resized();reference.resized();
        for(unsigned players:{1u,4u}) {
            std::vector<ModelView> views(players),references(players);
            for(unsigned n=0;n<players;++n) {
                auto &view=views[n];view.camera=camera;view.full_surface=true;
                view.left=players==1?0.f:float(n%2)*320.f;view.top=players==1?0.f:float(n/2)*240.f;
                view.width=players==1?640.f:320.f;view.height=players==1?480.f:240.f;view.objects=owners;
                references[n]=view;references[n].objects=reference_owners;
            }
            const auto stem=std::to_string(size[0])+"x"+std::to_string(size[1])+"-"+std::to_string(players);
            const auto shared=capture(actual,views,output/(stem+"-shared.ppm"));
            const auto isolated=capture(reference,references,output/(stem+"-independent.ppm"));
            check(shared.pixels==isolated.pixels,"shared PRT image slots leaked another owner's coefficient bank");
            std::swap(owners[0].prt_banks,owners[1].prt_banks);
            const auto swapped=capture(actual,views,output/(stem+"-swapped.ppm"));
            std::swap(owners[0].prt_banks,owners[1].prt_banks);
            unsigned changed=0;for(unsigned p=0;p<shared.pixels.size();++p)changed+=shared.pixels[p]!=swapped.pixels[p];
            // Actual SH can be isotropic; tiny coefficient differences can
            // also quantize to identical pixels. Exact shared/isolated output
            // above is the pass criterion. Report the swap response separately.
            check(different || changed==0,"identical PRT coefficient banks changed the image when swapped");
            for(auto &view:views)view.objects={};capture(actual,views,output/(stem+"-retired.ppm"));
            for(auto &view:views)view.objects=owners;
            check(shared.pixels==capture(actual,views,output/(stem+"-restored.ppm")).pixels,"retired PRT owner descriptors survived reuse");
            std::cout<<definition<<' '<<stem<<": exact shared/independent pixels; "<<changed<<" changed channels when banks are swapped\n"<<std::flush;
        }
    }
    std::cout<<definition<<": "<<bindings<<" authored material/section bindings across six geometry LODs\n";
}
}
int main(int argc,char **argv) {
    std::ostringstream diagnostics;auto *previous=std::cerr.rdbuf(diagnostics.rdbuf());
    try {
        check(argc==5,"Supply MAP SHARED_MAPS BSP NEW_OUTPUT");
        if(!std::getenv("DISPLAY")){std::cerr.rdbuf(previous);return 77;}
        const std::filesystem::path output=argv[4];check(!std::filesystem::exists(output),"PRT diagnostic output exists");
        Cache cache(argv[1],argv[2]);check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
        struct Sdl{~Sdl(){SDL_Quit();}} sdl;
        unsigned models=0;
        for(const auto &tag:cache.tags())if(tag.group=="vehi") {
            const auto initial=cache.load_initial_model_definition(tag.datum);
            if(initial.render_model==null_datum || !cache.load_source_prt_definition(initial.render_model))continue;
            verify(cache,tag.datum,unsigned(std::stoul(argv[3])),output/std::to_string(tag.datum));++models;
        }
        check(models>0,"map has no authored PRT vehicle fixtures");
        check(diagnostics.str().find("Vulkan validation:")==std::string::npos,"PRT owner Vulkan validation diagnostics");
        std::cerr.rdbuf(previous);std::cout<<"PASS "<<models<<" PRT vehicle definitions; native diagnostics, not matched original frames\n";
        return 0;
    }catch(const std::exception &e){std::cerr.rdbuf(previous);std::cerr<<diagnostics.str()<<e.what()<<'\n';return 1;}
}
