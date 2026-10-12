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
    std::uint32_t word(){require(cursor<words.size(),"model resource fixture truncated");return words[cursor++];}
    void done(){require(cursor==words.size(),"model resource fixture trailing words");}
};
std::vector<std::uint32_t> solve(std::span<const std::uint32_t> input,unsigned& checked_requests,unsigned& checked_acquires) {
    Reader r{input};const auto kind=r.word(),eax=r.word();require(kind<4,"model resource kind");
    std::array<std::uint32_t,7> a{};for(unsigned i=0;i<(kind==3?2u:kind==2?1u:kind?7u:6u);++i)a[i]=r.word();
    SourceModelResourceWorld w;w.banks.tag_table=r.word();w.banks.objects=r.word();
    w.view=std::bit_cast<std::int32_t>(r.word());w.render_cache_pool=r.word();w.render_frame=r.word();w.temporary_regions=r.word();
    std::vector<std::array<std::uint32_t,2>> owned(r.word());for(auto& row:owned)for(auto& v:row)v=r.word();
    std::map<std::uint32_t,std::vector<std::byte>> memory;const auto segments=r.word();
    for(unsigned i=0;i<segments;++i){const auto address=r.word(),count=r.word();auto& raw=memory[address];raw.resize(std::size_t(count)*4);for(unsigned n=0;n<count;++n){const auto v=r.word();std::memcpy(raw.data()+n*4,&v,4);}}
    const auto view=[&](std::uint32_t address,std::size_t extent)->std::span<std::byte> {
        auto it=memory.upper_bound(address);require(it!=memory.begin(),"model resource bank absent");--it;
        const auto offset=std::size_t(address-it->first);
        if(offset>it->second.size()||extent>it->second.size()-offset)
            throw FormatError("model resource bank missing at"+std::to_string(address));
        return std::span(it->second).subspan(offset,extent);
    };
    w.banks.read=[&](std::uint32_t address,std::size_t extent)->std::span<const std::byte>{return view(address,extent);};
    w.write=view;
    std::vector<std::array<std::uint32_t,3>> requests(r.word());for(auto& row:requests)for(auto& v:row)v=r.word();
    std::vector<std::array<std::uint32_t,2>> acquires(r.word());for(auto& row:acquires)for(auto& v:row)v=r.word();
    std::vector<std::array<std::uint32_t,2>> events(r.word());for(auto& row:events)for(auto& v:row)v=r.word();r.done();
    unsigned used_requests=0,used_acquires=0,used_events=0;
    const auto order=[&](std::uint32_t kind,unsigned index) {
        require(used_events<events.size(),"model resource extra ordered owner boundary");
        require(events[used_events++]==std::array<std::uint32_t,2>{kind,index},"model resource owner chronology differs");
    };
    w.banks.request=[&](std::uint32_t mode,std::uint32_t address) {
        order(0,used_requests);require(used_requests<requests.size(),"model resource extra12DE70 request");
        const auto& row=requests[used_requests++];
        if(row[0]!=mode || row[1]!=address)throw FormatError("model resource12DE70 request"+std::to_string(used_requests-1)+" got"+std::to_string(mode)+":"+std::to_string(address)+" expected"+std::to_string(row[0])+":"+std::to_string(row[1]));
        return row[2]!=0;
    };
    w.cache_acquired=[&](std::uint32_t owner,std::uint32_t result) {
        order(1,used_acquires);require(used_acquires<acquires.size(),"model resource extra3DDD0 acquire");
        require(acquires[used_acquires++]==std::array<std::uint32_t,2>{owner,result},"native model resource3DDD0 owner/result differs");
    };
    std::uint32_t result;
    if(kind==3) {
        source_model_refresh_colors(a[0],a[1],w);result=0;
    } else if(kind==2) {
        result=source_model_acquire_cache(a[0],w);w.cache_acquired(a[0],result);
    } else result=kind?source_model_resources(a[0],a[1],a[2],eax,a[3],a[4],a[5],a[6],w):
        source_model_resource_sections(eax,a[0],a[1],std::bit_cast<std::int32_t>(a[2]),a[3],a[4],a[5],w);
    require(used_requests==requests.size() && used_acquires==acquires.size() && used_events==events.size(),"model resource omitted original boundary");
    checked_requests+=used_requests;checked_acquires+=used_acquires;
    std::uint32_t nodes=0;
    if(kind==1 && result) {
        const auto lod_bytes=view(a[3],2);std::int16_t lod;std::memcpy(&lod,lod_bytes.data(),2);
        nodes=std::bit_cast<std::uint32_t>(source_model_geometry_node_count(a[2],lod,a[5],w));
    }
    std::vector<std::uint32_t> out{result,nodes};
    for(const auto& [address,count]:owned){const auto bytes=view(address,std::size_t(count)*4);for(unsigned n=0;n<count;++n){std::uint32_t v;std::memcpy(&v,bytes.data()+n*4,4);out.push_back(v);}}
    return out;
}
}
int main(int argc,char** argv)try {
    const char* path=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/source_model_resources.original.bin";
    std::ifstream f(path,std::ios::binary|std::ios::ate);require(bool(f),"model resource original fixture missing");const auto size=f.tellg();require(size>=12 && size%4==0,"model resource fixture size");
    std::vector<std::uint32_t> data(std::size_t(size)/4);f.seekg(0);require(bool(f.read(reinterpret_cast<char*>(data.data()),size)),"model resource fixture read");
    Reader r{data};require(r.word()==0x52534d53 && r.word()==3,"model resource fixture schema");const auto count=r.word();unsigned requests=0,acquires=0;std::uint64_t checked=0;
    for(unsigned i=0;i<count;++i){const auto ni=r.word(),no=r.word();require(std::uint64_t(r.cursor)+ni+no<=data.size(),"model resource case extent");std::vector<std::uint32_t> result;
        try{result=solve(r.words.subspan(r.cursor,ni),requests,acquires);}catch(const std::exception& e){throw FormatError("model resource case"+std::to_string(i)+": "+e.what());}
        r.cursor+=ni;require(result.size()==no,"model resource output size differs");
        for(unsigned j=0;j<no;++j)if(result[j]!=r.words[r.cursor+j])throw FormatError("model resource case"+std::to_string(i)+" word"+std::to_string(j)+" got"+std::to_string(result[j])+" expected"+std::to_string(r.words[r.cursor+j]));
        r.cursor+=no;checked+=no;
    }
    r.done();std::cout<<"PASS "<<count<<" whole original4C2B0/4C190/3DDD0/3DD10 cases / "<<checked<<" exact retained words / "<<requests<<" ordered12DE70 requests / "<<acquires<<" native3DDD0 acquisitions; native3D430/4C640/137650 and allocation/rebind; actual resource publication and live4B5D0 remain separate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
