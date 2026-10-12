#include "halo2/source_physical_cache.hpp"
#include "halo2/cache.hpp"
#include <cstring>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
namespace {
void require(bool value,const char* why){if(!value)throw FormatError(why);}
struct Reader {
    std::span<const std::uint32_t> data;std::size_t at{};
    std::uint32_t word(){require(at<data.size(),"physical cache fixture truncated");return data[at++];}
    void done(){require(at==data.size(),"physical cache fixture trailing words");}
};
std::vector<std::uint32_t> solve(std::span<const std::uint32_t> input) {
    Reader r{input};const auto operation=r.word(),allocator=r.word(),bytes=r.word(),threshold=r.word(),datum=r.word(),busy_mask=r.word(),count=r.word();
    std::map<std::uint32_t,std::vector<std::byte>> memory;
    for(unsigned i=0;i<count;++i) {
        const auto address=r.word(),n=r.word();auto& bytes=memory[address];bytes.resize(std::size_t(n)*4);
        for(unsigned j=0;j<n;++j){const auto value=r.word();std::memcpy(bytes.data()+j*4,&value,4);}
    }
    r.done();
    const auto view=[&](std::uint32_t at,std::size_t bytes)->std::span<std::byte> {
        auto found=memory.upper_bound(at);require(found!=memory.begin(),"physical cache bank absent");--found;
        const auto offset=std::uint64_t(at)-found->first;
        if(offset>found->second.size() || bytes>found->second.size()-offset)
            throw FormatError("physical cache missing bank at"+std::to_string(at));
        return std::span(found->second).subspan(offset,bytes);
    };
    SourcePhysicalCacheServices world;world.storage.memory=view;
    world.storage.read=[&](auto at,auto n)->std::span<const std::byte>{return view(at,n);};
    std::vector<std::uint32_t> events;
    world.busy=[&](auto callback,auto handle){
        const auto index=handle&65535u;const auto result=index<32?((busy_mask>>index)&1u):0;
        events.insert(events.end(),{1,callback,handle,result});return result!=0;
    };
    world.deleted=[&](auto callback,auto handle){events.insert(events.end(),{2,callback,handle});};
    std::uint32_t result=0;
    if(operation)source_physical_cache_release(allocator,datum,world);
    else result=source_physical_cache_allocate(allocator,bytes,std::bit_cast<std::int32_t>(threshold),world);
    std::vector<std::uint32_t> out{result,std::uint32_t(events.size())};out.insert(out.end(),events.begin(),events.end());
    for(const auto& [at,bytes]:memory) {
        const auto begin=out.size();out.resize(begin+bytes.size()/4);std::memcpy(out.data()+begin,bytes.data(),bytes.size());
    }
    return out;
}
}
int main(int argc,char** argv)try {
    const char* path=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/source_physical_cache.original.bin";
    std::ifstream file(path,std::ios::binary|std::ios::ate);require(bool(file),"physical cache original fixture missing");const auto bytes=file.tellg();require(bytes>=12 && bytes%4==0,"physical cache fixture extent");
    std::vector<std::uint32_t> data(std::size_t(bytes)/4);file.seekg(0);require(bool(file.read(reinterpret_cast<char*>(data.data()),bytes)),"physical cache fixture read");
    Reader r{data};require(r.word()==0x43504753 && r.word()==1,"physical cache fixture schema");const auto count=r.word();std::uint64_t checked=0;
    for(unsigned i=0;i<count;++i) {
        const auto ni=r.word(),no=r.word();require(r.at+ni+no<=data.size(),"physical cache case extent");std::vector<std::uint32_t> out;
        try{out=solve(std::span(data).subspan(r.at,ni));}catch(const std::exception& e){throw FormatError("physical cache case"+std::to_string(i)+": "+e.what());}
        r.at+=ni;require(out.size()==no,"physical cache output extent differs");
        for(unsigned n=0;n<no;++n)if(out[n]!=data[r.at+n])throw FormatError("physical cache case"+std::to_string(i)+" word"+std::to_string(n)+" got"+std::to_string(out[n])+" expected"+std::to_string(data[r.at+n]));
        r.at+=no;checked+=no;
    }
    r.done();std::cout<<"PASS "<<count<<" whole original13D370/13D830 calls / "<<checked<<" exact retained resource words; busy/deletion callbacks are explicit records; live geometry callback/async ownership and GPU submission remain separate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
