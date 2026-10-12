#include "halo2/source_object_light_mix.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
Words words(const std::string& line){Words out;std::istringstream in(line);std::uint32_t w;while(in>>std::hex>>w)out.push_back(w);return out;}
Words solve(const Words& row){
    if(row.size()!=17)throw FormatError("source object light mix fixture dimensions");
    const auto vec=[&](unsigned at){return Vec3{std::bit_cast<float>(row[at]),std::bit_cast<float>(row[at+1]),std::bit_cast<float>(row[at+2])};};
    std::optional<SourcePrtContactSample> prt,baked;
    if(row[0])prt=SourcePrtContactSample{vec(2),vec(5)};
    if(row[1])baked=SourcePrtContactSample{vec(8),vec(11)};
    const auto out=source_object_light_mix(prt,baked,vec(14));
    return {std::bit_cast<std::uint32_t>(out.incident.x),std::bit_cast<std::uint32_t>(out.incident.y),std::bit_cast<std::uint32_t>(out.incident.z),out.lightmap_rgb};
}
}
int main(){try{
    std::ifstream in(std::string(HALO2_SOURCE_DIR)+"/tests/fixtures/retail_source_object_light_mix.tsv");if(!in)throw std::runtime_error("source object light mix fixture missing");
    unsigned cases=0;std::size_t checked=0;std::string line;
    while(std::getline(in,line)){if(line.empty()||line[0]=='#')continue;const auto split=line.find('|');if(split==std::string::npos)throw FormatError("source object light mix separator");const auto expected=words(line.substr(split+1)),out=solve(words(line.substr(0,split)));if(out!=expected){std::cerr<<"case "<<cases<<'\n';for(unsigned i=0;i<out.size();++i)if(out[i]!=expected[i])std::cerr<<i<<" native "<<std::hex<<out[i]<<" source "<<expected[i]<<'\n';throw std::runtime_error("source object light mix differs");}checked+=out.size();++cases;}
    if(cases!=2128||checked!=8512)throw std::runtime_error("source object light mix independent coverage changed");
    std::cout<<cases<<" original object-light combinations, "<<checked<<" exact words passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
