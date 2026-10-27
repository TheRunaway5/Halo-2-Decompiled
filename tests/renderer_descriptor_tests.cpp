#include "halo2/renderer.hpp"
#include "halo2/source_decal_pass.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void require(bool value,const char* why){if(!value)throw FormatError(why);}
struct Image {unsigned width{},height{};std::vector<unsigned char> pixels;};
Image read(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);Image out;std::string magic;unsigned maximum;
    file>>magic>>out.width>>out.height>>maximum;file.get();
    require(magic=="P6" && maximum==255,"invalid descriptor capture");
    out.pixels.resize(std::size_t(out.width)*out.height*3);
    file.read(reinterpret_cast<char*>(out.pixels.data()),out.pixels.size());
    require(bool(file),"truncated descriptor capture");return out;
}
std::array<unsigned,2> changed(const Image& a,const Image& b) {
    require(a.width==b.width && a.height==b.height,"descriptor image dimensions changed");
    std::array<unsigned,2> counts{};
    for(unsigned y=0;y<a.height;++y)for(unsigned x=0;x<a.width;++x) {
        const auto at=(std::size_t(y)*a.width+x)*3;
        counts[x>=a.width/2]+=!std::equal(a.pixels.begin()+at,a.pixels.begin()+at+3,b.pixels.begin()+at);
    }
    return counts;
}
}
int main(int argc,char** argv)try {
    require(argc==2 || (argc==3 && std::string_view(argv[2])=="--active-updates"),"supply descriptor inspection output directory");
    const bool active=argc==3;
    const std::filesystem::path output=std::filesystem::absolute(argv[1]);
    std::filesystem::create_directories(output);
    require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    auto* window=SDL_CreateWindow("Material descriptor equivalence",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
    require(window,SDL_GetError());
    std::ostringstream diagnostics;auto* previous=std::cerr.rdbuf(diagnostics.rdbuf());
    try {
        const Texture red{1,1,11,{std::byte{180},std::byte{32},std::byte{16},std::byte{255}}};
        const Texture blue{1,1,11,{std::byte{16},std::byte{32},std::byte{180},std::byte{255}}};
        const Texture white{1,1,11,{std::byte{255},std::byte{255},std::byte{255},std::byte{255}}};
        Material material;material.diffuse=red;material.animated_textures[0]={red,blue};
        // Use the complete source43A30 layer4 program with white color inputs
        // and an opaque sprite. Its actual stage0 image then isolates animated
        // descriptor ownership. Neutral test raster keeps aliases independent.
        material.forward_pass=true;material.forward_vertex=3;material.baked_lightmap_texture=true;
        material.forward_raster.depth_enable=0;material.forward_raster.depth_write=0;material.forward_raster.cull=0;
        material.forward_transforms[0]=material.forward_transforms[1]={1,1,0,0};
        material.forward_transforms[2]={1,1,1,1};
        material.forward_program=source_decal_pass(4).program;
        MaterialPass pass;pass.name="descriptor_animation_probe";
        pass.program=pass.definition=material.forward_program;
        material.forward_pass_index=0;material.passes.push_back(std::move(pass));
        Mesh mesh;mesh.materials.assign(256,material);mesh.lightmaps={white,white};
        for(unsigned slot=0;slot<mesh.materials.size();++slot)
            for(unsigned lightmap:{1u,2u})mesh.resident_material_lightmaps.push_back({slot,lightmap});
        const auto quad=[&](float y0,float y1,unsigned slot,unsigned lightmap) {
            Vertex a{{4,y0,-1},{1,1,1},{0,0},slot},b{{4,y1,-1},{1,1,1},{1,0},slot},
                c{{4,y1,1},{1,1,1},{1,1},slot},d{{4,y0,1},{1,1,1},{0,1},slot};
            a.lightmap=b.lightmap=c.lightmap=d.lightmap=lightmap;
            mesh.vertices.insert(mesh.vertices.end(),{a,b,c,a,c,d});
        };
        quad(-2,0,0,1);quad(0,2,1,2);
        Renderer renderer(window,mesh,true);Camera camera;camera.field_of_view=60;
        std::vector<unsigned> active_roots{0,1};
        std::span<const Vertex> admitted;
        const auto update=[&] {
            if(active)renderer.update_material_constants(mesh.materials,active_roots);
            else renderer.update_material_constants(mesh.materials);
        };
        const auto capture=[&](const char* name) {
            const auto path=output/name;
            for(unsigned attempt=0;attempt<5;++attempt)if(renderer.draw(camera,path,admitted))return read(path);
            throw FormatError("descriptor inspection did not submit");
        };
        const auto original=capture("initial.ppm");
        mesh.materials[0].texture_frames[0]=1;update();
        const auto first=capture("first-animated.ppm");const auto first_changed=changed(original,first);
        if(!((first_changed[0]>100 && first_changed[1]==0) || (first_changed[1]>100 && first_changed[0]==0)))
            throw FormatError("animated baked-material descriptor did not isolate its owner: "+
                std::to_string(first_changed[0])+","+std::to_string(first_changed[1]));
        mesh.materials[0].texture_frames[0]=0;update();
        require(capture("restored.ppm").pixels==original.pixels,"animated descriptor did not restore exact pixels");
        for(auto& m:mesh.materials)m.texture_frames[0]=1;
        update();
        const auto all=capture("all-animated.ppm");const auto both=changed(original,all);
        require(both[0]>100 && both[1]>100,"material descriptor update omitted a lightmap alias");
        require(capture("all-repeat.ppm").pixels==all.pixels,"unchanged descriptor frame was not deterministic");
        SDL_SetWindowSize(window,1280,720);SDL_PumpEvents();renderer.resized();
        const auto resized=capture("resized.ppm");
        require(resized.width==1280 && resized.height==720,"descriptor resize did not change its target");
        for(auto& m:mesh.materials)m.texture_frames[0]=0;
        update();
        const auto reset=capture("resized-reset.ppm");const auto resize_changed=changed(resized,reset);
        require(resize_changed[0]>100 && resize_changed[1]>100,"resized descriptor aliases retained a stale image");
        // A previously hidden root must receive its current animated image on
        // admission, even though the filtered path did not prepare earlier frames.
        std::vector<Vertex> incoming(mesh.vertices.begin(),mesh.vertices.begin()+6);
        for(auto& v:incoming){v.position.x=3;v.material=2;}
        admitted=incoming;active_roots.push_back(2);
        mesh.materials[2].texture_frames[0]=1;update();
        const auto late=capture("late-admitted.ppm");
        mesh.materials[2].texture_frames[0]=0;update();
        const auto late_reset=capture("late-restored.ppm");const auto late_changed=changed(late,late_reset);
        require(late_changed[0]+late_changed[1]>100,"late-admitted material retained a stale animated image");
    }catch(...) {
        std::cerr.rdbuf(previous);std::cerr<<diagnostics.str();SDL_DestroyWindow(window);SDL_Quit();throw;
    }
    std::cerr.rdbuf(previous);SDL_DestroyWindow(window);SDL_Quit();
    require(diagnostics.str().empty(),diagnostics.str().c_str());
    std::cout<<"512 baked material/lightmap aliases; independent/all animated-image changes, restoration, repeat, resize and late admission passed\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
