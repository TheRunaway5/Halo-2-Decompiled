#include "halo2/source_model_lighting.hpp"
#include "halo2/object_lighting.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
namespace {
void require(bool value,const char* why){if(!value)throw FormatError(why);}
struct Reader {
    std::span<const std::uint32_t> words;unsigned cursor{};
    std::uint32_t word(){require(cursor<words.size(),"model lighting fixture truncated");return words[cursor++];}
    void done(){require(cursor==words.size(),"model lighting fixture trailing words");}
};
std::vector<std::uint32_t> solve(std::span<const std::uint32_t> input,unsigned& boundaries,bool typed,unsigned& typed_checked) {
    Reader r{input};const auto owner=r.word(),metric=r.word();SourceModelLightingWorld world;auto& m=world.memory;
    m.banks.tag_table=r.word();m.banks.objects=r.word();m.render_cache_pool=r.word();m.render_frame=r.word();m.view=std::bit_cast<std::int32_t>(r.word());
    world.default_lighting=r.word();world.bsp=r.word();world.fallback_lighting=r.word();world.scratch=r.word();world.interpolation=r.word()!=0;
    std::vector<std::array<std::uint32_t,2>> owned(r.word());for(auto& row:owned)for(auto& v:row)v=r.word();
    std::map<std::uint32_t,std::vector<std::byte>> memory;const auto segments=r.word();
    for(unsigned i=0;i<segments;++i){const auto address=r.word(),count=r.word();auto& raw=memory[address];raw.resize(std::size_t(count)*4);for(unsigned n=0;n<count;++n){const auto v=r.word();std::memcpy(raw.data()+n*4,&v,4);}}
    const auto view=[&](std::uint32_t address,std::size_t extent)->std::span<std::byte> {
        auto it=memory.upper_bound(address);require(it!=memory.begin(),"model lighting bank absent");--it;
        const auto offset=std::size_t(address-it->first);
        if(offset>it->second.size() || extent>it->second.size()-offset)throw FormatError("model lighting bank missing at"+std::to_string(address));
        return std::span(it->second).subspan(offset,extent);
    };
    const auto word=[&](std::uint32_t at){std::uint32_t value;const auto raw=view(at,4);std::memcpy(&value,raw.data(),4);return value;};
    const auto put=[&](std::uint32_t at,std::uint32_t value){auto raw=view(at,4);std::memcpy(raw.data(),&value,4);};
    m.banks.read=[&](std::uint32_t at,std::size_t n)->std::span<const std::byte>{return view(at,n);};m.write=view;
    std::vector<std::vector<std::uint32_t>> events(r.word());for(auto& row:events){row.resize(r.word());for(auto& v:row)v=r.word();}r.done();unsigned used=0;
    std::optional<ObjectLightingCache> typed_cache;std::uint32_t typed_record{},typed_kind{};Vec3 origin;
    const auto vector=[&](std::uint32_t at){return Vec3{std::bit_cast<float>(word(at)),std::bit_cast<float>(word(at+4)),std::bit_cast<float>(word(at+8))};};
    if(typed && world.interpolation) {
        const auto b=word(word(m.banks.objects+0x44)+(owner&65535u)*12+8);
        const auto token=word(b+0xcc);
        bool zero_velocity=false,query=false,incident=false;
        for(const auto& row:events) {
            if(row[0]==1)query=true;
            if(row[0]==4)incident=true;
            if(row[0]==3 && row.size()==5 && row[2]==0 && row[3]==0 && row[4]==0)zero_velocity=true;
        }
        if(zero_velocity && !query && !incident && token!=0xffffffffu && word(b+0x14)==0xffffffffu) {
            typed_record=word(m.render_cache_pool+0x44)+(token&65535u)*256;
            typed_kind=word(word(m.banks.tag_table+(word(b)&65535u)*16+8))&65535u;
            ObjectLightingCache c;c.current={vector(typed_record+0x1c),word(typed_record+0x28),word(typed_record+0x2c)};
            c.target={vector(typed_record+0x30),word(typed_record+0x3c),word(typed_record+0x40)};
            c.sample_valid=(word(typed_record)&0xff0000u)!=0;c.lighting_valid=(word(typed_record)&0xff000000u)!=0;
            c.last_query_frame=std::bit_cast<std::int32_t>(word(typed_record+8));c.last_rendered_frame=std::bit_cast<std::int32_t>(word(typed_record+12));
            c.query_origin=vector(typed_record+0x10);origin=vector(b+0x30);typed_cache=c;
        }
    }
    const auto event=[&](std::initializer_list<std::uint32_t> values)->Reader {
        require(used<events.size(),"model lighting extra owner event");Reader row{events[used++]};
        for(auto v:values){const auto expected=row.word();if(expected!=v)throw FormatError("model lighting event"+std::to_string(used-1)+" field"+std::to_string(row.cursor-1)+" got"+std::to_string(v)+" expected"+std::to_string(expected));}return row;
    };
    const auto check=[&](Reader& row,std::uint32_t at,unsigned count,const char* why){for(unsigned i=0;i<count;++i)if(row.word()!=word(at+i*4))throw FormatError(why);};
    const auto output=[&](Reader& row,std::uint32_t at,unsigned count){for(unsigned i=0;i<count;++i)put(at+i*4,row.word());};
    m.cache_acquired=[&](std::uint32_t owner,std::uint32_t token){auto row=event({0,owner,token});row.done();};
    world.query=[&](std::uint32_t owner,bool initial,std::uint32_t at){auto row=event({1,owner,std::uint32_t(initial)});check(row,at,5,"model lighting query initial sample differs");const auto result=row.word();output(row,at,5);row.done();return result!=0;};
    world.convert=[&](std::uint32_t sample,std::int32_t kind,bool flag,std::uint32_t out){auto row=event({2,std::uint32_t(kind),std::uint32_t(flag)});check(row,sample,5,"model lighting conversion sample differs");check(row,out,21,"model lighting conversion initial output differs");output(row,out,21);row.done();};
    world.velocity=[&](std::uint32_t owner,std::uint32_t out){auto row=event({3,owner});output(row,out,3);row.done();};
    world.incident=[&](std::uint32_t target,std::uint32_t current,float step){auto row=event({4,std::bit_cast<std::uint32_t>(step)});check(row,target,3,"model lighting incident target differs");check(row,current,3,"model lighting incident current differs");output(row,current,3);row.done();};
    const auto result=source_model_lighting(owner,std::bit_cast<float>(metric),world);
    require(used==events.size(),"model lighting omitted owner event");boundaries+=used;
    if(typed_cache) {
        static const auto definition=Cache(std::filesystem::path(HALO2_SOURCE_DIR)/"assets/disc/maps/lockout.map").load_object_lighting_definition();
        update_object_lighting_cache(*typed_cache,definition,typed_kind,std::bit_cast<std::int32_t>(m.render_frame),origin,{},false,{},true,false);
        const auto& sample=typed_cache->current;
        const std::array<std::uint32_t,5> current{std::bit_cast<std::uint32_t>(sample.incident.x),std::bit_cast<std::uint32_t>(sample.incident.y),std::bit_cast<std::uint32_t>(sample.incident.z),sample.diffuse_rgb,sample.lightmap_rgb};
        for(unsigned i=0;i<current.size();++i)require(current[i]==word(typed_record+0x1c+i*4),"gameplay lighting helper changed original preserved sample");
        ++typed_checked;
    }
    std::vector<std::uint32_t> out{result};for(unsigned i=0;i<21;++i)out.push_back(word(result+i*4));
    for(const auto& [at,count]:owned)for(unsigned i=0;i<count;++i)out.push_back(word(at+i*4));
    return out;
}
}
int main(int argc,char** argv)try {
    const char* path=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/source_model_lighting.original.bin";
    std::ifstream f(path,std::ios::binary|std::ios::ate);require(bool(f),"model lighting original fixture missing");const auto size=f.tellg();require(size>=12 && size%4==0,"model lighting fixture size");
    std::vector<std::uint32_t> data(std::size_t(size)/4);f.seekg(0);require(bool(f.read(reinterpret_cast<char*>(data.data()),size)),"model lighting fixture read");
    const bool typed=argc>2 && std::string(argv[2])=="--typed";
    Reader r{data};require(r.word()==0x4c494d53 && r.word()==1,"model lighting fixture schema");const auto count=r.word();unsigned boundaries=0,typed_checked=0;std::uint64_t checked=0;
    for(unsigned i=0;i<count;++i){const auto ni=r.word(),no=r.word();require(std::uint64_t(r.cursor)+ni+no<=data.size(),"model lighting case extent");std::vector<std::uint32_t> result;
        try{result=solve(r.words.subspan(r.cursor,ni),boundaries,typed,typed_checked);}catch(const std::exception& e){throw FormatError("model lighting case"+std::to_string(i)+": "+e.what());}
        r.cursor+=ni;require(result.size()==no,"model lighting output size differs");
        for(unsigned j=0;j<no;++j)if(result[j]!=r.words[r.cursor+j])throw FormatError("model lighting case"+std::to_string(i)+" word"+std::to_string(j)+" got"+std::to_string(result[j])+" expected"+std::to_string(r.words[r.cursor+j]));
        r.cursor+=no;checked+=no;
    }
    r.done();if(typed)require(typed_checked>0,"gameplay lighting helper had no original preserved-sample cases");std::cout<<"PASS "<<count<<" original3DB00/3DFE0 calls / "<<checked<<" exact retained words / "<<boundaries<<" ordered owner events / "<<typed_checked<<" gameplay helper preserved samples; native routing/cache chronology/RGB interpolation; query/conversion/velocity/incident and live submission remain separate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
