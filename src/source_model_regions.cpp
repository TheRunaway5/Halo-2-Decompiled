#include "halo2/source_model_regions.hpp"
#include <bit>
#include <utility>
#include <algorithm>

namespace halo2 {
SourceModelRegionBank source_model_region_bank(SourceInitialModelSelection selection){
    if(!selection.resolved() || selection.states.size()!=selection.permutations.size())
        throw FormatError("live model region bank requires resolved initial choices");
    SourceModelRegionBank result;result.looping_effects.assign(selection.states.size(),0xffffffffu);
    result.selection=std::move(selection);return result;
}
bool source_model_regions_change(const SourceInitialModelDefinition &definition,
    SourceModelRegionBank &bank,const SourceModelRegionRequest &request,const SourceModelRegionOwners &owners){
    auto &selection=bank.selection;
    if(!selection.resolved() || definition.regions.size()>16 ||
        selection.states.size()!=definition.regions.size() ||
        selection.permutations.size()!=definition.regions.size() ||
        bank.looping_effects.size()!=definition.regions.size() ||
        request.property_bit < -1 || request.property_bit>31)
        throw FormatError("invalid live model region bank/request");
    struct Choice {int permutation{},variant{},damage{};std::uint8_t properties{};
        std::uint32_t effect=0xffffffffu,marker{};float weight{};};
    bool changed=false;
    for(unsigned region=0;region<definition.regions.size();++region){
        auto &state=selection.states[region];
        const int previous=std::bit_cast<std::int8_t>(state.damage);
        const int wanted=request.damage==-1?previous:request.damage;
        std::uint32_t properties=state.properties;
        if(request.property_bit!=-1){const auto bit=1u<<request.property_bit;
            if(request.property_regions&(1u<<region))properties|=bit;else properties&=~bit;}
        if((wanted<previous && !request.force) || (request.region!=-1 && request.region!=int(region)))continue;
        std::vector<Choice> choices;
        if(definition.model!=0xffffffffu && selection.variant_index!=-1){
            const auto &variant=definition.variants.at(unsigned(selection.variant_index));
            const int mapped=variant.regions[region];
            if(mapped!=-1){const auto &permutations=variant.definitions.at(unsigned(mapped)).permutations;
                for(;;){
                    for(unsigned i=0;i<permutations.size();++i){
                        if(state.variant_permutation!=-1 && state.variant_permutation!=int(i))continue;
                        const auto &p=permutations[i];
                        Choice choice{p.permutation,int(i),wanted,std::uint8_t(properties),0xffffffffu,0,p.weight};
                        if(wanted || properties){
                            const SourceInitialModelState *match=nullptr;
                            for(const auto &s:p.states)if(s.damage==wanted && s.properties==properties){match=&s;break;}
                            if(!match)continue;
                            choice.permutation=match->permutation;choice.effect=match->looping_effect;choice.marker=match->looping_marker;
                        }
                        if(choices.size()<32)choices.push_back(choice);
                    }
                    if(!choices.empty() || !properties)break;
                    properties=0;
                }
            }
        }
        if(choices.empty())continue;
        unsigned selected=0;
        if(choices.size()>1){
            if(!owners.random)throw FormatError("model region weighted transition requires actual RNG owner");
            float total=0;for(const auto &choice:choices)total+=choice.weight;
            auto &random=*owners.random;random=random*0x19660du+0x3c6ef35fu;
            //16DD96..16DDB0 keeps both products in x87's 53-bit precision
            //until the final float store. Rounding the sample first changes
            //which choice wins at authored weight boundaries.
            const double sample=double(random>>16)*double(std::bit_cast<float>(0x37800080u));
            const float target=float(sample*double(total));float cumulative=0;
            for(;selected+1<choices.size();++selected){cumulative+=choices[selected].weight;if(cumulative>=target)break;}
        }
        const auto &choice=choices[selected];
        if(previous!=choice.damage){
            if(bank.looping_effects[region]!=0xffffffffu && !owners.stop_effect)
                throw FormatError("model region transition requires actual looping-effect retirement owner");
            if(choice.effect!=0xffffffffu && !owners.start_effect)
                throw FormatError("model region transition requires actual looping-effect allocation owner");
        }
        selection.permutations[region]=std::uint8_t(choice.permutation);
        state.variant_permutation=std::int8_t(choice.variant);
        if(previous!=choice.damage){
            if(bank.looping_effects[region]!=0xffffffffu)owners.stop_effect(bank.looping_effects[region]);
            // Original retains the stopped handle when no replacement is authored.
            if(choice.effect!=0xffffffffu)bank.looping_effects[region]=owners.start_effect(choice.effect,choice.marker);
        }
        state.damage=std::uint8_t(choice.damage);state.properties=choice.properties;changed=true;
    }
    return changed;
}
}
