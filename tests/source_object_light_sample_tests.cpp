#include "halo2/source_object_light_sample.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
using namespace halo2;
namespace {
struct Reader {
    std::vector<std::uint32_t> values;std::size_t at{};
    explicit Reader(std::string_view line){std::istringstream in{std::string(line)};std::uint32_t v;while(in>>std::hex>>v)values.push_back(v);if(!in.eof())throw FormatError("invalid object-light fixture");}
    std::uint32_t word(){if(at==values.size())throw FormatError("short object-light fixture");return values[at++];}
    float real(){return std::bit_cast<float>(word());}
    Vec3 vector(){const auto x=real(),y=real(),z=real();return {x,y,z};}
    std::vector<std::byte> raw(){const auto n=word();std::vector<std::byte> out;out.reserve(n);for(std::uint32_t i=0;i<(n+3)/4;++i){const auto w=word();for(unsigned b=0;b<4 && out.size()<n;++b)out.push_back(std::byte(w>>(8*b)));}return out;}
    void done(){if(at!=values.size())throw FormatError("extra object-light fixture words");}
};
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
}
int main(){try{
    std::ifstream file(std::string(HALO2_SOURCE_DIR)+"/tests/fixtures/retail_source_object_light_sample.tsv");require(bool(file),"missing original object-light fixture");
    std::unordered_map<std::uint32_t,SourceObjectLightDataGeometry> geometries;
    std::unordered_map<std::uint32_t,SourceObjectLightPrtResource> resources;
    struct CacheBank {std::vector<SourcePrtContactCacheRecord> rows;std::vector<std::byte> arena;};
    std::unordered_map<std::uint32_t,CacheBank> cache_banks;
    std::size_t cases=0,callbacks=0,exact=0;
    for(std::string line;std::getline(file,line);){if(line.empty()||line[0]=='#')continue;Reader r(std::string_view(line).substr(2));
        if(line[0]=='A'){const auto id=r.word(),n=r.word();CacheBank bank;bank.rows.resize(n);for(auto& row:bank.rows)for(auto& w:row.words)w=r.word();bank.arena=r.raw();cache_banks.emplace(id,std::move(bank));}
        else if(line[0]=='G'){const auto id=r.word();SourceObjectLightDataGeometry g;for(auto n=r.word();n;--n)g.indices.push_back(std::uint16_t(r.word()));for(auto n=r.word();n;--n){SourceObjectLightDataStream s;s.declaration=std::uint8_t(r.word());s.descriptor_offset=r.word();s.data=r.raw();g.streams.push_back(std::move(s));}geometries.emplace(id,std::move(g));}
        else if(line[0]=='R'){const auto id=r.word();SourceObjectLightPrtResource resource;resource.declaration=std::uint8_t(r.word());resource.points=r.raw();resources.emplace(id,std::move(resource));}
        else if(line[0]=='C'){
            const auto cache_id=r.word(),gid=r.word(),rid=r.word();SourceObjectLightSampleContext context{};
            context.model_prt_count=std::bit_cast<std::int32_t>(r.word());context.current_prt_count=std::bit_cast<std::int32_t>(r.word());
            for(auto& w:context.contact)w=r.word();context.default_incident=r.vector();
            context.current_prt_cache=cache_banks.at(cache_id).rows;context.current_prt_arena=cache_banks.at(cache_id).arena;if(rid!=0xffffffffu)context.prt_resource=&resources.at(rid);
            const auto u=r.word(),v=r.word();const auto diffuse_success=r.word();const auto diffuse_rgb=r.vector();
            const auto status=r.word();std::array<std::uint32_t,5> expected;for(auto& w:expected)w=r.word();
            const auto sentinel=std::bit_cast<float>(0xcdcdcdcdu);ObjectLightmapSample output{{sentinel,sentinel,sentinel},0xcdcdcdcdu,0xcdcdcdcdu};
            unsigned called=0;const auto diffuse=[&](Vec2 uv)->std::optional<Vec3>{++called;
                if(std::bit_cast<std::uint32_t>(uv.x)!=u || std::bit_cast<std::uint32_t>(uv.y)!=v){std::cerr<<"case "<<std::dec<<cases<<" native UV "<<std::hex<<std::bit_cast<std::uint32_t>(uv.x)<<' '<<std::bit_cast<std::uint32_t>(uv.y)<<" source "<<u<<' '<<v<<'\n';throw FormatError("original D30D0 UV inputs differ");}
                return diffuse_success?std::optional<Vec3>(diffuse_rgb):std::nullopt;};
            const auto result=source_object_light_sample(geometries.at(gid),context,diffuse,output);
            const std::array<std::uint32_t,5> actual{std::bit_cast<std::uint32_t>(output.incident.x),std::bit_cast<std::uint32_t>(output.incident.y),std::bit_cast<std::uint32_t>(output.incident.z),output.diffuse_rgb,output.lightmap_rgb};
            if(std::uint32_t(result)!=status || actual!=expected){std::cerr<<"case "<<cases<<" status "<<std::uint32_t(result)<<" source "<<status<<'\n';for(unsigned i=0;i<5;++i)if(actual[i]!=expected[i])std::cerr<<i<<" native "<<std::hex<<actual[i]<<" source "<<expected[i]<<'\n';throw FormatError("complete original object-light branch differs");}
            require(called==(context.contact[5]!=0xffffffffu?1u:0u),"object-light bitmap service call count differs");callbacks+=called;exact+=6+called*2;++cases;
        }else throw FormatError("unknown object-light fixture row");r.done();
    }
    require(cases==3456 && callbacks==1728 && geometries.size()==6 && resources.size()==9 && cache_banks.size()==5,"independent object-light coverage changed");
    std::cout<<cases<<" complete original D1E10 object branches / "<<exact<<" exact status, sample and UV words / "<<callbacks<<" original bitmap service inputs\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
