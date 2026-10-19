#include "halo2/native_tu15_ctf_static_data.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string& text) {
    std::vector<std::byte> bytes;
    for(std::size_t i=0;i<text.size();i+=2)bytes.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return bytes;
}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    const auto b=memory.view(address,4);std::uint32_t result{};
    for(unsigned i=0;i<4;++i)result|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return result;
}
template<class F>void rejects(F&& f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}need(rejected,"Ownership violation was accepted");}
}
int main(int argc,char** argv)try {
    need(argc==2,"Supply independent original data fixture");
    std::ifstream input(argv[1]);const auto original=nlohmann::json::parse(input);
    need(original.at("schema")=="H2_CTF_STATIC_ORIGINAL_DATA_V1"&&
        original.at("original_tu15_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
        original.at("instruction_patches").empty()&&original.at("supplied_gameplay_results").empty()&&
        !original.at("runtime_executable_input").get<bool>(),"Original fixture authority differs");
    unsigned cases=0;SourceRuntimeMemory memory;mount_native_tu15_ctf_static_data(memory);
    const auto state=memory.checkpoint();need(state.regions.size()==3,"Undeclared source storage was added");
    std::size_t total=0;
    for(const auto& row:original.at("regions")) {
        const auto address=row.at("address").get<std::uint32_t>();const auto bytes=unhex(row.at("bytes_hex"));
        need(std::ranges::equal(memory.view(address,bytes.size()),bytes),"Original metadata bytes differ");
        need(memory.is_read_only(address,bytes.size())==(row.at("access")=="read_only"),"Original section access differs");
        need(memory.source_address(memory.native_address(address,bytes.size()),bytes.size())==address,"Native backing lost original data identity");
        total+=bytes.size();++cases;
    }
    need(total==212,"Class metadata borrowed surrounding storage");
    for(const auto& row:original.at("callbacks")) {
        need(word(memory,0x466190u+4u*row.at("slot").get<unsigned>())==std::stoul(row.at("entry").get<std::string>(),nullptr,16),"Original callback identity differs");++cases;
    }
    const std::array mark{std::byte{0x11},std::byte{0x22},std::byte{0x33},std::byte{0x90}};
    rejects([&]{memory.write(0x466190u,mark);});need(memory.checkpoint()==state,"Read-only rejection changed original bytes");++cases;
    memory.write(0x52d710u,mark);const auto published=memory.checkpoint();
    rejects([&]{mount_native_tu15_ctf_static_data(memory);});
    need(memory.checkpoint()==published,"Repeated metadata declaration rewound the real publication cell");++cases;
    for(const auto& row:original.at("regions")) {
        const auto address=row.at("address").get<std::uint32_t>();const auto size=unhex(row.at("bytes_hex")).size();
        for(const auto offset:std::array<std::int64_t,3>{-1,0,static_cast<std::int64_t>(size)-1}) {
            SourceRuntimeMemory conflict;
            conflict.mount(std::uint32_t(address+offset),std::array<std::byte,2>{},SourceRuntimeMemory::Access::ReadOnly,"foreign partial region");
            const auto before=conflict.checkpoint();rejects([&]{mount_native_tu15_ctf_static_data(conflict);});
            need(conflict.checkpoint()==before,"Partial owner rejection published another class region");++cases;
        }
        SourceRuntimeMemory reserved;reserved.reserve(address,4,4,"foreign uncommitted source reservation");
        const auto before=reserved.checkpoint();rejects([&]{mount_native_tu15_ctf_static_data(reserved);});
        need(reserved.checkpoint()==before,"Uncommitted owner rejection published another region");++cases;
    }
    SourceRuntimeMemory neighbors;
    for(const auto address:{0x48bfa4u,0x48bfa8u,0x46625cu,0x52d714u})
        neighbors.mount(address,mark,SourceRuntimeMemory::Access::ReadOnly,"independent adjacent metadata");
    mount_native_tu15_ctf_static_data(neighbors);
    for(const auto address:{0x48bfa4u,0x48bfa8u,0x46625cu,0x52d714u})
        need(std::ranges::equal(neighbors.view(address,4),mark),"Adjacent data owner was changed");
    ++cases;
    std::cout<<cases<<" original-metadata/ownership cases passed; 51 exact callbacks, 212 bytes, no runtime executable or mode admission\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
