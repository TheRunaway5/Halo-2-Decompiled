#include "halo2/source_geometry_resource_acquire.hpp"
#include "halo2/source_physical_cache.hpp"
#include "halo2/source_geometry_resource_retirement.hpp"
#include "halo2/cache.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
namespace {
void require(bool value,const char* why){if(!value)throw FormatError(why);}
struct Reader {
    std::span<const std::uint32_t> data;std::size_t at{};
    std::uint32_t word(){require(at<data.size(),"geometry publication fixture truncated");return data[at++];}
    void done(){require(at==data.size(),"geometry publication fixture trailing words");}
};
std::vector<std::uint32_t> solve(std::span<const std::uint32_t> input) {
    Reader r{input};const auto header=r.word(),flags=r.word(),outcome=r.word(),pumps=r.word(),promote_complete=r.word(),entry=r.word(),token=r.word(),count=r.word();
    std::map<std::uint32_t,std::vector<std::byte>> memory;
    for(unsigned i=0;i<count;++i) {
        const auto address=r.word(),n=r.word();auto& bytes=memory[address];bytes.resize(std::size_t(n)*4);
        for(unsigned j=0;j<n;++j){const auto value=r.word();std::memcpy(bytes.data()+j*4,&value,4);}
    }
    r.done();
    const auto view=[&](std::uint32_t at,std::size_t bytes)->std::span<std::byte> {
        auto found=memory.upper_bound(at);require(found!=memory.begin(),"geometry publication bank absent");--found;
        const auto offset=std::uint64_t(at)-found->first;
        if(offset>found->second.size() || bytes>found->second.size()-offset)
            throw FormatError("geometry publication missing bank at"+std::to_string(at));
        return std::span(found->second).subspan(offset,bytes);
    };
    SourceGeometryResourceMemory world;
    world.read=[&](std::uint32_t at,std::size_t n)->std::span<const std::byte>{return view(at,n);};world.write=view;
    std::vector<std::uint32_t> events;unsigned pump_count=0;
    const auto put=[&](std::uint32_t at,std::uint32_t value,unsigned n=4) {
        auto b=view(at,n);for(unsigned k=0;k<n;++k)b[k]=std::byte(value>>(k*8));
    };
    const auto word=[&](std::uint32_t at){std::uint32_t n;std::memcpy(&n,view(at,4).data(),4);return n;};
    const auto done_address=[&]{return word(word(0x4e648c)+0x44)+(word(header+0x20)&65535u)*12+2;};
    SourceGeometryRetirementServices retirement;
    SourcePhysicalCacheServices physical_cache;physical_cache.storage={.memory=view,.read=world.read};
    physical_cache.busy=[&](auto callback,auto handle){require(callback==0x12e210,"physical cache busy identity differs");return source_geometry_cache_busy(handle,SourceGeometryRetirementGlobals::retail(),world,retirement);};
    physical_cache.deleted=[&](auto callback,auto handle){require(callback==0x12e290,"physical cache deleted identity differs");source_geometry_cache_deleted(handle,SourceGeometryRetirementGlobals::retail(),world,retirement);};
    SourceGeometryResourceRequestServices services;
    services.acquire=[&](auto at,bool blocking,bool selective,std::int32_t priority) {
        events.insert(events.end(),{1,at,std::uint32_t(blocking),std::uint32_t(selective),std::uint32_t(priority)});
        SourceGeometryResourceAcquireServices lower;
        lower.physical_allocate=[&](auto physical,auto bytes,std::int32_t threshold) {
            events.insert(events.end(),{5,physical,bytes,std::uint32_t(threshold)});
            return source_physical_cache_allocate(physical,bytes,threshold,physical_cache);
        };
        lower.async_read=[&](const SourceBitmapReadRequest& request) {
            events.insert(events.end(),{6,request.location,request.size,request.buffer,
                request.bytes_output,request.done,request.category,std::uint32_t(request.priority)});
            if(outcome==2)put(request.done,1,1);return 0x87654321u;
        };
        (void)source_geometry_resource_acquire(at,blocking,selective,priority,
            SourceGeometryResourceGlobals::retail(),world,lower);
    };
    services.promote=[&](auto job,std::int32_t priority) {
        events.insert(events.end(),{2,job,std::uint32_t(priority)});
        if(promote_complete)put(done_address(),1,1);
    };
    services.yield=[&]{events.push_back(3);};
    services.pump=[&]{events.push_back(4);if(++pump_count>=pumps)put(done_address(),1,1);};
    const auto result=source_geometry_resource_request(header,flags,
        SourceGeometryResourceGlobals::retail(),world,services);
    std::vector<std::uint32_t> out{std::uint32_t(result),std::uint32_t(events.size())};
    out.insert(out.end(),events.begin(),events.end());
    for(const auto& [at,bytes]:memory) {
        const auto begin=out.size();out.resize(begin+bytes.size()/4);std::memcpy(out.data()+begin,bytes.data(),bytes.size());
    }
    return out;
}
}
int main(int argc,char** argv)try {
    const char* path=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/source_geometry_cache_joined.original.bin";
    std::ifstream file(path,std::ios::binary|std::ios::ate);require(bool(file),"geometry publication original fixture missing");const auto bytes=file.tellg();require(bytes>=12 && bytes%4==0,"geometry publication fixture extent");
    std::vector<std::uint32_t> data(std::size_t(bytes)/4);file.seekg(0);require(bool(file.read(reinterpret_cast<char*>(data.data()),bytes)),"geometry publication fixture read");
    Reader r{data};require(r.word()==0x43524753 && r.word()==1,"geometry publication fixture schema");const auto count=r.word();std::uint64_t checked=0;
    for(unsigned i=0;i<count;++i) {
        const auto ni=r.word(),no=r.word();require(r.at+ni+no<=data.size(),"geometry publication case extent");std::vector<std::uint32_t> out;
        try{out=solve(std::span(data).subspan(r.at,ni));}catch(const std::exception& e){throw FormatError("geometry publication case"+std::to_string(i)+": "+e.what());}
        r.at+=ni;require(out.size()==no,"geometry publication output extent differs");
        for(unsigned n=0;n<no;++n)if(out[n]!=data[r.at+n])throw FormatError("geometry publication case"+std::to_string(i)+" word"+std::to_string(n)+" got"+std::to_string(out[n])+" expected"+std::to_string(data[r.at+n]));
        r.at+=no;checked+=no;
    }
    r.done();std::cout<<"PASS "<<count<<" whole original request/acquisition/physical-cache/callback/datum/fixup calls / "<<checked<<" exact retained resource words; declared IO/promotion/yield/pump service records are explicit; live IO/cache chronology and GPU submission remain separate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
