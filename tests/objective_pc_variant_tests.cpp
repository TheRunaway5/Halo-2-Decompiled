#include "halo2/vista_pcr_publication.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace halo2;
int main(int argc,char** argv)try {
    if(argc!=3)throw std::runtime_error("Supply original PC defaults and Xbox preset references");
    nlohmann::json pc,xbox;std::ifstream(argv[1])>>pc;std::ifstream(argv[2])>>xbox;
    unsigned defaults{},presets{};
    for(const auto& row:pc.at("defaults")) {
        const auto source=source_game_variant_default(row.at("engine"));const auto raw=row.at("raw").get<std::string>();
        if(raw.size()!=source.bytes.size()*2)throw std::runtime_error("Incomplete PC default");
        for(unsigned i=0;i<source.bytes.size();++i)if(source.bytes[i]!=std::stoul(raw.substr(i*2,2),nullptr,16))
            throw std::runtime_error("Original PC/Xbox CTF/Assault defaults differ");
        ++defaults;
    }
    for(const auto& row:xbox.at("variants")) {
        const auto type=row.at("source_type").get<unsigned>();if(type!=1 && type!=9)continue;
        const auto raw=row.at("raw").get<std::string>();SourceGameVariant source;
        for(unsigned i=0;i<source.bytes.size();++i)source.bytes[i]=std::uint8_t(std::stoul(raw.substr(i*2,2),nullptr,16));
        const auto projected=vista_pcr_objective_variant(source);
        if(vista_pcr_multiplayer_variant(source).bytes!=projected.bytes)
            throw std::runtime_error("Network PCR changed the original objective option union");
        if(!std::equal(source.bytes.begin(),source.bytes.end(),projected.bytes.begin(),
            [](auto a,auto b){return a==std::to_integer<unsigned>(b);}))throw std::runtime_error("PCR changed an original option byte");
        VistaPcrPublicationInput input;input.variant=projected;SourceResultStatistics stats;input.statistics=&stats;
        input.engine_present=true;input.teams[0]={1,2,0};input.teams[1]={1,1,1};input.recording={1,0};
        const auto publication=vista_pcr_publish_consumed_snapshot(input);
        if(publication.source[0x64]!=std::byte(type) || publication.source[0x13]!=std::byte{1} ||
            publication.source[0xcb0]!=std::byte{1} || publication.source[0xcb0+0x18]!=std::byte{1})
            throw std::runtime_error("PCR lost the selected objective family or team rows");
        ++presets;
    }
    if(defaults!=2 || presets!=9)throw std::runtime_error("Objective PC bridge reference cohort incomplete");
    std::cout<<"PASS pinned PC CTF/Assault defaults and all nine original Xbox preset/PCR buffers\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
