#include "halo2/source_euler.hpp"
#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_device.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/transform.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <memory>

using namespace halo2;
namespace {
void require(bool value,const char* message){if(!value)throw FormatError(message);}
std::array<std::uint32_t,9> result(const std::array<std::uint32_t,3>& in){
    const Vec3 angles{std::bit_cast<float>(in[0]),std::bit_cast<float>(in[1]),std::bit_cast<float>(in[2])};
    const auto direct=std::bit_cast<std::array<std::uint32_t,9>>(source_euler_basis(angles));
    require(direct==std::bit_cast<std::array<std::uint32_t,9>>(authored_basis(angles)),"authored Euler consumer bypassed the source helper");
    return direct;}
}
int main(int argc,char**argv){try{
    const bool probe=argc==2&&std::string_view(argv[1])=="--probe";
    std::ifstream fixture; if(!probe){fixture.open("tests/fixtures/retail_source_euler.tsv");require(bool(fixture),"missing original Euler fixture");}
    auto& input=probe?std::cin:fixture;std::array<std::uint32_t,3> angles;unsigned count=0;
    while(input>>angles[0]){input>>angles[1]>>angles[2];require(bool(input),"truncated Euler input");const auto actual=result(angles);
        if(probe){for(unsigned i=0;i<actual.size();++i)std::cout<<(i?" ":"")<<actual[i];std::cout<<'\n';}
        else {std::array<std::uint32_t,9> expected;for(auto&w:expected)input>>w;require(bool(input),"truncated original Euler output");require(actual==expected,"original scenario Euler basis differs");}
        ++count;
    }
    require(input.eof(),"invalid Euler fixture");
    if(!probe){
        require(count==2032,"incomplete original Euler fixture");
        // Initial generic B7B40/BDEF0 cache, before later device channel
        // updates. Source position/power overlays are separate live poses.
        std::ifstream machines("tests/fixtures/retail_machine_initial_bounds.tsv");
        require(bool(machines),"missing original machine constructor bounds fixture");
        std::unique_ptr<Cache> cache;std::string map,previous;LevelContent content;
        unsigned owners=0,node_exact=0;
        while(machines>>map){int placement;std::uint32_t definition,scale_word;
            std::array<std::uint32_t,13> node_expected;std::array<std::uint32_t,4> sphere_expected;
            machines>>placement>>definition>>scale_word;
            for(auto&w:node_expected)machines>>w;for(auto&w:sphere_expected)machines>>w;
            require(bool(machines),"truncated machine constructor fixture");
            if(map!=previous){auto path=std::filesystem::path("assets/disc/maps")/(map+".map");
                if(!std::filesystem::exists(path))path=std::filesystem::path("assets/map-pack/maps")/(map+".map");
                cache=std::make_unique<Cache>(path);content=cache->load_content(0,true);previous=map;}
            const auto object=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto&o){return o.type==7&&o.placement==placement;});
            require(object!=content.objects.end()&&object->definition==definition,"machine constructor source identity differs");
            require(std::bit_cast<std::uint32_t>(object->scale)==scale_word,"machine constructor scale differs");
            const auto basis=authored_basis(object->rotation);
            auto node=source_object_transform(object->position,basis.forward,basis.up,object->scale,false);
            const auto device=cache->load_source_machine_definition(definition);
            require(!device.initialized_idle,"machine generic constructor primary is not NONE");
            if(device.render_model!=0xffffffffu){
                const auto selection=cache->load_initial_model_selection(definition,object->variant);
                require(selection.resolved(),"machine constructor selection unresolved");
                const auto model=cache->load_model(definition,false,object->variant,selection);
                node=compose_transform(node,pose_model_node(model,0));
            }
            const auto sphere=source_shadow_sphere(cache->load_object_shadow_definition(definition),node,object->scale);
            const std::array<std::uint32_t,4> actual{std::bit_cast<std::uint32_t>(sphere.center.x),
                std::bit_cast<std::uint32_t>(sphere.center.y),std::bit_cast<std::uint32_t>(sphere.center.z),std::bit_cast<std::uint32_t>(sphere.radius)};
            require(actual==sphere_expected,"initial machine cached sphere differs from original");
            if(std::bit_cast<std::array<std::uint32_t,13>>(node)==node_expected)++node_exact;
            ++owners;
        }
        require(machines.eof()&&owners==49,"incomplete machine constructor fixture");
        require(node_exact==owners,"machine constructor node0 bank differs from original");
        std::cout<<count<<" untouched source Euler fixtures, "<<owners<<" machine cached spheres, "
                 <<node_exact<<" exact machine node0 banks passed\n";
    }
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
