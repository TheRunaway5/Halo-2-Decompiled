#include "halo2/source_portal_clip.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
std::vector<std::uint32_t> words(const std::string &line) {
    std::istringstream stream(line);std::vector<std::uint32_t> out;std::uint32_t word;
    while(stream>>std::hex>>word)out.push_back(word);return out;
}
std::vector<std::uint32_t> probe(const std::vector<std::uint32_t> &input) {
    std::size_t at=0;
    auto take=[&](){require(at<input.size(),"truncated portal probe");return input[at++];};
    auto scalar=[&](){return std::bit_cast<float>(take());};
    const auto mode=take();ShaderVector plane;for(auto &value:plane)value=scalar();
    Vec3 eye;bool positive=true;float epsilon=0.f;unsigned capacity=0;int clipped=0;
    if(mode==0){positive=take()!=0;epsilon=scalar();capacity=take();}
    else {eye={scalar(),scalar(),scalar()};if(mode==1)clipped=int(take());}
    const auto count=take();std::vector<Vec3> polygon;
    for(unsigned i=0;i<count;++i)polygon.push_back({scalar(),scalar(),scalar()});
    require(at==input.size(),"trailing portal probe words");std::vector<std::uint32_t> result;
    if(mode==0){const auto output=source_clip_portal_plane(plane,polygon,positive,epsilon,capacity);
        result.push_back(output.size());for(const auto p:output)
            for(float value:{p.x,p.y,p.z})result.push_back(std::bit_cast<std::uint32_t>(value));}
    else if(mode==1){const auto facing=source_portal_facing(plane,eye,polygon,clipped);
        result={facing.classification,facing.projected_eye_inside};}
    else if(mode==2)result={source_portal_projected_eye_inside(plane,eye,polygon)};
    else throw FormatError("unknown source portal probe");
    return result;
}
}
int main(int argc,char **argv){try {
    if(argc==2 && std::string_view(argv[1])=="--probe") {
        for(std::string line;std::getline(std::cin,line);)if(!line.empty()) {
            for(auto word:probe(words(line)))std::cout<<std::hex<<word<<' ';std::cout<<'\n';}
        return 0;
    }
    require(argc==2,"source portal fixture required");std::ifstream file(argv[1]);require(bool(file),"source portal fixture missing");
    std::array<unsigned,3> cases{};
    for(std::string line;std::getline(file,line);){if(line.empty()||line[0]=='#')continue;
        const auto split=line.find('|');require(split!=std::string::npos,"malformed source portal fixture");
        const auto input=words(line.substr(0,split));require(!input.empty()&&input[0]<3,"invalid portal fixture mode");
        if(probe(input)!=words(line.substr(split+1)))throw FormatError("original portal clipping differs at case "+std::to_string(cases[0]+cases[1]+cases[2]));
        ++cases[input[0]];
    }
    require(cases[0]>=256&&cases[1]>=256&&cases[2]>=128,"bounded portal fixture coverage incomplete");
    bool rejected=false;try{source_clip_portal_plane({},std::vector<Vec3>(65));}catch(const FormatError&){rejected=true;}
    require(rejected,"portal scratch capacity must remain explicit");
    std::cout<<cases[0]<<" plane clips, "<<cases[1]<<" portal facing and "<<cases[2]<<" projected-eye cases match original words\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
