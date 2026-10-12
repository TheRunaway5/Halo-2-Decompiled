#include "halo2/renderer.hpp"
#include "halo2/source_shader_lod.hpp"
#include "halo2/source_render_policy.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool ok,std::string_view message){if(!ok)throw FormatError(std::string(message));}
struct Image {unsigned width{},height{};std::vector<unsigned char> pixels;};
Image read(const std::filesystem::path &path) {
    std::ifstream input(path,std::ios::binary);std::string magic;unsigned maximum;Image image;
    input>>magic>>image.width>>image.height>>maximum;input.get();
    require(magic=="P6" && maximum==255,"invalid source stipple image");
    image.pixels.resize(std::size_t(image.width)*image.height*3);
    input.read(reinterpret_cast<char *>(image.pixels.data()),std::streamsize(image.pixels.size()));
    require(bool(input),"truncated source stipple image");return image;
}
Image capture(Renderer &renderer,std::span<const ModelView> views,const std::filesystem::path &path) {
    for(unsigned i=0;i<5;++i)if(renderer.draw(views.front().camera,path,{},false,{},{},{},views))return read(path);
    throw FormatError("source stipple frame did not submit");
}
Material source_base(const Cache &cache) {
    const auto bsp=cache.load_bsp(0);
    for(const auto &root:bsp.materials)if(source_material_layer(root)==1) {
        auto result=root;
        // Isolate the actual source layer1 program from its later lighting
        // passes. Uniform white inputs make every retained fragment measurable.
        result.environment_pass.reset();result.world_overlays_before.clear();
        result.world_overlays_after.clear();result.world_bloom_passes.clear();
        result.world_planar_passes={};result.animation.overlays.clear();
        for(auto &bank:result.animated_textures)bank.clear();
        const Texture white{1,1,11,std::vector<std::byte>(4,std::byte{255}),{}};
        result.diffuse=result.detail=result.bump=result.auxiliary=white;
        result.forward_raster.depth_enable=0;result.forward_raster.depth_write=0;
        result.forward_raster.cull=0;result.forward_blend=0;
        return result;
    }
    throw FormatError("actual source layer1 stipple fixture missing");
}
std::vector<Vertex> quad(float aspect,unsigned level) {
    std::vector<Vertex> result;
    for(const auto uv:std::array{Vec2{0,0},Vec2{1,0},Vec2{1,1},Vec2{0,0},Vec2{1,1},Vec2{0,1}}) {
        Vertex vertex;vertex.position={1,(1-uv.x*2)*aspect,(1-uv.y*2)};
        vertex.color={1,1,1};vertex.uv=uv;vertex.normal={-1,0,0};
        vertex.tangent={0,-1,0};vertex.binormal={0,0,-1};
        vertex.source_stipple_level=level;result.push_back(vertex);
    }
    return result;
}
using Window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>;
void verify(const std::filesystem::path &output) {
    const Cache cache("assets/disc/maps/midship.map");Mesh mesh;mesh.materials.push_back(source_base(cache));
    Window window(SDL_CreateWindow("Original screen stipple",0,0,640,480,
        SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN|SDL_WINDOW_RESIZABLE),&SDL_DestroyWindow);
    require(bool(window),SDL_GetError());Renderer renderer(window.get(),mesh,true);
    const std::array<std::array<int,2>,4> sizes{{{640,480},{1280,720},{1279,719},{640,480}}};
    std::size_t checked=0;
    for(unsigned step=0;step<sizes.size();++step) {
        const auto [width,height]=sizes[step];SDL_SetWindowSize(window.get(),width,height);
        SDL_PumpEvents();renderer.resized();std::array<ModelView,2> views;
        std::array<std::vector<Vertex>,2> geometry;
        for(unsigned view=0;view<2;++view) {
            auto &v=views[view];v.camera.oriented=true;v.camera.forward={1,0,0};v.camera.up={0,0,1};
            v.camera.field_of_view=90;v.left=float(view*320);v.width=320;v.height=480;v.full_surface=true;
            const int left=view*width/2,right=(view+1)*width/2;
            SourceFrustumProjectionInput input;input.projection_angle=1.5707963267948966f;
            input.outer={0,std::int16_t(left),std::int16_t(height),std::int16_t(right)};
            input.safe=input.outer;input.scenario_present=true;input.scenario_type=1;
            const auto program=source_frustum_projection(input);
            const auto camera=source_view_projection({{},v.camera.forward,v.camera.up,
                .015625f,1024.f,{},program.projection_program});
            SourceViewShaderConstants constants;
            constants.world=source_view_shader_constants(camera,1024.f,false);
            constants.reflection=constants.world;
            const auto foreground=source_view_shader_constants(camera,1024.f,true);
            std::copy_n(foreground.begin(),4,constants.foreground.begin());
            v.source_view_shader_constants=constants;
            geometry[view]=quad(float(right-left)/height,16);v.vertices=geometry[view];
        }
        const auto stem=std::to_string(step)+"-"+std::to_string(width)+"x"+std::to_string(height);
        const auto full=capture(renderer,views,output/(stem+"-full.ppm"));
        std::array<Image,2> empty;
        for(unsigned view=0;view<2;++view) {
            views[view].vertices={};
            empty[view]=capture(renderer,views,output/(stem+"-empty"+std::to_string(view)+".ppm"));
            views[view].vertices=geometry[view];
        }
        for(unsigned view=0;view<2;++view)for(unsigned alpha:{0u,16u,32u,48u,64u,80u,96u,112u,
            128u,144u,160u,176u,192u,208u,224u,240u,255u}) {
            const auto pattern=source_shader_instance_stipple(std::uint8_t(alpha));
            for(auto &vertex:geometry[view])vertex.source_stipple_level=pattern.level;
            const auto masked=capture(renderer,views,output/(stem+"-view"+std::to_string(view)+"-alpha"+std::to_string(alpha)+".ppm"));
            require(masked.width==full.width && masked.height==full.height,"stipple extent changed");
            for(unsigned y=height/4;y<unsigned(height)*3/4;++y)
                for(unsigned x=view*width/2+32;x<(view+1)*width/2-32;++x) {
                    // Independent original32-row pattern, hardware within-byte
                    // bit reversal, and explicit observed global row origin31.
                    // Odd heights verify the native phase-preserving extension.
                    const unsigned bit=(x&24u)|(7u-(x&7u));
                    const bool keep=((pattern.rows[(31u-y)&31u]>>bit)&1u)!=0;
                    const auto first=(std::size_t(y)*width+x)*3;
                    unsigned response=0;
                    for(unsigned channel=0;channel<3;++channel) {
                        const auto at=first+channel;
                        response+=unsigned(std::abs(int(full.pixels[at])-int(empty[view].pixels[at])));
                        const auto expected=keep?full.pixels[at]:empty[view].pixels[at];
                        if(std::abs(int(masked.pixels[at])-int(expected))>1)
                            throw FormatError("original source stipple differs at "+std::to_string(x)+","+
                                std::to_string(y)+" alpha"+std::to_string(alpha)+" view"+std::to_string(view));
                        ++checked;
                    }
                    require(response>24,"source stipple diagnostic produced no measurable fragment");
                }
            for(unsigned other=1-view,y=height/4;y<unsigned(height)*3/4;++y)
                for(unsigned x=(other*width/2)+32;x<(other+1)*width/2-32;++x)
                    for(unsigned channel=0;channel<3;++channel) {
                        const auto at=(std::size_t(y)*width+x)*3+channel;
                        require(std::abs(int(full.pixels[at])-int(masked.pixels[at]))<=1,
                            "source instance stipple leaked between player views");++checked;
                    }
            for(auto &vertex:geometry[view])vertex.source_stipple_level=16;
        }
    }
    std::cout<<checked<<" independent source stipple view bytes checked\n";
}
}
int main(int argc,char **argv){try {
    require(argc==2,"expected source stipple capture directory");
#if defined(__linux__)
    if(!std::getenv("DISPLAY") && !std::getenv("WAYLAND_DISPLAY"))return 77;
    setenv("VK_LAYER_VALIDATE_SYNC","1",1);
#endif
    require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());std::filesystem::path output=argv[1];
    std::filesystem::create_directories(output);std::ostringstream diagnostics;auto *saved=std::cerr.rdbuf(diagnostics.rdbuf());
    try{verify(output);}catch(...){std::cerr.rdbuf(saved);std::cerr<<diagnostics.str();throw;}
    std::cerr.rdbuf(saved);SDL_Quit();require(diagnostics.str().find("Vulkan validation:")==std::string::npos,
        "source stipple synchronization diagnostics:\n"+diagnostics.str());return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';SDL_Quit();return 1;}}
