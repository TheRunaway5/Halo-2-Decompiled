#include "halo2/source_object_spatial_scan.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
Words parse(std::string_view line){std::istringstream s{std::string(line)};Words w;std::uint32_t v;
    while(s>>std::hex>>v)w.push_back(v);if(!s.eof())throw FormatError("invalid source spatial scan fixture");return w;}
void print(const Words&w){for(std::size_t i=0;i<w.size();++i)std::cout<<(i?" ":"")<<std::hex<<w[i];std::cout<<'\n';}
struct Probe {
    std::unique_ptr<SourceObjectSpatialPool> pool;
    Words run(char kind,const Words&w){std::size_t at=0;
        auto take=[&](){if(at==w.size())throw FormatError("short source spatial scan fixture");return w[at++];};
        auto done=[&](){if(at!=w.size())throw FormatError("extra source spatial scan fixture words");};
        if(kind=='B') {
            const auto clusters=take(),count=take();pool=std::make_unique<SourceObjectSpatialPool>(clusters);
            if(count>2048)throw FormatError("source scan entry pool truncated");
            for(auto&head:pool->heads)head=take();
            for(unsigned i=0;i<count;++i)for(auto&v:pool->entries.records[i])v=take();done();
            Words out;
            for(std::int16_t c=0;c<std::int16_t(clusters);++c){
                const auto list=pool->cluster(c);out.push_back(std::uint32_t(list.size()));
                for(const auto&r:list){out.push_back(r.object);const auto p=std::bit_cast<std::array<std::uint32_t,5>>(r.primitive);out.insert(out.end(),p.begin(),p.end());}
            }
            return out;
        }
        if(kind!='S' || !pool)throw FormatError("source scan fixture bank absent");
        const auto query=take(),initial=take(),excluded_a=take(),excluded_b=take();
        auto vector=[&](){const float x=std::bit_cast<float>(take()),y=std::bit_cast<float>(take()),z=std::bit_cast<float>(take());return Vec3{x,y,z};};
        const auto origin=vector(),delta=vector();const auto count=take();
        std::vector<std::int16_t> leaves;for(unsigned i=0;i<count;++i)leaves.push_back(std::int16_t(take()));
        const auto success_cluster=std::int32_t(take());const auto success_object=take();done();
        Words calls;
        const bool hit=source_object_light_spatial_scan(*pool,leaves,query,origin,delta,excluded_a,excluded_b,initial!=0,
            [&](std::int16_t cluster){calls.push_back(0);calls.push_back(std::uint32_t(cluster));return cluster==success_cluster;},
            [&](const ObjectLightSpatialRecord&r){calls.push_back(1);calls.push_back(r.object);return r.object==success_object;});
        Words out{std::uint32_t(hit),std::uint32_t(calls.size()/2)};out.insert(out.end(),calls.begin(),calls.end());return out;
    }
};
}
int main(int argc,char**argv){try{
    Probe probe;if(argc==2 && std::string_view(argv[1])=="--probe"){
        std::string line;while(std::getline(std::cin,line))print(probe.run(line.at(0),parse(std::string_view(line).substr(2))));return 0;
    }
    if(argc!=2)throw FormatError("source spatial scan test requires original fixture");std::ifstream f(argv[1]);
    if(!f)throw FormatError("source spatial scan fixture missing");std::string line;unsigned banks=0,cases=0,words=0;
    while(std::getline(f,line)) {
        if(line.empty() || line[0]=='#')continue;const auto split=line.find('|');
        if(split==std::string::npos)throw FormatError("source scan fixture separator missing");
        const auto actual=probe.run(line[0],parse(std::string_view(line).substr(2,split-2))),expected=parse(std::string_view(line).substr(split+1));
        if(actual!=expected){std::size_t i=0;while(i<actual.size() && i<expected.size() && actual[i]==expected[i])++i;
            throw FormatError("source raw scan differs at case"+std::to_string(cases)+" word"+std::to_string(i));}
        banks+=line[0]=='B';cases+=line[0]=='S';words+=expected.size();
    }
    if(!f.eof() || banks<8 || cases<1024)throw FormatError("incomplete original raw spatial scan fixture");
    std::cout<<banks<<" original raw banks; "<<cases<<" untouched contact scans; "<<words<<" exact output words\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
