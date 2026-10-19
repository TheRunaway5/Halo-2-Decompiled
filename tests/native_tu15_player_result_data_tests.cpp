#include "halo2/native_tu15_player_result_data.hpp"
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
template<class F>void rejects(F&& f) {
    bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}
    need(rejected,"Ownership violation was accepted");
}
}
int main(int argc,char** argv)try {
    need(argc==2,"Supply independent original data fixture");
    std::ifstream input(argv[1]);const auto original=nlohmann::json::parse(input);
    need(original.at("schema")=="H2_PLAYER_RESULT_ORIGINAL_DATA_V1"&&
        original.at("original_tu15_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
        original.at("authority_report_sha256")=="c07c5a22c177acf23413fef9f802dc9ca6de9a2de78f9946c7d9ce1a4589c015"&&
        original.at("instruction_patches").empty()&&original.at("supplied_gameplay_results").empty()&&
        !original.at("runtime_executable_input").get<bool>(),"Original fixture authority differs");
    unsigned cases=0;SourceRuntimeMemory memory;mount_native_tu15_player_result_data(memory);
    const auto state=memory.checkpoint();need(state.regions.size()==38,"Undeclared result storage was added");
    std::size_t total=0;unsigned writable=0,readonly=0;
    for(const auto& row:original.at("regions")) {
        const auto address=row.at("address").get<std::uint32_t>();const auto bytes=unhex(row.at("bytes_hex"));
        need(std::ranges::equal(memory.view(address,bytes.size()),bytes),"Original descriptor/name bytes differ");
        const bool ro=row.at("access")=="read_only";
        need(memory.is_read_only(address,bytes.size())==ro,"Original section access differs");
        need(memory.source_address(memory.native_address(address,bytes.size()),bytes.size())==address,
            "Native backing lost original data identity");
        total+=bytes.size();ro?++readonly:++writable;++cases;
    }
    need(total==1325&&writable==1&&readonly==37,"Result metadata borrowed surrounding storage or lost original section access");
    for(const auto& row:original.at("descriptor_rows")) {
        const auto index=row.at("index").get<unsigned>(),address=row.at("address").get<std::uint32_t>();
        need(address==0x47a380u+16u*index&&word(memory,address)==row.at("name_pointer")&&
            word(memory,address+4u)==row.at("field_4")&&
            (word(memory,address+8u)&65535u)==row.at("minimum")&&
            (word(memory,address+8u)>>16)==row.at("maximum")&&
            word(memory,address+12u)==row.at("field_12"),"Original complete descriptor fields differ");
        const auto text=row.at("name").get<std::string>();
        const auto name=memory.view(word(memory,address),text.size()+1u);
        for(unsigned n=0;n<text.size();++n)
            need(std::to_integer<unsigned char>(name[n])==static_cast<unsigned char>(text[n]),"Original descriptor name target differs");
        need(name.back()==std::byte{},"Original descriptor name lost its NUL terminator");++cases;
    }
    const std::array mark{std::byte{0x11},std::byte{0x22},std::byte{0x33},std::byte{0x90}};
    for(const auto& row:original.at("regions"))if(row.at("access")=="read_only") {
        rejects([&]{memory.write(row.at("address").get<std::uint32_t>(),mark);});
        need(memory.checkpoint()==state,"Read-only rejection changed original data");++cases;
    }
    // The original descriptor table is FILE-BACKED WRITABLE .data, even
    // though these join/census consumers read its authored min/max values.
    memory.write(0x47a388u,mark);need(word(memory,0x47a388u)==0x90332211u,"Original .data writability was lost");
    const auto mutated=memory.checkpoint();rejects([&]{mount_native_tu15_player_result_data(memory);});
    need(memory.checkpoint()==mutated,"Repeated declaration rewound actual descriptor storage");++cases;
    for(const auto& row:original.at("regions")) {
        const auto address=row.at("address").get<std::uint32_t>();
        const auto size=unhex(row.at("bytes_hex")).size();
        for(const auto offset:std::array<std::int64_t,3>{-1,0,static_cast<std::int64_t>(size)-1}) {
            SourceRuntimeMemory conflict;
            conflict.mount(std::uint32_t(address+offset),std::array<std::byte,2>{},SourceRuntimeMemory::Access::ReadOnly,"foreign partial region");
            const auto before=conflict.checkpoint();rejects([&]{mount_native_tu15_player_result_data(conflict);});
            need(conflict.checkpoint()==before,"Partial owner rejection published another result region");++cases;
        }
        SourceRuntimeMemory reserved;reserved.reserve(address,4,4,"foreign uncommitted source reservation");
        const auto before=reserved.checkpoint();rejects([&]{mount_native_tu15_player_result_data(reserved);});
        need(reserved.checkpoint()==before,"Uncommitted owner rejection published another region");++cases;
    }
    SourceRuntimeMemory neighbors;
    for(const auto address:{0x47a37cu,0x47a650u,0x51dfa0u,0x51eef4u,0x522994u})
        neighbors.mount(address,mark,SourceRuntimeMemory::Access::ReadOnly,"independent source metadata/live state");
    mount_native_tu15_player_result_data(neighbors);
    for(const auto address:{0x47a37cu,0x47a650u,0x51dfa0u,0x51eef4u,0x522994u})
        need(std::ranges::equal(neighbors.view(address,4),mark),"Adjacent or live result owner was changed");
    ++cases;
    std::cout<<cases<<" original-data/ownership cases passed; 45 complete descriptors, 37 RO names, 1325 bytes; no initialized results session or mode admission\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
