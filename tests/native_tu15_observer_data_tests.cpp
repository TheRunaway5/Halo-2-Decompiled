#include "halo2/native_tu15_observer_data.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <stdexcept>

namespace {
using namespace halo2;
using Json=nlohmann::json;
void check(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
struct Shape {std::uint32_t address;std::size_t bytes;bool writable;};
constexpr std::array<Shape,14> shapes{{
    {0x44ea9c,4,false},{0x44eaac,4,false},{0x44eab8,24,false},
    {0x45f854,4,false},{0x46a124,4,false},{0x46a128,4,false},
    {0x46a2b8,4,false},{0x46a514,4,false},{0x46aa20,8,false},
    {0x475288,4,true},{0x475290,24,true},{0x4f64a0,4,true},
    {0x4f64a4,4*0x358,true},{0x4f7204,3,true}}};
std::vector<std::byte> bytes(const std::string& text) {
    check(text.size()%2==0,"Odd original loader byte string");
    std::vector<std::byte> result(text.size()/2);
    for(std::size_t i=0;i<result.size();++i) {
        std::size_t used{};const auto value=std::stoul(text.substr(i*2,2),&used,16);
        check(used==2,"Malformed original loader byte string");result[i]=std::byte(value);
    }
    return result;
}
void compare(const SourceRuntimeMemory& memory,const Json& fixture,bool isolated) {
    const auto checkpoint=memory.checkpoint();std::size_t total{};
    for(unsigned i=0;i<shapes.size();++i) {
        const auto& shape=shapes[i];const auto& expected=fixture.at("regions").at(i);
        check(expected.at("address")==shape.address&&expected.at("bytes")==shape.bytes&&
            expected.at("access")== (shape.writable?"ReadWrite":"ReadOnly"),
            "Original observer contract has an unexpected extent/access shape");
        const auto raw=bytes(expected.at("bytes_hex").get<std::string>());
        check(raw.size()==shape.bytes&&std::ranges::equal(memory.view(shape.address,shape.bytes),raw),
            "Mounted observer data differs from original loader bytes");
        check(memory.is_read_only(shape.address,shape.bytes)!=shape.writable,
            "Observer data changed original writable/immutable permissions");
        const auto region=std::ranges::find(checkpoint.regions,shape.address,
            &SourceRuntimeMemory::CheckpointRegion::address);
        check(region!=checkpoint.regions.end()&&region->bytes.size()==shape.bytes&&
            !region->page_bytes&&region->committed.empty(),
            "Observer data is not backed by its exact complete named region");
        check(memory.native_address(shape.address,shape.bytes).value>UINT32_MAX,
            "Observer data retained a narrowed native address");
        total+=shape.bytes;
    }
    check(total==3519,"Observer module extent total changed");
    if(isolated)check(checkpoint.regions.size()==shapes.size(),
        "Observer helper mounted unrelated source data");
    check(std::ranges::none_of(checkpoint.regions,[](const auto& region){return region.address==0x4f655cu;}),
        "Obsolete observer position fragment remains separately mounted");
}
void require_missing(const SourceRuntimeMemory& memory,std::uint32_t address) {
    bool missing=false;try{(void)memory.view(address,1);}catch(const std::out_of_range&){missing=true;}
    check(missing,"Observer module mapped an unauthenticated neighboring byte");
}
}
int main(int argc,char** argv)try {
    check(argc==2,"Supply original TU1.5 observer loader-data fixture");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    Json fixture;{std::ifstream input(argv[1]);input>>fixture;}
    check(fixture.at("schema")=="H2_NATIVE_TU15_OBSERVER_LOADER_DATA_V1"&&
        fixture.at("status")=="PASS_ORIGINAL_LOADER_BYTES_AND_EXTENTS"&&
        fixture.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
        fixture.at("regions").size()==shapes.size()&&fixture.at("code_patches").empty()&&
        fixture.at("service_returns").empty(),"Original observer loader-data authority differs");
    SourceRuntimeMemory isolated;mount_native_tu15_observer_data(isolated);compare(isolated,fixture,true);
    for(const auto gap:{0x47528cu,0x47528fu,0x4f649fu,0x4f7207u})require_missing(isolated,gap);
    const auto before=isolated.checkpoint();
    for(const auto& shape:shapes) {
        auto value=isolated.view(shape.address+std::uint32_t(shape.bytes-1),1).front()^std::byte{0x5a};
        bool rejected=false;
        try{isolated.write(shape.address+std::uint32_t(shape.bytes-1),std::span(&value,1));}
        catch(const std::exception&){rejected=true;}
        check(rejected!=shape.writable,"Observer data write admission differs from original access");
    }
    isolated.restore(before);check(isolated.checkpoint()==before,"Observer data checkpoint lost complete bytes");
    bool duplicate=false;try{mount_native_tu15_observer_data(isolated);}catch(const std::invalid_argument&){duplicate=true;}
    check(duplicate&&isolated.checkpoint()==before,"Duplicate observer composition was not rejected without mutation");
    auto cold=native_tu15_world_memory({0x60212000u,{},"observer contract retained tick-count input"});
    compare(cold,fixture,false);require_missing(cold,0x4f7207u);
    check(std::ranges::all_of(cold.view(0x4f64a0u,4),[](auto b){return b==std::byte{};})&&
        std::ranges::all_of(cold.view(0x4f64a4u,4*0x358),[](auto b){return b==std::byte{};})&&
        std::ranges::all_of(cold.view(0x4f7204u,3),[](auto b){return b==std::byte{};}),
        "Native cold factory preset an observer, frame result or readiness flag");
    std::cout<<"PASS14 exact original observer loader regions/3519 bytes; complete cold factory composition, permissions, gaps, native backing and checkpoint controls\n"
        <<"LIMIT data declaration only; no source reset/map/player/outer-frame, GPU or completed camera claim\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
