#include "halo2/source_bitmap_coverage.hpp"
#include "halo2/source_shader_lod.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace halo2;
int main(int argc,char **argv){try{
    const bool probe=argc==2 && std::string_view(argv[1])=="--probe";
    if(!probe && argc!=2)throw std::runtime_error("source bitmap coverage fixture required");
    std::ifstream file;if(!probe){file.open(argv[1]);if(!file)throw std::runtime_error("source bitmap coverage fixture missing");}
    auto &input=probe?std::cin:file;std::string line;unsigned cases=0,checked=0;
    while(std::getline(input,line)){
        if(line.empty() || line[0]=='#')continue;
        const auto separator=line.find('|');std::istringstream before(line.substr(0,separator));before>>std::hex;
        std::vector<std::uint32_t> values;std::uint32_t word;while(before>>word)values.push_back(word);
        if(!before.eof() || values.size()!=4 || values[0]>1 || (values[0] && values[1]>255))
            throw std::runtime_error("source bitmap coverage input differs");
        const float metric=values[0]?source_shader_part_metric(std::uint8_t(values[1]),
            std::bit_cast<float>(values[2]),std::bit_cast<float>(values[3])):std::bit_cast<float>(values[1]);
        const auto result=source_bitmap_coverage(metric);
        const std::vector<std::uint32_t> actual{std::bit_cast<std::uint32_t>(metric),
            std::bit_cast<std::uint32_t>(result.reciprocal),std::bit_cast<std::uint32_t>(result.adjustment)};
        if(probe){for(auto v:actual)std::cout<<std::hex<<v<<' ';std::cout<<'\n';continue;}
        if(separator==std::string::npos)throw std::runtime_error("source bitmap coverage fixture separator missing");
        std::istringstream after(line.substr(separator+1));after>>std::hex;std::vector<std::uint32_t> expected;
        while(after>>word)expected.push_back(word);
        if(!after.eof() || expected!=actual)throw std::runtime_error("source bitmap coverage differs at case"+std::to_string(cases));
        ++cases;checked+=3;
    }
    if(!probe){
        bool rejected=false;
        try{source_bitmap_coverage(std::bit_cast<float>(0x7f800000u));}
        catch(const std::invalid_argument&){rejected=true;}
        if(!rejected)throw std::runtime_error("unproved zero-reciprocal logarithm admitted");
        std::cout<<cases<<" original bitmap coverage cases /"<<checked<<" exact words; zero-log control rejected\n";
    }
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
