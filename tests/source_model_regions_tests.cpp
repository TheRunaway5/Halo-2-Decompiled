#include "halo2/source_model_regions.hpp"
#include <bit>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv)try{
    if(argc==2 && std::string(argv[1])=="--oracle"){
        std::string line;
        while(std::getline(std::cin,line)){
            std::istringstream input(line);unsigned count;input>>count;std::vector<std::uint32_t> words(count);
            for(auto &word:words)input>>std::hex>>word;
            if(!input)throw FormatError("invalid region oracle fixture");
            unsigned at=0;const auto next=[&](){return words.at(at++);};
            const auto integer=[&](){return std::bit_cast<std::int32_t>(next());};
            SourceInitialModelDefinition definition;definition.model=0x80010001;
            const auto regions=next();SourceModelRegionBank bank;bank.selection.variant_index=integer();
            SourceModelRegionRequest request;request.region=integer();request.damage=integer();request.force=next();
            request.property_bit=integer();request.property_regions=next();std::uint32_t random=next();
            SourceInitialModelVariant variant;variant.regions.fill(-1);
            for(unsigned r=0;r<regions;++r){
                definition.regions.emplace_back();bank.selection.permutations.push_back(next());
                SourceInitialModelRegionState state;state.variant_permutation=std::int8_t(integer());
                state.damage=next();state.properties=next();bank.selection.states.push_back(state);
                bank.looping_effects.push_back(next());variant.regions.at(r)=std::int8_t(integer());
                SourceInitialVariantRegion region;const auto permutations=next();
                for(unsigned p=0;p<permutations;++p){SourceInitialVariantPermutation permutation;
                    permutation.permutation=std::int8_t(integer());permutation.weight=std::bit_cast<float>(next());const auto states=next();
                    for(unsigned s=0;s<states;++s){SourceInitialModelState state;state.permutation=std::int8_t(integer());
                        state.properties=next();state.damage=next();state.looping_effect=next();state.looping_marker=next();
                        permutation.states.push_back(state);}
                    region.permutations.push_back(std::move(permutation));
                }
                variant.definitions.push_back(std::move(region));
            }
            definition.variants.push_back(std::move(variant));
            if(at!=words.size())throw FormatError("extra region oracle fixture data");
            std::vector<std::uint32_t> events;unsigned allocations=0;
            SourceModelRegionOwners owners{&random,[&](std::uint32_t handle){events.insert(events.end(),{0,handle,0,0});},
                [&](std::uint32_t tag,std::uint32_t marker){const auto handle=0x90010000u+allocations++;events.insert(events.end(),{1,tag,marker,handle});return handle;}};
            const bool changed=source_model_regions_change(definition,bank,request,owners);
            std::cout<<std::hex<<unsigned(changed)<<' '<<random<<' '<<regions;
            for(unsigned r=0;r<regions;++r){const auto &state=bank.selection.states[r];
                std::cout<<' '<<unsigned(bank.selection.permutations[r])<<' '<<unsigned(std::uint8_t(state.variant_permutation))
                    <<' '<<unsigned(state.damage)<<' '<<unsigned(state.properties)<<' '<<bank.looping_effects[r];}
            std::cout<<' '<<events.size()/4;for(auto value:events)std::cout<<' '<<value;std::cout<<'\n';
        }
        return 0;
    }
    SourceInitialModelSelection initial;initial.variant_index=0;initial.permutations={0};initial.states={{0,0,0}};
    auto bank=source_model_region_bank(initial);
    SourceInitialModelDefinition definition;definition.model=1;definition.regions.resize(1);
    SourceInitialModelVariant variant;variant.regions.fill(-1);variant.regions[0]=0;
    variant.definitions={{{{0,1,{{1,0,4,0,0xffffffffu,0}}}}}};definition.variants={variant};
    unsigned random=123;const SourceModelRegionOwners owners{.random=&random};
    if(!source_model_regions_change(definition,bank,{0,4},owners) || random!=123 || bank.selection.permutations[0]!=1 || bank.selection.states[0].damage!=4)
        throw FormatError("authored singleton destroyed transition changed RNG/choice");
    if(source_model_regions_change(definition,bank,{0,0},owners))throw FormatError("damage state regressed without force");
    if(!source_model_regions_change(definition,bank,{0,0,true},owners) || bank.selection.permutations[0]!=0)
        throw FormatError("forced original base transition failed");
    std::cout<<"source model region tests passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
