#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <iostream>
#include <unistd.h>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    if(cache.name()!="00a_introduction")throw FormatError("Supply the actual opening map for its authored identity fixture");
    auto path=(std::filesystem::temp_directory_path()/"halo2-source-campaign-identity-XXXXXX").string();
    if(!::mkdtemp(path.data()))throw FormatError("Source campaign identity storage unavailable");
    struct Cleanup{std::string path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}}cleanup{path};
    MultiplayerObjectRuntime objects(argv[1],cache,0,path);auto &world=objects.tu15_world();
    world.initialize_map();world.configure_campaign_session(1,1,30,0x12345678);
    auto &memory=world.memory();
    const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    const auto string=[&](std::uint32_t at) {
        const auto data=memory.view(at,256);const auto end=std::ranges::find(data,std::byte{});
        if(end==data.end())throw FormatError("Original campaign path has no terminator");
        return std::string(reinterpret_cast<const char *>(data.data()),std::size_t(end-data.begin()));
    };
    const auto game=word(0x4f3218u);
    if(word(game+0x1cu)!=1u || word(game+0x20u)!=1u)
        throw FormatError("Native campaign session omitted the actual opening campaign/map IDs: "+
            std::to_string(word(game+0x1cu))+"/"+std::to_string(word(game+0x20u)));
    if(string(game+0x24u)!=cache.scenario())throw FormatError("Native campaign options omitted the admitted scenario path");
    const auto next=[&](std::uint32_t current,const char *expected) {
        std::array<std::byte,4> bytes{};
        for(unsigned i=0;i<4;++i)bytes[i]=std::byte(current>>(8*i));
        memory.write(game+0x20u,bytes);
        const auto selected=world.call_actual(0x1664f0u);
        if(!selected || string(selected)!=expected)
            throw FormatError("Original campaign successor lookup differs from the authored map-key order");
    };
    next(1,"scenarios\\solo\\01a_tutorial\\01a_tutorial");
    // Original table contains801 before705. Its complete selector chooses
    // the smallest greater map ID, independently of array/filename order.
    next(701,"scenarios\\solo\\08a_deltacliffs\\08a_deltacliffs");
    next(705,"scenarios\\solo\\07b_forerunnership\\07b_forerunnership");
    next(801,"scenarios\\solo\\08b_deltacontrol\\08b_deltacontrol");
    std::array<std::byte,4> last{std::byte{0x25},std::byte{3},std::byte{},std::byte{}};
    memory.write(game+0x20u,last);
    if(world.call_actual(0x1664f0u))throw FormatError("Final original campaign level acquired a fabricated successor");
    std::cout<<"PASS actual opening options retain campaign/map1/1 and scenario path; original successor selection follows authored numeric keys\n";
    std::cout<<"LIMIT identity and source lookup; progress persistence, loader request and next-level gameplay remain separate\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
