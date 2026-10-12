#include "halo2/source_object_light_uv.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
using namespace halo2;
namespace {
struct Reader {
    std::vector<std::uint32_t> values;std::size_t at{};
    explicit Reader(std::string_view line){std::istringstream in{std::string(line)};std::uint32_t v;while(in>>std::hex>>v)values.push_back(v);if(!in.eof())throw FormatError("invalid UV fixture");}
    std::uint32_t word(){if(at==values.size())throw FormatError("short UV fixture");return values[at++];}
    float real(){return std::bit_cast<float>(word());}
    std::vector<std::byte> raw(){const auto n=word();std::vector<std::byte> out;out.reserve(n);for(std::uint32_t i=0;i<(n+3)/4;++i){const auto w=word();for(unsigned b=0;b<4 && out.size()<n;++b)out.push_back(std::byte(w>>(8*b)));}return out;}
    void done(){if(at!=values.size())throw FormatError("extra UV fixture words");}
};
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
}
int main(){try{
    std::ifstream file(std::string(HALO2_SOURCE_DIR)+"/tests/fixtures/retail_source_object_light_uv.tsv");require(bool(file),"missing original UV fixture");std::unordered_map<std::uint32_t,SourceObjectLightDataGeometry> banks;std::size_t cases=0;
    for(std::string line;std::getline(file,line);){if(line.empty()||line[0]=='#')continue;Reader r(std::string_view(line).substr(2));
        if(line[0]=='G'){const auto id=r.word(),declaration=r.word(),offset=r.word();SourceObjectLightDataGeometry g;g.streams.resize(2);auto& uv=g.streams[1];uv.declaration=std::uint8_t(declaration);uv.descriptor_offset=offset;
            for(auto n=r.word();n;--n)g.indices.push_back(std::uint16_t(r.word()));uv.data=r.raw();banks.emplace(id,std::move(g));}
        else if(line[0]=='C'){const auto id=r.word(),triangle=r.word();const auto b=r.real(),c=r.real();const auto out=source_object_light_uv(banks.at(id),std::int32_t(triangle),b,c);
            require(std::bit_cast<std::uint32_t>(out.x)==r.word(),"original object UV X differs");require(std::bit_cast<std::uint32_t>(out.y)==r.word(),"original object UV Y differs");++cases;}
        else throw FormatError("unknown UV fixture row");r.done();
    }
    require(cases>0,"empty UV cases");std::cout<<cases<<" original CPU UV cases / "<<cases*2<<" exact words / "<<banks.size()<<" raw geometry banks\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
