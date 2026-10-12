#include "halo2/model_selection.hpp"
#include "halo2/model.hpp"
#include "halo2/source_geometry_lod.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>

using namespace halo2;
namespace {
void require(bool value,const char* message){if(!value)throw FormatError(message);}
std::filesystem::path map_path(const std::string& name){
    auto path=std::filesystem::path("assets/disc/maps")/(name+".map");
    if(!std::filesystem::exists(path))path=std::filesystem::path("assets/map-pack/maps")/(name+".map");
    return path;
}
std::uint64_t digest(const RenderModel& model,std::size_t first,std::size_t end){
    std::uint64_t hash=1469598103934665603ull;
    const auto word=[&](std::uint32_t v){hash^=v;hash*=1099511628211ull;};
    const auto vector=[&](Vec3 v){word(std::bit_cast<std::uint32_t>(v.x));word(std::bit_cast<std::uint32_t>(v.y));word(std::bit_cast<std::uint32_t>(v.z));};
    for(auto i=first;i<end;++i){const auto&v=model.mesh.vertices[i];
        word(model.vertex_origins[i].vertex);vector(v.position);vector(v.normal);vector(v.tangent);vector(v.binormal);
        word(std::bit_cast<std::uint32_t>(v.uv.x));word(std::bit_cast<std::uint32_t>(v.uv.y));word(v.material);
        for(unsigned j=0;j<4;++j){word(model.skin[i].bones[j]);word(std::bit_cast<std::uint32_t>(model.skin[i].weights[j]));}
    }
    return hash;
}
}
int main(int argc,char**argv){try{
    std::ifstream input("tests/fixtures/retail_geometry_lod_sections.tsv");
    require(bool(input),"missing original4C2B0 geometry LOD fixtures");
    std::unique_ptr<Cache> cache;std::string previous,name;unsigned records=0,vertices=0,lower_changes=0;
    std::map<std::pair<std::uint32_t,unsigned>,std::pair<std::size_t,std::uint64_t>> resident_streams;
    std::set<std::uint32_t> checked_empty;
    while(input>>name){
        std::uint32_t datum,variant;unsigned lod,regions,sections;SourceInitialModelSelection selected;
        input>>datum>>variant>>selected.variant_index>>lod>>regions;
        selected.permutations.resize(regions);selected.states.resize(regions);
        for(auto&p:selected.permutations){unsigned v;input>>v;p=std::uint8_t(v);}
        for(auto&s:selected.states){int v;unsigned damage,properties;input>>v>>damage>>properties;
            s={std::int8_t(v),std::uint8_t(damage),std::uint8_t(properties)};}
        input>>sections;std::vector<unsigned> expected(sections);for(auto&id:expected)input>>id;
        require(bool(input),"truncated original geometry LOD fixture");
        if(name!=previous){cache=std::make_unique<Cache>(map_path(name));previous=name;resident_streams.clear();checked_empty.clear();}
        const auto definition=cache->load_initial_model_definition(datum);
        const auto collector=source_initial_model_sections(definition,selected,lod);
        require(collector==expected,"native source geometry collector differs from original4C2B0");
        const auto model=cache->load_model(datum,false,variant,selected,lod);
        require(model.vertex_origins.size()==model.mesh.vertices.size()&&model.skin.size()==model.mesh.vertices.size(),
            "source LOD geometry loses source vertices or skin bindings");
        std::vector<unsigned> actual;std::size_t first=0;
        while(first<model.vertex_origins.size()){
            const auto section=model.vertex_origins[first].section;auto end=first+1;
            while(end<model.vertex_origins.size()&&model.vertex_origins[end].section==section)++end;
            actual.push_back(section);const auto stream=std::pair{end-first,digest(model,first,end)};
            const auto [old,inserted]=resident_streams.try_emplace({datum,section},stream);
            require(inserted||old->second==stream,"same authored geometry section changes vertices/material/skin across LODs");
            first=end;
        }
        require(actual==expected,"loaded source LOD sections fall back or reorder the original collector");
        for(const auto&v:model.mesh.vertices)require(v.material<model.mesh.materials.size(),"source LOD shader-table index escapes original material bank");
        vertices+=unsigned(model.mesh.vertices.size());++records;
        lower_changes+=lod<5&&expected!=source_initial_model_sections(definition,selected,5);
        if(checked_empty.insert(datum).second){
            auto absent=selected;std::fill(absent.permutations.begin(),absent.permutations.end(),255);
            const auto empty=cache->load_model(datum,false,variant,absent,lod);
            require(empty.mesh.vertices.empty()&&empty.vertex_origins.empty()&&empty.skin.empty(),"NONE source collector must return empty geometry");
            require(empty.bones.size()==model.bones.size()&&empty.checksum==model.checksum&&empty.animation==model.animation,
                "empty source collector loses the source hierarchy/checksum/animation");
            bool invalid=false;try{cache->load_model(datum,false,variant,selected,6);}catch(const FormatError&){invalid=true;}
            require(invalid,"source model loader must reject LOD outside0..5");
            auto unresolved=selected;unresolved.unresolved_rng_regions.push_back(0);invalid=false;
            try{cache->load_model(datum,false,variant,unresolved,lod);}catch(const FormatError&){invalid=true;}
            require(invalid,"source LOD loader must reject unresolved active permutation bytes");
        }
    }
    require(input.eof()&&records==5550,"invalid original geometry LOD collector fixture count");
    // Source4C2B0 writes one section byte per region. The corresponding
    // independent original synthetic duplicate/absent fixtures are retained.
    std::ifstream edges("tests/fixtures/retail_geometry_lod_edges.tsv");
    unsigned region_count,lod,edge_records=0;
    while(edges>>region_count>>lod){SourceInitialModelDefinition definition;SourceInitialModelSelection selection;
        for(unsigned r=0;r<region_count;++r){unsigned active;edges>>active;selection.permutations.push_back(std::uint8_t(active));
            selection.states.emplace_back();std::array<std::int16_t,6> bank{};for(auto&v:bank){int s;edges>>s;v=std::int16_t(s);}
            definition.render_sections.push_back({bank});}
        unsigned count;edges>>count;std::vector<unsigned> expected(count);for(auto&id:expected)edges>>id;
        require(bool(edges)&&source_initial_model_sections(definition,selection,lod)==expected,"source duplicate/absent region collector fixture differs");++edge_records;
    }
    require(edges.eof()&&edge_records==24,"invalid source duplicate/absent fixture count");
    std::ifstream banks("tests/fixtures/retail_geometry_lod_banks.tsv");
    cache.reset();previous.clear();unsigned bank_records=0;
    while(banks>>name){std::uint32_t datum;banks>>datum;if(name!=previous){cache=std::make_unique<Cache>(map_path(name));previous=name;}
        const auto definition=cache->load_source_geometry_lod_definition(datum);require(definition.model_present,"source placed model LOD bank must exist");
        for(auto v:definition.model_bank){std::uint32_t expected;banks>>expected;require(bool(banks)&&std::bit_cast<std::uint32_t>(v)==expected,"source literal hlmt LOD bank differs");}++bank_records;
    }
    require(banks.eof()&&bank_records==925,"invalid source literal LOD bank fixture count");
    if(argc>1&&std::string_view(argv[1])=="--counts")std::cout<<records<<' '<<vertices<<' '<<lower_changes<<'\n';
    else std::cout<<"5550 original resident LOD collectors,24 source region edges and925 literal model banks passed\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
