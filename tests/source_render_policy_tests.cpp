#include "halo2/source_render_policy.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc,char **argv) {try {
    const bool probe=argc==2 && std::string_view(argv[1])=="--probe";
    std::ifstream file;
    if(!probe) {
        if(argc!=2)throw halo2::FormatError("source render policy fixture required");
        file.open(argv[1]);if(!file)throw halo2::FormatError("source render policy fixture missing");
    }
    auto &input=probe?std::cin:file;unsigned cases=0;
    for(std::string line;std::getline(input,line);) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream row(line);row>>std::hex;
        unsigned layer;halo2::SourceGeometryLodPolicy policy;
        if(!(row>>layer))throw halo2::FormatError("source render layer missing");
        for(auto &byte:policy.feature_bytes) {
            unsigned value;if(!(row>>value) || value>255)throw halo2::FormatError("source feature byte missing");
            byte=std::uint8_t(value);
        }
        const auto actual=halo2::source_render_layer_enabled(policy,layer);
        if(probe)std::cout<<unsigned(actual)<<'\n';
        else {unsigned expected;if(!(row>>expected) || actual!=bool(expected))
            throw halo2::FormatError("original source layer admission differs at "+std::to_string(cases));}
        std::string extra;if(row>>extra)throw halo2::FormatError("source layer fixture trailing data");++cases;
    }
    if(!cases)throw halo2::FormatError("source layer fixture empty");
    if(!probe)std::cout<<cases<<" original render-layer feature cases matched\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
