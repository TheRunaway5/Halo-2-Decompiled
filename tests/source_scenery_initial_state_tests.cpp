#include "halo2/source_scenery_initial_state.hpp"
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
#include <map>

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
    std::ifstream input("tests/fixtures/retail_scenery_initial_bounds.tsv");
    require(bool(input),"missing original scenery constructor bounds fixture");
    std::unique_ptr<Cache> cache;std::string previous,name;LevelContent content;
    unsigned count=0,placement_exact=0,node_exact=0;
    // Retain the historical eight differences for evidence. The exact-count
    // assertion below now rejects them as regressions after the141CE0 fix.
    std::ifstream node_differences("tests/fixtures/retail_scenery_initial_node_differences.tsv");
    require(bool(node_differences),"missing retained SCEN node0 precision boundary");
    std::map<std::pair<std::string,int>,std::map<unsigned,std::uint32_t>> known_node_words;
    std::string difference_map;int difference_placement;unsigned difference_count;
    while(node_differences>>difference_map>>difference_placement>>difference_count) {
        auto &known=known_node_words[{difference_map,difference_placement}];
        for(unsigned i=0;i<difference_count;++i) {unsigned index;std::uint32_t native,original;node_differences>>index>>native>>original;known[index]=native;}
        require(bool(node_differences),"truncated SCEN node0 precision boundary");
    }
    require(node_differences.eof()&&known_node_words.size()==8,"invalid SCEN node0 precision boundary");
    while(input>>name){
        int placement;std::uint32_t definition,flags,scale_word;input>>placement>>definition>>flags>>scale_word;
        std::array<std::uint32_t,13> node_words;std::array<std::uint32_t,4> expected;
        std::array<std::uint32_t,3> velocity;
        for(auto& word:node_words)input>>word;
        for(auto& word:expected)input>>word;
        for(auto& word:velocity)input>>word;
        unsigned control,c1;int variant;input>>control>>c1>>variant;
        require(control==0 && c1==1,"original SCEN control/frozen flags differ");
        require(bool(input),"truncated original scenery constructor fixture");
        if(name!=previous){cache=std::make_unique<Cache>(map_path(name));previous=name;
            content=cache->load_content(0,true);}
        Transform node;node.scale=value(node_words[0]);
        for(unsigned i=0;i<3;++i)node.basis[i]={value(node_words[1+i*3]),value(node_words[2+i*3]),value(node_words[3+i*3])};
        node.position={value(node_words[10]),value(node_words[11]),value(node_words[12])};
        const auto scale=value(scale_word);
        const auto handle=0x12340000u+count;
        const auto bounds=cache->load_object_shadow_definition(definition);
        SourceSceneryInitialState state;
        const auto& first=state.capture(handle,definition,bounds,node,scale);
        require(first.frozen,"original SCEN constructor is frozen");
        require(words(first.sphere)==expected,"original first BDEF0 cached sphere differs");
        require(first.query_runtime_flags==(flags&0x2000u),"original constructor query bit13 differs");
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
            const auto object=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto& p){return p.type==6&&p.placement==placement;});
            require(object!=content.objects.end()&&object->definition==definition,"native scenery placement differs");
            const auto model_definition=cache->load_initial_model_definition(definition);
            std::uint32_t local_fixture_rng=0x12345678;
            const auto selection=source_initial_model_selection(model_definition,object->variant?object->variant:model_definition.default_variant,local_fixture_rng);
            require(selection.variant_index==variant,"original SCEN variant differs");
            require(selection.resolved(),"native scenery initial model state unresolved");
            const auto model=cache->load_model(definition,false,object->variant,selection);
            const auto basis=authored_basis(object->rotation);
            const auto root=source_object_transform(object->position,basis.forward,basis.up,object->scale,false);
            const auto idle=cache->load_initialized_object_idle(definition);
            const auto native_node=compose_transform(root,pose_model_node(model,0,idle?&*idle:nullptr,0));
            std::array<std::uint32_t,13> native_node_words;native_node_words[0]=std::bit_cast<std::uint32_t>(native_node.scale);
            for(unsigned j=0;j<3;++j){native_node_words[1+j*3]=std::bit_cast<std::uint32_t>(native_node.basis[j].x);native_node_words[2+j*3]=std::bit_cast<std::uint32_t>(native_node.basis[j].y);native_node_words[3+j*3]=std::bit_cast<std::uint32_t>(native_node.basis[j].z);}
            native_node_words[10]=std::bit_cast<std::uint32_t>(native_node.position.x);native_node_words[11]=std::bit_cast<std::uint32_t>(native_node.position.y);native_node_words[12]=std::bit_cast<std::uint32_t>(native_node.position.z);
            if(native_node_words==node_words)++node_exact;
            else {
                const auto known=known_node_words.find({name,placement});
                require(known!=known_node_words.end(),"new native SCEN initial node0 mismatch");
                for(unsigned i=0;i<13;++i)if(native_node_words[i]!=node_words[i]) {
                    const auto permitted=known->second.find(i);
                    require(i>0&&i<10&&permitted!=known->second.end()&&permitted->second==native_node_words[i],
                        "SCEN constructor node0 mismatch exceeds retained basis precision boundary");
                }
            }
            const auto native=words(source_shadow_sphere(bounds,native_node,object->scale));
            if(native==expected)++placement_exact;
            else if(placement_probe){std::cout<<"DIFF "<<name<<' '<<placement;for(auto w:native)std::cout<<' '<<w;
                std::cout<<" expected";for(auto w:expected)std::cout<<' '<<w;std::cout<<'\n';}
        }
        ++count;
    }
    require(input.eof()&&count==278,"invalid original scenery constructor fixture count");
    require(node_exact==count,"native SCEN constructor node0 bank differs from original");
    require(placement_exact==count,"native initial scenery placements differ from original cached spheres");
    std::ifstream refresh_input("tests/fixtures/retail_scenery_animation_refresh.tsv");
    require(bool(refresh_input),"missing original SCEN animation refresh fixtures");
    unsigned refresh_count=0,multi_frame=0;int controller_offset;
    while(refresh_input>>controller_offset) {
        SourceSceneryAnimationRefreshInput state;state.controller_offset=std::int16_t(controller_offset);
        int frames;unsigned initial_flags,expected_result,expected_flags;
        refresh_input>>state.graph_datum>>state.animation_graph_datum>>state.animation_binding
            >>frames>>initial_flags>>expected_result>>expected_flags;
        require(bool(refresh_input),"truncated original SCEN animation refresh fixture");
        state.animation_frames=std::int16_t(frames);state.root_header_flags=std::uint8_t(initial_flags);
        const auto actual=source_scenery_animation_refresh(state);
        require(actual.multi_frame==bool(expected_result)&&actual.root_header_flags==expected_flags,
            "SCEN animation refresh/header admission differs from original10A2F0/BBA20");
        multi_frame+=actual.multi_frame;++refresh_count;
    }
    require(refresh_input.eof()&&refresh_count==1390&&multi_frame==210,
        "invalid original SCEN animation refresh fixture count");
    SourceSceneryAnimationRefreshInput attached;attached.root_header_flags=0x80;
    rejected([&]{source_scenery_animation_refresh(attached);},"attached SCEN must require its actual root owner");
    std::ifstream bounds_input("tests/fixtures/retail_scenery_bounds_refresh.tsv");
    require(bool(bounds_input),"missing original frozen SCEN bounds refresh fixtures");
    unsigned bounds_count=0;previous.clear();
    while(bounds_input>>name) {
        std::uint32_t definition,scale_word;unsigned frozen;bounds_input>>definition>>frozen>>scale_word;
        std::array<std::uint32_t,13> node_words;std::array<std::uint32_t,4> expected;
        for(auto&word:node_words)bounds_input>>word;for(auto&word:expected)bounds_input>>word;
        require(bool(bounds_input),"truncated original frozen SCEN bounds refresh fixture");
        if(name!=previous){cache=std::make_unique<Cache>(map_path(name));previous=name;}
        const auto node=std::bit_cast<Transform>(node_words);const auto scale=value(scale_word);
        const auto bounds=cache->load_object_shadow_definition(definition);SourceSceneryInitialState state;
        const auto handle=0x43210000u+bounds_count;
        state.capture(handle,definition,bounds,Transform{},scale);
        rejected([&]{state.refresh_cached_bounds(handle,definition,bounds,node,scale,false);},
            "SCEN cache refresh must require an admitted pose/bounds update");
        const auto& refreshed=state.refresh_cached_bounds(handle,definition,bounds,node,scale,true);
        require(words(refreshed.sphere)==expected,"frozen SCEN regular BDEF0 refresh differs");
        require(refreshed.frozen,"bounds refresh must preserve SCEN frozen state");
        require(&state.retained(handle,definition,node,scale,false)==&refreshed,
            "SCEN refreshed pose was not retained");
        (void)frozen;++bounds_count;
    }
    require(bounds_input.eof()&&bounds_count==556,"invalid original frozen SCEN bounds refresh fixture count");
    if(placement_probe)std::cout<<"Native node0 exact "<<node_exact<<" / "<<count<<'\n';
    if(placement_probe)std::cout<<"Native placement spheres exact "<<placement_exact<<" / "<<count<<'\n';
    std::cout<<"278 original SCEN constructor spheres,1390 refresh admissions,556 frozen bounds updates passed\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
