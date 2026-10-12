#include "halo2/renderer.hpp"
#include "halo2/model.hpp"
#include "halo2/math.hpp"
#include "halo2/transform.hpp"
#include "halo2/appearance.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_render_policy.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

namespace {
using namespace halo2;
void check(bool value,const std::string &message){if(!value)throw std::runtime_error(message);}
struct Frame {unsigned width{},height{};std::vector<unsigned char> pixels;};
Frame read(const std::filesystem::path &path) {
    std::ifstream f(path,std::ios::binary);std::string magic;unsigned maximum;Frame frame;
    f>>magic>>frame.width>>frame.height>>maximum;f.get();
    check(magic=="P6" && maximum==255,"invalid lighting capture");
    frame.pixels.resize(std::size_t(frame.width)*frame.height*3);
    f.read(reinterpret_cast<char*>(frame.pixels.data()),frame.pixels.size());
    check(bool(f),"truncated lighting capture");return frame;
}
void compare(const Frame &a,const Frame &b,unsigned slot) {
    check(a.width==b.width && a.height==b.height,"lighting capture extent changed");
    unsigned max_error=0;
    for(unsigned y=0;y<a.height;++y)for(unsigned x=slot*a.width/2;x<(slot+1)*a.width/2;++x)
        for(unsigned channel=0;channel<3;++channel) {
            const auto p=(std::size_t(y)*a.width+x)*3+channel;
            max_error=std::max(max_error,unsigned(std::abs(int(a.pixels[p])-int(b.pixels[p]))));
        }
    check(max_error<=1,"per-player object lighting leaked across viewport uniforms");
}
void prepare(const Cache &cache,RenderModel &model,const ObjectColors &colors) {
    for(auto &material:model.mesh.materials) {
        if(!material.lods.empty())
            check(prepare_dynamic_material(cache,material,{},true),"original dynamic program unavailable");
        apply_object_colors(material,colors);
    }
}
void bind_view_direction(Material &material,Vec3 forward) {
    const auto child=[&](std::shared_ptr<Material> &pass) {
        if(!pass)return;
        if(!pass.unique())pass=std::make_shared<Material>(*pass);
        bind_view_direction(*pass,forward);
    };
    child(material.environment_pass);
    for(auto &pass:material.world_overlays_before)child(pass);
    for(auto &pass:material.world_overlays_after)child(pass);
    for(auto &pass:material.world_bloom_passes)child(pass);
    if(!material.forward_pass)return;
    auto &pass=material.passes.at(material.forward_pass_index);
    std::vector<ShaderRemap> mappings;
    for(const auto &mapping:pass.pixel_externs)
        if((mapping.component&7)==4 && mapping.source==0)mappings.push_back(mapping);
    const std::array<ShaderVector,1> colors{{{forward.x,forward.y,forward.z,0}}};
    remap_pixel_externs(material.forward_program,{},colors,mappings,pass.fragments,material.shader_switches);
    // This expected mesh already has the source context bound on the CPU.
    // Prevent a second renderer remap from masking fragment-address errors.
    pass.pixel_externs.clear();
}
void transform(std::vector<Vertex> &vertices,unsigned material,const Transform &placement) {
    const auto rotate=[&](Vec3 v){return placement.basis[0]*v.x+placement.basis[1]*v.y+placement.basis[2]*v.z;};
    for(auto &v:vertices) {
        v.position=placement.position+rotate(v.position)*placement.scale;
        v.normal=rotate(v.normal);v.tangent=rotate(v.tangent);v.binormal=rotate(v.binormal);v.material+=material;
    }
}
}
int main(int argc,char **argv) {
    SDL_Window *window=nullptr;
    try {
        check(argc==3,"expected map directory and lighting capture directory");
        const Cache cache(std::filesystem::path(argv[1])/"lockout.map");
        const auto content=cache.load_content(0,true);
        const auto weapon=content.weapons.at(cache.slayer_starting_weapon(5));
        const auto colors=cache.load_object_colors(content.player.datum);
        auto gun=cache.load_model(weapon.first_person_model),hands=cache.load_model(content.first_person_hands);
        prepare(cache,gun,cache.load_object_colors(weapon.datum));prepare(cache,hands,colors);
        Mesh mesh;mesh.materials=gun.mesh.materials;
        const auto hands_base=unsigned(mesh.materials.size());
        mesh.materials.insert(mesh.materials.end(),hands.mesh.materials.begin(),hands.mesh.materials.end());
        const auto idle=cache.load_action(weapon.first_person_animation,"any","any","any","idle");
        auto pose=pose_first_person_models(gun,hands,idle,0,weapon.first_person_offset);
        Camera camera;camera.field_of_view=source_camera_angles(content.player.camera_field_of_view).projection_angle*57.295779513f;
        const auto placement=camera_transform(camera.position,{1,0,0},{0,0,1});
        transform(pose.weapon,0,placement);transform(pose.arms,hands_base,placement);
        std::vector<Vertex> foreground=std::move(pose.weapon);
        foreground.insert(foreground.end(),pose.arms.begin(),pose.arms.end());
        const auto definition=cache.load_object_lighting_definition();
        const std::array lighting{
            object_lighting(definition,{{.2f,.3f,.7f},0xff8080,0xc04020},0),
            object_lighting(definition,{{-.7f,.2f,.3f},0x204080,0x2080c0},0)};
        std::array<ObjectLightingBinding,2> bindings;
        for(unsigned i=0;i<2;++i)bindings[i]={0,unsigned(mesh.materials.size()),lighting[i]};
        std::array<Mesh,2> expected_meshes{mesh,mesh};
        for(unsigned i=0;i<2;++i)for(auto &material:expected_meshes[i].materials) {
            apply_object_lighting(material,lighting[i]);
            bind_view_direction(material,direction(camera.yaw,camera.pitch));
        }
        check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
        window=SDL_CreateWindow("Original object-light uniform isolation",0,0,1280,480,
            SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN);
        check(window,SDL_GetError());
        const std::filesystem::path output=argv[2];std::filesystem::create_directories(output);
        {
            Renderer renderer(window,mesh,true);
            for(const auto size:std::array<std::array<int,2>,3>{{{1280,480},{1920,720},{1280,480}}}) {
                SDL_SetWindowSize(window,size[0],size[1]);SDL_PumpEvents();renderer.resized();
                std::array<ModelView,2> views;
                for(unsigned i=0;i<2;++i) {
                    auto &view=views[i];view.camera=camera;view.left=float(i*320);view.width=320;view.height=480;
                    view.full_surface=true;view.foreground=foreground;view.object_lighting=std::span(&bindings[i],1);
                }
                const auto stem=std::to_string(size[0])+"x"+std::to_string(size[1]);
                const auto capture=[&](std::span<const ModelView> selected,const std::string &suffix) {
                    const auto path=output/(stem+suffix+".ppm");
                    for(unsigned attempt=0;attempt<5;++attempt)
                        if(renderer.draw(camera,path,{},false,{},{},{},selected))return read(path);
                    throw std::runtime_error("lighting capture did not render");
                };
                const auto left=capture(std::span(&views[0],1),"-owner-a");
                const auto right=capture(std::span(&views[1],1),"-owner-b");
                const auto together=capture(views,"-two-owners");
                compare(left,together,0);compare(right,together,1);
                for(unsigned owner=0;owner<2;++owner) {
                    std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> expected_window(
                        SDL_CreateWindow("Source-bound shader context",0,0,size[0],size[1],
                            SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN),&SDL_DestroyWindow);
                    check(bool(expected_window),SDL_GetError());
                    Renderer expected_renderer(expected_window.get(),expected_meshes[owner],true);
                    auto expected_view=views[owner];expected_view.object_lighting={};
                    const auto path=output/(stem+"-source-bound-"+std::to_string(owner)+".ppm");
                    bool rendered=false;
                    for(unsigned attempt=0;attempt<5 && !rendered;++attempt)
                        rendered=expected_renderer.draw(camera,path,{},false,{},{},{},std::span(&expected_view,1));
                    check(rendered,"source-bound lighting capture did not render");
                    compare(read(path),together,owner);
                }
                unsigned different=0;
                for(unsigned y=0;y<together.height;++y)for(unsigned x=0;x<together.width/2;++x)
                    for(unsigned c=0;c<3;++c) {
                        const auto a=(std::size_t(y)*together.width+x)*3+c,b=a+together.width/2*3;
                        different+=std::abs(int(together.pixels[a])-int(together.pixels[b]))>2;
                    }
                check(different>100,"original lighting states produced no visible material response");
                // 44370 suppresses authored layer8 under the multi-player
                // bank. Compare its actual GPU response against removing only
                // that source pass from an independently prepared mesh.
                for(auto &view:views)view.source_render_policy=source_geometry_lod_policy(1);
                const auto enabled=capture(views,"-source-layers-enabled");
                for(unsigned owner=0;owner<2;++owner) {
                    views[owner].source_render_policy=source_geometry_lod_policy(2);
                    const auto disabled=capture(views,"-layer8-disabled-"+std::to_string(owner));
                    compare(enabled,disabled,1-owner);
                    unsigned changed=0;
                    for(unsigned y=0;y<enabled.height;++y)
                        for(unsigned x=owner*enabled.width/2;x<(owner+1)*enabled.width/2;++x)
                            for(unsigned c=0;c<3;++c) {
                                const auto at=(std::size_t(y)*enabled.width+x)*3+c;
                                changed+=std::abs(int(enabled.pixels[at])-int(disabled.pixels[at]))>2;
                            }
                    check(changed>10,"disabled original reflection pass had no measured GPU response");
                    auto pruned=expected_meshes[owner];unsigned removed=0;
                    for(auto &material:pruned.materials) {
                        const auto before=material.world_overlays_after.size();
                        std::erase_if(material.world_overlays_after,[](const auto &pass) {
                            return source_material_layer(*pass)==8;
                        });
                        removed+=before-material.world_overlays_after.size();
                    }
                    check(removed>0,"actual Chief layer8 reflection pass missing");
                    std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> expected_window(
                        SDL_CreateWindow("Original layer8 omitted",0,0,size[0],size[1],
                            SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN),&SDL_DestroyWindow);
                    check(bool(expected_window),SDL_GetError());
                    Renderer expected_renderer(expected_window.get(),pruned,true);
                    auto expected_view=views[owner];expected_view.object_lighting={};expected_view.source_render_policy.reset();
                    const auto path=output/(stem+"-layer8-pruned-"+std::to_string(owner)+".ppm");
                    bool rendered=false;
                    for(unsigned attempt=0;attempt<5 && !rendered;++attempt)
                        rendered=expected_renderer.draw(camera,path,{},false,{},{},{},std::span(&expected_view,1));
                    check(rendered,"source-pruned layer8 capture did not render");
                    compare(read(path),disabled,owner);
                    views[owner].source_render_policy=source_geometry_lod_policy(1);
                    std::cout<<stem<<": owner "<<owner<<" source layer8 removed, "<<changed<<" changed components\n";
                }
                std::cout<<stem<<": independent original player lighting retained after resize\n";
            }
        }
        SDL_DestroyWindow(window);SDL_Quit();return 0;
    } catch(const std::exception &e) {
        if(window)SDL_DestroyWindow(window);SDL_Quit();std::cerr<<e.what()<<'\n';return 1;
    }
}
