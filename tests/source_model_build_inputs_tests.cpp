#include "halo2/source_model_resources.hpp"
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
    std::uint32_t word(){require(cursor<words.size(),"model build input fixture truncated");return words[cursor++];}
    void done(){require(cursor==words.size(),"model build input fixture trailing words");}
};
std::vector<std::uint32_t> solve(std::span<const std::uint32_t> input) {
    Reader r{input};const auto kind=r.word(),eax=r.word();require(kind<2,"model build input kind");
    std::array<std::uint32_t,7> a{};for(unsigned i=0;i<(kind?2u:7u);++i)a[i]=r.word();
    SourceModelResourceWorld w;w.banks.tag_table=r.word();w.banks.objects=r.word();const auto cached_models=r.word();
    std::vector<std::array<std::uint32_t,2>> owned(r.word());for(auto& row:owned)for(auto& v:row)v=r.word();
    std::map<std::uint32_t,std::vector<std::byte>> memory;const auto segments=r.word();
    for(unsigned i=0;i<segments;++i){const auto address=r.word(),count=r.word();auto& raw=memory[address];raw.resize(std::size_t(count)*4);for(unsigned n=0;n<count;++n){const auto v=r.word();std::memcpy(raw.data()+n*4,&v,4);}}
    r.done();
    const auto view=[&](std::uint32_t address,std::size_t extent)->std::span<std::byte> {
        auto it=memory.upper_bound(address);require(it!=memory.begin(),"model build input bank absent");--it;
        const auto offset=std::size_t(address-it->first);
        if(offset>it->second.size() || extent>it->second.size()-offset)
            throw FormatError("model build input bank missing at"+std::to_string(address));
        return std::span(it->second).subspan(offset,extent);
    };
    w.banks.read=[&](std::uint32_t address,std::size_t extent)->std::span<const std::byte>{return view(address,extent);};w.write=view;
    std::uint32_t result=0;
    if(kind)result=std::bit_cast<std::uint32_t>(source_model_geometry_node_count(eax,std::bit_cast<std::int32_t>(a[0]),a[1],w));
    else source_model_build_inputs(eax,a[0],a[1],a[2],a[3],a[4],a[5],a[6],cached_models,w);
    std::vector<std::uint32_t> out{result};
    for(const auto& [address,count]:owned){const auto bytes=view(address,std::size_t(count)*4);for(unsigned n=0;n<count;++n){std::uint32_t v;std::memcpy(&v,bytes.data()+n*4,4);out.push_back(v);}}
    return out;
}
}
int main(int argc,char** argv)try {
    const char* path=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/source_model_build_inputs.original.bin";
    std::ifstream f(path,std::ios::binary|std::ios::ate);require(bool(f),"model build input original fixture missing");const auto size=f.tellg();require(size>=12 && size%4==0,"model build input fixture size");
    std::vector<std::uint32_t> data(std::size_t(size)/4);f.seekg(0);require(bool(f.read(reinterpret_cast<char*>(data.data()),size)),"model build input fixture read");
    Reader r{data};require(r.word()==0x49424d53 && r.word()==1,"model build input fixture schema");const auto count=r.word();std::uint64_t checked=0;
    for(unsigned i=0;i<count;++i){const auto ni=r.word(),no=r.word();require(std::uint64_t(r.cursor)+ni+no<=data.size(),"model build input case extent");std::vector<std::uint32_t> result;
        try{result=solve(r.words.subspan(r.cursor,ni));}catch(const std::exception& e){throw FormatError("model build input case"+std::to_string(i)+": "+e.what());}
        r.cursor+=ni;require(result.size()==no,"model build input output size differs");
        for(unsigned j=0;j<no;++j)if(result[j]!=r.words[r.cursor+j])throw FormatError("model build input case"+std::to_string(i)+" word"+std::to_string(j)+" got"+std::to_string(result[j])+" expected"+std::to_string(r.words[r.cursor+j]));
        r.cursor+=no;checked+=no;
    }
    r.done();std::cout<<"PASS "<<count<<" whole original3D4F0/137650 calls / "<<checked<<" exact retained words; independent current object/first-person/model inputs; live node/collector production remains separate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
