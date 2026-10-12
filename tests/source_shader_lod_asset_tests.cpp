#include "halo2/source_shader_lod.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>

int main(int argc,char **argv){try {
    if(argc!=2)throw std::runtime_error("source shader LOD asset fixture required");
    std::ifstream file(argv[1]);if(!file)throw std::runtime_error("source shader LOD asset fixture missing");
    std::unique_ptr<halo2::Cache> cache;std::string current,line;unsigned cases=0,thresholds=0;
    while(std::getline(file,line)) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream input(line);std::string map;std::uint32_t datum,bias,count;input>>map>>std::hex>>datum>>bias>>count;
        if(!input)throw std::runtime_error("source shader LOD asset input truncated");
        if(map!=current) {
            auto path=std::filesystem::path("assets/disc/maps")/(map+".map");
            if(!std::filesystem::exists(path))path=std::filesystem::path("assets/map-pack/maps")/(map+".map");
            cache=std::make_unique<halo2::Cache>(path);current=map;
        }
        const auto actual=cache->load_source_shader_lod_definition(datum);
        if(actual.bias!=bias || actual.thresholds.size()!=count)throw std::runtime_error("source shader LOD asset bank mismatch");
        for(auto value:actual.thresholds) {
            std::uint32_t expected;input>>expected;
            if(!input || std::bit_cast<std::uint32_t>(value)!=expected)throw std::runtime_error("source shader LOD authored threshold mismatch");
            ++thresholds;
        }
        std::string extra;if(input>>extra)throw std::runtime_error("source shader LOD asset trailing data");
        ++cases;
    }
    if(cases!=6450)throw std::runtime_error("source shader LOD all-map fixture coverage changed");
    std::cout<<cases<<" authored shader bias banks and "<<thresholds<<" raw thresholds matched across21 caches\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
