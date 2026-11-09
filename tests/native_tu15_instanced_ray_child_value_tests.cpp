#include "halo2/native_tu15_instanced_ray_callbacks.hpp"
#include "reference/native_tu15_instanced_ray_child_value_reference.hpp"
#include <cstring>
#include <iostream>
#include <random>
#include <stdexcept>
#include <tuple>
using namespace halo2;
namespace {
struct Boundary{};
struct Access {bool write;std::uint32_t address,bytes;bool operator==(const Access&)const=default;};
struct Store {std::uint32_t pc,address;std::vector<std::byte>before,after;bool operator==(const Store&)const=default;};
struct Memory:SourcePhysicsContactMemory {
    std::vector<std::byte>bytes;mutable std::vector<Access>accesses;mutable unsigned reads{};unsigned writes{};int read_fault=-1,write_fault=-1;
    std::span<std::byte>view(std::uint32_t address,std::size_t count){
        if(address<0x10000000u||std::uint64_t(address-0x10000000u)+count>bytes.size())throw std::runtime_error("Value child escaped declared bank");
        return std::span(bytes).subspan(address-0x10000000u,count);
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        if(int(reads++)==read_fault)throw Boundary{};accesses.push_back({false,address,std::uint32_t(out.size())});
        std::ranges::copy(const_cast<Memory*>(this)->view(address,out.size()),out.begin());
    }
    void write(std::uint32_t address,std::span<const std::byte>in)override{
        if(int(writes++)==write_fault)throw Boundary{};accesses.push_back({true,address,std::uint32_t(in.size())});std::ranges::copy(in,view(address,in.size()).begin());
    }
};
struct Outcome {Memory memory;std::vector<Store>stores;NativeTu15InstancedRayCallbackStats stats{};std::uint32_t result{};bool failed{};};
}
int main()try{
    std::mt19937_64 random{0x24dec0};unsigned normal{},prefixes{},interrupted{};
    for(unsigned n=0;n<256;++n){
        Memory initial;initial.bytes.resize(8192);for(auto&byte:initial.bytes)byte=std::byte(random());
        const auto collection=0x10000100u+n%16u;
        const auto child=std::array{0u,0x10000400u+n%16u,collection,collection+4u,collection+12u,0x10001000u+n%16u}[n%6u];
        const auto key=std::array{0u,1u,0xffffffffu,std::uint32_t(random())}[n%4u];
        for(unsigned observer=0;observer<2;++observer)for(int mode=0;mode<4;++mode){
            const auto limit=mode==0?1u:mode==1?6u:5u;
            for(unsigned ordinal=0;ordinal<limit;++ordinal){
                std::array<Outcome,2>out;
                for(unsigned route=0;route<2;++route){
                    auto&o=out[route];o.memory.bytes=initial.bytes;
                    if(mode==1)o.memory.read_fault=int(ordinal);if(mode==2)o.memory.write_fault=int(ordinal);
                    unsigned observed{};SourcePhysicsContactRecordObserver observe;
                    if(observer)observe=[&](const auto&event){o.stores.push_back({event.instruction,event.address,event.before,event.written});if(mode==3&&observed++==ordinal)throw Boundary{};};
                    try{
                        o.result=route?native_tu15_resolve_instanced_ray_child(o.memory,collection,key,child,observe,&o.stats):
                            instanced_ray_child_value_reference::resolve(o.memory,collection,key,child,observe,&o.stats);
                    }catch(const Boundary&){o.failed=true;}
                }
                if(out[0].result!=out[1].result||out[0].failed!=out[1].failed||out[0].memory.bytes!=out[1].memory.bytes||
                   out[0].memory.accesses!=out[1].memory.accesses||out[0].stores!=out[1].stores||std::memcmp(&out[0].stats,&out[1].stats,sizeof(out[0].stats)))
                    throw std::runtime_error("Shared public child policy changed value/read/store/observer/stats behavior");
                if(mode==0)++normal;else ++prefixes;if(out[0].failed)++interrupted;
            }
        }
    }
    std::cout<<"PASS instanced child public values normal="<<normal<<" prefix_controls="<<prefixes<<" interrupted="<<interrupted<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
