#include "halo2/model_selection.hpp"
#include "halo2/source_geometry_lod.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
struct Choice {
    int permutation=-1;
    SourceInitialModelRegionState state;
    float weight{};
};
float random_sample(unsigned high){
    return float(high)*std::bit_cast<float>(0x37800080u); //retail45DC50.
}
float advance_random(std::uint32_t &rng){
    rng=rng*0x19660du+0x3c6ef35fu;return random_sample(unsigned(rng>>16));
}
Choice state_choice(const SourceInitialVariantPermutation&p,unsigned ordinal,float sample){
    Choice r{p.permutation,{std::int8_t(ordinal),0,0},p.weight};float cumulative=0;
    for(const auto&s:p.states){
        cumulative+=s.probability;
        if(sample<cumulative){r.permutation=s.permutation;r.state.damage=std::uint8_t(s.damage);
            r.state.properties=s.properties;break;}
    }return r;
}
std::vector<Choice> choices(const SourceInitialModelDefinition&d,int variant,unsigned region,
                           std::uint32_t &rng){
    std::vector<Choice> result;
    if(variant>=0){const auto&v=d.variants[unsigned(variant)];const int selected=v.regions[region];
        if(selected>=0){const auto&perms=v.definitions.at(unsigned(selected)).permutations;
            for(unsigned i=0;i<perms.size();++i){const auto&p=perms[i];
                const float sample=p.states.empty()?0.f:advance_random(rng);
                if(result.size()<32)result.push_back(state_choice(p,i,sample));}
        }
    }
    if(result.empty()){
        const auto&perms=d.regions[region].permutation_flags;
        for(unsigned i=0;i<perms.size();++i)if(!(perms[i]&1))result.push_back({int(i),{},1});
        if(result.empty()&&!perms.empty())result.push_back({0,{},1});
    }return result;
}
int weighted_choice(std::span<const Choice>choices,float sample){
    if(choices.empty())return -1;if(choices.size()==1)return 0;
    float total=0;for(const auto&c:choices)total+=c.weight;
    const float target=sample*total;float cumulative=0;
    for(unsigned i=0;i<choices.size();++i){cumulative+=choices[i].weight;
        if(!(cumulative<target)||i+1==choices.size())return int(i);}
    return -1;
}
void valid(const SourceInitialModelDefinition&d){
    if(d.regions.size()>16)throw FormatError("initial model region count exceeds source runtime slots");
}
}
std::int32_t source_initial_model_variant(const SourceInitialModelDefinition&d,std::uint32_t name){
    if(d.model==0xffffffffu)return -1;
    if(!name)return d.variants.empty()?-1:0;
    for(unsigned i=0;i<d.variants.size();++i)if(d.variants[i].name==name)return int(i);
    return -1;
}
SourceInitialModelSelection source_initial_model_selection(const SourceInitialModelDefinition&d,
    std::uint32_t variant,std::uint32_t&rng,std::uint32_t skipped){
    valid(d);SourceInitialModelSelection result;result.variant_index=source_initial_model_variant(d,variant);
    result.permutations.assign(d.regions.size(),255);result.states.resize(d.regions.size());
    for(unsigned i=0;i<d.regions.size();++i){const auto c=choices(d,result.variant_index,i,rng);
        if(skipped&(1u<<i))continue;
        const int selected=weighted_choice(c,c.size()>1?advance_random(rng):0.f);
        if(selected>=0){result.permutations[i]=std::uint8_t(c[unsigned(selected)].permutation);
            result.states[i]=c[unsigned(selected)].state;}
    }return result;
}
SourceInitialModelSelection source_deterministic_initial_model_selection(
    const SourceInitialModelDefinition&d,std::uint32_t variant){
    valid(d);SourceInitialModelSelection result;result.variant_index=source_initial_model_variant(d,variant);
    result.permutations.assign(d.regions.size(),255);result.states.resize(d.regions.size());
    for(unsigned region=0;region<d.regions.size();++region){
        std::uint32_t scratch=0;auto c=choices(d,result.variant_index,region,scratch);
        std::vector<std::vector<Choice>> possible(c.size());
        bool has_variant=false;
        if(result.variant_index>=0){const auto&v=d.variants[unsigned(result.variant_index)];const int selected=v.regions[region];
            if(selected>=0){const auto&perms=v.definitions.at(unsigned(selected)).permutations;
                has_variant=!perms.empty();
                for(unsigned i=0;i<std::min<std::size_t>(perms.size(),32);++i){
                    for(unsigned high=0;high<65536;++high){const auto choice=state_choice(perms[i],i,random_sample(high));
                        auto&set=possible[i];if(std::none_of(set.begin(),set.end(),[&](const auto&s){
                            return s.permutation==choice.permutation&&s.state==choice.state;}))set.push_back(choice);}
                }
            }
        }
        if(!has_variant)for(unsigned i=0;i<c.size();++i)possible[i].push_back(c[i]);
        std::vector<bool> admitted(c.size());
        for(unsigned high=0;high<65536;++high){const int chosen=weighted_choice(c,random_sample(high));
            if(chosen>=0)admitted[unsigned(chosen)]=true;}
        std::optional<Choice> fixed;bool unresolved=false;
        for(unsigned i=0;i<c.size();++i)if(admitted[i])for(const auto&choice:possible[i]){
            if(!fixed)fixed=choice;
            else if(fixed->permutation!=choice.permutation||fixed->state!=choice.state)unresolved=true;
        }
        if(unresolved)result.unresolved_rng_regions.push_back(region);
        else if(fixed){result.permutations[region]=std::uint8_t(fixed->permutation);result.states[region]=fixed->state;}
    }return result;
}
std::vector<unsigned> source_initial_model_sections(const SourceInitialModelDefinition&d,
    const SourceInitialModelSelection&selection,unsigned lod){
    if(lod>5||!selection.resolved()||selection.permutations.size()!=d.render_sections.size())
        throw FormatError("source initial model section selection requires resolved runtime regions");
    std::vector<unsigned> sections;
    for(unsigned i=0;i<d.render_sections.size();++i){const auto p=selection.permutations[i];if(p==255)continue;
        if(p>=d.render_sections[i].size())throw FormatError("source initial permutation outside render region");
        const auto section=d.render_sections[i][p][lod];if(section<0)continue;
        if(section>=255)throw FormatError("source selected section exceeds runtime byte representation");
        sections.push_back(unsigned(section));
    }return sections;
}
SourceInitialModelDefinition Cache::load_initial_model_definition(std::uint32_t datum)const{
    const Bytes b(storage_);const auto find=[&](std::uint32_t id)->const Tag&{
        const auto t=query_tag({.handle=TagHandle{id}, .require_address=true});
        if(!t)throw FormatError("initial model definition tag missing");return *t;};
    
    const auto byte=[&](std::size_t p){return std::to_integer<std::uint8_t>(b.slice(p,1)[0]);};
    const auto real=[&](std::size_t p){const auto v=b.f32(p);if(!std::isfinite(v))throw FormatError("initial model probability is nonfinite");return v;};
    SourceInitialModelDefinition result;const auto*tag=&find(datum);
    if(tag->group!="hlmt"){
        if(tag->group=="mode")throw FormatError("render model has no object initialization definition");
        const auto p=metadata_offset(tag->address,0x3c);result.default_variant=b.u32(p+0x30);
        result.model=b.u32(p+0x38);if(result.model==0xffffffffu)return result;tag=&find(result.model);
    }else result.model=datum;
    if(tag->group!="hlmt")throw FormatError("object initial model is not hlmt");
    const auto p=metadata_offset(tag->address,0x78);result.render_model=b.u32(p+4);
    for(const auto v:metadata_block_offsets(p+0x50, 0x38, 65536)){
        SourceInitialModelVariant variant;variant.name=b.u32(v);
        for(unsigned i=0;i<16;++i)variant.regions[i]=std::bit_cast<std::int8_t>(byte(v+4+i));
        for(const auto r:metadata_block_offsets(v+0x14, 0x14, 65536)){
            SourceInitialVariantRegion region;
            for(const auto q:metadata_block_offsets(r+8, 0x20, 65536)){
                SourceInitialVariantPermutation permutation;permutation.permutation=std::bit_cast<std::int8_t>(byte(q+4));permutation.weight=real(q+8);
                for(const auto s:metadata_block_offsets(q+0xc, 0x18, 65536))permutation.states.push_back({std::bit_cast<std::int8_t>(byte(s+4)),byte(s+5),b.u16(s+6),real(s+0x14)});
                region.permutations.push_back(std::move(permutation));
            }variant.definitions.push_back(std::move(region));
        }result.variants.push_back(std::move(variant));
    }
    for(const auto r:metadata_block_offsets(p+0x70, 0x10, 65536)){
        SourceInitialModelRegion region;region.name=b.u32(r);for(const auto q:metadata_block_offsets(r+8, 8, 65536))region.permutation_flags.push_back(byte(q+4));
        result.regions.push_back(std::move(region));
    }
    if(result.render_model!=0xffffffffu){const auto &render=find(result.render_model);const auto rp=metadata_offset(render.address,0x24);
        for(const auto r:metadata_block_offsets(rp+0x1c, 16, 65536)){
            std::vector<std::array<std::int16_t,6>> permutations;
            for(const auto q:metadata_block_offsets(r+8, 16, 65536)){std::array<std::int16_t,6> sections{};
                for(unsigned i=0;i<6;++i)sections[i]=std::bit_cast<std::int16_t>(b.u16(q+4+i*2));permutations.push_back(sections);}
            result.render_sections.push_back(std::move(permutations));
        }
    }valid(result);return result;
}
SourceInitialModelSelection Cache::load_initial_model_selection(std::uint32_t datum,std::uint32_t variant)const{
    const auto d=load_initial_model_definition(datum);return source_deterministic_initial_model_selection(d,variant?variant:d.default_variant);
}
SourceGeometryLodDefinition Cache::load_source_geometry_lod_definition(std::uint32_t datum)const{
    const auto find=[&](std::uint32_t id)->const Tag&{
        const auto tag=query_tag({.handle=TagHandle{id}, .require_address=true});
        if(!tag)throw FormatError("geometry LOD object/model definition missing");
        return *tag;
    };
    const Bytes data(storage_);const auto*tag=&find(datum);
    if(tag->group=="mode")throw FormatError("render model has no object geometry LOD definition");
    if(tag->group!="hlmt"){
        const auto root=metadata_offset(tag->address,0x3c);const auto model=data.u32(root+0x38);
        if(model==0xffffffffu)return {};
        tag=&find(model);
    }
    if(tag->group!="hlmt")throw FormatError("geometry LOD object does not reference hlmt");
    const auto root=metadata_offset(tag->address,0x4c);
    SourceGeometryLodDefinition result;result.model_present=true;
    //4BCC0 supplies hlmt+28 to4C450. Preserve every literal, including the
    //+48 zero guard; the source chooser decides whether the bank is usable.
    for(unsigned i=0;i<result.model_bank.size();++i)result.model_bank[i]=data.f32(root+0x28+i*4);
    return result;
}
}
