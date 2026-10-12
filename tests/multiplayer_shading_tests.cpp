#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/math.hpp"
#include "halo2/model.hpp"
#include "halo2/sky.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_machine_scene.hpp"
#include "halo2/collision.hpp"
#include "halo2/renderer.hpp"
#include "halo2/vertex_program.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>

namespace {
using namespace halo2;
void require(bool valid,const std::string &message) {
    if(!valid)throw std::runtime_error(message);
}
struct Image {
    unsigned width{},height{};
    std::vector<unsigned char> pixels;
};
Image read(const std::filesystem::path &path) {
    std::ifstream in(path,std::ios::binary);
    std::string magic;
    unsigned maximum{};
    Image result;
    in>>magic>>result.width>>result.height>>maximum;
    in.get();
    require(magic=="P6" && maximum==255,"invalid GPU capture "+path.string());
    result.pixels.resize(std::size_t(result.width)*result.height*3);
    in.read(reinterpret_cast<char*>(result.pixels.data()),static_cast<std::streamsize>(result.pixels.size()));
    require(bool(in),"truncated GPU capture "+path.string());
    return result;
}
Image crop(const Image& image,unsigned left,unsigned top,unsigned width=640,unsigned height=480) {
    require(left+width<=image.width && top+height<=image.height,"baseline viewport crop outside image");
    Image result{width,height,std::vector<unsigned char>(std::size_t(width)*height*3)};
    for(unsigned y=0;y<height;++y)
        std::copy_n(image.pixels.begin()+(std::size_t(y+top)*image.width+left)*3,width*3,
                    result.pixels.begin()+std::size_t(y)*width*3);
    return result;
}
std::size_t compare(const Image &reference,const Image &image,unsigned left,unsigned top,
                    const std::string &context,unsigned rounding=0,bool projected_boundary=false,
                    bool quantify_only=false) {
    require(left+reference.width<=image.width && top+reference.height<=image.height,
            "matching viewport crop outside image: "+context);
    std::size_t changed=0;
    unsigned maximum=0;
    std::size_t edge_changed=0;
    unsigned edge_maximum=0;
    for(unsigned y=0;y<reference.height;++y)
        for(unsigned x=0;x<reference.width;++x)
            for(unsigned c=0;c<3;++c) {
                const auto a=reference.pixels[(std::size_t(y)*reference.width+x)*3+c];
                const auto b=image.pixels[(std::size_t(y+top)*image.width+x+left)*3+c];
                // The original projected extern 3 binds the complete target,
                // while c8/c9 retain +0.53125 and oPos uses +0.5. NV2A's
                // 1/16-pixel position truncation adds a subpixel residual.
                // At a subview's last row/column this can sample the adjacent
                // view; full-target clamping differs there by design. Preserve
                // and report those source-defined samples independently.
                if(projected_boundary && (x+1==reference.width || y+1==reference.height)) {
                    if(a!=b)++edge_changed;
                    edge_maximum=std::max(edge_maximum,unsigned(std::abs(int(a)-int(b))));
                    continue;
                }
                if(a!=b)++changed;
                maximum=std::max(maximum,unsigned(std::abs(int(a)-int(b))));
            }
    if(!quantify_only)require(maximum<=rounding,context+": "+std::to_string(changed)+" channel samples differ, maximum "+std::to_string(maximum));
    if(changed)std::cout<<context<<": "<<changed<<" channel samples differ by at most "<<maximum<<"/255 after atlas resize\n";
    if(projected_boundary)std::cout<<context<<": source projected boundary "<<edge_changed<<" changed channel samples, maximum "<<edge_maximum<<"/255\n";
    return projected_boundary ? std::size_t(reference.width-1)*(reference.height-1)*3 :
        std::size_t(reference.width)*reference.height*3;
}
std::vector<std::filesystem::path> fixtures(const std::filesystem::path &maps) {
    if(maps.extension()==".map")return {maps};
    constexpr const char* original[]{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar"};
    std::vector<std::filesystem::path> result;
    for(const auto name:original)result.push_back(maps/(std::string(name)+".map"));
    const auto pack=maps.parent_path().parent_path()/"map-pack/maps";
    if(std::filesystem::exists(pack)) {
        constexpr const char* extra[]{"backwash","containment","deltatap","dune","elongation","gemini","triplicate","turf","warlock"};
        for(const auto name:extra)result.push_back(pack/(std::string(name)+".map"));
    }
    return result;
}
void verify_projected_boundary_source(const Mesh& mesh) {
    for(const auto& m:mesh.materials) {
        const auto* pass=m.accumulation_texture ? &m :
            m.environment_pass && m.environment_pass->accumulation_texture ? m.environment_pass.get() : nullptr;
        if(!pass || pass->vertex_program.empty())continue;
        VertexProgramState state;
        state.attributes=pass->program_constants[0];
        for(unsigned i=0;i<16;++i)state.constants[18+i]=pass->program_constants[1][i];
        for(unsigned i=0;i<4;++i)state.constants[i][i]=1;
        state.constants[6]={0,0,-1,2};state.constants[7]={0,0,1,767.8125};
        state.constants[8]={320,0,0,320.53125};state.constants[9]={0,-240,0,240.53125};
        state.constants[10]={320,-240,16777215,0};state.constants[11]={320.5,240.5,0,0};
        state.constants[172]={1,1,0,0};
        state.attributes[0]={0,0,.5,1};state.attributes[1]={.1,.2,0,1};
        state.attributes[2]={0,0,1,1};state.attributes[3]={1,0,0,1};state.attributes[4]={0,1,0,1};
        state.attributes[5]={0,0,1,1};state.attributes[6]={.1,.2,0,1};
        execute_vertex_program(decode_vertex_program(pass->vertex_program),state);
        const auto& position=state.outputs[0];const auto& texture=state.outputs[9];
        require(position[0]==320.5f && position[1]==240.5f &&
                texture[0]/texture[3]-position[0]==.03125f &&
                texture[1]/texture[3]-position[1]==.03125f,
                "authored vertex program changed the projected 1/32-pixel sampling bias");
        return;
    }
}
std::size_t verify(SDL_Window *window,const std::filesystem::path &map,
                   const std::filesystem::path &directory,bool validation) {
    const Cache cache(map);
    auto mesh=cache.load_bsp(0);
    const auto atmosphere=cache.load_bsp_atmosphere(0);
    const auto content=cache.load_content(0,true);
    const auto lighting=cache.load_bsp_lighting(0);
    std::size_t scenery_count=0;
    for(const auto& object:content.objects) {
        if(object.type!=6 || object.definition==0xffffffffu ||
            (object.origin_bsp>=0 && object.origin_bsp!=0))continue;
        auto model=cache.load_model(object.definition,true,object.variant);
        if(!cache.apply_scenery_lighting(model,lighting,object))continue;
        const auto material=std::uint32_t(mesh.materials.size());
        mesh.materials.insert(mesh.materials.end(),model.mesh.materials.begin(),model.mesh.materials.end());
        const auto basis=authored_basis(object.rotation);
        for(auto vertex:pose_model(model)) {
            vertex.position=object.position+basis.rotate(vertex.position)*object.scale;
            vertex.normal=basis.rotate(vertex.normal);vertex.tangent=basis.rotate(vertex.tangent);
            vertex.binormal=basis.rotate(vertex.binormal);vertex.material+=material;
            mesh.vertices.push_back(vertex);
        }
        ++scenery_count;
    }
    require(!cache.spawns().empty(),"retail multiplayer spawn missing: "+cache.name());
    Camera camera;
    camera.position=cache.spawns().front().position+Vec3{0,0,content.player.camera_height};
    camera.yaw=cache.spawns().front().facing;
    camera.field_of_view=source_camera_angles(content.player.camera_field_of_view).projection_angle*57.295779513f;
    const CollisionWorld collision(content.collision);
    SourceMachineScene machines(cache,content.objects,0,mesh,collision);
    machines.update(0,0,camera.position,source_camera_angles(content.player.camera_field_of_view).lighting_distance_scale,mesh);
    std::vector<ObjectLightingBinding> machine_lighting;
    machines.append(mesh.vertices,machine_lighting);
    for(const auto &binding:machine_lighting)
        for(unsigned i=0;i<binding.material_count;++i)
            apply_object_lighting(mesh.materials.at(binding.first_material+i),binding.state);
    prepare_bsp_planar_fog(cache,mesh,atmosphere);
    verify_projected_boundary_source(mesh);
    // Keep every opaque authored world category, including bitmap-lighted,
    // RGB bucket-lighted, authored static scenery and projected environment
    // passes. Transparent ordering, water and HUD have separate regressions.
    std::erase_if(mesh.vertices,[&](const auto &v) {
        const auto &m=mesh.materials.at(v.material);
        return !(m.static_lightmap_pass || m.world_pass) || m.forward_blend ||
               m.water_pass || m.water_definition>=0;
    });
    require(!mesh.vertices.empty(),"retail opaque lighting fixture missing: "+cache.name());
    std::vector<Vertex> background;
    const auto sky_index=atmosphere.sample(camera.position,direction(camera.yaw,camera.pitch)).visible_sky;
    if(sky_index>=0)if(const auto sky=cache.load_sky(std::size_t(sky_index))) {
        const auto model=cache.load_model(sky->model);
        const auto base=unsigned(mesh.materials.size());
        mesh.materials.insert(mesh.materials.end(),model.mesh.materials.begin(),model.mesh.materials.end());
        background=pose_sky(model,*sky,camera.position);
        for(auto& vertex:background)vertex.material+=base;
    }
    SDL_SetWindowSize(window,640,480);
    SDL_PumpEvents();
    Renderer renderer(window,mesh,validation);
    std::cout<<cache.name()<<": native GPU "<<renderer.device_name()<<'\n';
    renderer.scene_clear_color({0,0,0});
    const auto capture=[&](const std::string &suffix,std::span<const ModelView> views) {
        const auto path=directory/(map.stem().string()+"-"+suffix+".ppm");
        bool drawn=false;
        for(unsigned retry=0;retry<4 && !drawn;++retry)
            drawn=renderer.draw(camera,path,{},false,{},{},{},views,
                views.empty()?std::span<const Vertex>(background):std::span<const Vertex>{});
        require(drawn,"world GPU draw failed: "+path.string());
        return read(path);
    };
    const auto direct=capture("direct",{});
    require(direct.width==640 && direct.height==480,"unexpected framebuffer size");
    require(std::count_if(direct.pixels.begin(),direct.pixels.end(),[](auto v){return v!=0;})>300,
            "authored camera did not exercise visible surfaces: "+cache.name());
    ModelView single;
    single.camera=camera;single.width=640;single.height=480;
    single.full_surface=true;single.vertices=mesh.vertices;
    single.background=background;
    const auto reference=capture("one-view",{&single,1});
    std::size_t samples=compare(direct,reference,0,0,cache.name()+" direct vs ModelView");
    for(const unsigned count:{2u,4u}) {
        const unsigned columns=2,rows=count/2;
        SDL_SetWindowSize(window,640*columns,480*rows);
        SDL_PumpEvents();renderer.resized();
        std::array<ModelView,4> views;
        for(unsigned i=0;i<count;++i) {
            auto &view=views[i];
            view.camera=camera;view.full_surface=true;view.vertices=mesh.vertices;view.background=background;
            // ModelView coordinates scale from the full 640x480 surface.
            // Every crop remains exactly 640x480 physical pixels, so each
            // shader's local derivatives and projected coordinates must agree.
            view.left=float(i%columns)*640.f/columns;
            view.top=float(i/columns)*480.f/rows;
            view.width=640.f/columns;view.height=480.f/rows;
        }
        std::array<Image,4> baselines;
        for(unsigned i=0;i<count;++i) {
            const auto baseline=capture(std::to_string(count)+"-single-slot-"+std::to_string(i),{&views[i],1});
            baselines[i]=crop(baseline,(i%columns)*640,(i/columns)*480);
            if(i==0)samples+=compare(reference,baselines[i],0,0,cache.name()+" origin0 atlas resize "+std::to_string(count),1,true);
        }
        const auto image=capture(std::to_string(count)+"-views",{views.data(),count});
        require(image.width==640*columns && image.height==480*rows,"split framebuffer size drifted");
        try {
            for(unsigned i=0;i<count;++i)
                samples+=compare(baselines[i],image,(i%columns)*640,(i/columns)*480,
                                 cache.name()+" "+std::to_string(count)+" views, crop "+std::to_string(i),1,true);
        } catch(...) {
            // Preserve the first failure and a following frame. This exposes
            // whether recreating the dynamic vertex buffer changes one frame
            // or leaves a persistent native binding failure.
            const auto repeated=capture(std::to_string(count)+"-views-repeat",{views.data(),count});
            for(unsigned i=0;i<count;++i)
                compare(baselines[i],repeated,(i%columns)*640,(i/columns)*480,
                        cache.name()+" repeated "+std::to_string(count)+" views, crop "+std::to_string(i),1,true,true);
            const auto after=capture(std::to_string(count)+"-single-after-failure",{views.data(),1});
            compare(baselines[0],after,0,0,cache.name()+" single after failure",1,true,true);
            auto isolated=views;
            for(unsigned i=1;i<count;++i)isolated[i].width=0;
            const auto draw_one=capture(std::to_string(count)+"-geometry-all-draw-one",{isolated.data(),count});
            compare(baselines[0],draw_one,0,0,cache.name()+" full upload, one draw",1,true,true);
            for(unsigned i=1;i<count;++i) {isolated[i].vertices={};isolated[i].background={};}
            const auto upload_one=capture(std::to_string(count)+"-banks-all-geometry-one",{isolated.data(),count});
            compare(baselines[0],upload_one,0,0,cache.name()+" full banks, one upload",1,true,true);
            throw;
        }
    }
    // Repeat the matching-view comparison with the authored per-camera BSP
    // fog state and containing-geometry layer 6. This exercises viewport-local
    // depth, fog gradients and original planar palette admission independently
    // of the no-atmosphere direct/ModelView material-binding check above.
    renderer.scene_bsp_atmosphere(atmosphere);
    SDL_SetWindowSize(window,640,480);SDL_PumpEvents();renderer.resized();
    const auto forward=Vec3{std::cos(camera.yaw),std::sin(camera.yaw),0};
    single.atmosphere=atmosphere.sample(camera.position,forward);
    single.atmosphere_source=&atmosphere;
    const auto initial_postprocess=renderer.bsp_postprocess_state();
    const auto fog_reference=capture("fog-one-view",{&single,1});
    for(const unsigned count:{2u,4u}) {
        const unsigned columns=2,rows=count/2;
        SDL_SetWindowSize(window,640*columns,480*rows);SDL_PumpEvents();renderer.resized();
        std::array<ModelView,4> views;
        for(unsigned i=0;i<count;++i) {
            views[i]=single;
            views[i].left=float(i%columns)*640.f/columns;
            views[i].top=float(i/columns)*480.f/rows;
            views[i].width=640.f/columns;views[i].height=480.f/rows;
        }
        std::array<Image,4> baselines;
        for(unsigned i=0;i<count;++i) {
            if(single.atmosphere->patchy) {
                // Patchy noise consumes one shared RNG in viewport order.
                // Give the isolated geometry baseline the same admissions
                // and restore the same captured source state; empty other
                // meshes still consume their views. Resizing itself preserves
                // source motion and RNG and must not reset either.
                auto isolated=views;
                for(unsigned j=0;j<count;++j)if(j!=i)isolated[j].vertices={};
                require(bool(initial_postprocess),"patchy fixture has no source postprocess state");
                renderer.restore_bsp_postprocess_state(*initial_postprocess);
                baselines[i]=crop(capture("fog-"+std::to_string(count)+"-isolated-slot-"+std::to_string(i),
                                         {isolated.data(),count}),(i%columns)*640,(i/columns)*480);
            } else {
                baselines[i]=crop(capture("fog-"+std::to_string(count)+"-single-slot-"+std::to_string(i),
                                         {&views[i],1}),(i%columns)*640,(i/columns)*480);
            }
            // Fog/bloom includes several original RGBA8 intermediates. A
            // different full-target extent changes projected sampling roundoff
            // before those passes. Quantify that additional cross-target
            // comparison; the same-origin/target checks below are enforced.
            if(i==0)compare(fog_reference,baselines[i],0,0,cache.name()+" fog origin0 atlas resize "+std::to_string(count),1,true,true);
        }
        if(single.atmosphere->patchy)renderer.restore_bsp_postprocess_state(*initial_postprocess);
        const auto image=capture("fog-"+std::to_string(count)+"-views",{views.data(),count});
        require(image.width==640*columns && image.height==480*rows,"fog split framebuffer size drifted");
        for(unsigned i=0;i<count;++i)
            samples+=compare(baselines[i],image,(i%columns)*640,(i/columns)*480,
                             cache.name()+" fog "+std::to_string(count)+" views, crop "+std::to_string(i),1,true);
    }
    std::cout<<cache.name()<<": direct/1 exact; 2/4 matching interiors within one channel LSB; "<<scenery_count
             <<" authored static scenery objects; "<<machines.stats().render_models<<" source machine models; "
             <<background.size()/3<<" visible sky triangles ("<<samples<<" channel samples)\n";
    return samples;
}
}
int main(int argc,char **argv) {
    try {
        require(argc==3 || argc==4,"maps and capture directory required; optional validation argument");
        const auto maps=fixtures(argv[1]);
        const auto directory=std::filesystem::path(argv[2]);
        std::filesystem::create_directories(directory);
        if(SDL_Init(SDL_INIT_VIDEO)!=0)throw std::runtime_error(SDL_GetError());
        auto *window=SDL_CreateWindow("Multiplayer shading regression",0,0,640,480,
                                     SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
        require(window,SDL_GetError());
        std::size_t samples=0;
        for(const auto &map:maps)samples+=verify(window,map,directory,argc==4);
        SDL_DestroyWindow(window);SDL_Quit();
        std::cout<<samples<<" channel samples preserve authored lighting across "<<maps.size()
                 <<" multiplayer maps and 1/2/4 views\n";
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';SDL_Quit();return 1;}
}
