#include "halo2/renderer.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void require(bool value,const char *message) {
    if(!value)throw std::runtime_error(message);
}
struct Image {unsigned width{},height{};std::vector<unsigned char> pixels;};
Image read(const std::filesystem::path &path) {
    std::ifstream input(path,std::ios::binary);std::string magic;unsigned maximum;
    Image image;input>>magic>>image.width>>image.height>>maximum;input.get();
    require(magic=="P6" && maximum==255,"invalid residency capture");
    image.pixels.resize(image.width*image.height*3);
    input.read(reinterpret_cast<char*>(image.pixels.data()),image.pixels.size());
    require(bool(input),"incomplete residency capture");return image;
}
std::array<unsigned,2> changed_halves(const Image &a,const Image &b) {
    require(a.width==b.width && a.height==b.height,"residency capture dimensions changed");
    std::array<unsigned,2> changed{};
    for(unsigned y=0;y<a.height;++y)for(unsigned x=0;x<a.width;++x) {
        const auto offset=(y*a.width+x)*3;
        if(!std::equal(a.pixels.begin()+offset,a.pixels.begin()+offset+3,b.pixels.begin()+offset))
            ++changed[x>=a.width/2];
    }
    return changed;
}
}
int main(int argc,char **argv) {
    try {
        require(argc==2,"usage: renderer_material_residency_tests output-directory");
        const auto output=std::filesystem::absolute(argv[1]);std::filesystem::create_directories(output);
        require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
        auto *window=SDL_CreateWindow("Independent weapon material residency",0,0,640,480,
            SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
        require(window,SDL_GetError());
        Texture red{1,1,11,{std::byte{180},std::byte{32},std::byte{16},std::byte{255}}};
        red.mips={{1,1,0,4,1}};
        // Dynamic uploads are raw single-image payloads; retain the authored
        // mip metadata on the immutable asset used to build the renderer.
        auto red_update=red;red_update.mips.clear();
        Material material;material.diffuse=material.detail=material.auxiliary=red;
        material.bump={1,1,11,{std::byte{128},std::byte{128},std::byte{255},std::byte{255}}};
        for(auto &frames:material.animated_textures)frames={red,red};
        Mesh mesh;mesh.materials.assign(16,material);
        auto volume=material;volume.diffuse.depth=1;mesh.materials.push_back(volume);
        const auto quad=[&](float y0,float y1,unsigned slot) {
            const Vertex a{{4,y0,-1},{1,1,1},{0,0},slot},b{{4,y1,-1},{1,1,1},{1,0},slot},
                c{{4,y1,1},{1,1,1},{1,1},slot},d{{4,y0,1},{1,1,1},{0,1},slot};
            mesh.vertices.insert(mesh.vertices.end(),{a,b,c,a,c,d});
        };
        quad(-2,0,0);quad(0,2,1);
        std::ostringstream validation;auto *previous=std::cerr.rdbuf(validation.rdbuf());
        try {
            Renderer renderer(window,mesh,true);Camera camera;camera.field_of_view=60;
            const auto resident=renderer.resident_material_images();
            require(resident==3,"identical weapon/stage/animated images did not share or a volume aliased 2D storage");
            renderer.require_lens_bitmap(1,red);
            auto writable=red;auto *retained=writable.pixels.data();
            renderer.require_lens_bitmap(1,writable);retained[0]^=std::byte{1};
            bool mutation_rejected=false;
            try{renderer.require_lens_bitmap(1,writable);}catch(const FormatError &){mutation_rejected=true;}
            require(mutation_rejected,"fingerprint cache admitted a retained writer's changed bitmap");
            renderer.require_lens_bitmap(1,red);
            bool volume_rejected=false;
            try {renderer.update_image(16,red_update);}catch(const std::runtime_error &) {volume_rejected=true;}
            require(volume_rejected && renderer.resident_material_images()==resident,
                "2D update replaced a volume image");
            const auto capture=[&](const char *name) {
                const auto path=output/name;
                require(renderer.draw(camera,path),"residency draw failed");return read(path);
            };
            const auto before=capture("shared.ppm");
            auto blue=red_update;blue.pixels={std::byte{16},std::byte{32},std::byte{180},std::byte{255}};
            renderer.update_image(0,blue);
            require(renderer.resident_material_images()==resident+1,"mutable material did not detach once");
            const auto first=capture("first-private.ppm");
            const auto changed=changed_halves(before,first);
            require((changed[0]>100 && changed[1]==0) || (changed[1]>100 && changed[0]==0),
                "one material update changed the other owner's framebuffer");
            renderer.update_image(0,red_update);
            require(renderer.resident_material_images()==resident+1,"private material allocated again on update");
            const auto restored=capture("restored.ppm");
            require(before.pixels==restored.pixels,"private material update did not restore original pixels");
            renderer.update_image(1,blue);
            const auto second=capture("second-private.ppm");
            const auto other=changed_halves(before,second);
            require(renderer.resident_material_images()==resident+2 &&
                ((changed[0]>100 && other[0]==0 && other[1]>100) ||
                 (changed[1]>100 && other[1]==0 && other[0]>100)),
                "second owner's image update did not remain independent");
        }catch(...) {
            std::cerr.rdbuf(previous);std::cerr<<validation.str();
            SDL_DestroyWindow(window);SDL_Quit();throw;
        }
        std::cerr.rdbuf(previous);SDL_DestroyWindow(window);SDL_Quit();
        require(validation.str().empty(),validation.str().c_str());
        std::cout<<"16 non-HUD material banks, all four stages and animated frames share two immutable images; a distinct volume stays private; "
            <<"independent updates and framebuffer restoration passed with no validation messages\n";
        return 0;
    }catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
