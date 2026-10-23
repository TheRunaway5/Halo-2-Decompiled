#define SDL_MAIN_HANDLED
#include "halo2/renderer.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/object_lighting.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void check(bool ok,const char* why){if(!ok)throw FormatError(why);}
std::vector<unsigned char> rgb(const std::filesystem::path& path) {
    std::ifstream in(path,std::ios::binary);std::string magic;unsigned width{},height{},max{};
    in>>magic>>width>>height>>max;in.get();check(magic=="P6" && max==255,"Bad equipment frame");
    std::vector<unsigned char> data(std::size_t(width)*height*3);in.read(reinterpret_cast<char*>(data.data()),data.size());
    check(bool(in),"Truncated equipment frame");return data;
}
}
int main(int argc,char** argv)try {
    check(argc==4,"Supply prepared MCC Headlong, owned image directory and report path");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::getenv("DISPLAY"))return 77;
    check(std::getenv("HALO2_TEST_SCRATCH"),"Equipment GPU regression requires owned scratch");
    Cache cache(argv[1]);check(cache.is_mcc(),"Equipment GPU acceptance requires prepared MCC assets");
    SDL_SetMainReady();check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> window(SDL_CreateWindow("Equipment authored materials",0,0,640,480,
        SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN),&SDL_DestroyWindow);check(bool(window),SDL_GetError());
    const std::filesystem::path output=argv[2];std::filesystem::create_directories(output);
    const std::set<std::string> names{"battle_rifle_ammo","pistol_ammo","needler_ammo","shotgun_ammo","smg_ammo",
        "sniper_rifle_ammo","rocket_launcher_ammo","active_camouflage","over_shield","frag_grenade","plasma_grenade"};
    nlohmann::json report={{"passed",false},{"cases",nlohmann::json::array()},{"validation_layer",true},
        {"scope","Eleven original equipment model families with prepared MCC textures, authored passes and bind hierarchies. Declared uniform object lighting; live pickup lighting remains the ordinary runtime's owner."}};
    unsigned cases{};
    for(const auto& tag:cache.select_tags())if(tag.group=="eqip") {
        const auto parent=tag.name.substr(0,tag.name.find_last_of('\\'));
        const auto family=parent.substr(parent.find_last_of('\\')+1);
        if(!names.contains(family))continue;
        const auto definition=cache.load_initial_model_definition(tag.datum);
        auto model=cache.load_model(definition.render_model,true);Mesh mesh=model.mesh;mesh.vertices=pose_model(model);
        ObjectLightingState light;light.ambient={1,1,1};light.primary_color={1,1,1};light.primary_direction={1,0,0};
        for(auto &m:mesh.materials)if(!m.lods.empty())check(prepare_dynamic_material(cache,m,light,true,0.f),"Equipment authored material rejected");
        Vec3 lo{1e9f,1e9f,1e9f},hi{-1e9f,-1e9f,-1e9f};
        for(const auto &v:mesh.vertices) {
            lo.x=std::min(lo.x,v.position.x);lo.y=std::min(lo.y,v.position.y);lo.z=std::min(lo.z,v.position.z);
            hi.x=std::max(hi.x,v.position.x);hi.y=std::max(hi.y,v.position.y);hi.z=std::max(hi.z,v.position.z);
        }
        const auto center=(lo+hi)*.5f;const auto radius=std::max(length(hi-lo)*.5f,.02f);
        Camera camera;camera.position=center+Vec3{radius*2.6f,0,0};camera.yaw=3.14159265358979323846f;
        Renderer renderer(window.get(),mesh,true);renderer.scene_clear_color({.1f,.15f,.2f});
        ModelView view;view.camera=camera;view.width=640;view.height=480;view.full_surface=true;view.vertices=mesh.vertices;
        const auto capture=[&](const char* name) {
            const auto path=output/(family+"-"+name+".ppm");
            for(unsigned n=0;n<4;++n)if(renderer.draw(camera,path,{},false,{},{},{},{&view,1}))return rgb(path);
            throw FormatError("Equipment frame did not complete");
        };
        const auto first=capture("authored"),repeat=capture("repeat");
        check(first==repeat,"Equipment draw changed without source input changes");
        view.vertices={};const auto empty=capture("empty");unsigned changed{};
        for(unsigned n=0;n<first.size();++n)changed+=first[n]!=empty[n];
        check(changed>30,"Authored equipment model produced no measured pixels");
        report["cases"].push_back({{"name",tag.name},{"shader_materials",mesh.materials.size()},
            {"vertices",mesh.vertices.size()},{"visible_rgb_bytes",changed},{"passed",true}});++cases;
        std::cout<<family<<": "<<changed<<" visible RGB bytes\n";
    }
    check(cases==11,"Equipment GPU cohort omitted an original model family");report["passed"]=true;
    std::ofstream file(argv[3]);file<<report.dump(2)<<'\n';check(bool(file),"Cannot retain equipment GPU report");return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
