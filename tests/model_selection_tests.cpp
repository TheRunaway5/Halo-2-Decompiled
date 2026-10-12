#include "halo2/model_selection.hpp"
#include "halo2/model.hpp"
#include "halo2/content.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
void selection(const SourceInitialModelSelection&s,std::uint32_t rng){
    std::cout<<s.variant_index<<' '<<s.permutations.size();
    for(auto p:s.permutations)std::cout<<' '<<unsigned(p);
    for(const auto&t:s.states)std::cout<<' '<<int(t.variant_permutation)<<' '<<unsigned(t.damage)<<' '<<unsigned(t.properties);
    std::cout<<' '<<rng<<'\n';
}
void metadata(const SourceInitialModelDefinition&d){
    std::cout<<d.model<<' '<<d.render_model<<' '<<d.default_variant<<' '<<d.regions.size();
    for(const auto&r:d.regions){std::cout<<' '<<r.name<<' '<<r.permutation_flags.size();for(auto f:r.permutation_flags)std::cout<<' '<<unsigned(f);}
    std::cout<<' '<<d.variants.size();
    for(const auto&v:d.variants){std::cout<<' '<<v.name;for(auto r:v.regions)std::cout<<' '<<int(r);std::cout<<' '<<v.definitions.size();
        for(const auto&r:v.definitions){std::cout<<' '<<r.permutations.size();for(const auto&p:r.permutations){
            std::cout<<' '<<int(p.permutation)<<' '<<std::bit_cast<std::uint32_t>(p.weight)<<' '<<p.states.size();
            for(const auto&s:p.states)std::cout<<' '<<int(s.permutation)<<' '<<unsigned(s.properties)<<' '<<s.damage<<' '<<std::bit_cast<std::uint32_t>(s.probability);
        }}
    }
    std::cout<<' '<<d.render_sections.size();for(const auto&r:d.render_sections){std::cout<<' '<<r.size();for(const auto&p:r)for(auto s:p)std::cout<<' '<<s;}
    std::cout<<'\n';
}
}
int main(int argc,char**argv){try{
    if(argc==3&&std::string(argv[1])=="--metadata"){
        Cache cache(argv[2]);std::uint32_t datum;while(std::cin>>datum)metadata(cache.load_initial_model_definition(datum));
        require(std::cin.eof(),"invalid initial model metadata fixture");return 0;
    }
    if(argc==3&&std::string(argv[1])=="--select"){
        Cache cache(argv[2]);std::uint32_t datum,variant,rng,skip;
        while(std::cin>>datum>>variant>>rng>>skip){const auto d=cache.load_initial_model_definition(datum);
            const auto selected=source_initial_model_selection(d,variant,rng,skip);selection(selected,rng);}
        require(std::cin.eof(),"invalid initial model selection fixture");return 0;
    }
    if(argc==3&&std::string(argv[1])=="--deterministic"){
        Cache cache(argv[2]);std::uint32_t datum,variant;
        while(std::cin>>datum>>variant){const auto s=source_deterministic_initial_model_selection(cache.load_initial_model_definition(datum),variant);
            std::cout<<s.variant_index<<' '<<s.permutations.size();for(auto p:s.permutations)std::cout<<' '<<unsigned(p);
            std::cout<<' '<<s.unresolved_rng_regions.size();for(auto i:s.unresolved_rng_regions)std::cout<<' '<<i;std::cout<<'\n';}
        require(std::cin.eof(),"invalid deterministic initial model fixture");return 0;
    }
    if(argc==3&&std::string(argv[1])=="--sections"){
        Cache cache(argv[2]);std::uint32_t datum,variant;unsigned lod;
        while(std::cin>>datum>>variant>>lod){const auto d=cache.load_initial_model_definition(datum);
            const auto s=source_deterministic_initial_model_selection(d,variant);const auto sections=source_initial_model_sections(d,s,lod);
            std::cout<<sections.size();for(auto id:sections)std::cout<<' '<<id;std::cout<<'\n';}
        require(std::cin.eof(),"invalid source model section fixture");return 0;
    }
    SourceInitialModelDefinition d;d.model=7;d.regions.push_back({1,{1,1}});
    SourceInitialModelVariant v;v.name=99;v.regions.fill(-1);v.regions[0]=0;
    v.definitions.push_back({{{1,1,{}}}});d.variants.push_back(v);
    require(source_initial_model_variant(d,0)==0,"NONE must select first authored variant");
    require(source_initial_model_variant(d,100)==-1,"unknown variant must remain NONE");
    std::uint32_t rng=123;auto s=source_initial_model_selection(d,0,rng);
    require(s.permutations==std::vector<std::uint8_t>{1}&&rng==123,"singleton variant must not consume weighted RNG");
    d.variants[0].definitions[0].permutations[0].states.push_back({0,0,0,1});
    rng=123;s=source_initial_model_selection(d,0,rng,1);
    require(s.permutations[0]==255&&rng==123u*0x19660du+0x3c6ef35fu,"skipped region must still consume initial state RNG");
    require(!source_deterministic_initial_model_selection(d,0).resolved(),"probability1 endpoint must preserve base-state uncertainty");
    d.variants[0].definitions[0].permutations[0].states[0].permutation=1;
    require(source_deterministic_initial_model_selection(d,0).resolved(),"state RNG with identical output must be proven deterministic");
    d.variants.clear();d.regions[0].permutation_flags.clear();rng=1;s=source_initial_model_selection(d,0,rng);
    require(s.permutations[0]==255,"empty source region must stay absent");
    d.regions[0].permutation_flags={1,1};s=source_initial_model_selection(d,0,rng);
    require(s.permutations[0]==0,"source no-random-permutation fallback must select permutation0");
    const std::vector<std::string> names={"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar","backwash","containment","deltatap","dune","elongation","gemini","triplicate","turf","warlock"};
    unsigned machines=0;for(const auto&name:names){auto path=std::filesystem::path("assets/disc/maps")/(name+".map");
        if(!std::filesystem::exists(path))path=std::filesystem::path("assets/map-pack/maps")/(name+".map");
        Cache cache(path);const auto content=cache.load_content(0,true);
        for(const auto&o:content.objects)if(o.type==7){const auto definition=cache.load_initial_model_definition(o.definition);
            if(definition.render_model==0xffffffffu)continue;const auto selected=cache.load_initial_model_selection(o.definition,o.variant);
            require(selected.resolved(),"machine initial active bytes must be source resolved");const auto ids=source_initial_model_sections(definition,selected,5);
            const auto model=cache.load_model(o.definition,false,o.variant);std::set<unsigned> expected(ids.begin(),ids.end()),actual;
            for(auto origin:model.vertex_origins)actual.insert(origin.section);
            require(expected==actual,"machine loaded sections differ from original active selection");++machines;
        }
        if(name=="colossus")for(const auto&o:content.objects)if(o.type==11){
            const auto definition=cache.load_initial_model_definition(o.definition);if(definition.render_model==0xffffffffu)continue;
            const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto&t){return t.datum==o.definition;});
            if(tag==cache.tags().end()||!tag->name.ends_with("gas_container\\gas_container"))continue;
            const auto selected=cache.load_initial_model_selection(o.definition,o.variant);
            require(selected.permutations==std::vector<std::uint8_t>({0,255}),"gas container must omit its absent source region");
        }
    }
    require(machines==49,"initial model selection must cover all49 authored machines");
    std::cout<<"49 original machine selections and initial-state boundaries passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
