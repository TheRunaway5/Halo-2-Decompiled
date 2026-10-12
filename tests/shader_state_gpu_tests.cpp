#define SDL_MAIN_HANDLED
#include "halo2/renderer.hpp"
#include "halo2/world_material.hpp"
#include "halo2/source_shader_state.hpp"
#include "halo2/source_shader_texture.hpp"
#include "halo2/model.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct Frame {unsigned width{},height{};std::vector<unsigned char> pixels;};
Frame read(const std::filesystem::path &path) {
    std::ifstream file(path,std::ios::binary);Frame result;std::string magic;unsigned maximum;
    file>>magic>>result.width>>result.height>>maximum;file.get();
    check(magic=="P6" && maximum==255,"Invalid state-color capture");
    result.pixels.resize(std::size_t(result.width)*result.height*3);
    file.read(reinterpret_cast<char*>(result.pixels.data()),result.pixels.size());
    check(bool(file),"Truncated state-color capture");return result;
}
Frame capture(Renderer &renderer,const Camera &camera,std::span<const ModelView> views,const std::filesystem::path &path) {
    for(unsigned n=0;n<5;++n)if(renderer.draw(camera,path,{},false,{},{},{},views))return read(path);
    throw FormatError("State-color capture did not render");
}
void compare(const Frame &actual,const Frame &source,const std::array<std::uint32_t,2> &colors) {
    check(actual.width==source.width && actual.height==source.height,"State-color capture dimensions changed");
    constexpr unsigned background[]{32,64,96},shift[]{16,8,0};
    unsigned tested=0;
    for(unsigned y=0;y<actual.height;++y)for(unsigned x=0;x<actual.width;++x) {
        const auto at=(std::size_t(y)*actual.width+x)*3;
        // The unblended reference identifies actual shaded surface pixels;
        // the background and discarded fragments are outside this comparison.
        if(std::max({source.pixels[at],source.pixels[at+1],source.pixels[at+2]})<8)continue;
        const auto color=colors[x/(actual.width/2)];
        for(unsigned channel=0;channel<3;++channel) {
            const auto constant=(color>>shift[channel])&255u;
            const auto expected=std::min(255.0,double(source.pixels[at+channel])+
                double(background[channel])*(1-double(constant)/255));
            if(std::abs(double(actual.pixels[at+channel])-expected)>2.1)
                throw FormatError("Dynamic source state75 blend differs at "+std::to_string(x)+","+
                    std::to_string(y)+" channel "+std::to_string(channel));
            ++tested;
        }
    }
    check(tested>10000,"Dynamic blend comparison had insufficient shaded coverage");
}
}
int main(int argc,char **argv)try {
    check(argc==3,"Supply disc maps and capture directory");if(!std::getenv("DISPLAY"))return 77;
    Cache cache(std::filesystem::path(argv[1])/"lockout.map");const auto content=cache.load_content(0,true);
    const auto weapon=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto &tag){
        return tag.group=="weap" && tag.name=="objects\\weapons\\rifle\\covenant_carbine\\covenant_carbine";
    });check(weapon!=cache.tags().end(),"Original display fixture weapon unavailable");
    const auto gun=cache.load_model(content.weapons.at(weapon->datum).first_person_model);
    const auto display=std::find_if(gun.mesh.materials.begin(),gun.mesh.materials.end(),[](const auto &m){
        return m.name=="objects\\weapons\\rifle\\covenant_carbine\\shaders\\carbine_display";
    });check(display!=gun.mesh.materials.end(),"Original display fixture material unavailable");
    auto material=*display;check(prepare_dynamic_material(cache,material,{},true,0.f),"Original display program unresolved");
    animate_forward_material(material,0,[](auto){return 1.f;});
    // Keep the actual display program/textures fixed. Only the render-state
    // rows come from the actual global camouflage implementation; asymmetric
    // RGB controls and quads are declared isolation inputs, not a camo draw.
    const auto global=cache.load_material(source_camouflage_shader(cache));
    const auto states=std::find_if(global.passes.begin(),global.passes.end(),[](const auto &p){return !p.state_externs.empty();});
    check(states!=global.passes.end(),"Authored camouflage state records unavailable");
    Mesh mesh;mesh.materials={material,material};Camera camera;
    std::array<std::vector<Vertex>,2> vertices;
    for(unsigned owner=0;owner<2;++owner)for(const auto uv:std::array{Vec2{0,0},Vec2{1,0},Vec2{1,1},Vec2{0,0},Vec2{1,1},Vec2{0,1}}) {
        Vertex v;v.position={2,(uv.x*2-1)*.5f,(1-uv.y*2)*.5f};v.uv=uv;
        v.color={1,1,1};v.normal={-1,0,0};v.tangent={0,1,0};v.binormal={0,0,-1};v.material=owner;
        vertices[owner].push_back(v);
    }
    SDL_SetMainReady();check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> window(SDL_CreateWindow("Source shader state colors",0,0,1280,480,
        SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN),&SDL_DestroyWindow);check(bool(window),SDL_GetError());
    const std::filesystem::path output=argv[2];std::filesystem::create_directories(output);
    for(const auto size:std::array<std::array<int,2>,2>{{{1280,480},{1920,720}}}) {
        SDL_SetWindowSize(window.get(),size[0],size[1]);SDL_PumpEvents();
        std::array<ModelView,2> views;
        for(unsigned n=0;n<2;++n){views[n].camera=camera;views[n].left=float(n*320);views[n].width=320;
            views[n].height=480;views[n].full_surface=true;views[n].foreground=vertices[n];}
        Frame reference;
        for(auto &m:mesh.materials)m.forward_blend=0;
        {Renderer renderer(window.get(),mesh,true);renderer.scene_clear_color({});
            reference=capture(renderer,camera,views,output/(std::to_string(size[0])+"-source.ppm"));}
        for(auto &m:mesh.materials)m.forward_blend=11;
        Renderer renderer(window.get(),mesh,true);renderer.scene_clear_color({32.f/255,64.f/255,96.f/255});
        const ShaderVector controls[]{{0,.25f,1,0},{1,.75f,0,0}};
        Frame first;
        unsigned sequence=0;
        for(const auto owners:std::array<std::array<unsigned,2>,5>{{{0,1},{0,0},{1,1},{1,0},{0,1}}}) {
            std::array<std::uint32_t,2> words;
            for(unsigned n=0;n<2;++n) {
                std::array<std::uint32_t,256> bank{};bank[75]=0xffaabbcc;
                std::array<ShaderVector,41> colors{};colors[40]=controls[owners[n]];
                const auto writes=remap_source_shader_states(bank,{},colors,states->state_externs,0x027f);
                check(writes.size()==1 && writes[0].index==75,"Authored state range changed");
                words[n]=mesh.materials[n].forward_blend_color=writes[0].value;
            }
            renderer.update_material_constants(mesh.materials);
            const auto frame=capture(renderer,camera,views,output/(std::to_string(size[0])+"-state-"+std::to_string(sequence)+".ppm"));
            compare(frame,reference,words);
            if(!sequence)first=frame;
            if(sequence==4)check(frame.pixels==first.pixels,"Dynamic blend state retained a previous owner/frame");
            ++sequence;
        }
        std::cout<<size[0]<<'x'<<size[1]<<": five ordered dynamic RGB states matched independent unblended reference\n";
    }
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
