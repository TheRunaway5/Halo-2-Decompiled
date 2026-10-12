#include "halo2/source_prop_initial_state.hpp"
#include "halo2/content.hpp"
#include "halo2/math.hpp"
#include "halo2/model.hpp"
#include "halo2/model_selection.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>

using namespace halo2;
namespace {
void require(bool value,const char* message){if(!value)throw FormatError(message);}
float value(std::uint32_t word){return std::bit_cast<float>(word);}
std::array<std::uint32_t,4> words(ShadowCasterSphere sphere){return {
    std::bit_cast<std::uint32_t>(sphere.center.x),std::bit_cast<std::uint32_t>(sphere.center.y),
    std::bit_cast<std::uint32_t>(sphere.center.z),std::bit_cast<std::uint32_t>(sphere.radius)};}
std::filesystem::path map_path(const std::string& name){
    auto path=std::filesystem::path("assets/disc/maps")/(name+".map");
    if(!std::filesystem::exists(path))path=std::filesystem::path("assets/map-pack/maps")/(name+".map");
    return path;
}
template<class F>void rejected(F action,const char* message){
    bool failed=false;try{action();}catch(const FormatError&){failed=true;}
    require(failed,message);
}
}
int main(int argc,char**argv){try{
    const bool placement_probe=argc==2&&std::string_view(argv[1])=="--placement-probe";
    std::ifstream input("tests/fixtures/retail_prop_initial_bounds.tsv");
    require(bool(input),"missing original prop constructor bounds fixture");
    std::unique_ptr<Cache> cache;std::string previous,name;LevelContent content;
    unsigned count=0,placement_exact=0;
    while(input>>name){
        int placement;std::uint32_t definition,flags,scale_word;input>>placement>>definition>>flags>>scale_word;
        std::array<std::uint32_t,13> node_words;std::array<std::uint32_t,4> expected;
        std::array<std::uint32_t,3> velocity;
        for(auto& word:node_words)input>>word;
        for(auto& word:expected)input>>word;
        for(auto& word:velocity)input>>word;
        require(bool(input),"truncated original prop constructor fixture");
        if(name!=previous){cache=std::make_unique<Cache>(map_path(name));previous=name;
            content=cache->load_content(0,true);}
        Transform node;node.scale=value(node_words[0]);
        for(unsigned i=0;i<3;++i)node.basis[i]={value(node_words[1+i*3]),value(node_words[2+i*3]),value(node_words[3+i*3])};
        node.position={value(node_words[10]),value(node_words[11]),value(node_words[12])};
        const auto scale=value(scale_word);
        const auto handle=0x12340000u+count;
        const auto bounds=cache->load_object_shadow_definition(definition);
        SourcePropInitialState state;
        const auto& first=state.capture(handle,definition,bounds,node,scale);
        require(words(first.sphere)==expected,"original first BDEF0 cached sphere differs");
        require(first.query_runtime_flags==(flags&0x2000u),"original constructor query bit13 differs");
        // Literal D5560 force1/D5060 mask0x1883/B7B40 request-bit binder;
        // independently checked over1024 original instruction cases.
        require(first.frozen,"cold original factory frozen flag was lost");
        require(velocity==std::array<std::uint32_t,3>{0,0,0}&&first.velocity.x==0&&first.velocity.y==0&&first.velocity.z==0,
                "original constructor velocity differs");
        require(&state.retained(handle,definition,node,scale,false)==&first,"stationary constructor cache was recomputed");
        if(count==0){
            rejected([&]{state.capture(handle,definition,bounds,node,scale);},"duplicate full handle must be rejected");
            rejected([&]{state.retained(handle,definition,node,scale,true);},"advanced physics must require its source producer");
            rejected([&]{state.retained(handle+1,definition,node,scale,false);},"different full handle must not reuse constructor cache");
            rejected([&]{state.retained(handle,definition+1,node,scale,false);},"different definition must not reuse constructor cache");
            auto changed=node;changed.position.x=std::nextafter(changed.position.x,INFINITY);
            rejected([&]{state.retained(handle,definition,changed,scale,false);},"changed root must require its source producer");
            rejected([&]{state.retained(handle,definition,node,std::nextafter(scale,INFINITY),false);},"changed scale must require its source producer");
            state.destroy(handle);require(!state.contains(handle),"destroy must retire constructor bounds");
            state.capture(handle+0x10000u,definition,bounds,node,scale);
        }
        {
            const auto object=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto& p){return p.type==11&&p.placement==placement;});
            require(object!=content.objects.end()&&object->definition==definition,"native prop placement differs");
            const auto selection=cache->load_initial_model_selection(definition,object->variant);
            require(selection.resolved(),"native prop initial model state unresolved");
            const auto model=cache->load_model(definition,false,object->variant,selection);
            const auto basis=authored_basis(object->rotation);
            const auto root=source_object_transform(object->position,basis.forward,basis.up,object->scale,false);
            const auto native_node=compose_transform(root,pose_model_node(model,0));
            const auto native=words(source_shadow_sphere(bounds,native_node,object->scale));
            if(native==expected)++placement_exact;
            else if(placement_probe){std::cout<<"DIFF "<<name<<' '<<placement;for(auto w:native)std::cout<<' '<<w;
                std::cout<<" expected";for(auto w:expected)std::cout<<' '<<w;std::cout<<'\n';}
        }
        ++count;
    }
    require(input.eof()&&count==415,"invalid original prop constructor fixture count");
    require(placement_exact==count,"native initial prop placements differ from original cached spheres");
    if(placement_probe)std::cout<<"Native placement spheres exact "<<placement_exact<<" / "<<count<<'\n';
    std::cout<<"415 original first BDEF0 cached spheres and retained-state boundaries passed\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
