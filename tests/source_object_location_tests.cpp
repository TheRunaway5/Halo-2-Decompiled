#include "halo2/source_object_location.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void require(bool v,const char *m){if(!v)throw FormatError(m);}
std::vector<std::uint32_t> words(const std::string &line){std::istringstream s(line);std::vector<std::uint32_t> out;std::uint32_t v;while(s>>std::hex>>v)out.push_back(v);return out;}
std::vector<std::uint32_t> probe(const std::vector<std::uint32_t> &input,const BspAtmosphere *authored=nullptr) {
    std::size_t at=0;auto take=[&](){require(at<input.size(),"truncated source object location fixture");return input[at++];};auto scalar=[&](){return std::bit_cast<float>(take());};
    BspAtmosphere bank;if(!authored){const auto nodes=take();for(unsigned i=0;i<nodes;++i)bank.nodes.push_back({take(),take(),take()});const auto planes=take();for(unsigned i=0;i<planes;++i)bank.planes.push_back({scalar(),scalar(),scalar(),scalar()});const auto leaves=take();for(unsigned i=0;i<leaves;++i)bank.leaves.push_back(std::int16_t(take()));}
    const auto &source=authored?*authored:bank;const auto bsp=std::int16_t(take());const auto count=take();std::vector<std::uint32_t> result;
    for(unsigned i=0;i<count;++i){const auto kind=take();const Vec3 center{scalar(),scalar(),scalar()};
        if(kind==1){const auto out=source_object_sphere_leaves(source,{center,scalar()});result.push_back(unsigned(out.size()));result.insert(result.end(),out.begin(),out.end());continue;}
        SourceObjectLocation out;
        if(kind==0)out=source_object_point_location(source,bsp,center);
        else if(kind==2){const float radius=scalar();const Vec3 root{scalar(),scalar(),scalar()};out=source_object_initial_location(source,bsp,{center,radius},root);}
        else {require(kind==3,"unknown source object location query");const Vec3 root{scalar(),scalar(),scalar()};out=source_object_default_registration_location(source,bsp,center,root);}
        result.insert(result.end(),{out.leaf,unsigned(out.cluster)&65535,unsigned(out.bsp)&65535});}
    require(at==input.size(),"trailing source object location fixture");return result;
}
void print(const auto &v){for(auto w:v)std::cout<<std::hex<<w<<' ';std::cout<<'\n';}
}
int main(int argc,char **argv){try{
    if(argc==2 && std::string(argv[1])=="--probe"){std::string line;while(std::getline(std::cin,line))if(!line.empty())print(probe(words(line)));return 0;}
    if(argc==3 && std::string(argv[1])=="--cache-probe"){const auto bank=Cache(argv[2]).load_bsp_atmosphere(0);std::string line;while(std::getline(std::cin,line))if(!line.empty())print(probe(words(line),&bank));return 0;}
    require(argc==2,"expected source object location fixture");std::ifstream file(argv[1]);require(bool(file),"source object location fixture unavailable");std::string line;unsigned count=0;std::filesystem::path previous;BspAtmosphere bank;
    while(std::getline(file,line)){if(line.empty()||line[0]=='#')continue;const auto split=line.find('|');require(split!=std::string::npos,"missing original location expected words");std::istringstream source(line.substr(0,split));std::string kind;source>>kind;const BspAtmosphere *authored=nullptr;
        if(kind=="asset"){std::filesystem::path path;source>>path;if(path!=previous){bank=Cache(path).load_bsp_atmosphere(0);previous=path;}authored=&bank;}else require(kind=="synthetic","unknown source location bank");std::string input;std::getline(source,input);const auto actual=probe(words(input),authored),expected=words(line.substr(split+1));
        if(actual!=expected){std::cerr<<"source object location fixture "<<count<<" differs\n";print(actual);throw FormatError("native registration location differs from original");}++count;}
    require(count!=0,"empty original location fixture");std::cout<<count<<" original registration location fixtures exact\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
