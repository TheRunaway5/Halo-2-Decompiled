#include "halo2/native_tu15_contrail_draw_data.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {using namespace halo2;
constexpr std::array<std::uint32_t,3> addresses{0x45c438u,0x45c408u,0x45c3d8u};
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
    check(admitted.regions.size()==3,"Contrail blend owner mounted extra banks");
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
    // Deliberately conflict with each possible publication order, including
    // the last table, so failure cannot leave an earlier new table mounted.
    unsigned conflicts{};
    for(const auto address:addresses)for(const auto [at,count]:
        std::array<std::pair<std::uint32_t,unsigned>,4>{{{address-1u,2},{address,44},{address+43u,2},{address-4u,52}}})
        for(auto access:{SourceRuntimeMemory::Access::ReadOnly,SourceRuntimeMemory::Access::ReadWrite}) {
            SourceRuntimeMemory foreign;
            foreign.mount(at,std::vector<std::byte>(count,std::byte{0xa5}),access,"foreign exact/partial overlap");
            const auto before=foreign.checkpoint();bool failed=false;
            try{mount_native_tu15_contrail_draw_data(foreign);}catch(const std::invalid_argument&){failed=true;}
            check(failed && foreign.checkpoint()==before,"Foreign overlap caused partial contrail data publication");++conflicts;
        }
    SourceRuntimeMemory reserved;
    reserved.reserve(0x45c000u,4096,4096,"foreign uncommitted source reservation");
    const auto before=reserved.checkpoint();bool failed=false;
    try{mount_native_tu15_contrail_draw_data(reserved);}catch(const std::invalid_argument&){failed=true;}
    check(failed && reserved.checkpoint()==before,"An uncommitted reservation was treated as free data storage");
    failed=false;try{mount_native_tu15_contrail_draw_data(memory);}catch(const std::invalid_argument&){failed=true;}
    check(failed && memory.checkpoint()==admitted,"Duplicate cold admission changed existing immutable tables");
    SourceRuntimeMemory beside;
    beside.mount(0x45c404u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"actual four-byte gap owner");
    mount_native_tu15_contrail_draw_data(beside);
    check(beside.view(0x45c404u,4).size()==4,"Exact gap ownership was consumed by a broad bank");
    std::cout<<"Portable: three exact immutable banks, "<<conflicts<<" overlap rejections, reservation and duplicate controls\n";
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
}
int main(int argc,char** argv)try {
    if(argc!=1 && argc!=2)return 2;
    portable();
    if(argc==2) {
        if(!*argv[1] || !std::filesystem::is_regular_file(argv[1]))return 77;
        original(argv[1]);
    }
    std::cout<<"PASS native contrail immutable draw data; no World or activation\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
