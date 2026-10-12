#include "halo2/source_machine_scene.hpp"
#include "halo2/source_prop_scene.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/collision.hpp"
#include "halo2/content.hpp"
#include "halo2/renderer.hpp"
#include "halo2/math.hpp"
#include <SDL_vulkan.h>
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>

namespace {
using namespace halo2;
void require(bool value,const std::string &message) {
    if(!value)throw std::runtime_error(message);
}
struct Image {unsigned width{},height{};std::vector<unsigned char> pixels;};
Image read(const std::filesystem::path &path) {
    std::ifstream file(path,std::ios::binary);std::string magic;unsigned maximum{};Image image;
    file>>magic>>image.width>>image.height>>maximum;file.get();
    require(magic=="P6" && maximum==255,"invalid machine capture: "+path.string());
    image.pixels.resize(std::size_t(image.width)*image.height*3);
    file.read(reinterpret_cast<char*>(image.pixels.data()),std::streamsize(image.pixels.size()));
    require(bool(file),"truncated machine capture: "+path.string());return image;
}
struct Difference {unsigned maximum{};std::size_t changed{};};
Difference difference(const Image &a,const Image &b) {
    require(a.width==b.width && a.height==b.height,"machine capture extent differs");
    Difference result;
    for(std::size_t i=0;i<a.pixels.size();++i) {
        const auto error=unsigned(std::abs(int(a.pixels[i])-int(b.pixels[i])));
        result.maximum=std::max(result.maximum,error);result.changed+=error>1;
    }return result;
}
// Retain warnings through renderer destruction, including synchronization and
// descriptor lifetime diagnostics, rather than checking draw's return alone.
struct ValidationLog {
    std::ostringstream output;
    std::streambuf *previous=std::cerr.rdbuf(output.rdbuf());
    ~ValidationLog(){std::cerr.rdbuf(previous);if(!output.str().empty())std::cerr<<output.str();}
};
using Window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>;
Window make_window(int width,int height) {
    Window window(SDL_CreateWindow("Source machine bank update",0,0,width,height,
        SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN),&SDL_DestroyWindow);
    require(bool(window),SDL_GetError());return window;
}
Material copy_material(const Material &source) {
    Material result=source;
    const auto copy=[](const std::shared_ptr<Material> &value) {
        return value?std::make_shared<Material>(copy_material(*value)):std::shared_ptr<Material>{};
    };
    result.environment_pass=copy(source.environment_pass);result.planar_fog_pass=copy(source.planar_fog_pass);
    for(unsigned i=0;i<source.world_planar_passes.size();++i)result.world_planar_passes[i]=copy(source.world_planar_passes[i]);
    for(unsigned i=0;i<source.world_overlays_before.size();++i)result.world_overlays_before[i]=copy(source.world_overlays_before[i]);
    for(unsigned i=0;i<source.world_overlays_after.size();++i)result.world_overlays_after[i]=copy(source.world_overlays_after[i]);
    for(unsigned i=0;i<source.world_bloom_passes.size();++i)result.world_bloom_passes[i]=copy(source.world_bloom_passes[i]);
    return result;
}
Mesh copy_mesh(const Mesh &source) {
    Mesh result=source;
    for(unsigned i=0;i<source.materials.size();++i)result.materials[i]=copy_material(source.materials[i]);
    return result;
}
Mesh zero_coefficients(const Mesh &source) {
    auto result=copy_mesh(source);
    for(auto &material:result.materials) {
        auto pass=material.environment_pass;
        if(!pass || pass->forward_vertex!=9)continue;
        require(bool(pass->vertex_bank),"negative control missing PRT bank");
        auto bank=std::make_shared<VertexProgramBank>(*pass->vertex_bank);
        for(unsigned i=0;i<bank->constants.size();++i)if(bank->first_constant+i>=70)bank->constants[i]={};
        pass->vertex_bank=std::move(bank);
    }return result;
}
std::vector<std::uint32_t> coefficient_words(const Mesh &mesh) {
    std::vector<std::uint32_t> result;
    for(const auto &material:mesh.materials) {
        const auto pass=material.environment_pass;
        if(!pass || pass->forward_vertex!=9)continue;
        require(pass->vertex_bank && pass->vertex_bank->prt_inputs,"source machine PRT bank/input binding missing");
        const auto &bank=*pass->vertex_bank;
        // Ignore c68 incident and c69 fixed scales. Require actual c70+ SH/PCA
        // words, since changing an unrelated light would not test this bank.
        for(unsigned i=0;i<bank.constants.size();++i)if(bank.first_constant+i>=70)
            for(const auto value:bank.constants[i])result.push_back(std::bit_cast<std::uint32_t>(value));
    }
    require(!result.empty(),"fixture contains no original PRT coefficient words");return result;
}
Camera camera_for(const Cache &cache,const LevelContent &content,const Mesh &mesh,
                  std::span<const Vertex> vertices,bool captured_zanzibar) {
    Camera camera;camera.oriented=true;
    if(captured_zanzibar) {
        // Literal final capture20 render camera, independently regenerated
        // through retail 2F970/2FB70/12FA0: all48 c0..11 words match that draw.
        camera.position={-10.165480613708496f,13.262889862060547f,.7419255971908569f};
        camera.forward={.267412394285202f,-.9597073793411255f,.0863272175192833f};
        camera.up={-.023171469569206238f,.08315932005643845f,.9962669610977173f};
    } else {
        Vec3 lower{INFINITY,INFINITY,INFINITY},upper{-INFINITY,-INFINITY,-INFINITY};
        const auto first_prt=std::find_if(mesh.materials.begin(),mesh.materials.end(),[](const auto &material) {
            return material.environment_pass && material.environment_pass->forward_vertex==9;
        });
        require(first_prt!=mesh.materials.end(),"fixture has no PRT machine material");
        const auto selected_material=unsigned(first_prt-mesh.materials.begin());
        for(const auto &vertex:vertices) {
            if(vertex.material!=selected_material)continue;
            lower=minimum(lower,vertex.position);upper=maximum(upper,vertex.position);
        }
        require(std::isfinite(lower.x),"fixture has no PRT machine geometry");
        const auto target=(lower+upper)*.5f;
        // A diagnostic view of one authored crane section, outside its bounds.
        // Framing both distant placements together leaves only tiny strips.
        // This does not define a spawn or an original-frame camera fixture;
        // only Zanzibar has that source-matched view.
        camera.position=target+normalized(Vec3{-1,-1,.35f})*length(upper-lower)*1.5f;
        camera.forward=normalized(target-camera.position);
        const auto left=normalized(cross(Vec3{0,0,1},camera.forward));
        camera.up=normalized(cross(camera.forward,left));
    }
    return camera;
}
std::array<ModelView,2> views_for(Camera camera,float authored_fov,unsigned players,
        int width,int height,std::span<const Vertex> geometry,std::span<const ObjectLightingBinding> bindings) {
    std::array<ModelView,2> views;
    for(unsigned i=0;i<players;++i) {
        const auto grid=source_view_grid(players,i,false);
        const auto angles=source_camera_angles(authored_fov,1.3333333730697632f,grid.grid,grid.span);
        auto &view=views[i];view.camera=camera;view.camera.field_of_view=angles.projection_angle*57.295779513f;
        view.camera.projection_vertical_angle=angles.projection_angle;
        view.left=float(i*640/players);view.width=float(640/players);view.height=480;
        const RenderViewport outer{view.left*float(width)/640,0,
            view.width*float(width)/640,float(height)};
        const RenderViewport safe{outer.left+outer.width*.075f,outer.top+outer.height*.075f,
            outer.width*.85f,outer.height*.85f};
        const auto projection=source_camera_projection(angles.projection_angle,outer,safe,false);
        view.camera.projection_clip_scale=projection.clip_scale;
        view.camera.projection_clip_offset=projection.clip_offset;
        view.vertices=geometry;view.object_lighting=bindings;view.full_surface=true;
    }return views;
}
Image capture(Renderer &renderer,std::span<const ModelView> views,const std::filesystem::path &path) {
    for(unsigned attempt=0;attempt<5;++attempt)
        if(renderer.draw(views.front().camera,path,{},false,{},{},{},views))return read(path);
    throw std::runtime_error("source machine frame not submitted: "+path.string());
}
unsigned verify(SDL_Window *window,const std::filesystem::path &path,const std::filesystem::path &output) {
    const Cache cache(path);const auto content=cache.load_content(0,true);CollisionWorld collision(content.collision);
    Mesh mesh;SourceMachineScene scene(cache,content.objects,0,mesh,collision);
    require(scene.stats().prt_materials>0 && scene.stats().position_overlays>0,"empty machine/PRT fixture");
    std::vector<Vertex> geometry;std::vector<ObjectLightingBinding> bindings;
    scene.update(0,0,{},1,mesh);scene.append(geometry,bindings);
    require(!geometry.empty() && !bindings.empty(),"machine fixture has no geometry or light owners");
    auto camera=camera_for(cache,content,mesh,geometry,path.stem()=="zanzibar");
    // This immutable material snapshot is also used as a stale-bank negative
    // control with current geometry/current owner lighting at later ticks.
    const auto initial_mesh=copy_mesh(mesh);auto previous_words=coefficient_words(mesh);
    std::cout<<path.stem().string()<<": "<<scene.stats().render_models<<" models, "
        <<scene.stats().prt_materials<<" PRT materials, "<<scene.stats().initial_state_only
        <<" initial-state-only; "<<std::count_if(previous_words.begin(),previous_words.end(),
        [](auto word){return (word&0x7fffffffu)!=0;})<<" nonzero coefficient words\n"<<std::flush;
    SDL_SetWindowSize(window,640,480);SDL_PumpEvents();Renderer renderer(window,mesh,true);
    unsigned checked=0;bool visible_bank_change=false;
    const std::array<std::uint64_t,3> ticks{0,1,30};
    const std::array<std::array<int,2>,3> extents{{{640,480},{1280,720},{640,480}}};
    for(unsigned step=0;step<ticks.size();++step) {
        scene.update(ticks[step],int(ticks[step]),camera.position,1,mesh);
        geometry.clear();bindings.clear();scene.append(geometry,bindings);
        // Keep the moving Triplicate train inside its diagnostic view. Every
        // compared renderer receives these same frozen current vertices and
        // camera; Zanzibar retains the literal source-frame camera throughout.
        if(path.stem()=="triplicate")camera=camera_for(cache,content,mesh,geometry,false);
        const auto current_words=coefficient_words(mesh);
        require(current_words.size()==previous_words.size(),"machine coefficient bank layout changed");
        const auto changed=std::inner_product(current_words.begin(),current_words.end(),previous_words.begin(),
            std::size_t(0),std::plus<>{},[](auto a,auto b){return std::size_t(a!=b);});
        if(step && path.stem()=="zanzibar")
            require(changed>0,"authored moving machine changed no source PRT coefficients");
        previous_words=current_words;
        const auto [width,height]=extents[step];
        SDL_SetWindowSize(window,width,height);SDL_PumpEvents();renderer.resized();
        renderer.update_material_constants(mesh.materials);
        for(unsigned players=1;players<=2;++players) {
            auto views=views_for(camera,content.player.camera_field_of_view,players,width,height,geometry,bindings);
            const auto selected=std::span(views.data(),players);
            const auto name=path.stem().string()+"-tick"+std::to_string(ticks[step])+"-"+
                std::to_string(width)+"x"+std::to_string(height)+"-"+std::to_string(players)+"view";
            const auto actual=capture(renderer,selected,output/(name+"-reused.ppm"));
            require(actual.width==unsigned(width) && actual.height==unsigned(height),"stale machine swapchain extent");
            auto expected_window=make_window(width,height);
            Renderer fresh(expected_window.get(),mesh,true);
            const auto expected=capture(fresh,selected,output/(name+"-fresh.ppm"));
            const auto error=difference(actual,expected);
            require(error.maximum<=1,"source machine PRT/input bank update differs from fresh renderer: "+name+
                " maximum "+std::to_string(error.maximum));
            const auto [minimum,maximum]=std::minmax_element(actual.pixels.begin(),actual.pixels.end());
            require(*maximum>*minimum+8 && std::count_if(actual.pixels.begin(),actual.pixels.end(),
                [](auto value){return value>16;})>300,"machine PRT render is blank/uniform: "+name);
            if(!step && players==1 && std::any_of(current_words.begin(),current_words.end(),
                    [](auto word){return (word&0x7fffffffu)!=0;})) {
                // This deliberate zero-PCA control changes only the original
                // nonzero bank, retaining real geometry/owner lighting. The
                // two Triplicate trains have literal zero PCA at their authored
                // initial position, so they cannot provide this negative gate.
                const auto zero_mesh=zero_coefficients(mesh);auto zero_window=make_window(width,height);
                Renderer zero(zero_window.get(),zero_mesh,true);
                const auto black=capture(zero,selected,output/(name+"-zero-bank.ppm"));
                const auto response=difference(actual,black);
                require(response.changed>100,"source coefficient bank is not visibly consumed: "+name);
                std::cout<<name<<": zero-bank negative control changes "<<response.changed
                    <<" bytes, maximum "<<response.maximum<<'\n'<<std::flush;
            }
            if(step && players==1) {
                auto stale_window=make_window(width,height);Renderer stale(stale_window.get(),initial_mesh,true);
                const auto old=capture(stale,selected,output/(name+"-stale-bank.ppm"));
                const auto response=difference(actual,old);visible_bank_change|=response.changed>100;
                std::cout<<name<<": "<<changed<<" changed source coefficient words; stale-bank response "
                    <<response.changed<<" bytes, maximum "<<response.maximum<<"; reused/fresh maximum "
                    <<error.maximum<<'\n'<<std::flush;
            } else std::cout<<name<<": reused/fresh maximum "<<error.maximum<<'\n'<<std::flush;
            ++checked;
        }
    }
    if(path.stem()=="zanzibar")require(visible_bank_change,
        "changed source PRT coefficients had no measured GPU shading response");
    return checked;
}
unsigned verify_crane(SDL_Window *window,const std::filesystem::path &path,const std::filesystem::path &output) {
    const Cache cache(path);const auto content=cache.load_content(0,true);CollisionWorld collision(content.collision);
    std::vector<ScenarioObject> objects;
    for(const auto&object:content.objects)if(object.type==11 && object.definition==0xe93107bcu)objects.push_back(object);
    require(objects.size()==1 && objects[0].placement==24,"original crane placement identity changed");
    Mesh mesh;SourcePropScene scene(cache,objects,0,mesh,collision);
    require(scene.stats().render_models==1 && scene.stats().prt_materials==3,"empty crane source PRT fixture");
    std::vector<Vertex> world;std::vector<ObjectLightingBinding> bindings;scene.append(world,bindings);
    std::vector<Vertex> geometry;
    for(const auto&vertex:world){const auto&material=mesh.materials.at(vertex.material);
        if(material.environment_pass && material.environment_pass->forward_vertex==9)geometry.push_back(vertex);}
    require(!geometry.empty() && !bindings.empty(),"crane source section0 has no PRT geometry/owner");
    const auto words=coefficient_words(mesh);
    require(std::any_of(words.begin(),words.end(),[](auto w){return(w&0x7fffffffu)!=0;}),"crane source coefficient bank is empty");
    const auto camera=camera_for(cache,content,mesh,geometry,true);
    // Constructor/append retains source initial geometry. Calling update
    // would require the actual1D44F0 cached/physics sphere callback; that
    // independent gameplay service is intentionally outside this GPU bank test.
    SDL_SetWindowSize(window,640,480);SDL_PumpEvents();Renderer renderer(window,mesh,true);
    const std::array<std::array<int,2>,3> extents{{{640,480},{1280,720},{640,480}}};unsigned checked=0;
    for(unsigned step=0;step<extents.size();++step){const auto[width,height]=extents[step];
        SDL_SetWindowSize(window,width,height);SDL_PumpEvents();renderer.resized();renderer.update_material_constants(mesh.materials);
        for(unsigned players=1;players<=2;++players){auto views=views_for(camera,content.player.camera_field_of_view,players,width,height,geometry,bindings);
            const auto selected=std::span(views.data(),players);const auto name="zanzibar-crane-step"+std::to_string(step)+"-"+
                std::to_string(width)+"x"+std::to_string(height)+"-"+std::to_string(players)+"view";
            const auto actual=capture(renderer,selected,output/(name+"-reused.ppm"));auto fresh_window=make_window(width,height);
            Renderer fresh(fresh_window.get(),mesh,true);const auto expected=capture(fresh,selected,output/(name+"-fresh.ppm"));
            const auto error=difference(actual,expected);require(error.maximum<=1,"crane resident PRT bank changed on resize: "+name);
            require(actual.width==unsigned(width) && actual.height==unsigned(height),"stale crane resize extent");
            if(!step && players==1){const auto zero_mesh=zero_coefficients(mesh);auto zero_window=make_window(width,height);
                Renderer zero(zero_window.get(),zero_mesh,true);const auto dark=capture(zero,selected,output/(name+"-zero-bank.ppm"));
                const auto response=difference(actual,dark);require(response.changed>100,"crane source PRT bank is not visibly consumed");
                std::cout<<name<<": zero-bank changes "<<response.changed<<" bytes, maximum "<<response.maximum<<'\n'<<std::flush;}
            std::cout<<name<<": reused/fresh maximum "<<error.maximum<<'\n'<<std::flush;++checked;
        }
    }
    return checked;
}
}
int main(int argc,char **argv) {
    try {
        require(argc==4,"expected retail maps, map-pack maps and capture directory");
#if defined(__linux__)
        if(!std::getenv("DISPLAY") && !std::getenv("WAYLAND_DISPLAY"))return 77;
        setenv("VK_LAYER_VALIDATE_SYNC","1",1);
#endif
        require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
        const std::filesystem::path output=argv[3];std::filesystem::create_directories(output);
        unsigned checked=0;std::string validation;
        {
            ValidationLog log;auto window=make_window(640,480);
            checked+=verify(window.get(),std::filesystem::path(argv[1])/"zanzibar.map",output);
            checked+=verify(window.get(),std::filesystem::path(argv[2])/"triplicate.map",output);
            checked+=verify_crane(window.get(),std::filesystem::path(argv[1])/"zanzibar.map",output);
            window.reset();validation=log.output.str();
        }
        SDL_Quit();require(validation.find("Vulkan validation:")==std::string::npos,
            "Vulkan/synchronization validation reported:\n"+validation);
        std::cout<<checked<<" source machine tick/layout/resize cases match fresh renderer; coefficient updates visibly exercised\n";
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';SDL_Quit();return 1;}
}
