#include "halo2/source_model_node_bank.hpp"
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
    std::uint32_t word(){require(cursor<words.size(),"model node bank fixture truncated");return words[cursor++];}
    void done(){require(cursor==words.size(),"model node bank fixture trailing words");}
};
std::vector<std::uint32_t> solve(std::span<const std::uint32_t> input,unsigned& boundaries) {
    Reader r{input};std::array<std::uint32_t,7> a{};for(auto& v:a)v=r.word();
    SourceModelNodeWorld w;auto& m=w.memory;
    m.banks.tag_table=r.word();m.banks.objects=r.word();w.resource_cache_present=r.word()!=0;
    std::vector<std::array<std::uint32_t,2>> owned(r.word());for(auto& row:owned)for(auto& v:row)v=r.word();
    std::map<std::uint32_t,std::vector<std::byte>> memory;const auto segments=r.word();
    for(unsigned i=0;i<segments;++i){const auto address=r.word(),count=r.word();auto& raw=memory[address];raw.resize(std::size_t(count)*4);for(unsigned n=0;n<count;++n){const auto v=r.word();std::memcpy(raw.data()+n*4,&v,4);}}
    const auto view=[&](std::uint32_t address,std::size_t extent)->std::span<std::byte> {
        auto it=memory.upper_bound(address);require(it!=memory.begin(),"model node bank bank absent");--it;
        const auto offset=std::size_t(address-it->first);
        if(offset>it->second.size() || extent>it->second.size()-offset)throw FormatError("model node bank bank missing at"+std::to_string(address));
        return std::span(it->second).subspan(offset,extent);
    };
    m.banks.read=[&](std::uint32_t at,std::size_t n)->std::span<const std::byte>{return view(at,n);};m.write=view;
    std::vector<std::vector<std::uint32_t>> events(r.word());for(auto& row:events){row.resize(r.word());for(auto& v:row)v=r.word();}r.done();unsigned used=0;
    const auto event=[&](std::initializer_list<std::uint32_t> values)->Reader {
        require(used<events.size(),"model node bank extra owner event");Reader row{events[used++]};
        for(auto v:values)require(row.word()==v,"model node bank ordered owner input differs");return row;
    };
    m.banks.request=[&](std::uint32_t mode,std::uint32_t address) {
        auto row=event({0,mode,address});const auto value=row.word();row.done();return value!=0;
    };
    w.deferred_nodes=[&](std::uint32_t owner,std::uint32_t selected,std::uint32_t nodes) {
        auto row=event({1,owner,selected,nodes});const auto result=row.word();row.done();return result;
    };
    source_model_geometry_node_bank(a[0],a[1],a[2],a[3],std::bit_cast<std::int32_t>(a[4]),a[5],a[6],w);
    require(used==events.size(),"model node bank omitted owner event");boundaries+=used;
    std::vector<std::uint32_t> out;
    for(const auto& [address,count]:owned){const auto bytes=view(address,std::size_t(count)*4);for(unsigned n=0;n<count;++n){std::uint32_t v;std::memcpy(&v,bytes.data()+n*4,4);out.push_back(v);}}
    return out;
}
}
int main(int argc,char** argv)try {
    const char* path=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/source_model_node_bank.original.bin";
    std::ifstream f(path,std::ios::binary|std::ios::ate);require(bool(f),"model node bank original fixture missing");const auto size=f.tellg();require(size>=12 && size%4==0,"model node bank fixture size");
    std::vector<std::uint32_t> data(std::size_t(size)/4);f.seekg(0);require(bool(f.read(reinterpret_cast<char*>(data.data()),size)),"model node bank fixture read");
    Reader r{data};require(r.word()==0x4e424d53 && r.word()==1,"model node bank fixture schema");const auto count=r.word();unsigned boundaries=0;std::uint64_t checked=0;
    for(unsigned i=0;i<count;++i){const auto ni=r.word(),no=r.word();require(std::uint64_t(r.cursor)+ni+no<=data.size(),"model node bank case extent");std::vector<std::uint32_t> result;
        try{result=solve(r.words.subspan(r.cursor,ni),boundaries);}catch(const std::exception& e){throw FormatError("model node bank case"+std::to_string(i)+": "+e.what());}
        r.cursor+=ni;require(result.size()==no,"model node bank output size differs");
        for(unsigned j=0;j<no;++j)if(result[j]!=r.words[r.cursor+j])throw FormatError("model node bank case"+std::to_string(i)+" word"+std::to_string(j)+" got"+std::to_string(result[j])+" expected"+std::to_string(r.words[r.cursor+j]));
        r.cursor+=no;checked+=no;
    }
    r.done();std::cout<<"PASS "<<count<<" whole original137800 calls / "<<checked<<" exact retained words / "<<boundaries<<" ordered request/deferred events; native skin/compound/compact/map owners; deepBD610/publication/live4B5D0 remain separate\n";

}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
