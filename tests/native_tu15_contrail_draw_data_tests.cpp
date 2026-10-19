#include "halo2/native_tu15_contrail_draw_data.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {using namespace halo2;
constexpr std::array<std::uint32_t,3> addresses{0x45c438u,0x45c408u,0x45c3d8u};
constexpr std::uint32_t geometry_address=0x436f0u;
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    const auto bytes=memory.view(address,4);std::uint32_t value{};
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8u*i);
    return value;
}
std::string sha(std::span<const char> bytes) {
    std::array<unsigned char,32> digest{};unsigned size{};
    check(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&size,EVP_sha256(),nullptr)==1 &&
        size==digest.size(),"Fixture SHA256 computation failed");
    constexpr char hex[]="0123456789abcdef";std::string out;
    for(auto byte:digest){out+=hex[byte>>4];out+=hex[byte&15u];}return out;
}
void portable() {
    SourceRuntimeMemory memory;mount_native_tu15_contrail_draw_data(memory);
    const auto admitted=memory.checkpoint();
    check(admitted.regions.size()==4,"Contrail data owner lacks its four exact banks");
    for(const auto address:addresses) {
        const auto region=std::ranges::find(admitted.regions,address,&SourceRuntimeMemory::CheckpointRegion::address);
        check(region!=admitted.regions.end() && region->bytes.size()==44 &&
            region->access==SourceRuntimeMemory::Access::ReadOnly && !region->page_bytes && region->committed.empty(),
            "Contrail blend bank lacks its exact immutable extent");
        check(memory.native_address(address,44).value>0xffffffffu,"Contrail blend backing narrowed to a source token");
        check(word(memory,address+40)==0xffffffffu,"Original blend10 sentinel changed");
        for(const auto [at,count]:std::array<std::pair<std::uint32_t,unsigned>,3>{{
            {address-1u,2},{address,45},{address+43u,2}}}) {
            bool failed=false;try{(void)memory.view(at,count);}catch(const std::out_of_range&){failed=true;}
            check(failed,"Contrail blend table borrowed unowned neighboring bytes");
        }
        bool immutable=false;try{(void)memory.mutable_view(address,44);}catch(const std::runtime_error&){immutable=true;}
        check(immutable && memory.checkpoint()==admitted,"Contrail blend bank became writable");
    }
    const auto geometry=std::ranges::find(admitted.regions,geometry_address,&SourceRuntimeMemory::CheckpointRegion::address);
    check(geometry!=admitted.regions.end() && geometry->bytes.size()==20 &&
        geometry->access==SourceRuntimeMemory::Access::ReadOnly && !geometry->page_bytes && geometry->committed.empty(),
        "Contrail geometry dispatch lacks its separate exact immutable extent");
    check(memory.native_address(geometry_address,20).value>0xffffffffu,
        "Contrail geometry dispatch backing narrowed to a source token");
    for(const auto [at,count]:std::array<std::pair<std::uint32_t,unsigned>,3>{{
        {geometry_address-1u,2},{geometry_address,21},{geometry_address+19u,2}}}) {
        bool failed=false;try{(void)memory.view(at,count);}catch(const std::out_of_range&){failed=true;}
        check(failed,"Contrail geometry dispatch borrowed unowned neighboring bytes");
    }
    bool immutable=false;try{(void)memory.mutable_view(geometry_address,20);}catch(const std::runtime_error&){immutable=true;}
    check(immutable && memory.checkpoint()==admitted,"Contrail geometry dispatch became writable");
    // Deliberately conflict with each possible publication order, including
    // the last table, so failure cannot leave an earlier new table mounted.
    unsigned conflicts{};
    for(const auto [address,bytes]:std::array<std::pair<std::uint32_t,unsigned>,4>{{
        {addresses[0],44},{addresses[1],44},{addresses[2],44},{geometry_address,20}}})
      for(const auto [at,count]:std::array<std::pair<std::uint32_t,unsigned>,4>{{
        {address-1u,2},{address,bytes},{address+bytes-1u,2},{address-4u,bytes+8u}}})
        for(auto access:{SourceRuntimeMemory::Access::ReadOnly,SourceRuntimeMemory::Access::ReadWrite}) {
            SourceRuntimeMemory foreign;
            foreign.mount(at,std::vector<std::byte>(count,std::byte{0xa5}),access,"foreign exact/partial overlap");
            const auto before=foreign.checkpoint();bool failed=false;
            try{mount_native_tu15_contrail_draw_data(foreign);}catch(const std::invalid_argument&){failed=true;}
            check(failed && foreign.checkpoint()==before,"Foreign overlap caused partial contrail data publication");++conflicts;
        }
    for(const auto page:{0x45c000u,0x43000u}) {
        SourceRuntimeMemory reserved;
        reserved.reserve(page,4096,4096,"foreign uncommitted source reservation");
        const auto before=reserved.checkpoint();bool failed=false;
        try{mount_native_tu15_contrail_draw_data(reserved);}catch(const std::invalid_argument&){failed=true;}
        check(failed && reserved.checkpoint()==before,"An uncommitted reservation was treated as free data storage");
    }
    bool failed=false;try{mount_native_tu15_contrail_draw_data(memory);}catch(const std::invalid_argument&){failed=true;}
    check(failed && memory.checkpoint()==admitted,"Duplicate cold admission changed existing immutable tables");
    SourceRuntimeMemory beside;
    beside.mount(0x45c404u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"actual four-byte gap owner");
    beside.mount(geometry_address+20u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "independent adjacent geometry-bank owner");
    mount_native_tu15_contrail_draw_data(beside);
    check(beside.view(0x45c404u,4).size()==4,"Exact gap ownership was consumed by a broad bank");
    check(beside.view(geometry_address+20u,4).size()==4,"Geometry dispatch consumed its independent adjacent owner");
    std::cout<<"Portable: three blend banks plus one geometry bank, "<<conflicts<<" overlap rejections, two reservations and duplicate controls\n";
}
void original(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    check(bool(file) && file.tellg()>0 && file.tellg()<=8*1024*1024,"Original table fixture extent is invalid");
    std::vector<char> raw(static_cast<std::size_t>(file.tellg()));file.seekg(0);
    check(bool(file.read(raw.data(),std::streamsize(raw.size()))),"Original table fixture is truncated");
    check(sha(raw)=="e926ad8121e9e0f030b3a147edbd503b7b0eef4c2ecb909ad2d915b3853e42aa",
        "Original contrail table fixture bytes changed");
    const auto fixture=nlohmann::json::parse(raw.begin(),raw.end());
    check(fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        fixture.at("instruction_patches").empty() && fixture.at("blend_tables").size()==3,
        "Original contrail table authority differs");
    SourceRuntimeMemory memory;mount_native_tu15_contrail_draw_data(memory);unsigned words{};
    for(unsigned table=0;table<addresses.size();++table) {
        const auto& expected=fixture.at("blend_tables").at(table);
        check(expected.size()==11,"Original contrail table word count differs");
        for(unsigned n=0;n<11;++n) {
            check(word(memory,addresses[table]+n*4u)==expected.at(n),"Literal original contrail blend word differs");++words;
        }
    }
    std::cout<<"Original: "<<words<<" authenticated static words; no original GPU-service completion claim\n";
}
void original_geometry(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    check(bool(file) && file.tellg()>0 && file.tellg()<=16*1024,"Original geometry fixture extent is invalid");
    std::vector<char> raw(static_cast<std::size_t>(file.tellg()));file.seekg(0);
    check(bool(file.read(raw.data(),std::streamsize(raw.size()))),"Original geometry fixture is truncated");
    check(sha(raw)=="32e2d14e5f35933767f55619845630f6023c8df56e8920f57cefe3d5b2ab97a8",
        "Independent original geometry table fixture bytes changed");
    const auto fixture=nlohmann::json::parse(raw.begin(),raw.end());
    check(fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        fixture.at("offline_proof_sha256")=="bb73ef2142efdca45992a75a5b526c2461c6d78503a6af6d9dbd17d1a12e9092" &&
        fixture.at("instruction_patches").empty(),"Original geometry dispatch authority differs");
    const auto& table=fixture.at("table");const auto hex=table.at("raw_hex").get<std::string>();
    check(table.at("address")=="000436f0" && table.at("bytes")==20 && !table.at("writable").get<bool>() &&
        hex.size()==40 && table.at("word_hex").size()==5,"Original geometry table extent differs");
    SourceRuntimeMemory memory;mount_native_tu15_contrail_draw_data(memory);
    const auto actual=memory.view(geometry_address,20);
    for(unsigned n=0;n<actual.size();++n)
        check(std::to_integer<unsigned>(actual[n])==std::stoul(hex.substr(n*2u,2),nullptr,16),
            "Independent original geometry dispatch byte differs");
    for(unsigned n=0;n<5;++n)
        check(word(memory,geometry_address+n*4u)==std::stoul(table.at("word_hex").at(n).get<std::string>(),nullptr,16),
            "Independent original geometry dispatch word differs");
    std::cout<<"Original geometry: five authenticated words; separate from the 33 blend words; no activation\n";
}
}
int main(int argc,char** argv)try {
    if((argc!=1 && argc!=2 && argc!=3) || (argc==3 && std::string_view(argv[1])!="--geometry"))return 2;
    portable();
    if(argc>1) {
        const auto* path=argv[argc-1];
        if(!*path || !std::filesystem::is_regular_file(path))return 77;
        if(argc==2)original(path);else original_geometry(path);
    }
    std::cout<<"PASS native contrail immutable draw data; no World or activation\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
