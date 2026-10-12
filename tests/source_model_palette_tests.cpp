#include "halo2/source_model_palette.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

namespace {
using namespace halo2;
using Words=std::vector<std::uint32_t>;
void require(bool value,const char *message){if(!value)throw FormatError(message);}
Words evaluate(const Words &row) {
    unsigned at=0;const auto word=[&]{return row.at(at++);};
    SourceModelPaletteDefinition definition;definition.model_flags=std::uint16_t(word());
    const auto lod=word(),compound=word(),nodes=word();definition.region_count=word();
    require(lod<6 && nodes<=256 && definition.region_count<=16,"palette fixture exceeds source banks");
    definition.compound_node_counts[lod]=compound;std::vector<Transform> world;
    const auto transform=[&]{std::array<float,13> data;for(auto &f:data)f=std::bit_cast<float>(word());return std::bit_cast<Transform>(data);};
    for(unsigned i=0;i<nodes;++i){world.push_back(transform());definition.inverse_binds.push_back(transform());}
    std::vector<SourceModelPaletteRegion> regions;
    for(unsigned i=0;i<definition.region_count;++i) {
        SourceModelPaletteRegion region;region.selected_section=std::uint8_t(word());region.no_map_node_count=std::uint8_t(word());
        const auto count=word();require(count<=256,"palette fixture node map exceeds bank");
        for(unsigned j=0;j<count;++j)region.node_map.push_back(std::uint8_t(word()));regions.push_back(std::move(region));
    }
    require(at==row.size(),"palette fixture has trailing input");
    const auto palette=source_model_palette(definition,lod,world,regions);
    Words out{palette.base_count,unsigned(palette.regions.size()),unsigned(palette.matrices.size())};
    for(const auto&r:palette.regions){out.push_back(r.first);out.push_back(r.count);}
    for(const auto&m:palette.matrices)for(const auto f:m)out.push_back(std::bit_cast<std::uint32_t>(f));
    //1376C0's complete base bank is independently checked even when137800
    //later collapses it to one matrix for mode-mask4.
    for(const auto&m:source_model_skin_palette(world,definition.inverse_binds))
        for(const auto f:m)out.push_back(std::bit_cast<std::uint32_t>(f));
    for(unsigned i=0;i<regions.size();++i)if(regions[i].selected_section!=255)
        for(const auto rigid:std::array<std::int16_t,3>{-1,0,std::int16_t(nodes-1)}) {
            const auto center=source_model_palette_part_center(palette,i,rigid,definition.model_flags);
            for(auto v:std::bit_cast<std::array<std::uint32_t,3>>(center))out.push_back(v);
        }
    return out;
}
void controls() {
    SourceModelPaletteDefinition definition;definition.model_flags=4;definition.region_count=3;
    definition.inverse_binds.resize(2);std::vector<Transform> world(2);world[0].position={1,2,3};world[1].position={9,8,7};
    std::vector<SourceModelPaletteRegion> regions{{255,0,{}},{2,0,{1,0}},{2,0,{1,0}}};
    auto bank=source_model_palette(definition,4,world,regions);
    require(bank.base_count==1 && bank.matrices.size()==5 && bank.regions[0].count==0 &&
        bank.regions[1].first==1 && bank.regions[2].first==3,"compact palette dropped NONE/duplicate region identity");
    require(source_model_palette_part_center(bank,2,1,4).x==9 && source_model_palette_part_center(bank,2,-1,4).x==1,
        "transparent compact node center used a rigid ordinal or object bounds");
    definition.compound_node_counts[4]=1;bool rejected=false;
    try{(void)source_model_palette(definition,4,world,regions);}catch(const FormatError&){rejected=true;}
    require(rejected,"unported compound palette silently admitted");definition.compound_node_counts[4]=0;
    regions[1].node_map={3};rejected=false;
    try{(void)source_model_palette(definition,4,world,regions);}catch(const FormatError&){rejected=true;}
    require(rejected,"resident map escaped actual node bank");
}
void assets(const std::filesystem::path &path) {
    std::ifstream input(path);require(bool(input),"literal palette asset fixture missing");
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR);
    std::map<std::string,std::unique_ptr<Cache>> maps;
    std::string line;unsigned models=0,sections=0,words=0;
    while(std::getline(input,line)) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream row(line);char kind;std::string name;unsigned datum;row>>kind>>name>>std::hex>>datum;
        require(bool(row),"literal palette identity missing");
        if(!maps.contains(name)) {
            auto file=root/"assets/disc/maps"/(name+".map");if(!std::filesystem::exists(file))file=root/"assets/map-pack/maps"/(name+".map");
            maps.emplace(name,std::make_unique<Cache>(file));
        }
        const auto &cache=*maps.at(name);
        if(kind=='D') {
            const auto actual=cache.load_source_model_palette_definition(datum);
            unsigned flags,regions,n;row>>flags>>regions>>n;
            require(bool(row) && actual.model_flags==flags && actual.region_count==regions && actual.inverse_binds.size()==n,
                "native palette definition differs from literal model fields");
            for(auto v:actual.compound_node_counts){unsigned expected;row>>expected;require(bool(row) && v==expected,"native palette LOD compound count differs");}
            for(const auto&transform:actual.inverse_binds)for(auto v:std::bit_cast<std::array<std::uint32_t,13>>(transform)) {
                unsigned expected;row>>expected;require(bool(row) && v==expected,"native palette inverse-bind word differs from original");++words;
            }
            ++models;
        } else if(kind=='S') {
            unsigned section,fallback,count;row>>section>>fallback>>count;
            const auto actual=cache.load_source_transparent_model_parts(datum,std::span<const unsigned>(&section,1));
            require(actual.sections.size()==1 && actual.sections[0].section==section &&
                actual.sections[0].no_map_node_count==fallback && actual.sections[0].resident_node_map.size()==count,
                "native palette section differs from literal resident map/fallback fields");
            for(auto v:actual.sections[0].resident_node_map) {unsigned expected;row>>expected;
                require(bool(row) && v==expected,"native resident node-map byte differs from original");++words;}
            ++sections;
        } else throw FormatError("unknown literal palette asset record");
        require(bool(row),"literal palette asset truncated");std::string extra;require(!(row>>extra),"literal palette asset has trailing fields");
    }
    require(maps.size()==21 && models==193 && sections==328,"literal palette all-map coverage changed");
    std::cout<<maps.size()<<" maps / "<<models<<" models / "<<sections<<" resident sections / "<<words<<" literal node-bank words matched\n";
}
}
int main(int argc,char **argv){try {
    if(argc==3 && std::string(argv[1])=="--assets"){assets(argv[2]);return 0;}
    require(argc==2,"source palette fixture required");const bool probe=std::string(argv[1])=="--probe";
    std::ifstream file;if(!probe){file.open(argv[1]);require(bool(file),"source palette fixture missing");}
    auto&stream=probe?std::cin:file;std::string line;unsigned cases=0,words=0;
    while(std::getline(stream,line)) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream input(line);input>>std::hex;unsigned count;require(bool(input>>count),"palette input count missing");
        Words row(count);for(auto&v:row)require(bool(input>>v),"palette input truncated");const auto actual=evaluate(row);
        if(probe){for(auto v:actual)std::cout<<std::hex<<v<<' ';std::cout<<'\n';}
        else {
            require(bool(input>>count) && count==actual.size(),"palette output count differs");
            for(auto v:actual){unsigned expected;require(bool(input>>expected),"palette output truncated");
                if(v!=expected)throw FormatError("source compact palette mismatch case "+std::to_string(cases));}
            std::string extra;require(!(input>>extra),"palette fixture trailing output");
        }
        ++cases;words+=unsigned(actual.size());
    }
    require(cases!=0,"source palette fixture empty");
    if(!probe){controls();std::cout<<cases<<" original model palette cases / "<<words<<" words matched; strict producer guards passed\n";}
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
