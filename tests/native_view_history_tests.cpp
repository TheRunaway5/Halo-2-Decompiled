#include "halo2/renderer.hpp"
#include "halo2/reference_player.hpp"
#include "halo2/model.hpp"
#include "halo2/scope_copy.hpp"
#include <SDL_vulkan.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const std::string &message) { if(!value)throw FormatError(message); }
struct Capture {
    unsigned width{},height{};
    std::vector<unsigned char> rgb,stencil;
    std::vector<std::uint32_t> depth;
};
std::vector<unsigned char> bytes(const std::filesystem::path &path,std::size_t size) {
    std::ifstream input(path,std::ios::binary|std::ios::ate);
    require(bool(input) && input.tellg()==std::streamoff(size),"unexpected history diagnostic size: "+path.string());
    input.seekg(0);std::vector<unsigned char> result(size);
    input.read(reinterpret_cast<char *>(result.data()),std::streamsize(size));
    require(bool(input),"truncated history diagnostic: "+path.string());return result;
}
Capture read(const std::filesystem::path &path) {
    std::ifstream input(path,std::ios::binary);Capture result;std::string magic;unsigned maximum{};
    input>>magic>>result.width>>result.height>>maximum;input.get();
    require(magic=="P6" && maximum==255,"invalid history RGB capture");
    const auto count=std::size_t(result.width)*result.height;result.rgb.resize(count*3);
    input.read(reinterpret_cast<char *>(result.rgb.data()),std::streamsize(result.rgb.size()));
    require(bool(input),"truncated history RGB capture");
    const auto raw=bytes(path.string()+".depth-d24-low24.bin",count*4);
    result.depth.resize(count);
    for(std::size_t i=0;i<count;++i)
        result.depth[i]=std::uint32_t(raw[i*4])|(std::uint32_t(raw[i*4+1])<<8)|(std::uint32_t(raw[i*4+2])<<16);
    // The upper byte of a Vulkan depth-aspect DWORD is padding, not stencil.
    result.stencil=bytes(path.string()+".stencil-u8.bin",count);return result;
}
using Window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>;
Window window(unsigned width,unsigned height) {
    Window result(SDL_CreateWindow("Native view history diagnostic",0,0,int(width),int(height),
        SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN),SDL_DestroyWindow);
    require(bool(result),SDL_GetError());return result;
}
Capture capture(Renderer &renderer,std::span<const ModelView> views,const std::filesystem::path &path) {
    for(unsigned retry=0;retry<5;++retry)
        if(renderer.draw(views.front().camera,path,{},false,{},{},{},views))return read(path);
    throw FormatError("native history frame did not submit");
}
std::vector<Vertex> quad(unsigned material,float x,float half_width,float half_height) {
    std::vector<Vertex> result;
    for(const auto uv:std::array{Vec2{0,0},Vec2{1,0},Vec2{1,1},Vec2{0,0},Vec2{1,1},Vec2{0,1}}) {
        Vertex v;v.position={x,(1-2*uv.x)*half_width,(1-2*uv.y)*half_height};
        v.normal={-1,0,0};v.tangent={0,-1,0};v.binormal={0,0,-1};
        v.color={1,1,1};v.uv=uv;v.material=material;result.push_back(v);
    }
    return result;
}
Texture solid(std::array<unsigned char,4> color) {
    Texture result;result.width=result.height=1;result.format=11;
    for(const auto value:color)result.pixels.push_back(std::byte(value));return result;
}
Material diagnostic(Material material,std::array<unsigned char,4> color,bool environment) {
    require(material.forward_pass && !material.vertex_program.empty(),"authored history VP is unavailable");
    const auto child=material.environment_pass;
    material.environment_pass.reset();material.world_overlays_before.clear();material.world_overlays_after.clear();
    material.world_bloom_passes.clear();material.world_planar_passes={};material.planar_fog_pass.reset();
    material.world_bloom_pass=false;material.water_pass=false;material.water_definition=-1;
    material.water_reflection_texture=material.water_refraction_texture=material.scope_texture=false;
    material.static_lightmap_pass=false;material.baked_lightmap_texture=false;
    material.forward_program=scope_copy_frame({}).program;
    // scope_copy_frame copies RGB only and leaves final alpha zero. The
    // authored sky pipeline uses SRCALPHA, so this controlled opaque copy
    // explicitly selects T0.a for final G (operand0x18). Keep its real blend
    // state; an invisible zero-alpha quad cannot test sky occlusion.
    material.forward_program.words[9]=0x1800;
    material.diffuse=material.detail=material.bump=material.auxiliary=solid(color);
    material.accumulation_texture=false;
    for(auto &frames:material.animated_textures)frames.clear();
    if(environment) {
        require(bool(child),"authored history accumulation child is unavailable");
        material.environment_pass=std::make_shared<Material>(diagnostic(*child,color,false));
        // Retain the original VP's projected extern3 coordinates and consume
        // the actual accumulation target. Only the diagnostic copy PS and
        // solid base texture replace appearance; no authored parity is claimed.
        material.environment_pass->accumulation_texture=true;
        material.environment_pass->forward_raster.depth_enable=1;
        material.environment_pass->forward_raster.depth_write=0;
        material.environment_pass->forward_raster.depth_function=0x202;
        material.environment_pass->forward_raster.cull=0;
        material.environment_pass->forward_blend=0;
    }
    return material;
}
struct Rectangle {unsigned left{},top{},right{},bottom{};};
Rectangle physical(const ModelView &view,unsigned width,unsigned height) {
    const auto v=full_surface_viewport({view.left,view.top,view.width,view.height},width,height);
    return {unsigned(v.left),unsigned(v.top),unsigned(v.left+v.width),unsigned(v.top+v.height)};
}
void same_extent(const Capture &a,const Capture &b) {
    require(a.width==b.width && a.height==b.height,"history capture extent changed");
}
void same_region(const Capture &a,const Capture &b,Rectangle rectangle,const std::string &name) {
    same_extent(a,b);
    for(unsigned y=rectangle.top;y<rectangle.bottom;++y)for(unsigned x=rectangle.left;x<rectangle.right;++x) {
        const auto at=std::size_t(y)*a.width+x;
        require(a.depth[at]==b.depth[at] && a.stencil[at]==b.stencil[at] &&
            std::equal(a.rgb.begin()+at*3,a.rgb.begin()+at*3+3,b.rgb.begin()+at*3),name+": RGB/D24/S8 changed");
    }
}
void initialized_stencil(const Capture &frame) {
    // Current native pipelines have no nonzero stencil writer. This preserves
    // native initialized zero only, and is weaker than an FF sentinel test.
    require(std::all_of(frame.stencil.begin(),frame.stencil.end(),[](auto value){return value==0;}),
        "native initialized stencil changed");
}
struct View {
    std::vector<Vertex> world,foreground,blocker,sky;
    ModelView model;
};
std::array<View,3> fixture(unsigned count,unsigned width,unsigned height) {
    std::array<View,3> result;
    for(unsigned i=0;i<count;++i) {
        auto &v=result[i];auto &model=v.model;
        model.width=640;model.height=480;
        if(count==2) {model.top=float(i)*240;model.height=240;}
        if(count==3) {
            model.height=240;
            if(i) {model.top=240;model.left=float(i-1)*320;model.width=320;}
        }
        model.full_surface=true;model.camera.oriented=true;model.camera.forward={1,0,0};
        model.camera.up={0,0,1};model.camera.field_of_view=90;
        BspFogSample fog;fog.clear=Vec3{0,0,0};fog.bloom={1,.75f,0,1};model.atmosphere=fog;
        const auto rectangle=physical(model,width,height);
        const float aspect=float(rectangle.right-rectangle.left)/float(rectangle.bottom-rectangle.top);
        // These are explicit diagnostic geometry/camera services, not a title
        // owner, animation phase, original camera or original-frame replay.
        v.world=quad(i,4,4*aspect*.9f,3.6f);
        v.foreground=quad(3,.4f,.4f*aspect*.12f,.4f*.12f);
        v.blocker=quad(i,4,4*aspect*.25f,1.f);
        v.sky=quad(4,8,8*aspect*.95f,7.6f);
        model.vertices=v.world;
    }
    return result;
}
std::vector<ModelView> views(const std::array<View,3> &fixture,unsigned count,bool foreground=false,bool sky=false) {
    std::vector<ModelView> result;
    for(unsigned i=0;i<count;++i) {
        auto model=fixture[i].model;
        if(foreground)model.foreground=fixture[i].foreground;
        if(sky) {model.vertices=fixture[i].blocker;model.background=fixture[i].sky;}
        result.push_back(model);
    }
    return result;
}
std::size_t foreground_history(const Capture &baseline,const Capture &with_foreground,
    const ModelView &view,const std::string &name) {
    same_extent(baseline,with_foreground);const auto r=physical(view,baseline.width,baseline.height);
    const unsigned w=r.right-r.left,h=r.bottom-r.top;
    const Rectangle protected_area{r.left+w/3,r.top+h/3,r.right-w/3,r.bottom-h/3};
    std::size_t world_depth=0,changed_depth=0,changed_rgb=0,protected_world=0;
    for(unsigned y=r.top;y<r.bottom;++y)for(unsigned x=r.left;x<r.right;++x) {
        const auto at=std::size_t(y)*baseline.width+x;
        world_depth+=baseline.depth[at]!=0xffffffu;
        changed_depth+=baseline.depth[at]!=with_foreground.depth[at];
        changed_rgb+=!std::equal(baseline.rgb.begin()+at*3,baseline.rgb.begin()+at*3+3,with_foreground.rgb.begin()+at*3);
        if(x<protected_area.left || x>=protected_area.right || y<protected_area.top || y>=protected_area.bottom) {
            require(baseline.depth[at]==with_foreground.depth[at],name+": foreground accumulation cleared world D24 outside its footprint");
            require(std::equal(baseline.rgb.begin()+at*3,baseline.rgb.begin()+at*3+3,with_foreground.rgb.begin()+at*3),
                name+": foreground draw changed RGB outside its footprint");
            protected_world+=baseline.depth[at]!=0xffffffu;
        }
    }
    const auto area=std::size_t(w)*h;
    const auto counts=" (world="+std::to_string(world_depth)+", protected="+std::to_string(protected_world)+
        ", changed D24="+std::to_string(changed_depth)+", RGB="+std::to_string(changed_rgb)+", area="+std::to_string(area)+")";
    require(world_depth>area/3 && protected_world>area/4,name+": world depth diagnostic is vacuous"+counts);
    require(changed_depth>10 && changed_depth<area/20,name+": foreground D24 footprint is absent or unexpectedly large"+counts);
    require(changed_rgb>10 && changed_rgb<area/20,name+": foreground RGB footprint is absent or unexpectedly large"+counts);
    return protected_world;
}
void sky_history(const Capture &blocker,const Capture &with_sky,const ModelView &view,const std::string &name) {
    same_extent(blocker,with_sky);const auto r=physical(view,blocker.width,blocker.height);
    std::size_t hidden=0,visible=0;
    for(unsigned y=r.top;y<r.bottom;++y)for(unsigned x=r.left;x<r.right;++x) {
        const auto at=std::size_t(y)*blocker.width+x;
        require(blocker.depth[at]==with_sky.depth[at],name+": sky changed stored world D24");
        const bool color_same=std::equal(blocker.rgb.begin()+at*3,blocker.rgb.begin()+at*3+3,with_sky.rgb.begin()+at*3);
        if(blocker.depth[at]!=0xffffffu) {require(color_same,name+": sky covered nearer world geometry");++hidden;}
        else visible+=!color_same;
    }
    require(hidden>100 && visible>100,name+": sky occlusion diagnostic is vacuous (hidden="+
        std::to_string(hidden)+", visible="+std::to_string(visible)+")");
    std::cout<<name<<": sky hidden="<<hidden<<", visible="<<visible<<" pixels\n";
}
void verify(const std::filesystem::path &source,const std::filesystem::path &output) {
    Cache cache(source/"assets/disc/maps/zanzibar.map");const auto bsp=cache.load_bsp(0);
    const auto world=std::find_if(bsp.materials.begin(),bsp.materials.end(),[](const Material &m){
        return m.forward_pass && !m.vertex_program.empty() && m.environment_pass &&
            m.environment_pass->forward_pass && !m.environment_pass->vertex_program.empty();
    });require(world!=bsp.materials.end(),"Zanzibar authored accumulation material is unavailable");
    const auto sky_definition=cache.load_sky(0);require(sky_definition && sky_definition->model!=null_datum,"Zanzibar sky model missing");
    const auto sky_model=cache.load_model(sky_definition->model);
    require(sky_model.mesh.materials.size()>2,"Zanzibar sky A material missing");
    const auto &authored_sky=sky_model.mesh.materials[2];
    require(authored_sky.forward_raster.depth_enable && !authored_sky.forward_raster.depth_write &&
        authored_sky.forward_raster.depth_function==0x203,"authored sky LEQUAL/no-write state changed");
    Mesh mesh;
    for(const auto color:std::array{std::array<unsigned char,4>{200,40,30,255},
        std::array<unsigned char,4>{35,180,45,255},std::array<unsigned char,4>{45,65,200,255},
        std::array<unsigned char,4>{230,220,35,255}}) {
        auto material=diagnostic(*world,color,true);
        material.forward_blend=0;material.forward_raster.depth_enable=1;material.forward_raster.depth_write=1;
        material.forward_raster.depth_function=0x203;material.forward_raster.cull=0;
        mesh.materials.push_back(std::move(material));
    }
    mesh.materials.push_back(diagnostic(authored_sky,{30,150,220,255},false));
    const auto bank=cache.load_bsp_atmosphere(0);
    auto reused_window=window(640,480);Renderer reused(reused_window.get(),mesh,true);reused.scene_bsp_atmosphere(bank);
    const auto source_clear=load_reference_player_state(source/"tests/fixtures/retail_zanzibar_view_clear.reference").view_clear;
    require(source_clear.has_value(),"actual original clear fixture absent");
    std::size_t protected_world=0;unsigned checks=0;
    for(const auto dimensions:std::array{std::array<unsigned,3>{640,480,1},
        std::array<unsigned,3>{853,479,2},std::array<unsigned,3>{1281,721,3},std::array<unsigned,3>{640,480,1}}) {
        const auto [width,height,count]=dimensions;
        SDL_SetWindowSize(reused_window.get(),int(width),int(height));SDL_PumpEvents();reused.resized();
        int actual_width{},actual_height{};SDL_Vulkan_GetDrawableSize(reused_window.get(),&actual_width,&actual_height);
        require(actual_width==int(width) && actual_height==int(height),"history drawable rejected requested size");
        auto data=fixture(count,width,height);auto base=views(data,count);auto full=views(data,count,true);
        const auto name=std::to_string(checks)+"-"+std::to_string(width)+"x"+std::to_string(height)+"-"+std::to_string(count)+"p";
        const auto baseline=capture(reused,base,output/(name+"-world.ppm"));
        const auto foreground=capture(reused,full,output/(name+"-foreground.ppm"));
        initialized_stencil(baseline);initialized_stencil(foreground);
        for(unsigned i=0;i<count;++i)protected_world+=foreground_history(baseline,foreground,base[i],name+" view "+std::to_string(i));
        auto blocker=base;for(unsigned i=0;i<count;++i)blocker[i].vertices=data[i].blocker;
        auto sky=views(data,count,false,true);
        const auto no_sky=capture(reused,blocker,output/(name+"-blocker.ppm"));
        const auto with_sky=capture(reused,sky,output/(name+"-sky.ppm"));
        initialized_stencil(no_sky);initialized_stencil(with_sky);
        for(unsigned i=0;i<count;++i)sky_history(no_sky,with_sky,base[i],name+" view "+std::to_string(i));
        if(count>1) {
            auto isolated=base;isolated[0].foreground=data[0].foreground;
            const auto first_only=capture(reused,isolated,output/(name+"-first-foreground-only.ppm"));
            for(unsigned i=0;i<count;++i)same_region(first_only,i?baseline:foreground,physical(base[i],width,height),name+" split isolation");
        }
        auto fresh_window=window(width,height);Renderer fresh(fresh_window.get(),mesh,true);fresh.scene_bsp_atmosphere(bank);
        const auto fresh_frame=capture(fresh,full,output/(name+"-fresh.ppm"));
        same_region(foreground,fresh_frame,{0,0,width,height},name+" fresh/reused history");
        const auto repeated=capture(reused,full,output/(name+"-repeat.ppm"));
        std::vector<ModelView> clear_views;
        for(const auto &view:base) {
            ModelView clear;clear.camera=view.camera;clear.left=view.left;clear.top=view.top;
            clear.width=view.width;clear.height=view.height;clear.full_surface=true;clear.source_view_clear=source_clear;
            // The renderer has a BSP postprocess bank. Supply an explicit
            // empty atmosphere for this storage diagnostic, with no geometry
            // or fog contribution; this is not an authored view fixture.
            clear.atmosphere=BspFogSample{};
            clear_views.push_back(clear);
        }
        const auto cleared=capture(reused,clear_views,output/(name+"-original-clear.ppm"));
        for(const auto &view:clear_views) {
            const auto r=physical(view,width,height);
            for(unsigned y=r.top;y<r.bottom;++y)for(unsigned x=r.left;x<r.right;++x) {
                const auto at=std::size_t(y)*width+x;
                require(cleared.rgb[at*3]==0x47 && cleared.rgb[at*3+1]==0x5b && cleared.rgb[at*3+2]==0x63 &&
                    cleared.depth[at]==0xffffff && cleared.stencil[at]==0,name+": captured original RGB/D24/S8 clear differs");
            }
        }
        same_region(foreground,repeated,{0,0,width,height},name+" repeated history");
        ++checks;std::cout<<name<<": D24/foreground/sky/split/reuse controls passed\n"<<std::flush;
    }
    std::cout<<checks<<" native history layouts; "<<protected_world<<" nonclear world depth samples preserved outside FP; "
        "initialized S8 zero only (no sentinel writer); controlled copy-PS/material fixture, not original appearance parity\n";
}
} // namespace
int main(int argc,char **argv) {try {
    require(argc==2,"native view history test requires capture directory");
#if defined(__linux__)
    if(!std::getenv("DISPLAY") && !std::getenv("WAYLAND_DISPLAY"))return 77;
    setenv("VK_LAYER_VALIDATE_SYNC","1",1);setenv("HALO2_DEPTH_DIAGNOSTICS","1",1);
#endif
    const auto source=std::filesystem::path(HALO2_SOURCE_DIR);
    if(!std::filesystem::exists(source/"assets/disc/maps/zanzibar.map"))return 77;
    const std::filesystem::path output=argv[1];std::filesystem::create_directories(output);
    require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    std::ostringstream validation;auto *previous=std::cerr.rdbuf(validation.rdbuf());
    try {verify(source,output);}catch(...) {std::cerr.rdbuf(previous);std::cerr<<validation.str();throw;}
    std::cerr.rdbuf(previous);SDL_Quit();
    require(validation.str().find("Vulkan validation:")==std::string::npos,"native history Vulkan diagnostics:\n"+validation.str());return 0;
}catch(const std::exception &error) {std::cerr<<error.what()<<'\n';SDL_Quit();return 1;}}
