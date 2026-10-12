#include "halo2/source_object_light_streams.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

using namespace halo2;
namespace {
struct Reader {
    std::vector<std::uint32_t> values;std::size_t at{};
    explicit Reader(std::string_view line) {
        std::istringstream in{std::string(line)};std::uint32_t value;
        while(in>>std::hex>>value)values.push_back(value);
        if(!in.eof())throw FormatError("invalid object light stream fixture word");
    }
    std::uint32_t take() {
        if(at==values.size())throw FormatError("short object light stream fixture");
        return values[at++];
    }
    float scalar(){return std::bit_cast<float>(take());}
    std::vector<std::byte> bytes() {
        const auto count=take();std::vector<std::byte> out;out.reserve(count);
        for(std::size_t i=0;i<(std::uint64_t(count)+3)/4;++i) {
            const auto value=take();
            for(unsigned b=0;b<4 && out.size()<count;++b)out.push_back(std::byte(value>>(b*8)));
        }
        return out;
    }
    void done() const {if(at!=values.size())throw FormatError("extra object light stream fixture words");}
};
std::array<std::uint32_t,3> words(Vec3 p) {
    return {std::bit_cast<std::uint32_t>(p.x),std::bit_cast<std::uint32_t>(p.y),std::bit_cast<std::uint32_t>(p.z)};
}
void guards() {
    SourceObjectLightDataGeometry g;g.streams.resize(3);
    g.streams[0].declaration=48;g.streams[1].declaration=46;g.streams[2].declaration=47;
    //Source rejects count<=3 before touching triangle/index/data services.
    if(source_object_light_radiance(g,-1,0,0) || source_object_light_incident(g,-1,0,0))
        throw FormatError("source initial-three descriptors were sampled");
    g.streams.resize(4);g.streams[3].declaration=48;
    const auto reject=[](const auto& operation) {
        try{operation();}catch(const FormatError&){return;}
        throw FormatError("incomplete resident light stream bank silently admitted");
    };
    reject([&]{source_object_light_radiance(g,-1,0,0);});
    reject([&]{source_object_light_incident(g,0,0,0);});
    g.indices={0,1,2};
    reject([&]{source_object_light_radiance(g,0,0,0);});
    reject([&]{source_object_light_incident(g,0,0,0);});
    g.streams[3].data.resize(8);
    reject([&]{source_object_light_radiance(g,0,0,0);});
    reject([&]{source_object_light_incident(g,0,0,0);});
}
}
int main(int argc,char** argv) {try {
    if(argc!=2)throw FormatError("object light stream test requires unchanged source fixture");
    std::ifstream file(argv[1]);if(!file)throw FormatError("object light stream fixture missing");
    std::unordered_map<std::uint32_t,SourceObjectLightDataGeometry> geometries;
    std::size_t cases=0,radiance_success=0,incident_success=0,component_words=0;
    std::size_t no_match_radiance=0,no_match_incident=0,nonzero_offsets=0;
    std::string line;
    while(std::getline(file,line)) {
        if(line.empty() || line[0]=='#')continue;
        Reader r{std::string_view(line).substr(2)};
        if(line[0]=='G') {
            const auto id=r.take();SourceObjectLightDataGeometry geometry;
            for(auto n=r.take();n;--n)geometry.indices.push_back(std::uint16_t(r.take()));
            for(auto n=r.take();n;--n) {
                SourceObjectLightDataStream stream;stream.declaration=std::uint8_t(r.take());
                stream.descriptor_offset=r.take();stream.data=r.bytes();
                if(stream.descriptor_offset)++nonzero_offsets;
                geometry.streams.push_back(std::move(stream));
            }
            r.done();if(!geometries.emplace(id,std::move(geometry)).second)
                throw FormatError("duplicate object light geometry fixture");
        } else if(line[0]=='C') {
            const auto id=r.take();const auto triangle=std::int32_t(r.take());
            const auto u=r.scalar(),v=r.scalar();const auto status=r.take();
            const std::array radiance_expected{r.take(),r.take(),r.take()};
            const std::array incident_expected{r.take(),r.take(),r.take()};r.done();
            const auto& geometry=geometries.at(id);
            const auto radiance=source_object_light_radiance(geometry,triangle,u,v);
            const auto incident=source_object_light_incident(geometry,triangle,u,v);
            if(bool(radiance)!=bool(status&1) || bool(incident)!=bool(status&2))
                throw FormatError("original stream-service result differs at case"+std::to_string(cases));
            if(radiance) {
                if(words(*radiance)!=radiance_expected)
                    throw FormatError("originalD33A0 radiance words differ at case"+std::to_string(cases));
                ++radiance_success;component_words+=3;
            }
            if(incident) {
                if(words(*incident)!=incident_expected)
                    throw FormatError("originalD31C0 incident words differ at case"+std::to_string(cases));
                ++incident_success;component_words+=3;
            }
            if(geometry.streams.size()>3) {
                bool color=false,normal=false;
                for(std::size_t i=3;i<geometry.streams.size();++i) {
                    color|=geometry.streams[i].declaration==46 || geometry.streams[i].declaration==47;
                    normal|=geometry.streams[i].declaration==48;
                }
                no_match_radiance+=!color;no_match_incident+=!normal;
            }
            ++cases;
        } else throw FormatError("unknown object light stream fixture record");
    }
    guards();
    if(geometries.size()!=252 || cases!=17060 || radiance_success!=16668 ||
       incident_success!=16668 || component_words!=100008 ||
       !no_match_radiance || !no_match_incident || !nonzero_offsets)
        throw FormatError("original stream-service coverage changed");
    std::cout<<"originalD33A0/D31C0: "<<geometries.size()<<" explicit resident banks, "<<cases
        <<" cases, "<<component_words<<" exact RGB/incident float words and "<<cases*2
        <<" success bytes; "<<no_match_radiance<<'/'<<no_match_incident
        <<" original fallback controls, "<<nonzero_offsets<<" ignored-offset descriptors; guards pass\n";
    return 0;
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
