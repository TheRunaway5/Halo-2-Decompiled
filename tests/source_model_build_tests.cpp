#include "halo2/source_model_build.hpp"
#include "halo2/source_model_lighting.hpp"
#include "halo2/source_object_render_entry.hpp"
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
    std::uint32_t word(){require(cursor<words.size(),"model build fixture truncated");return words[cursor++];}
    void done(){require(cursor==words.size(),"model build fixture trailing words");}
};
std::vector<std::uint32_t> solve(std::span<const std::uint32_t> input,unsigned& boundaries,std::uint32_t schema) {
    Reader r{input};std::array<std::uint32_t,6> a{};for(auto& v:a)v=r.word();
    const auto force=r.word(),mode=r.word();SourceModelBuildWorld build;build.scratch=r.word();
    auto& w=build.admission;auto& m=w.memory;
    m.banks.tag_table=r.word();m.banks.objects=r.word();m.render_cache_pool=r.word();m.render_frame=r.word();m.view=std::bit_cast<std::int32_t>(r.word());
    w.first_person_owner=r.word();w.first_person_models=std::bit_cast<std::int16_t>(std::uint16_t(r.word()));w.view_modifier=r.word()!=0;w.ordinary_maximum=std::bit_cast<std::int32_t>(r.word());w.rasterizer=r.word();
    build.nodes.resource_cache_present=r.word()!=0;build.cached_models=r.word();build.view_owner=r.word();build.view_players=r.word();build.arena_cursor=r.word();build.arena=r.word();
    SourceModelLightingWorld lighting;
    if(schema>=2){lighting.default_lighting=r.word();lighting.bsp=r.word();lighting.fallback_lighting=r.word();lighting.interpolation=r.word()!=0;}
    std::vector<std::array<std::uint32_t,2>> owned(r.word());for(auto& row:owned)for(auto& v:row)v=r.word();
    std::map<std::uint32_t,std::vector<std::byte>> memory;const auto segments=r.word();
    for(unsigned i=0;i<segments;++i){const auto address=r.word(),count=r.word();auto& raw=memory[address];raw.resize(std::size_t(count)*4);for(unsigned n=0;n<count;++n){const auto v=r.word();std::memcpy(raw.data()+n*4,&v,4);}}
    const auto view=[&](std::uint32_t address,std::size_t extent)->std::span<std::byte> {
        auto it=memory.upper_bound(address);require(it!=memory.begin(),"model build bank absent");--it;
        const auto offset=std::size_t(address-it->first);
        if(offset>it->second.size() || extent>it->second.size()-offset)throw FormatError("model build bank missing at"+std::to_string(address));
        return std::span(it->second).subspan(offset,extent);
    };
    m.banks.read=[&](std::uint32_t at,std::size_t n)->std::span<const std::byte>{return view(at,n);};m.write=view;
    const auto word=[&](std::uint32_t at){std::uint32_t value;const auto raw=view(at,4);std::memcpy(&value,raw.data(),4);return value;};
    std::vector<std::vector<std::uint32_t>> events(r.word());for(auto& row:events){row.resize(r.word());for(auto& v:row)v=r.word();}r.done();unsigned used=0;
    const auto event=[&](std::initializer_list<std::uint32_t> values)->Reader {
        require(used<events.size(),"model build extra owner event");Reader row{events[used++]};
        for(auto v:values)require(row.word()==v,"model build ordered owner input differs");return row;
    };
    const auto patches=[&](Reader& row) {
        const auto count=row.word();
        for(unsigned i=0;i<count;++i) {
            const auto address=row.word(),value=row.word();auto raw=view(address,4);std::memcpy(raw.data(),&value,4);
        }
    };
    const auto check=[&](Reader& row,std::uint32_t at,unsigned count,const char* why){for(unsigned i=0;i<count;++i)if(row.word()!=word(at+i*4))throw FormatError(why);};
    const auto output=[&](Reader& row,std::uint32_t at,unsigned count){for(unsigned i=0;i<count;++i){const auto value=row.word();auto raw=view(at+i*4,4);std::memcpy(raw.data(),&value,4);}};
    m.banks.request=[&](std::uint32_t mode,std::uint32_t address) {
        auto row=event({0,mode,address});const auto value=row.word();row.done();return value!=0;
    };
    m.cache_acquired=[&](std::uint32_t owner,std::uint32_t result) {
        auto row=event({1,owner,result});row.done();
    };
    w.scalar=[&](std::uint32_t owner,std::uint32_t name) {
        auto row=event({2,owner,name});const auto enabled=row.word(),value=row.word();row.done();return std::pair{enabled!=0,std::bit_cast<float>(value)};
    };
    build.nodes.deferred_nodes=[&](std::uint32_t owner,std::uint32_t selected,std::uint32_t nodes) {
        auto row=event({3,owner,selected,nodes});const auto result=row.word();patches(row);row.done();return result;
    };
    build.rebase=[&](std::uint32_t owner,std::int32_t count,std::uint32_t nodes,std::uint32_t output) {
        auto row=event({4,owner,std::uint32_t(count),nodes,output});const auto n=row.word();
        for(unsigned i=0;i<n;++i){const auto value=row.word();auto bytes=view(output+i*4,4);std::memcpy(bytes.data(),&value,4);}row.done();
    };
    build.prt=[&](const std::array<std::uint32_t,7>& args) {
        auto row=event({5});for(auto v:args)require(row.word()==v,"model build PRT argument differs");const auto value=row.word();patches(row);row.done();return value!=0;
    };
    build.lighting=[&](std::uint32_t owner,float metric) {
        auto row=event({6,owner,std::bit_cast<std::uint32_t>(metric)});std::array<std::uint32_t,21> bank;
        if(schema>=2) {
            lighting.memory=m;lighting.scratch=row.word();
            const auto at=source_model_lighting(owner,metric,lighting);
            for(unsigned i=0;i<bank.size();++i){bank[i]=word(at+i*4);require(bank[i]==row.word(),"model build native lighting bank differs");}
        } else {for(auto& v:bank)v=row.word();patches(row);}
        row.done();return bank;
    };
    lighting.query=[&](std::uint32_t owner,bool initial,std::uint32_t at){auto row=event({7,owner,std::uint32_t(initial)});check(row,at,5,"model build lighting query initial sample differs");const auto result=row.word();output(row,at,5);row.done();return result!=0;};
    lighting.convert=[&](std::uint32_t sample,std::int32_t kind,bool flag,std::uint32_t out){auto row=event({8,std::uint32_t(kind),std::uint32_t(flag)});check(row,sample,5,"model build lighting conversion sample differs");check(row,out,21,"model build lighting conversion initial output differs");output(row,out,21);row.done();};
    lighting.velocity=[&](std::uint32_t owner,std::uint32_t out){auto row=event({9,owner});output(row,out,3);row.done();};
    lighting.incident=[&](std::uint32_t target,std::uint32_t current,float step){auto row=event({10,std::bit_cast<std::uint32_t>(step)});check(row,target,3,"model build incident target differs");check(row,current,3,"model build incident current differs");output(row,current,3);row.done();};
    SourceObjectDrawCollector collector;collector.address=a[3]-4;
    std::memcpy(collector.bytes.data(),&a[0],4);
    const auto retained=view(a[3],collector.bytes.size()-4);
    std::memcpy(collector.bytes.data()+4,retained.data(),retained.size());
    source_object_build_collector(collector,{a[0],a[1],a[2],a[5],mode,{},std::uint8_t(force)},build);
    require(std::memcmp(collector.bytes.data(),&a[0],4)==0,"model build collector changed caller owner");
    require(std::memcmp(collector.bytes.data()+4,view(a[3],collector.bytes.size()-4).data(),collector.bytes.size()-4)==0,
        "model build typed collector differs from current output bank");
    require(used==events.size(),"model build omitted owner event");boundaries+=used;
    std::vector<std::uint32_t> out;
    for(const auto& [address,count]:owned){const auto bytes=view(address,std::size_t(count)*4);for(unsigned n=0;n<count;++n){std::uint32_t v;std::memcpy(&v,bytes.data()+n*4,4);out.push_back(v);}}
    return out;
}
}
int main(int argc,char** argv)try {
    const char* path=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/source_model_build.original.bin";
    std::ifstream f(path,std::ios::binary|std::ios::ate);require(bool(f),"model build original fixture missing");const auto size=f.tellg();require(size>=12 && size%4==0,"model build fixture size");
    std::vector<std::uint32_t> data(std::size_t(size)/4);f.seekg(0);require(bool(f.read(reinterpret_cast<char*>(data.data()),size)),"model build fixture read");
    Reader r{data};require(r.word()==0x42444d53,"model build fixture magic");const auto schema=r.word();require(schema==1 || schema==2,"model build fixture schema");const auto count=r.word();unsigned boundaries=0;std::uint64_t checked=0;
    for(unsigned i=0;i<count;++i){const auto ni=r.word(),no=r.word();require(std::uint64_t(r.cursor)+ni+no<=data.size(),"model build case extent");std::vector<std::uint32_t> result;
        try{result=solve(r.words.subspan(r.cursor,ni),boundaries,schema);}catch(const std::exception& e){throw FormatError("model build case"+std::to_string(i)+": "+e.what());}
        r.cursor+=ni;require(result.size()==no,"model build output size differs");
        for(unsigned j=0;j<no;++j)if(result[j]!=r.words[r.cursor+j])throw FormatError("model build case"+std::to_string(i)+" word"+std::to_string(j)+" got"+std::to_string(result[j])+" expected"+std::to_string(r.words[r.cursor+j]));
        r.cursor+=no;checked+=no;
    }
    r.done();std::cout<<"PASS "<<count<<" whole original4B5D0 calls / "<<checked<<" exact retained words / "<<boundaries<<" ordered owner events; native admission/cache/resource/node/allocation"<<(schema>=2?"/lighting-cache chain":" chain")<<"; deep animation/sky/PRT/query/conversion/publication and live submission remain separate\n";

}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
