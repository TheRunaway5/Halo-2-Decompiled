#include "halo2/source_object_colors.hpp"
#include "halo2/content.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/object_lighting.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <memory>

using namespace halo2;
namespace {
void require(bool value,const char* message){if(!value)throw FormatError(message);}
std::filesystem::path map_path(const std::string& name){
    auto path=std::filesystem::path("assets/disc/maps")/(name+".map");
    if(!std::filesystem::exists(path))path=std::filesystem::path("assets/map-pack/maps")/(name+".map");
    return path;
}
void selector_fixtures(){
    std::ifstream input("tests/fixtures/retail_object_colors.tsv");
    require(bool(input),"missing original BE240 selector fixtures");
    unsigned count,fixtures=0;Vec3 position;std::uint32_t variant,mask;ObjectColors overrides;
    while(input>>count>>position.x>>position.y>>position.z>>variant>>mask){
        for(auto& v:overrides)input>>v.x>>v.y>>v.z;
        SourceObjectColorDefinition definition;
        for(unsigned i=0;i<count;++i){SourceObjectColorChannel channel;unsigned permutations;
            input>>permutations>>channel.function_count;
            for(unsigned j=0;j<permutations;++j){SourceObjectColorPermutation p;
                input>>p.weight>>p.lower.x>>p.lower.y>>p.lower.z>>p.upper.x>>p.upper.y>>p.upper.z>>p.variant_name;
                channel.permutations.push_back(p);}
            definition.channels.push_back(std::move(channel));
        }
        const auto colors=source_object_initial_colors(definition,position,variant,mask,overrides);
        for(auto color:colors)for(auto value:{color.x,color.y,color.z}){
            std::uint32_t expected;input>>expected;
            require(bool(input)&&std::bit_cast<std::uint32_t>(value)==expected,
                    "original weighted/hash/HSV object color fixture differs");}
        ++fixtures;
    }
    require(input.eof()&&fixtures==376,"invalid original BE240 selector fixture count");
}
std::array<std::uint32_t,28> render_words(const ObjectColors& colors){
    const auto bank=source_object_render_colors(colors);
    std::array<std::uint32_t,28> result{};unsigned out=0;
    for(auto color:colors)result[out++]=pack_object_lighting_rgb(color);
    for(auto color:bank)for(float value:{color.x,color.y,color.z})result[out++]=std::bit_cast<std::uint32_t>(value);
    Material material;material.forward_pass=true;material.forward_pass_index=0;material.passes.resize(1);
    auto& pass=material.passes.front();
    constexpr std::array<unsigned,4> selectors{3,4,21,22};
    for(unsigned i=0;i<4;++i){
        pass.program.words[10+i]=0x77010203;pass.program.words[18+i]=0x88040506;
        pass.pixel_externs.push_back({std::uint16_t(i<<6),4,std::uint8_t(selectors[i])});
        pass.pixel_externs.push_back({std::uint16_t((i<<6)|0xc00),4,std::uint8_t(selectors[i])});
    }
    pass.vertex_externs.push_back({0x1c8a,0,3});
    material.program_constants[0][10]={-1,-2,-3,1.375f};
    pass.constants[0][10]=material.program_constants[0][10];
    // The per-owner renderer alias consumes the same independent original
    // draw-binding words below, while leaving the shared material untouched.
    auto owner_program=pass.program;auto owner_constants=pass.constants;
    apply_object_render_color_bindings(owner_program,owner_constants,pass,bank,{});
    // Each recursive draw bank must receive the same already-converted RGB.
    material.environment_pass=std::make_shared<Material>(material);
    material.world_overlays_before.push_back(std::make_shared<Material>(material));
    material.world_overlays_after.push_back(std::make_shared<Material>(material));
    material.world_bloom_passes.push_back(std::make_shared<Material>(material));
    material.world_planar_passes[0]=std::make_shared<Material>(material);
    const auto shared=material.environment_pass;
    apply_object_colors(material,colors);
    for(unsigned i=0;i<4;++i)result[out++]=owner_program.words[10+i];
    for(unsigned i=0;i<4;++i)result[out++]=owner_program.words[18+i];
    for(float value:owner_constants[0][10])result[out++]=std::bit_cast<std::uint32_t>(value);
    require(material.passes[0].constants[0][10]==material.program_constants[0][10],"authored vertex RGB bank differs from active bank");
    for(const auto& child:{material.environment_pass,material.world_overlays_before[0],material.world_overlays_after[0],
                          material.world_bloom_passes[0],material.world_planar_passes[0]})
        require(child->forward_program.words==material.forward_program.words&&
                child->program_constants[0][10]==material.program_constants[0][10],"recursive object render color bank differs");
    require(shared->program_constants[0][10][0]==-1,"object render color rebinding altered another shared material");
    const auto repeated=source_object_render_colors(bank);
    for(unsigned i=0;i<4;++i)
        require(std::bit_cast<std::uint32_t>(repeated[i].x)==std::bit_cast<std::uint32_t>(bank[i].x)&&
                std::bit_cast<std::uint32_t>(repeated[i].y)==std::bit_cast<std::uint32_t>(bank[i].y)&&
                std::bit_cast<std::uint32_t>(repeated[i].z)==std::bit_cast<std::uint32_t>(bank[i].z),
                "packed render RGB must be idempotent for retained source-player inputs");
    return result;
}
void render_fixtures(){
    std::ifstream input("tests/fixtures/retail_object_render_colors.tsv");
    require(bool(input),"missing original object render color fixtures");
    unsigned fixtures=0;std::uint32_t first;
    while(input>>first){
        ObjectColors colors;unsigned word=0;
        for(auto& color:colors)for(float* value:{&color.x,&color.y,&color.z}){
            std::uint32_t bits=first;if(word++)input>>bits;*value=std::bit_cast<float>(bits);}
        const auto actual=render_words(colors);
        for(auto word:actual){std::uint32_t expected;input>>expected;
            require(bool(input)&&word==expected,"original packed object color draw binding differs");}
        ++fixtures;
    }
    require(input.eof()&&fixtures==512,"invalid original object render color fixture count");
}
}
int main(int argc,char**argv){try{
    if(argc==2&&std::string_view(argv[1])=="--render-probe"){
        std::uint32_t first;
        while(std::cin>>first){ObjectColors colors;unsigned word=0;
            for(auto& color:colors)for(float* value:{&color.x,&color.y,&color.z}){
                std::uint32_t bits=first;if(word++)std::cin>>bits;*value=std::bit_cast<float>(bits);}
            for(auto value:render_words(colors))std::cout<<value<<' ';
            std::cout<<'\n';
        }return 0;
    }
    if(argc==2&&std::string_view(argv[1])=="--probe"){
        unsigned count;Vec3 position;std::uint32_t variant,mask;ObjectColors overrides;
        while(std::cin>>count>>position.x>>position.y>>position.z>>variant>>mask){
            for(auto& v:overrides)std::cin>>v.x>>v.y>>v.z;
            SourceObjectColorDefinition definition;
            for(unsigned i=0;i<count;++i){SourceObjectColorChannel channel;unsigned permutations;
                std::cin>>permutations>>channel.function_count;
                for(unsigned j=0;j<permutations;++j){SourceObjectColorPermutation p;
                    std::cin>>p.weight>>p.lower.x>>p.lower.y>>p.lower.z>>p.upper.x>>p.upper.y>>p.upper.z>>p.variant_name;
                    channel.permutations.push_back(p);}
                definition.channels.push_back(std::move(channel));
            }
            const auto colors=source_object_initial_colors(definition,position,variant,mask,overrides);
            for(auto color:colors)for(auto value:{color.x,color.y,color.z})std::cout<<std::bit_cast<std::uint32_t>(value)<<' ';
            std::cout<<'\n';
        }return 0;
    }
    selector_fixtures();render_fixtures();
    std::ifstream input("tests/fixtures/retail_prop_initial_colors.tsv");
    require(bool(input),"missing original prop constructor color fixtures");
    std::unique_ptr<Cache> cache;LevelContent content;std::string previous,name;unsigned records=0,colored=0,words=0;
    while(input>>name){
        int placement;std::uint32_t datum,variant;unsigned channels;
        input>>placement>>datum>>variant>>channels;
        if(name!=previous){cache=std::make_unique<Cache>(map_path(name));content=cache->load_content(0,true);previous=name;}
        const auto object=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto& p){return p.type==11&&p.placement==placement;});
        require(object!=content.objects.end()&&object->definition==datum,"native prop color placement differs");
        const auto definition=cache->load_source_object_color_definition(datum);
        require(definition.channels.size()==channels,"original constructor color channel count differs");
        require(std::all_of(definition.channels.begin(),definition.channels.end(),[](const auto& channel){return channel.function_count==0;}),
                "authored prop current colors require BE8E0 function inputs");
        const auto model=cache->load_initial_model_definition(datum);
        const auto selected=cache->load_initial_model_selection(datum,object->variant);
        require(selected.resolved(),"prop selected variant remains unresolved");
        const auto actual_variant=selected.variant_index<0?0:model.variants.at(unsigned(selected.variant_index)).name;
        require(actual_variant==variant,"native selected color variant differs from original B1");
        const auto colors=source_object_initial_colors(definition,object->position,actual_variant);
        for(unsigned i=0;i<channels;++i)for(auto color:{colors[i].x,colors[i].y,colors[i].z}){
            std::uint32_t expected;input>>expected;
            if(std::bit_cast<std::uint32_t>(color)!=expected){
                std::cerr<<name<<' '<<placement<<" channel "<<i<<" native "<<std::bit_cast<std::uint32_t>(color)<<" original "<<expected<<'\n';
                throw FormatError("original BE240 prop color differs");}
            ++words;
        }
        ++records;colored+=channels!=0;
    }
    require(input.eof()&&records==415&&colored==79&&words==264,"invalid original prop constructor color fixture count");
    std::ifstream scenery("tests/fixtures/retail_scenery_initial_colors.tsv");
    require(bool(scenery),"missing original scenery constructor color fixtures");
    records=0;words=0;unsigned preview_differences=0;
    while(scenery>>name){
        int placement;std::uint32_t datum,variant;unsigned channels;
        scenery>>placement>>datum>>variant>>channels;
        if(name!=previous){cache=std::make_unique<Cache>(map_path(name));content=cache->load_content(0,true);previous=name;}
        const auto object=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto& p){return p.type==6&&p.placement==placement;});
        require(object!=content.objects.end()&&object->definition==datum,"native colored scenery placement differs");
        const auto definition=cache->load_source_object_color_definition(datum);
        require(definition.channels.size()==channels&&channels==1,"original scenery color channel count differs");
        require(definition.channels[0].function_count==0,"source scenery current colors require BE8E0 function inputs");
        const auto model=cache->load_initial_model_definition(datum);
        const auto selected=cache->load_initial_model_selection(datum,object->variant);
        require(selected.resolved(),"colored scenery selected variant remains unresolved");
        const auto actual_variant=selected.variant_index<0?0:model.variants.at(unsigned(selected.variant_index)).name;
        require(actual_variant==variant,"native scenery color variant differs from original B1");
        const auto colors=source_object_initial_colors(definition,object->position,actual_variant);
        const auto preview=cache->load_object_colors(datum,object->variant);
        for(unsigned i=0;i<channels;++i){const std::array actual{colors[i].x,colors[i].y,colors[i].z};
            const std::array old{preview[i].x,preview[i].y,preview[i].z};
            for(unsigned j=0;j<3;++j){std::uint32_t expected;scenery>>expected;
                require(bool(scenery)&&std::bit_cast<std::uint32_t>(actual[j])==expected,"original BE240 scenery constructor color differs");
                preview_differences+=std::bit_cast<std::uint32_t>(old[j])!=expected;++words;}}
        ++records;
    }
    require(scenery.eof()&&records==24&&words==72&&preview_differences>0,"invalid original scenery constructor color fixture count");
    std::cout<<"376 source selector fixtures, 512 packed draw-color fixtures, 415 prop constructors and 24 colored scenery constructors match original instructions exactly\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
