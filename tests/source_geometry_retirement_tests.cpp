#include "halo2/source_physical_cache.hpp"
#include "halo2/source_geometry_resource_retirement.hpp"
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
    Reader r{input};const auto operation=r.word(),header=r.word(),allocation=r.word(),datum=r.word(),allocator=r.word(),bytes=r.word(),threshold=r.word(),initial_query=r.word(),progress=r.word(),flight=r.word(),slots=r.word(),count=r.word();
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
    SourceGeometryResourceMemory bank{.read=[&](auto at,auto n)->std::span<const std::byte>{return view(at,n);},.write=view};
    const auto globals=SourceGeometryRetirementGlobals::retail();
    std::vector<std::uint32_t> events;auto query=initial_query;std::uint32_t waits=0;
    SourceGeometryRetirementServices retirement;
    retirement.resource_busy=[&](auto token){events.insert(events.end(),{1,token,query});return std::bit_cast<std::int32_t>(query);};
    retirement.wait_resource=[&](auto token){events.insert(events.end(),{2,token});query=0;};
    retirement.wait_mutex=[&](auto handle,auto time){events.insert(events.end(),{3,handle,time});if(++waits>=progress)for(unsigned i=0;i<slots;++i)view(flight+i*12+2,1)[0]=std::byte{1};};
    retirement.release_mutex=[&](auto handle){events.insert(events.end(),{4,handle});};
    SourcePhysicalCacheServices physical;physical.storage.memory=view;physical.storage.read=bank.read;
    physical.busy=[&](auto callback,auto handle){require(callback==0x12e210,"unexpected physical busy callback");return source_geometry_cache_busy(handle,globals,bank,retirement);};
    physical.deleted=[&](auto callback,auto handle){require(callback==0x12e290,"unexpected physical delete callback");source_geometry_cache_deleted(handle,globals,bank,retirement);};
    std::uint32_t result=0;
    switch(operation){
      case 0:result=source_geometry_resource_in_use(header,allocation,bank,retirement);break;
      case 1:source_geometry_resource_wait(header,allocation,bank,retirement);break;
      case 2:result=source_geometry_cache_busy(datum,globals,bank,retirement);break;
      case 3:source_geometry_cache_deleted(datum,globals,bank,retirement);break;
      case 4:result=source_physical_cache_allocate(allocator,bytes,std::bit_cast<std::int32_t>(threshold),physical);break;
      default:throw FormatError("unknown retirement operation");
    }
    std::vector<std::uint32_t> out{result,std::uint32_t(events.size())};out.insert(out.end(),events.begin(),events.end());
    for(const auto& [at,bytes]:memory) {
        const auto begin=out.size();out.resize(begin+bytes.size()/4);std::memcpy(out.data()+begin,bytes.data(),bytes.size());
    }
    return out;
}
}
int main(int argc,char** argv)try {
    const char* path=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/source_geometry_retirement.original.bin";
    std::ifstream file(path,std::ios::binary|std::ios::ate);require(bool(file),"physical cache original fixture missing");const auto bytes=file.tellg();require(bytes>=12 && bytes%4==0,"physical cache fixture extent");
    std::vector<std::uint32_t> data(std::size_t(bytes)/4);file.seekg(0);require(bool(file.read(reinterpret_cast<char*>(data.data()),bytes)),"physical cache fixture read");
    Reader r{data};require(r.word()==0x54524753 && r.word()==1,"physical cache fixture schema");const auto count=r.word();std::uint64_t checked=0;
    for(unsigned i=0;i<count;++i) {
        const auto ni=r.word(),no=r.word();require(r.at+ni+no<=data.size(),"physical cache case extent");std::vector<std::uint32_t> out;
        try{out=solve(std::span(data).subspan(r.at,ni));}catch(const std::exception& e){throw FormatError("physical cache case"+std::to_string(i)+": "+e.what());}
        r.at+=ni;require(out.size()==no,"physical cache output extent differs");
        for(unsigned n=0;n<no;++n)if(out[n]!=data[r.at+n])throw FormatError("physical cache case"+std::to_string(i)+" word"+std::to_string(n)+" got"+std::to_string(out[n])+" expected"+std::to_string(data[r.at+n]));
        r.at+=no;checked+=no;
    }
    r.done();std::cout<<"PASS "<<count<<" whole original geometry retirement/physical-cache calls / "<<checked<<" exact retained resource words; GPU query/wait and mutex/completion operations are explicit records; live async chronology and GPU submission remain separate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
